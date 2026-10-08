#include "editor_ui.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
namespace opensim::app {
namespace {
const char *labels[] = {"Mass (kg)",        "Position X (m)",   "Position Y (m)",
                        "Velocity X (m/s)", "Velocity Y (m/s)", "Radius (m)",
                        "Gravity X (m/s2)", "Gravity Y (m/s2)"};
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
    out << std::setprecision(precision) << n;
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
    objects_tab_ = false;
    field_ = field;
    const float available = host.workspace().property_height();
    panel_scroll_ = std::clamp(panel_scroll_, 0.0f, std::max(0.0f, 352 - available));
    const float top = static_cast<float>(field) * 44;
    if (top < panel_scroll_)
        panel_scroll_ = top;
    if (top + 40 > panel_scroll_ + available)
        panel_scroll_ = top + 40 - available;
    buffer_ = number(value(session, field), 17);
    replace_text_ = true;
    require_sdl(SDL_StartTextInput(host.window()), "Start property input");
    message_ = "Type a number, Enter to apply, Escape to cancel.";
}
bool EditorUI::event(const SDL_Event &e, Session &s, Host &host) {
    if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        return false;
    if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST || e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
        e.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
        report(s.cancel_drag());
        const float available = host.workspace().property_height();
        panel_scroll_ = std::clamp(panel_scroll_, 0.0f, std::max(0.0f, 352 - available));
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
        const double panel = layout.width - layout.right;
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
            if (uix >= panel && uiy >= 120 && uiy < 146 && layout.left == 0) {
                objects_tab_ = uix >= panel + layout.right / 2;
                return true;
            }
            const bool list = (layout.left > 0 && uix < layout.left) ||
                              (layout.left == 0 && objects_tab_ && uix >= panel);
            if (list) {
                if (uiy >= 172 && uiy < layout.height - 46 && s.authoring()) {
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
            if (uix >= panel) {
                const int field = static_cast<int>((uiy - 184 + panel_scroll_) / 44);
                if (uiy >= 184 && uiy < layout.height - 58 && field >= 0 && field < 8)
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
        const float panel = layout.width - layout.right;
        if ((layout.left > 0 && px < layout.left) ||
            (layout.left == 0 && objects_tab_ && px >= panel)) {
            const auto count = s.editing().document().particles().size();
            scene_scroll_ = std::clamp(scene_scroll_ - e.wheel.y * 32, 0.0f,
                                       std::max(0.0f, float(count) * 32 - layout.list_height()));
            return true;
        }
        if (px >= panel) {
            panel_scroll_ = std::clamp(panel_scroll_ - e.wheel.y * 44, 0.0f,
                                       std::max(0.0f, 352 - layout.property_height()));
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
    const float w = layout.width, h = layout.height, panel = w - layout.right;
    const auto clipped_text = [&](float x, float y, const std::string &value, float width) {
        text(r, x, y, value.substr(0, static_cast<std::size_t>(std::max(0.0f, width / 8))));
    };
    color(r, 19, 24, 33);
    box(r, 0, 0, w, 38);
    color(r, 105, 219, 202);
    text(r, 16, 15, "OPEN SIMPHY");
    color(r, 173, 185, 204);
    text(r, 128, 15, "/  Untitled scene");
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
    text(r, layout.left + 12, 97, "2D VIEWPORT");
    color(r, 105, 219, 202);
    clipped_text(layout.left + 122, 97,
                 std::string(s.authoring() ? "EDIT" : (s.running() ? "RUNNING" : "PAUSED")) +
                     "  t=" + number(s.snapshot().time(), 4) + " s",
                 panel - layout.left - 130);
    color(r, 24, 31, 43);
    box(r, panel, 112, layout.right, h - 144);
    if (layout.left > 0)
        box(r, 0, 112, layout.left, h - 144);
    if (layout.left == 0) {
        color(r, objects_tab_ ? 29 : 47, objects_tab_ ? 38 : 64, objects_tab_ ? 52 : 82);
        box(r, panel + 6, 120, layout.right / 2 - 8, 26);
        color(r, objects_tab_ ? 47 : 29, objects_tab_ ? 64 : 38, objects_tab_ ? 82 : 52);
        box(r, panel + layout.right / 2, 120, layout.right / 2 - 6, 26);
        color(r, 222, 233, 246);
        text(r, panel + 12, 129, "Properties");
        text(r, panel + layout.right / 2 + 6, 129, "Objects");
    } else {
        color(r, 216, 227, 242);
        text(r, panel + 14, 130, "PROPERTIES");
    }
    const bool show_list = layout.left > 0 || objects_tab_;
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
            if (y + 30 < 172 || y > h - 46)
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
        text(r, x + 14, h - 44, "N: add / Del: remove");
    }
    if (layout.left > 0 || !objects_tab_) {
        color(r, 134, 152, 178);
        clipped_text(panel + 14, 160,
                     s.editing().selected()
                         ? "Particle #" + std::to_string(s.editing().selected()->value)
                         : "Select an object",
                     layout.right - 28);
        const SDL_Rect clip{static_cast<int>(panel), 182, static_cast<int>(layout.right),
                            static_cast<int>(layout.property_height() + 2)};
        require_sdl(SDL_SetRenderClipRect(r, &clip), "Clip properties");
        const float scroll =
            std::clamp(panel_scroll_, 0.0f, std::max(0.0f, 352 - layout.property_height()));
        for (int i = 0; i < 8; ++i) {
            const float y = 184 + float(i) * 44 - scroll;
            color(r, 151, 169, 193);
            text(r, panel + 14, y, labels[i]);
            if (i == field_)
                color(r, 47, 79, 94);
            else
                color(r, 32, 43, 60);
            box(r, panel + 10, y + 12, layout.right - 20, 28);
            color(r, 230, 238, 249);
            clipped_text(panel + 18, y + 22,
                         i == field_ ? buffer_ + "_"
                                     : ((i < 6 && !selected(s)) ? "--" : number(value(s, i))),
                         layout.right - 36);
        }
        require_sdl(SDL_SetRenderClipRect(r, nullptr), "Restore property clip");
        if (352 > layout.property_height()) {
            const float track = layout.property_height();
            const float thumb = std::max(12.0f, track * track / 352);
            const float offset = (track - thumb) * scroll / (352 - track);
            color(r, 81, 105, 128);
            box(r, w - 5, 184 + offset, 3, thumb);
        }
        color(r, 134, 152, 178);
        text(r, panel + 12, h - 44, "Scroll / Tab: next field");
    }
    color(r, 17, 23, 32);
    box(r, 0, h - 32, w, 32);
    color(r, 196, 211, 230);
    clipped_text(12, h - 20, message_, w - 24);
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
                               "Save/Open and more object types are coming.",
                               "F1, Escape or click to close this guide."};
        for (int i = 0; i < 9; ++i) {
            color(r, 195, 210, 231);
            clipped_text(hx + 20, hy + 54 + float(i) * 20, lines[i], hw - 40);
        }
    }
    require_sdl(SDL_SetRenderScale(r, 1, 1), "Restore physical-pixel rendering");
}
} // namespace opensim::app
