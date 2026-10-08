#include "source_view.hpp"
#include "grid.hpp"
#include "session.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 4096
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
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
} // namespace
SourceView::SourceView(compat::MechanicalSource s)
    : source_(std::move(s)), camera_(source_.camera_center()), scale_(source_.camera_scale()) {
    std::size_t pixels = 0;
    for (const auto &[name, bytes] : source_.images()) {
        int w = 0, h = 0, channels = 0;
        if (!stbi_info_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h,
                                   &channels) ||
            w <= 0 || h <= 0 || w > 4096 || h > 4096 ||
            std::size_t(w) * h > 16 * 1024 * 1024 - pixels)
            throw std::runtime_error("Source image dimension budget");
        pixels += std::size_t(w) * h;
        std::unique_ptr<unsigned char, decltype(&stbi_image_free)> image(
            stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &channels,
                                  4),
            stbi_image_free);
        if (!image)
            throw std::runtime_error("Decode source image failed");
        decoded_.emplace(
            name, DecodedImage{w, h,
                               std::vector<std::uint8_t>(image.get(),
                                                         image.get() + std::size_t(w) * h * 4)});
    }
    replace_world();
}
void SourceView::replace_world() {
    auto w = physics::Mechanism::create(source_.definition());
    if (w.error())
        throw std::runtime_error(w.error()->message);
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
            auto state = world_->snapshot();
            const auto &current = *std::find_if(state.bodies.begin(), state.bodies.end(),
                                                [&](const auto &b) { return b.id == *dragging_; });
            const double desired = std::atan2(target.x - j.local_b.x, j.local_b.y - target.y);
            const double initial =
                std::atan2(current.center.x - j.local_b.x, j.local_b.y - current.center.y);
            const double delta = std::remainder(desired - initial, 2 * std::numbers::pi);
            const unsigned count = static_cast<unsigned>(std::ceil(std::abs(delta) / .01));
            for (unsigned i = 1; i <= count; ++i) {
                const double a = initial + delta * i / count;
                const math::Vec2 p{j.local_b.x + j.length * std::sin(a),
                                   j.local_b.y - j.length * std::cos(a)};
                checked(world_->relocate(*dragging_, p));
                auto moved = world_->snapshot();
                const auto &b = *std::find_if(moved.bodies.begin(), moved.bodies.end(),
                                              [&](const auto &v) { return v.id == *dragging_; });
                if (std::hypot(b.center.x - p.x, b.center.y - p.y) > 1e-6)
                    break;
            }
            return;
        }
    checked(world_->relocate(*dragging_, target));
}
void SourceView::zoom_at(double steps, math::Vec2 pointer, const Workspace &layout) {
    const auto before = local(pointer, layout);
    scale_ = std::clamp(scale_ * std::pow(1.2, steps), 1.0, 4096.0);
    const auto after = local(pointer, layout);
    camera_ = camera_ + before - after;
}
bool SourceView::event(const SDL_Event &e, Host &host) {
    const auto l = host.workspace();
    const auto action = [&](int button) {
        if (button == 0) {
            running_ = !running_;
            remainder_ = 0;
        } else if (button == 1) {
            running_ = false;
            checked(world_->step());
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
            pan_button_ = 0;
            SDL_CaptureMouse(false);
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
            if (!pointer.value() || !l.contains(*pointer.value()) || e.wheel.y == 0)
                return true;
            zoom_at(e.wheel.y, *pointer.value(), l);
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
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                (e.button.button == SDL_BUTTON_RIGHT || e.button.button == SDL_BUTTON_MIDDLE) &&
                l.contains(p)) {
                pan_button_ = e.button.button;
                pan_pointer_ = p;
                SDL_CaptureMouse(true);
                return true;
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == pan_button_) {
                pan_button_ = 0;
                SDL_CaptureMouse(false);
                return true;
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION && pan_button_) {
                camera_.x -= (p.x - pan_pointer_.x) / (scale_ * l.scale);
                camera_.y += (p.y - pan_pointer_.y) / (scale_ * l.scale);
                pan_pointer_ = p;
                return true;
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
                if (dragging_) {
                    drag(local(p, l));
                    dragging_.reset();
                    SDL_CaptureMouse(false);
                    remainder_ = 0;
                }
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
                for (const auto &w : source_.widgets())
                    if (w.button) {
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
                        if (found != defs.end() && !found->static_body &&
                            std::hypot(world.x - b.center.x, world.y - b.center.y) <=
                                found->radius) {
                            selected_ = b.id;
                            dragging_ = b.id;
                            SDL_CaptureMouse(true);
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
void SourceView::tick(double dt) {
    if (!running_)
        return;
    remainder_ += std::clamp(dt, 0.0, .25);
    unsigned steps = 0;
    while (remainder_ >= source_.definition().fixed_dt && steps++ < 32) {
        auto r = world_->step();
        if (r.error()) {
            status_ = r.error()->message;
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
        std::vector<SDL_Vertex> vertices{{center, fill, {.5f, .5f}}};
        std::vector<int> indices;
        std::vector<SDL_FPoint> outline;
        for (int i = 0; i <= 64; ++i) {
            double a = 2 * std::numbers::pi * i / 64;
            float cx = float(std::cos(a)), cy = float(std::sin(a));
            SDL_FPoint p{center.x + radius * cx, center.y + radius * cy};
            vertices.push_back({p, fill, {.5f + .5f * cx, .5f + .5f * cy}});
            outline.push_back(p);
            if (i < 64) {
                indices.push_back(0);
                indices.push_back(i + 1);
                indices.push_back(i + 2);
            }
        }
        SDL_Texture *image = style.image.empty() ? nullptr : images_.at(style.image).get();
        require_sdl(SDL_RenderGeometry(r, image, vertices.data(), static_cast<int>(vertices.size()),
                                       indices.data(), static_cast<int>(indices.size())),
                    "Draw textured circle");
        color(r, selected_ == b.id ? scene::Color{255 / 255.0, 255 / 255.0, 255 / 255.0, 1}
                                   : style.outline);
        require_sdl(SDL_RenderLines(r, outline.data(), static_cast<int>(outline.size())),
                    "Draw circle outline");
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
        for (int offset = -1; offset <= 1; ++offset)
            require_sdl(SDL_RenderLine(r, pa.x + float(offset), pa.y, pb.x + float(offset), pb.y),
                        "Draw suspension");
        panel(r, pb.x - 2, pb.y - 2, 4, 4);
    }
    float textY = top + 136;
    for (const auto &w : source_.widgets()) {
        if (w.button) {
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
    label(14, 150,
          std::to_string(state.bodies.size()) + " circles / " +
              std::to_string(source_.definition().links.size()) + " distance joints",
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
        property("Click / drag a ball to inspect.");
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
