#include "source_view.hpp"
#include "grid.hpp"
#include "session.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 8192
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <set>
#include <sstream>
#include <stb_image.h>
#include <stdexcept>
namespace opensim::app {
namespace {
void checked(const core::Result<void> &r) {
    if (r.error())
        throw std::runtime_error(r.error()->message);
}
void color(SDL_Renderer *r, scene::Color c) {
    require_sdl(SDL_SetRenderDrawColor(r, static_cast<Uint8>(std::round(c.r * 255)),
                                       static_cast<Uint8>(std::round(c.g * 255)),
                                       static_cast<Uint8>(std::round(c.b * 255)),
                                       static_cast<Uint8>(std::round(c.a * 255))),
                "Source color");
}
void panel(SDL_Renderer *r, float x, float y, float w, float h) {
    const SDL_FRect b{x, y, w, h};
    require_sdl(SDL_RenderFillRect(r, &b), "Source panel");
}
std::string number(double n) {
    std::ostringstream s;
    s.precision(4);
    s << n;
    return s.str();
}
math::Vec2 rotated(math::Vec2 p, double a) {
    return {p.x * std::cos(a) - p.y * std::sin(a), p.x * std::sin(a) + p.y * std::cos(a)};
}
bool hit(const scene::CircleBody &body, const scene::CircleSample &sample, math::Vec2 world) {
    auto q = rotated({world.x - sample.center.x, world.y - sample.center.y}, -sample.angle);
    if (body.fixtures.empty())
        return std::hypot(q.x, q.y) <= body.radius;
    for (const auto &f : body.fixtures) {
        if (f.vertices.empty()) {
            if (std::hypot(q.x - f.center.x, q.y - f.center.y) <= f.radius)
                return true;
            continue;
        }
        bool inside = true;
        for (std::size_t i = 0; i < f.vertices.size(); ++i) {
            auto a = f.vertices[i], b = f.vertices[(i + 1) % f.vertices.size()];
            if ((b.x - a.x) * (q.y - a.y) - (b.y - a.y) * (q.x - a.x) < 0) {
                inside = false;
                break;
            }
        }
        if (inside)
            return true;
    }
    return false;
}
} // namespace
SourceView::SourceView(compat::MechanicalSource s)
    : source_(std::move(s)), camera_(source_.camera_center()), scale_(source_.camera_scale()) {
    std::size_t pixels = 0;
    for (const auto &[name, bytes] : source_.images()) {
        int w = 0, h = 0, channels = 0;
        if (!stbi_info_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h,
                                   &channels) ||
            w <= 0 || h <= 0 || w > 8192 || h > 8192 || std::size_t(w) * h > 32 * 1024 * 1024)
            throw std::runtime_error("Source image dimension budget");
        std::unique_ptr<unsigned char, decltype(&stbi_image_free)> image(
            stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &channels,
                                  4),
            stbi_image_free);
        if (!image)
            throw std::runtime_error("Decode source image failed");
        const double reduction = std::min(1.0, 2048.0 / std::max(w, h));
        const int tw = std::max(1, int(w * reduction)), th = std::max(1, int(h * reduction));
        if (std::size_t(tw) * th > 16 * 1024 * 1024 - pixels)
            throw std::runtime_error("Retained source image budget");
        pixels += std::size_t(tw) * th;
        std::vector<std::uint8_t> retained(std::size_t(tw) * th * 4);
        for (int y = 0; y < th; ++y)
            for (int x = 0; x < tw; ++x) {
                const double sx = std::clamp((x + .5) * w / tw - .5, 0.0, double(w - 1));
                const double sy = std::clamp((y + .5) * h / th - .5, 0.0, double(h - 1));
                const int ix = int(sx), iy = int(sy), jx = std::min(ix + 1, w - 1),
                          jy = std::min(iy + 1, h - 1);
                const double dx = sx - ix, dy = sy - iy;
                for (unsigned channel = 0; channel < 4; ++channel) {
                    const auto at = [&](int px, int py) {
                        return double(image.get()[(std::size_t(py) * w + px) * 4 + channel]);
                    };
                    const double upper = (1 - dx) * at(ix, iy) + dx * at(jx, iy),
                                 bottom = (1 - dx) * at(ix, jy) + dx * at(jx, jy);
                    retained[(std::size_t(y) * tw + x) * 4 + channel] =
                        static_cast<std::uint8_t>(std::round((1 - dy) * upper + dy * bottom));
                }
            }
        decoded_.emplace(name, DecodedImage{tw, th, std::move(retained)});
    }
    replace_world();
}
void SourceView::replace_world() {
    auto w = physics::Mechanism::create(source_.definition());
    if (w.error())
        throw std::runtime_error(w.error()->message);
    apply_controls(**w.value());
    world_ = *w.value();
    running_ = false;
    remainder_ = 0;
    selected_.reset();
    dragging_.reset();
}
void SourceView::reset() {
    checked(world_->reset());
    running_ = false;
    remainder_ = 0;
    dragging_.reset();
}
math::Vec2 SourceView::local(math::Vec2 p, const Workspace &l) const {
    return {camera_.x + (p.x - l.x - double(l.canvas.width) / 2) / (scale_ * l.scale),
            camera_.y - (p.y - l.y - double(l.canvas.height) / 2) / (scale_ * l.scale)};
}
void SourceView::drag(math::Vec2 target) {
    if (!dragging_)
        return;
    for (const auto &j : source_.definition().links)
        if (j.body_a == *dragging_ && !j.body_b.valid() &&
            std::hypot(j.local_a.x, j.local_a.y) < 1e-6) {
            const double dx = target.x - j.local_b.x, dy = target.y - j.local_b.y,
                         len = std::hypot(dx, dy);
            if (len < 1e-6)
                return;
            double distance = j.length;
            if (j.limit)
                distance = std::clamp(len, j.minimum, j.maximum);
            else if (j.spring || j.stiffness > 0 || j.damping_coefficient >= 0)
                continue;
            target = {j.local_b.x + dx * distance / len, j.local_b.y + dy * distance / len};
            break;
        }
    checked(world_->relocate(*dragging_, target));
}
bool SourceView::event(const SDL_Event &e, Host &host) {
    const auto l = host.workspace();
    const auto action = [&](int button) {
        if (button == 0) {
            running_ = !running_;
            remainder_ = 0;
        } else if (button == 1) {
            running_ = false;
            step();
        } else if (button == 2)
            reset();
        else if (button == 7) {
            camera_ = source_.camera_center();
            scale_ = source_.camera_scale();
        }
    };
    try {
        if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
            dragging_.reset();
            running_ = false;
            remainder_ = 0;
        }
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            if (e.key.key == SDLK_SPACE)
                action(0);
            else if (e.key.key == SDLK_RIGHT)
                action(1);
            else if (e.key.key == SDLK_R)
                action(2);
            else if (e.key.key == SDLK_HOME)
                action(7);
            else if (e.key.key == SDLK_W)
                camera_.y += 40 / scale_;
            else if (e.key.key == SDLK_S)
                camera_.y -= 40 / scale_;
            else if (e.key.key == SDLK_A)
                camera_.x -= 40 / scale_;
            else if (e.key.key == SDLK_D)
                camera_.x += 40 / scale_;
            return true;
        }
        if (e.type == SDL_EVENT_MOUSE_WHEEL) {
            int ww = 0, wh = 0;
            require_sdl(SDL_GetWindowSize(host.window(), &ww, &wh), "Source wheel coordinates");
            const auto pointer = physical_pointer({e.wheel.mouse_x, e.wheel.mouse_y},
                                                  {double(ww), double(wh)}, host.extent());
            if (pointer.value() && pointer.value()->x / l.scale < l.left) {
                list_scroll_ =
                    std::clamp(list_scroll_ - e.wheel.y * 20, 0.0f,
                               std::max(0.0f, float(source_.definition().bodies.size()) * 20 -
                                                  (l.property_y() - 40 - 176)));
                return true;
            }
            scale_ = std::clamp(scale_ * (e.wheel.y > 0 ? 1.2 : .833333333), 1.0, 4096.0);
            return true;
        }
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP ||
            e.type == SDL_EVENT_MOUSE_MOTION) {
            int width = 0, height = 0;
            require_sdl(SDL_GetWindowSize(host.window(), &width, &height), "Source pointer size");
            math::Vec2 raw = e.type == SDL_EVENT_MOUSE_MOTION ? math::Vec2{e.motion.x, e.motion.y}
                                                              : math::Vec2{e.button.x, e.button.y};
            auto mapped = physical_pointer(raw, {double(width), double(height)}, host.extent());
            if (mapped.error())
                return true;
            auto p = *mapped.value();
            const double x = p.x / l.scale, y = p.y / l.scale;
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
                slider_.reset();
                if (dragging_) {
                    drag(local(p, l));
                    dragging_.reset();
                    running_ = resume_drag_;
                    remainder_ = 0;
                }
                return true;
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION && slider_) {
                const auto &w = source_.widgets().at(*slider_);
                const double left = double(l.x) / l.scale, vw = double(l.canvas.width) / l.scale;
                const double wx =
                    left + std::clamp(w.position.x, 8.0, std::max(8.0, vw - w.size.x - 8));
                const double fraction = std::clamp((x - wx) / w.size.x, 0.0, 1.0);
                checked(
                    source_.set_slider(*slider_, w.minimum + fraction * (w.maximum - w.minimum)));
                return true;
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION && dragging_) {
                drag(local(p, l));
                return true;
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                if (y >= 46 && y < 82 && x >= 12) {
                    const int b = static_cast<int>((x - 12) / l.button_width());
                    if (b >= 0 && b < 9 && x < 12 + b * l.button_width() + l.button_width() - 5) {
                        action(b);
                        return true;
                    }
                }
                const float left = float(l.x) / l.scale, top = float(l.y) / l.scale,
                            vw = float(l.canvas.width) / l.scale,
                            vh = float(l.canvas.height) / l.scale;
                for (std::size_t i = 0; i < source_.widgets().size(); ++i) {
                    const auto &w = source_.widgets()[i];
                    if (!w.slider || !w.visible || !w.enabled)
                        continue;
                    const double wx = left + std::clamp(w.position.x, 8.0,
                                                        std::max(8.0, double(vw) - w.size.x - 8));
                    const double wy = top + std::clamp(w.position.y, 8.0,
                                                       std::max(8.0, double(vh) - w.size.y - 8));
                    if (x >= wx && x < wx + w.size.x && y >= wy && y < wy + w.size.y) {
                        slider_ = i;
                        checked(source_.set_slider(
                            i, w.minimum + std::clamp((x - wx) / w.size.x, 0.0, 1.0) *
                                               (w.maximum - w.minimum)));
                        return true;
                    }
                }
                for (const auto &w : source_.widgets())
                    if (w.button && w.visible && w.enabled) {
                        const float wx =
                                        left + std::clamp(float(w.position.x), 8.0f,
                                                          std::max(8.0f, vw - float(w.size.x) - 8)),
                                    wy = top + std::clamp(float(w.position.y), 8.0f,
                                                          std::max(8.0f, vh - float(w.size.y) - 8));
                        if (x >= wx && x < wx + w.size.x && y >= wy && y < wy + w.size.y) {
                            checked(source_.apply_action(w.action));
                            replace_world();
                            status_ = "Script action applied; experimental mechanics.";
                            return true;
                        }
                    }
                if (x < l.left && y >= 176 && y < l.property_y() - 40) {
                    const auto index = static_cast<std::size_t>((y - 176 + list_scroll_) / 20);
                    if (index < source_.definition().bodies.size())
                        selected_ = source_.definition().bodies[index].id;
                    return true;
                }
                if (l.contains(p)) {
                    auto world = local(p, l);
                    for (const auto &b : world_->snapshot().bodies) {
                        const auto &defs = source_.definition().bodies;
                        auto found = std::find_if(defs.begin(), defs.end(),
                                                  [&](const auto &d) { return d.id == b.id; });
                        if (found != defs.end() && !found->static_body && hit(*found, b, world)) {
                            selected_ = b.id;
                            dragging_ = b.id;
                            resume_drag_ = running_;
                            running_ = false;
                            drag(world);
                            break;
                        }
                    }
                }
            }
        }
    } catch (const std::exception &error) {
        status_ = error.what();
        running_ = false;
        dragging_.reset();
        std::cerr << "Open Simphy: " << status_ << std::endl;
    }
    return true;
}
void SourceView::apply_controls(physics::Mechanism &world) {
    const auto controls = source_.controls(world.snapshot());
    if (controls.error())
        throw std::runtime_error(controls.error()->message);
    if (controls.value()->time)
        checked(world.set_time(*controls.value()->time));
    checked(world.update(controls.value()->updates));
    checked(world.forces(controls.value()->forces));
    for (const auto &[id, mu] : controls.value()->friction)
        checked(world.friction(id, mu));
}
void SourceView::step() {
    apply_controls(*world_);
    checked(world_->step());
}
void SourceView::tick(double dt) {
    if (!running_ || dragging_)
        return;
    remainder_ += std::clamp(dt, 0.0, .25);
    unsigned steps = 0;
    while (remainder_ >= source_.definition().fixed_dt && steps++ < 32) {
        try {
            step();
        } catch (const std::exception &error) {
            status_ = error.what();
            running_ = false;
            std::cerr << "Open Simphy: " << status_ << std::endl;
            break;
        }
        remainder_ -= source_.definition().fixed_dt;
    }
}
void SourceView::paint(SDL_Renderer *r, renderer::Extent extent, float density) {
    const auto l = Workspace::layout(extent, density);
    require_sdl(SDL_SetRenderScale(r, l.scale, l.scale), "Scale source mechanics");
    if (!text_)
        text_ = std::make_unique<TextRenderer>(r);
    const auto label = [&](float x, float y, const std::string &s, float width = 100000) {
        text_->draw(x, y, s, width);
    };
    const float left = float(l.x) / l.scale, top = float(l.y) / l.scale,
                vw = float(l.canvas.width) / l.scale, vh = float(l.canvas.height) / l.scale;
    color(r, {42 / 255.0, 42 / 255.0, 42 / 255.0, 1});
    panel(r, left, top, vw, vh);
    color(r, {32 / 255.0, 32 / 255.0, 32 / 255.0, 1});
    panel(r, l.left, 112, 64, l.height - 144);
    const auto project = [&](math::Vec2 p) {
        return SDL_FPoint{left + vw / 2 + float((p.x - camera_.x) * scale_),
                          top + vh / 2 - float((p.y - camera_.y) * scale_)};
    };
    const auto ticksX = grid_ticks(camera_.x, scale_, static_cast<unsigned>(vw)),
               ticksY = grid_ticks(-camera_.y, scale_, static_cast<unsigned>(vh));
    for (const auto &t : ticksX) {
        color(r, t.major ? scene::Color{65 / 255.0, 65 / 255.0, 65 / 255.0, 1}
                         : scene::Color{48 / 255.0, 48 / 255.0, 48 / 255.0, 1});
        require_sdl(SDL_RenderLine(r, left + float(t.pixel), top, left + float(t.pixel), top + vh),
                    "Source grid");
        if (t.major) {
            color(r, {200 / 255.0, 200 / 255.0, 200 / 255.0, 1});
            label(left + float(t.pixel), 98, number(t.world), 64);
        }
    }
    for (const auto &t : ticksY) {
        color(r, t.major ? scene::Color{65 / 255.0, 65 / 255.0, 65 / 255.0, 1}
                         : scene::Color{48 / 255.0, 48 / 255.0, 48 / 255.0, 1});
        require_sdl(SDL_RenderLine(r, left, top + float(t.pixel), left + vw, top + float(t.pixel)),
                    "Source grid");
        if (t.major) {
            color(r, {200 / 255.0, 200 / 255.0, 200 / 255.0, 1});
            label(l.left + 2, top + float(t.pixel), number(-t.world), 62);
        }
    }
    const SDL_Rect clip{int(left), int(top), int(vw), int(vh)};
    require_sdl(SDL_SetRenderClipRect(r, &clip), "Clip mechanism");
    if (images_.empty() && !source_.images().empty()) {
        for (const auto &[name, image] : decoded_) {
            const int w = image.width, h = image.height;
            Texture tex(
                SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h),
                SDL_DestroyTexture);
            require_sdl(bool(tex), "Source image texture");
            require_sdl(SDL_UpdateTexture(tex.get(), nullptr, image.pixels.data(), w * 4),
                        "Upload source image");
            require_sdl(SDL_SetTextureBlendMode(tex.get(), SDL_BLENDMODE_BLEND),
                        "Blend source image");
            require_sdl(SDL_SetTextureScaleMode(tex.get(), SDL_SCALEMODE_LINEAR),
                        "Filter source image");
            images_.emplace(name, std::move(tex));
        }
    }
    const auto state = world_->snapshot();
    std::map<core::EntityId, scene::CircleSample> samples;
    for (const auto &b : state.bodies)
        samples.emplace(b.id, b);
    for (const auto &b : source_.definition().bodies) {
        const auto &sample = samples.at(b.id);
        auto center = project(sample.center);
        const float radius = float(b.radius * scale_);
        if (center.x + radius < left || center.x - radius > left + vw || center.y + radius < top ||
            center.y - radius > top + vh)
            continue;
        const auto &style = source_.styles().at(b.id);
        SDL_FColor fill{static_cast<float>(style.fill.r), static_cast<float>(style.fill.g),
                        static_cast<float>(style.fill.b), static_cast<float>(style.fill.a)};
        std::vector<scene::RigidFixture> fallback;
        if (b.fixtures.empty()) {
            scene::RigidFixture f;
            f.radius = b.radius;
            fallback.push_back(f);
        }
        const auto &fixtures = b.fixtures.empty() ? fallback : b.fixtures;
        std::set<std::array<double, 4>> polygon_edges;
        if (fixtures.size() > 1)
            for (const auto &fixture : fixtures)
                for (std::size_t k = 0; k < fixture.vertices.size(); ++k) {
                    auto a = fixture.vertices[k],
                         end = fixture.vertices[(k + 1) % fixture.vertices.size()];
                    polygon_edges.insert({a.x, a.y, end.x, end.y});
                }
        double body_minx = 1e30, body_maxx = -1e30, body_miny = 1e30, body_maxy = -1e30;
        for (const auto &f : fixtures) {
            if (f.vertices.empty()) {
                body_minx = std::min(body_minx, f.center.x - f.radius);
                body_maxx = std::max(body_maxx, f.center.x + f.radius);
                body_miny = std::min(body_miny, f.center.y - f.radius);
                body_maxy = std::max(body_maxy, f.center.y + f.radius);
            } else
                for (auto q : f.vertices) {
                    body_minx = std::min(body_minx, q.x);
                    body_maxx = std::max(body_maxx, q.x);
                    body_miny = std::min(body_miny, q.y);
                    body_maxy = std::max(body_maxy, q.y);
                }
        }
        for (const auto &f : fixtures) {
            std::vector<math::Vec2> points = f.vertices;
            if (points.empty())
                for (int i = 0; i < 64; ++i) {
                    const double a = 2 * std::numbers::pi * i / 64;
                    points.push_back(
                        {f.center.x + f.radius * std::cos(a), f.center.y + f.radius * std::sin(a)});
                }
            std::vector<SDL_Vertex> vertices;
            std::vector<int> indices;
            std::vector<SDL_FPoint> outline;
            for (auto q : points) {
                const SDL_FPoint uv{float((q.x - body_minx) / (body_maxx - body_minx)),
                                    float(1 - (q.y - body_miny) / (body_maxy - body_miny))};
                q = rotated(q, sample.angle);
                auto projected = project({sample.center.x + q.x, sample.center.y + q.y});
                vertices.push_back({projected, fill, uv});
                outline.push_back(projected);
            }
            for (int i = 1; i + 1 < int(vertices.size()); ++i) {
                indices.push_back(0);
                indices.push_back(i);
                indices.push_back(i + 1);
            }
            outline.push_back(outline.front());
            SDL_Texture *image = style.image.empty() ? nullptr : images_.at(style.image).get();
            require_sdl(SDL_RenderGeometry(r, image, vertices.data(), int(vertices.size()),
                                           indices.data(), int(indices.size())),
                        "Draw rigid fixture");
            color(r, selected_ == b.id ? scene::Color{1, 1, 1, 1} : style.outline);
            if (fixtures.size() == 1 || f.vertices.empty()) {
                require_sdl(SDL_RenderLines(r, outline.data(), int(outline.size())),
                            "Draw fixture outline");
                continue;
            }
            for (std::size_t edge = 0; edge < points.size(); ++edge) {
                const auto a = points[edge], end = points[(edge + 1) % points.size()];
                const bool internal = polygon_edges.contains({end.x, end.y, a.x, a.y});
                if (!internal)
                    require_sdl(SDL_RenderLine(r, outline[edge].x, outline[edge].y,
                                               outline[edge + 1].x, outline[edge + 1].y),
                                "Draw fixture boundary");
            }
        }
    }
    for (const auto &h : source_.definition().hinges) {
        const auto &a = samples.at(h.body_a);
        auto q = rotated(h.local_a, a.angle);
        auto point = project({a.center.x + q.x, a.center.y + q.y});
        color(r, {.8, .8, .3, 1});
        panel(r, point.x - 3, point.y - 3, 6, 6);
    }
    for (const auto &w : source_.definition().welds) {
        const auto &a = samples.at(w.body_a);
        auto q = rotated(w.local_a, a.angle);
        auto point = project({a.center.x + q.x, a.center.y + q.y});
        color(r, {.85, .85, .85, 1});
        require_sdl(SDL_RenderLine(r, point.x - 4, point.y - 4, point.x + 4, point.y + 4),
                    "Draw weld");
        require_sdl(SDL_RenderLine(r, point.x - 4, point.y + 4, point.x + 4, point.y - 4),
                    "Draw weld");
    }
    for (const auto &w : source_.definition().windings) {
        const auto &a = samples.at(w.body_a), &b = samples.at(w.body_b);
        const double dx = b.center.x - a.center.x, dy = b.center.y - a.center.y,
                     d = std::hypot(dx, dy);
        if (d <= std::abs(w.radius_a - w.radius_b))
            continue;
        const double angle = std::atan2(dy, dx) + std::asin((w.radius_a - w.radius_b) / d);
        auto pa = project(
            {a.center.x + std::sin(angle) * w.radius_a, a.center.y - std::cos(angle) * w.radius_a});
        auto pb = project(
            {b.center.x + std::sin(angle) * w.radius_b, b.center.y - std::cos(angle) * w.radius_b});
        color(r, {1, .65, 0, 1});
        require_sdl(SDL_RenderLine(r, pa.x, pa.y, pb.x, pb.y), "Draw winding thread");
    }
    for (const auto &j : source_.definition().links) {
        const auto &a = samples.at(j.body_a);
        auto da = rotated(j.local_a, a.angle);
        math::Vec2 wa{a.center.x + da.x, a.center.y + da.y}, wb = j.local_b;
        if (j.body_b.valid()) {
            const auto &b = samples.at(j.body_b);
            auto db = rotated(j.local_b, b.angle);
            wb = {b.center.x + db.x, b.center.y + db.y};
        }
        auto pa = project(wa), pb = project(wb);
        color(r, source_.joint_colors().at(j.id));
        if (j.stiffness > 0) {
            const float dx = pb.x - pa.x, dy = pb.y - pa.y, length = std::hypot(dx, dy);
            std::vector<SDL_FPoint> coil{pa};
            if (length > 1)
                for (int k = 1; k < 20; ++k) {
                    const float t = float(k) / 20,
                                offset = (k == 1 || k == 19) ? 0 : (k % 2 ? 4.0f : -4.0f);
                    coil.push_back({pa.x + t * dx - offset * dy / length,
                                    pa.y + t * dy + offset * dx / length});
                }
            coil.push_back(pb);
            require_sdl(SDL_RenderLines(r, coil.data(), int(coil.size())), "Draw spring");
        } else
            for (int offset = -1; offset <= 1; ++offset)
                require_sdl(
                    SDL_RenderLine(r, pa.x + float(offset), pa.y, pb.x + float(offset), pb.y),
                    "Draw suspension");
        panel(r, pb.x - 2, pb.y - 2, 4, 4);
    }
    float textY = top + 136;
    for (const auto &w : source_.widgets()) {
        if (!w.visible)
            continue;
        if (w.slider) {
            const float x = left + std::clamp(float(w.position.x), 8.0f,
                                              std::max(8.0f, vw - float(w.size.x) - 8));
            const float y = top + std::clamp(float(w.position.y), 8.0f,
                                             std::max(8.0f, vh - float(w.size.y) - 8));
            std::string caption = w.text;
            auto marker = caption.find("value");
            if (marker != std::string::npos)
                caption.replace(marker, 5, number(w.value));
            color(r, {.18, .18, .18, 1});
            panel(r, x, y, float(w.size.x), float(w.size.y));
            color(r, {.9, .9, .9, 1});
            label(x + 6, y + 2, caption, float(w.size.x) - 12);
            color(r, {.4, .4, .4, 1});
            panel(r, x, y + 27, float(w.size.x), 3);
            color(r, {.9, .65, .2, 1});
            const float value = float((w.value - w.minimum) / (w.maximum - w.minimum));
            panel(r, x + float(w.size.x) * value - 3, y + 23, 6, 11);
        } else if (w.button) {
            float x = left + std::clamp(float(w.position.x), 8.0f,
                                        std::max(8.0f, vw - float(w.size.x) - 8)),
                  y = top + std::clamp(float(w.position.y), 8.0f,
                                       std::max(8.0f, vh - float(w.size.y) - 8));
            color(r, {85 / 255.0, 85 / 255.0, 85 / 255.0, 1});
            panel(r, x, y, float(w.size.x), float(w.size.y));
            color(r, {255 / 255.0, 255 / 255.0, 255 / 255.0, 1});
            label(x + 8, y + 8, w.text, float(w.size.x) - 16);
        } else {
            const float width = std::min(float(w.size.x), std::max(80.0f, vw * .55f)),
                        x = left + vw - width - 20;
            const unsigned columns = static_cast<unsigned>(std::max(10.0f, (width - 16) / 7));
            std::istringstream words(w.text);
            std::string word, line;
            std::vector<std::string> lines;
            while (words >> word) {
                if (line.size() + word.size() + 1 > columns && !line.empty()) {
                    lines.push_back(line);
                    line.clear();
                }
                if (!line.empty())
                    line += ' ';
                line += word;
            }
            if (!line.empty())
                lines.push_back(line);
            color(r, {62 / 255.0, 62 / 255.0, 62 / 255.0, 1});
            panel(r, x, textY, width, float(lines.size()) * 18 + 16);
            color(r, {220 / 255.0, 220 / 255.0, 220 / 255.0, 1});
            for (const auto &s : lines) {
                label(x + 8, textY + 8, s, width - 16);
                textY += 18;
            }
            textY += 28;
        }
    }
    require_sdl(SDL_SetRenderClipRect(r, nullptr), "Restore mechanism clip");
    color(r, {32 / 255.0, 32 / 255.0, 32 / 255.0, 1});
    panel(r, 0, 112, l.left, l.height - 144);
    color(r, {230 / 255.0, 230 / 255.0, 230 / 255.0, 1});
    label(14, 128, "IMPORTED BODIES");
    label(
        14, 150,
        std::to_string(state.bodies.size()) + " bodies / " +
            std::to_string(source_.definition().links.size() + source_.definition().hinges.size() +
                           source_.definition().windings.size() +
                           source_.definition().welds.size() + source_.definition().slides.size()) +
            " joints",
        l.left - 24);
    float row = 176 - list_scroll_;
    for (const auto &b : source_.definition().bodies) {
        if (row < 176) {
            row += 20;
            continue;
        }
        if (row > l.property_y() - 40)
            break;
        color(r, {180 / 255.0, 180 / 255.0, 180 / 255.0, 1});
        label(14, row, b.name + " #" + std::to_string(b.id.value), l.left - 24);
        row += 20;
    }
    color(r, {225 / 255.0, 225 / 255.0, 225 / 255.0, 1});
    label(14, l.property_y() - 24, "PROPERTIES");
    row = l.property_y();
    const auto property = [&](const std::string &s) {
        color(r, {190 / 255.0, 190 / 255.0, 190 / 255.0, 1});
        label(14, row, s, l.left - 24);
        row += 25;
    };
    if (selected_) {
        const auto &b =
            *std::find_if(source_.definition().bodies.begin(), source_.definition().bodies.end(),
                          [&](const auto &v) { return v.id == *selected_; });
        const auto &s = samples.at(*selected_);
        property("Name: " + b.name);
        property("Mass: " + number(b.mass) + " kg");
        property("Radius: " + number(b.radius) + " m");
        property("Position: " + number(s.center.x) + ", " + number(s.center.y));
        property("Velocity: " + number(s.velocity.x) + ", " + number(s.velocity.y));
        property("Restitution: " + number(b.restitution));
        property("Friction: " + number(b.friction));
        property("Damping: " + number(b.damping));
    } else
        property("Click / drag a body to inspect.");
    color(r, {28 / 255.0, 28 / 255.0, 28 / 255.0, 1});
    panel(r, l.left, 90, l.width - l.left, 22);
    color(r, {225 / 255.0, 225 / 255.0, 225 / 255.0, 1});
    for (const auto &tick : ticksX)
        if (tick.major)
            label(left + float(tick.pixel), 94, number(tick.world), 64);
    color(r, {28 / 255.0, 28 / 255.0, 28 / 255.0, 1});
    panel(r, 0, 90, l.left, 22);
    color(r, {225 / 255.0, 225 / 255.0, 225 / 255.0, 1});
    label(12, 90, std::string(running_ ? "RUNNING" : "PAUSED") + "  t=" + number(state.time) + " s",
          l.left - 24);
    const char *buttons[] = {running_ ? "Pause" : "Play",
                             "Step",
                             "Reset",
                             "Add",
                             "Delete",
                             "Undo",
                             "Redo",
                             "Home",
                             "Help"};
    for (int i = 0; i < 9; ++i) {
        color(r, (i < 3 || i == 7) ? scene::Color{70 / 255.0, 70 / 255.0, 70 / 255.0, 1}
                                   : scene::Color{36 / 255.0, 36 / 255.0, 36 / 255.0, 1});
        panel(r, 12 + float(i) * l.button_width(), 46, l.button_width() - 5, 36);
        color(r, {220 / 255.0, 220 / 255.0, 220 / 255.0, 1});
        label(19 + float(i) * l.button_width(), 56, buttons[i], l.button_width() - 14);
    }
    color(r, {24 / 255.0, 24 / 255.0, 24 / 255.0, 1});
    panel(r, 0, l.height - 32, l.width, 32);
    color(r, {210 / 255.0, 210 / 255.0, 210 / 255.0, 1});
    label(12, l.height - 26, status_, l.width - 24);
    require_sdl(SDL_SetRenderScale(r, 1, 1), "Restore source rendering");
}
} // namespace opensim::app
