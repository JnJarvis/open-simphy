#include "editor_ui.hpp"
#include "grid.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
namespace opensim::app {
namespace {
const char *labels[] = {"Mass (kg)",   "Pos X (m)",  "Pos Y (m)",  "Vel X (m/s)",
                        "Vel Y (m/s)", "Radius (m)", "g X (m/s2)", "g Y (m/s2)"};
const scene::Particle *selected(const Session &s) {
    for (const auto &p : s.editing().display_document().particles())
        if (p.id == s.editing().selected())
            return &p;
    return nullptr;
}
double value(const Session &s, int field) {
    const auto p = selected(s);
    const auto g = s.editing().document().gravity();
    if (field == 6)
        return g.x;
    if (field == 7)
        return g.y;
    if (!p)
        return 0;
    const auto &f = p->fields;
    switch (field) {
    case 0:
        return f.mass;
    case 1:
        return f.initial_position.x;
    case 2:
        return f.initial_position.y;
    case 3:
        return f.initial_velocity.x;
    case 4:
        return f.initial_velocity.y;
    default:
        return f.display_radius;
    }
}
std::string number(double n, int precision = 6) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(precision) << (n == 0 ? 0 : n);
    return out.str();
}
void text(SDL_Renderer *r, float x, float y, const std::string &s) {
    require_sdl(SDL_RenderDebugText(r, x, y, s.c_str()), "Draw editor text");
}
void color(SDL_Renderer *r, Uint8 red, Uint8 green, Uint8 blue) {
    require_sdl(SDL_SetRenderDrawColor(r, red, green, blue, 255), "Set UI color");
}
void box(SDL_Renderer *r, float x, float y, float w, float h) {
    const SDL_FRect rect{x, y, w, h};
    require_sdl(SDL_RenderFillRect(r, &rect), "Draw editor panel");
}
core::Result<void> input_error(const char *message) {
    return core::Result<void>::failure(
        {core::Code::invalid_argument, core::Severity::error, message, {}, "app.property"});
}
} // namespace
void EditorUI::report(const core::Result<void> &result) {
    message_ = result.error() ? result.error()->message : "Ready";
}
void EditorUI::end_text(Host &host) {
    if (field_ >= 0)
        require_sdl(SDL_StopTextInput(host.window()), "Stop property input");
    field_ = -1;
    buffer_.clear();
}
void EditorUI::edit_field(int field, Session &session, Host &host) {
    if (!session.authoring()) {
        message_ = "Reset before editing properties.";
        return;
    }
    if (field < 6 && !selected(session)) {
        message_ = "Select a particle first.";
        return;
    }
    field_ = field;
    const float available = host.workspace().property_height();
    panel_scroll_ = std::clamp(panel_scroll_, 0.0f, std::max(0.0f, 224 - available));
    const float top = static_cast<float>(field) * 28;
    if (top < panel_scroll_)
        panel_scroll_ = top;
    if (top + 28 > panel_scroll_ + available)
        panel_scroll_ = top + 28 - available;
    buffer_ = number(value(session, field), 17);
    replace_text_ = true;
    require_sdl(SDL_StartTextInput(host.window()), "Start property input");
    message_ = "Type a number, Enter to apply, Escape to cancel.";
}
bool EditorUI::open_file(const std::string &path, Session &session) {
    auto result = read_project(utf8_path(path));
    if (result.error()) {
        message_ = result.error()->message;
        return false;
    }
    auto next = std::move(*result.value());
    report(session.cancel_drag());
    report(session.set_running(false));
    source_ = std::move(next);
    source_scroll_ = 0;
    message_ = "SSIM source opened. Physics support is still being built.";
    return true;
}
void EditorUI::poll_open(Session &session, Host &host) {
    if (auto path = host.take_open(); path && !path->empty()) {
        end_text(host);
        open_file(*path, session);
    }
}
bool EditorUI::event(const SDL_Event &e, Session &s, Host &host) {
    if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        return false;
    if (e.type == SDL_EVENT_DROP_FILE) {
        end_text(host);
        open_file(e.drop.data, s);
        return true;
    }
    if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat && e.key.key == SDLK_O &&
        (e.key.mod & SDL_KMOD_CTRL)) {
        end_text(host);
        host.request_open();
        return true;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
        int window_w = 0, window_h = 0;
        require_sdl(SDL_GetWindowSize(host.window(), &window_w, &window_h),
                    "Query Open button coordinates");
        const auto l = host.workspace();
        const auto point = physical_pointer({e.button.x, e.button.y},
                                            {double(window_w), double(window_h)}, host.extent());
        if (point.value() && point.value()->x / l.scale >= 12 && point.value()->x / l.scale < 92 &&
            point.value()->y / l.scale >= 4 && point.value()->y / l.scale < 32) {
            end_text(host);
            host.request_open();
            return true;
        }
    }
    if (source_) {
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
            source_.reset();
            message_ = "Returned to authored scene.";
        }
        if (e.type == SDL_EVENT_MOUSE_WHEEL)
            source_scroll_ = std::max(0.0f, source_scroll_ - e.wheel.y * 24);
        return true;
    }
    if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST || e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
        e.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
        report(s.cancel_drag());
        const float available = host.workspace().property_height();
        panel_scroll_ = std::clamp(panel_scroll_, 0.0f, std::max(0.0f, 224 - available));
        end_text(host);
        report(s.resize(host.canvas_extent()));
        return true;
    }
    if (e.type == SDL_EVENT_TEXT_INPUT && field_ >= 0) {
        if (replace_text_) {
            buffer_.clear();
            replace_text_ = false;
        }
        for (const char *p = e.text.text; *p && buffer_.size() < 64; ++p)
            if ((*p >= '0' && *p <= '9') || *p == '+' || *p == '-' || *p == '.' || *p == 'e' ||
                *p == 'E')
                buffer_ += *p;
        return true;
    }
    if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
        const auto key = e.key.key;
        const bool ctrl = (e.key.mod & SDL_KMOD_CTRL) != 0;
        if (help_) {
            if (key == SDLK_ESCAPE || key == SDLK_F1)
                help_ = false;
            return true;
        }
        if (field_ >= 0) {
            if (key == SDLK_ESCAPE) {
                end_text(host);
                message_ = "Property edit cancelled.";
            } else if (key == SDLK_TAB) {
                const int next = (field_ + 1) % 8;
                end_text(host);
                edit_field(next, s, host);
            } else if (key == SDLK_BACKSPACE) {
                if (replace_text_)
                    buffer_.clear();
                else if (!buffer_.empty())
                    buffer_.pop_back();
                replace_text_ = false;
            } else if (ctrl && key == SDLK_A)
                replace_text_ = true;
            else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                double parsed = 0;
                std::istringstream input(buffer_);
                input.imbue(std::locale::classic());
                if (!(input >> parsed) || !std::isfinite(parsed) ||
                    (input >> std::ws && !input.eof())) {
                    report(input_error("Enter a finite number."));
                    return true;
                }
                if (field_ >= 6) {
                    auto g = s.editing().document().gravity();
                    if (field_ == 6)
                        g.x = parsed;
                    else
                        g.y = parsed;
                    report(s.apply(s.editing().set_gravity(g)));
                } else if (const auto p = selected(s)) {
                    auto f = p->fields;
                    switch (field_) {
                    case 0:
                        f.mass = parsed;
                        break;
                    case 1:
                        f.initial_position.x = parsed;
                        break;
                    case 2:
                        f.initial_position.y = parsed;
                        break;
                    case 3:
                        f.initial_velocity.x = parsed;
                        break;
                    case 4:
                        f.initial_velocity.y = parsed;
                        break;
                    default:
                        f.display_radius = parsed;
                    }
                    report(s.apply(s.editing().replace(p->id, f)));
                }
                if (message_ == "Ready")
                    end_text(host);
            }
            return true;
        }
        if (key == SDLK_ESCAPE) {
            if (help_) {
                help_ = false;
                return true;
            }
            if (s.editing().dragging()) {
                report(s.cancel_drag());
                return true;
            }
            return false;
        }
        if (key == SDLK_F1) {
            action(8, s, host);
            return true;
        }
        if (key == SDLK_HOME) {
            action(7, s, host);
            return true;
        }
        if (key == SDLK_SPACE)
            report(s.set_running(!s.running()));
        else if (key == SDLK_RIGHT) {
            report(s.set_running(false));
            report(s.advance());
        } else if (key == SDLK_R)
            report(s.reset());
        else if (ctrl && key == SDLK_Z)
            report(s.apply(s.editing().undo()));
        else if (ctrl && key == SDLK_Y)
            report(s.apply(s.editing().redo()));
        else if (key == SDLK_DELETE && s.editing().selected())
            report(s.apply(s.editing().erase(*s.editing().selected())));
        else if (key == SDLK_N)
            report(s.apply(s.editing().add({1, s.editing().view().center, {0, 0}, .2})));
        else if (key == SDLK_TAB)
            edit_field(0, s, host);
        else if (key == SDLK_W || key == SDLK_A || key == SDLK_S || key == SDLK_D) {
            auto v = s.editing().view();
            const double delta = 40 / v.pixels_per_meter;
            if (key == SDLK_W)
                v.center.y += delta;
            if (key == SDLK_S)
                v.center.y -= delta;
            if (key == SDLK_A)
                v.center.x -= delta;
            if (key == SDLK_D)
                v.center.x += delta;
            report(s.navigate(v));
        }
        return true;
    }
    if (e.type == SDL_EVENT_MOUSE_MOTION || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        int w = 0, h = 0;
        require_sdl(SDL_GetWindowSize(host.window(), &w, &h), "Query window coordinates");
        const math::Vec2 raw = e.type == SDL_EVENT_MOUSE_MOTION
                                   ? math::Vec2{e.motion.x, e.motion.y}
                                   : math::Vec2{e.button.x, e.button.y};
        const auto mapped = physical_pointer(raw, {double(w), double(h)}, host.extent());
        if (mapped.error()) {
            message_ = mapped.error()->message;
            return true;
        }
        last_pointer_ = *mapped.value();
        const auto layout = host.workspace();
        const double uix = last_pointer_.x / layout.scale, uiy = last_pointer_.y / layout.scale;
        const auto local = layout.local(last_pointer_);
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            if (help_) {
                help_ = false;
                return true;
            }
            end_text(host);
            if (uiy >= 46 && uiy < 82 && uix >= 12 && layout.button_width() > 0) {
                const int button = static_cast<int>((uix - 12) / layout.button_width());
                if (button >= 0 && button < 9 &&
                    uix < 12 + button * layout.button_width() + layout.button_width() - 5)
                    action(button, s, host);
                return true;
            }
            const bool list = uix < layout.left && uiy < 172 + layout.list_height();
            if (list) {
                if (uiy >= 172 && uiy < 172 + layout.list_height() && s.authoring()) {
                    const auto particles = s.editing().document().particles();
                    scene_scroll_ = std::clamp(
                        scene_scroll_, 0.0f,
                        std::max(0.0f, float(particles.size()) * 32 - layout.list_height()));
                    const auto index = static_cast<std::size_t>((uiy - 172 + scene_scroll_) / 32);
                    if (index < particles.size())
                        report(s.apply(s.editing().select(particles[index].id)));
                }
                return true;
            }
            if (uix < layout.left) {
                const int field =
                    static_cast<int>((uiy - layout.property_y() + panel_scroll_) / 28);
                if (uiy >= layout.property_y() && uiy < layout.height - 58 && field >= 0 &&
                    field < 8)
                    edit_field(field, s, host);
                return true;
            }
            if (!layout.contains(last_pointer_))
                return true;
            if (!s.authoring()) {
                message_ = "Reset to select and edit the initial scene.";
                return true;
            }
            const auto hit = s.editing().hit_test(local);
            if (hit.error()) {
                message_ = hit.error()->message;
                return true;
            }
            report(s.apply(s.editing().select(*hit.value())));
            if (s.editing().selected())
                report(s.apply(s.editing().begin_drag(local)));
        } else if (e.type == SDL_EVENT_MOUSE_MOTION && s.editing().dragging())
            report(s.apply(s.editing().update_drag(local)));
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT &&
                 s.editing().dragging()) {
            const auto updated = s.apply(s.editing().update_drag(local));
            if (updated.error()) {
                report(updated);
                return true;
            }
            report(s.apply(s.editing().commit_drag()));
        }
        return true;
    }
    if (e.type == SDL_EVENT_MOUSE_WHEEL) {
        if (help_)
            return true;
        const auto layout = host.workspace();
        const double px = last_pointer_.x / layout.scale, py = last_pointer_.y / layout.scale;
        if (py < 112 || py >= layout.height - 32)
            return true;
        if (px < layout.left && py < 172 + layout.list_height()) {
            const auto count = s.editing().document().particles().size();
            scene_scroll_ = std::clamp(scene_scroll_ - e.wheel.y * 32, 0.0f,
                                       std::max(0.0f, float(count) * 32 - layout.list_height()));
            return true;
        }
        if (px < layout.left) {
            panel_scroll_ = std::clamp(panel_scroll_ - e.wheel.y * 28, 0.0f,
                                       std::max(0.0f, 224 - layout.property_height()));
            return true;
        }
        if (!layout.contains(last_pointer_))
            return true;
        auto view = s.editing().view();
        view.pixels_per_meter =
            std::clamp(view.pixels_per_meter * (e.wheel.y > 0 ? 1.25 : (e.wheel.y < 0 ? .8 : 1)),
                       1.0 / 1024, 4096.0);
        report(s.navigate(view));
    }
    return true;
}
void EditorUI::action(int action, Session &s, Host &host) {
    end_text(host);
    if (action == 0)
        report(s.set_running(!s.running()));
    else if (action == 1) {
        report(s.set_running(false));
        report(s.advance());
    } else if (action == 2)
        report(s.reset());
    else if (action == 3)
        report(s.apply(s.editing().add({1, s.editing().view().center, {0, 0}, .2})));
    else if (action == 4 && s.editing().selected())
        report(s.apply(s.editing().erase(*s.editing().selected())));
    else if (action == 5 && s.editing().can_undo())
        report(s.apply(s.editing().undo()));
    else if (action == 6 && s.editing().can_redo())
        report(s.apply(s.editing().redo()));
    else if (action == 7) {
        auto view = s.editing().view();
        view.center = {0, 0};
        view.pixels_per_meter = 80;
        report(s.navigate(view));
    } else if (action == 8) {
        report(s.cancel_drag());
        help_ = !help_;
    }
}
void EditorUI::paint(SDL_Renderer *r, const Session &s, renderer::Extent extent,
                     float scale) const {
    const auto layout = Workspace::layout(extent, scale);
    scale = layout.scale;
    require_sdl(SDL_SetRenderScale(r, scale, scale), "Scale workspace");
    const float w = layout.width, h = layout.height, panel = 0;
    const auto clipped_text = [&](float x, float y, const std::string &value, float width) {
        text(r, x, y, value.substr(0, static_cast<std::size_t>(std::max(0.0f, width / 8))));
    };
    color(r, 19, 24, 33);
    box(r, 0, 0, w, 38);
    color(r, 105, 219, 202);
    text(r, 16, 15, "Open...");
    color(r, 173, 185, 204);
    text(r, 128, 15, source_ ? "/  SSIM source" : "/  Untitled scene");
    color(r, 107, 123, 145);
    text(r, w - 144, 15, "WORKSPACE PREVIEW");
    color(r, 29, 36, 49);
    box(r, 0, 38, w, 52);
    const char *buttons[] = {s.running() ? "Pause" : "Play",
                             "Step",
                             "Reset",
                             "Add",
                             "Delete",
                             "Undo",
                             "Redo",
                             "Home",
                             "Help"};
    const bool active[] = {true,
                           true,
                           true,
                           s.authoring(),
                           s.authoring() && bool(s.editing().selected()),
                           s.authoring() && s.editing().can_undo(),
                           s.authoring() && s.editing().can_redo(),
                           true,
                           true};
    const float bw = layout.button_width();
    for (int i = 0; i < 9; ++i) {
        const float x = 12 + float(i) * bw;
        if (i == 0)
            color(r, 42, 114, 102);
        else if (active[i])
            color(r, 43, 54, 72);
        else
            color(r, 31, 39, 52);
        box(r, x, 46, bw - 5, 36);
        if (active[i])
            color(r, 230, 238, 249);
        else
            color(r, 100, 113, 133);
        text(r, x + 7, 60, buttons[i]);
    }
    color(r, 23, 29, 40);
    box(r, 0, 90, w, 22);
    color(r, 145, 161, 184);
    text(r, layout.left + 12, 90, "2D");
    color(r, 105, 219, 202);
    clipped_text(layout.left + 50, 90,
                 std::string(s.authoring() ? "EDIT" : (s.running() ? "RUNNING" : "PAUSED")) +
                     "  t=" + number(s.snapshot().time(), 4) + " s",
                 w - layout.left - 130);
    // Rulers use physical camera coordinates, just like picking and the grid.
    const auto view = s.editing().view();
    color(r, 32, 34, 37);
    box(r, layout.left, 112, 64, h - 144);
    color(r, 190, 194, 199);
    for (const auto &tick : grid_ticks(view.center.x, view.pixels_per_meter, layout.canvas.width))
        if (tick.major)
            text(r, (float(layout.x) + float(tick.pixel)) / scale, 103, number(tick.world, 3));
    for (const auto &tick : grid_ticks(-view.center.y, view.pixels_per_meter, layout.canvas.height))
        if (tick.major)
            clipped_text(layout.left + 2, (float(layout.y) + float(tick.pixel)) / scale,
                         number(-tick.world, 3), 62);
    color(r, 24, 31, 43);
    box(r, panel, 112, layout.right, h - 144);
    if (layout.left > 0)
        box(r, 0, 112, layout.left, h - 144);
    color(r, 216, 227, 242);
    text(r, 14, layout.property_y() - 36, "PROPERTIES");
    const bool show_list = layout.left > 0;
    if (show_list) {
        const float x = layout.left > 0 ? 0 : panel;
        const float width = layout.left > 0 ? layout.left : layout.right;
        color(r, 216, 227, 242);
        if (layout.left > 0)
            text(r, x + 14, 130, "SCENE OBJECTS");
        color(r, 134, 152, 178);
        const auto particles = s.editing().document().particles();
        text(r, x + 14, 154, std::to_string(particles.size()) + " particles");
        const SDL_Rect clip{static_cast<int>(x), 172, static_cast<int>(width),
                            static_cast<int>(layout.list_height())};
        require_sdl(SDL_SetRenderClipRect(r, &clip), "Clip object list");
        const float scroll =
            std::clamp(scene_scroll_, 0.0f,
                       std::max(0.0f, float(particles.size()) * 32 - layout.list_height()));
        for (std::size_t i = 0; i < particles.size(); ++i) {
            const float y = 172 + float(i) * 32 - scroll;
            if (y + 30 < 172 || y > 172 + layout.list_height())
                continue;
            const bool chosen = s.editing().selected() == particles[i].id;
            if (chosen) {
                color(r, 43, 68, 84);
                box(r, x + 6, y, width - 12, 28);
            }
            color(r, 94, 208, 219);
            box(r, x + 14, y + 10, 6, 6);
            color(r, chosen ? 237 : 165, chosen ? 243 : 184, chosen ? 252 : 205);
            clipped_text(x + 28, y + 10, "Particle " + std::to_string(particles[i].id.value),
                         width - 38);
        }
        require_sdl(SDL_SetRenderClipRect(r, nullptr), "Restore list clip");
        if (particles.empty()) {
            color(r, 134, 152, 178);
            text(r, x + 14, 184, "Add your first particle");
        }
        color(r, 134, 152, 178);
    }
    if (layout.left > 0) {
        color(r, 134, 152, 178);
        clipped_text(panel + 14, layout.property_y() - 16,
                     s.editing().selected()
                         ? "Particle #" + std::to_string(s.editing().selected()->value)
                         : "Select an object",
                     layout.right - 28);
        const SDL_Rect clip{static_cast<int>(panel), static_cast<int>(layout.property_y()),
                            static_cast<int>(layout.right),
                            static_cast<int>(layout.property_height() + 2)};
        require_sdl(SDL_SetRenderClipRect(r, &clip), "Clip properties");
        const float scroll =
            std::clamp(panel_scroll_, 0.0f, std::max(0.0f, 224 - layout.property_height()));
        for (int i = 0; i < 8; ++i) {
            const float y = layout.property_y() + float(i) * 28 - scroll;
            color(r, 151, 169, 193);
            clipped_text(panel + 10, y + 9, labels[i], layout.right * .53f - 14);
            if (i == field_)
                color(r, 47, 79, 94);
            else
                color(r, 32, 43, 60);
            box(r, panel + layout.right * .53f, y + 2, layout.right * .47f - 10, 24);
            color(r, 230, 238, 249);
            clipped_text(panel + layout.right * .53f + 6, y + 9,
                         i == field_ ? buffer_ + "_"
                                     : ((i < 6 && !selected(s)) ? "--" : number(value(s, i))),
                         layout.right * .47f - 22);
        }
        require_sdl(SDL_SetRenderClipRect(r, nullptr), "Restore property clip");
        if (224 > layout.property_height()) {
            const float track = layout.property_height();
            const float thumb = std::max(12.0f, track * track / 224);
            const float offset = (track - thumb) * scroll / (224 - track);
            color(r, 81, 105, 128);
            box(r, layout.left - 5, layout.property_y() + offset, 3, thumb);
        }
        color(r, 134, 152, 178);
        text(r, panel + 12, h - 44, "Scroll / Tab: next field");
    }
    color(r, 17, 23, 32);
    box(r, 0, h - 32, w, 32);
    color(r, 196, 211, 230);
    clipped_text(12, h - 20, message_, w - 24);
    if (source_) {
        paint_source(r, extent, scale);
        require_sdl(SDL_SetRenderScale(r, 1, 1), "Restore source scale");
        return;
    }
    if (help_) {
        const float hw = std::min(520.0f, w - 32), hx = (w - hw) / 2,
                    hy = std::max(116.0f, (h - 264) / 2);
        color(r, 55, 76, 97);
        box(r, hx - 1, hy - 1, hw + 2, 254);
        color(r, 25, 35, 49);
        box(r, hx, hy, hw, 252);
        color(r, 105, 219, 202);
        text(r, hx + 20, hy + 20, "YOUR WORKSPACE");
        const char *lines[] = {"Click a particle or object row to select.",
                               "Drag in the viewport to move a particle.",
                               "Space: play/pause    Right: one step",
                               "Reset: return to editing the starting scene",
                               "N: add   Delete: remove   Ctrl+Z/Y: history",
                               "Wheel: zoom   WASD: pan   Home: reset view",
                               "Properties: type, Enter applies, Esc cancels",
                               "Ctrl+O: open SSIM source / drop a project",
                               "F1, Escape or click to close this guide."};
        for (int i = 0; i < 9; ++i) {
            color(r, 195, 210, 231);
            clipped_text(hx + 20, hy + 54 + float(i) * 20, lines[i], hw - 40);
        }
    }
    require_sdl(SDL_SetRenderScale(r, 1, 1), "Restore physical-pixel rendering");
}
void EditorUI::paint_source(SDL_Renderer *r, renderer::Extent extent, float scale) const {
    const auto l = Workspace::layout(extent, scale);
    const auto &p = *source_;
    const float sidebar = std::min(340.0f, l.width * .45f);
    color(r, 24, 29, 35);
    box(r, 0, 38, l.width, l.height - 38);
    color(r, 224, 231, 239);
    const auto label = [&](float x, float y, const std::string &value, float width) {
        std::string safe = value;
        for (auto &c : safe)
            if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) >= 127)
                c = '?';
        text(r, x, y, safe.substr(0, static_cast<std::size_t>(std::max(0.0f, width / 8))));
    };
    label(12, 50, p.title.empty() ? "Untitled SSIM" : p.title, l.width - 24);
    color(r, 244, 190, 93);
    text(r, 12, 74, "SOURCE PREVIEW / simulation unavailable");
    color(r, 31, 34, 38);
    box(r, sidebar, 100, l.width - sidebar, l.height - 132);
    std::vector<std::string> rows{"Version: " + p.version,
                                  "Archive members: " + std::to_string(p.members.size()),
                                  "Fixture outlines: " + std::to_string(p.outlines.size()),
                                  "Omitted outlines: " + std::to_string(p.omitted_outlines),
                                  "Nonempty scripts: " + std::to_string(p.scripts),
                                  "SHAPES"};
    for (const auto &[name, count] : p.shapes)
        rows.push_back(name + ": " + std::to_string(count));
    rows.push_back("JOINTS");
    for (const auto &[name, count] : p.joints)
        rows.push_back(name + ": " + std::to_string(count));
    rows.push_back("ALL SOURCE ELEMENTS");
    for (const auto &[name, count] : p.elements)
        rows.push_back(name + ": " + std::to_string(count));
    rows.push_back("ARCHIVE MEMBERS");
    rows.insert(rows.end(), p.members.begin(), p.members.end());
    rows.push_back("PREVIEW LIMITATIONS");
    rows.insert(rows.end(), p.diagnostics.begin(), p.diagnostics.end());
    const SDL_Rect clip{0, 100, static_cast<int>(sidebar),
                        static_cast<int>(std::max(0.0f, l.height - 132))};
    require_sdl(SDL_SetRenderClipRect(r, &clip), "Clip source report");
    const float scroll = std::clamp(source_scroll_, 0.0f,
                                    std::max(0.0f, float(rows.size()) * 24 - (l.height - 132)));
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const float y = 112 + float(i) * 24 - scroll;
        if (y < 88 || y > l.height - 32)
            continue;
        color(r, 186, 202, 218);
        label(12, y, rows[i], sidebar - 20);
    }
    require_sdl(SDL_SetRenderClipRect(r, nullptr), "Restore source clip");
    if (!p.outlines.empty()) {
        double xmin = 1e30, xmax = -1e30, ymin = 1e30, ymax = -1e30;
        for (const auto &shape : p.outlines)
            for (auto v : shape.points) {
                xmin = std::min(xmin, v.x);
                xmax = std::max(xmax, v.x);
                ymin = std::min(ymin, v.y);
                ymax = std::max(ymax, v.y);
            }
        const double factor =
            std::min(std::max(1.0f, l.width - sidebar - 40) / std::max(1.0, xmax - xmin),
                     std::max(1.0f, l.height - 180) / std::max(1.0, ymax - ymin));
        const auto point = [&](math::Vec2 v) {
            return SDL_FPoint{
                float(sidebar + (l.width - sidebar) / 2 + (v.x - (xmin + xmax) / 2) * factor),
                float(100 + (l.height - 132) / 2 - (v.y - (ymin + ymax) / 2) * factor)};
        };
        color(r, 84, 199, 230);
        for (const auto &shape : p.outlines) {
            std::vector<SDL_FPoint> points;
            for (auto v : shape.points)
                points.push_back(point(v));
            if (shape.closed)
                points.push_back(points.front());
            require_sdl(SDL_RenderLines(r, points.data(), static_cast<int>(points.size())),
                        "Draw source outlines");
        }
    } else {
        color(r, 190, 202, 218);
        label(sidebar + 16, 120, "No supported fixture outlines", l.width - sidebar - 32);
    }
    color(r, 20, 24, 29);
    box(r, 0, l.height - 32, l.width, 32);
    color(r, 213, 224, 236);
    label(12, l.height - 20, "Ctrl+O: open / Esc: return / Wheel: report | " + message_,
          l.width - 24);
}
} // namespace opensim::app
