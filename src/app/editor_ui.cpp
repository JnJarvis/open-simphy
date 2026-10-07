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
    field_ = field;
    const auto scale = std::max(1.0f, SDL_GetWindowDisplayScale(host.window()));
    const float available = static_cast<float>(host.extent().height) / scale - 222;
    panel_scroll_ = std::clamp(panel_scroll_, 0.0f, std::max(0.0f, 384 - available));
    const float top = static_cast<float>(field) * 48;
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
        const float scale = std::max(1.0f, SDL_GetWindowDisplayScale(host.window()));
        const float available = static_cast<float>(host.extent().height) / scale - 222;
        panel_scroll_ = std::clamp(panel_scroll_, 0.0f, std::max(0.0f, 384 - available));
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
            if (s.editing().dragging()) {
                report(s.cancel_drag());
                return true;
            }
            return false;
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
        const float scale = SDL_GetWindowDisplayScale(host.window());
        const double uix = last_pointer_.x / scale, uiy = last_pointer_.y / scale;
        const double panel = double(host.extent().width) / scale - 280;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            end_text(host);
            if (uiy < 60) {
                const double step = std::min(100.0, double(host.extent().width) / scale / 6);
                const int action = static_cast<int>(uix / step);
                if (action == 0)
                    report(s.set_running(!s.running()));
                else if (action == 1) {
                    report(s.set_running(false));
                    report(s.advance());
                } else if (action == 2)
                    report(s.reset());
                else if (action == 3)
                    report(s.apply(s.editing().add({1, s.editing().view().center, {0, 0}, .2})));
                else if (action == 4)
                    report(s.apply(s.editing().undo()));
                else if (action == 5)
                    report(s.apply(s.editing().redo()));
                return true;
            }
            if (uix >= panel) {
                const int field = static_cast<int>((uiy - 160 + panel_scroll_) / 48);
                if (uiy >= 160 && uiy < double(host.extent().height) / scale - 62 && field >= 0 &&
                    field < 8)
                    edit_field(field, s, host);
                return true;
            }
            if (!s.authoring()) {
                message_ = "Reset to select and edit the initial scene.";
                return true;
            }
            const auto hit = s.editing().hit_test(last_pointer_);
            if (hit.error()) {
                message_ = hit.error()->message;
                return true;
            }
            report(s.apply(s.editing().select(*hit.value())));
            if (s.editing().selected())
                report(s.apply(s.editing().begin_drag(last_pointer_)));
        } else if (e.type == SDL_EVENT_MOUSE_MOTION && s.editing().dragging())
            report(s.apply(s.editing().update_drag(last_pointer_)));
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT &&
                 s.editing().dragging()) {
            const auto updated = s.apply(s.editing().update_drag(last_pointer_));
            if (updated.error()) {
                report(updated);
                return true;
            }
            report(s.apply(s.editing().commit_drag()));
        }
        return true;
    }
    if (e.type == SDL_EVENT_MOUSE_WHEEL) {
        const float scale = std::max(1.0f, SDL_GetWindowDisplayScale(host.window()));
        const float panel = static_cast<float>(host.extent().width) / scale - 280;
        const float available = static_cast<float>(host.extent().height) / scale - 222;
        if (last_pointer_.x / scale >= panel) {
            panel_scroll_ =
                std::clamp(panel_scroll_ - e.wheel.y * 48, 0.0f, std::max(0.0f, 384 - available));
            return true;
        }
        auto view = s.editing().view();
        view.pixels_per_meter =
            std::clamp(view.pixels_per_meter * (e.wheel.y > 0 ? 1.25 : (e.wheel.y < 0 ? .8 : 1)),
                       1.0 / 1024, 4096.0);
        report(s.navigate(view));
    }
    return true;
}
void EditorUI::paint(SDL_Renderer *r, const Session &s, renderer::Extent extent,
                     float scale) const {
    if (!std::isfinite(scale) || scale <= 0)
        scale = 1;
    require_sdl(SDL_SetRenderScale(r, scale, scale), "Scale editor UI");
    const float w = static_cast<float>(extent.width) / scale,
                h = static_cast<float>(extent.height) / scale, panel = w - 280;
    color(r, 25, 33, 47);
    box(r, 0, 0, w, 60);
    box(r, panel, 60, 280, h - 60);
    const char *buttons[] = {
        s.running() ? "Pause" : "Play", "Step", "Reset", "Add", "Undo", "Redo"};
    const float button_width = std::min(100.0f, w / 6);
    for (int i = 0; i < 6; ++i) {
        color(r, 42, 55, 73);
        box(r, float(i) * button_width + 6, 10, button_width - 12, 36);
        color(r, 229, 236, 246);
        text(r, float(i) * button_width + 14, 24, buttons[i]);
    }
    color(r, 128, 209, 250);
    text(r, panel + 16, 80, s.authoring() ? "AUTHORING" : "SIMULATION");
    color(r, 225, 232, 240);
    text(r, panel + 16, 104, "Time: " + number(s.snapshot().time()) + " s");
    text(r, panel + 16, 128,
         s.editing().selected() ? "Particle #" + std::to_string(s.editing().selected()->value)
                                : "No particle selected");
    const SDL_Rect clip{static_cast<int>(panel), 156, 280, std::max(0, static_cast<int>(h) - 218)};
    require_sdl(SDL_SetRenderClipRect(r, &clip), "Clip properties panel");
    const float scroll = std::clamp(panel_scroll_, 0.0f, std::max(0.0f, 384 - (h - 222)));
    for (int i = 0; i < 8; ++i) {
        const float y = 160 + float(i) * 48 - scroll;
        color(r, 155, 170, 190);
        text(r, panel + 16, y, labels[i]);
        color(r, i == field_ ? 53 : 34, i == field_ ? 70 : 43, i == field_ ? 91 : 60);
        box(r, panel + 12, y + 12, 256, 28);
        color(r, 235, 240, 249);
        text(r, panel + 20, y + 22,
             i == field_ ? buffer_ : ((i < 6 && !selected(s)) ? "--" : number(value(s, i))));
    }
    require_sdl(SDL_SetRenderClipRect(r, nullptr), "Restore UI clipping");
    color(r, 155, 170, 190);
    text(r, panel + 16, h - 52, "Scroll for more / Tab: next");
    color(r, 25, 33, 47);
    box(r, 0, h - 30, w, 30);
    color(r, 235, 202, 121);
    const auto max_chars = static_cast<std::size_t>(std::max(0.0f, (w - 24) / 8));
    text(r, 12, h - 20, message_.substr(0, max_chars));
    require_sdl(SDL_SetRenderScale(r, 1, 1), "Restore physical-pixel rendering");
}
} // namespace opensim::app
