#include "editor_ui.hpp"
#include "sdl_host.hpp"
#include "session.hpp"
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
using namespace opensim;
namespace {
void checked(const core::Result<void> &result) {
    if (result.error())
        throw std::runtime_error(result.error()->message);
}
bool events(app::Session &session, app::EditorUI &ui, app::Host &host) {
    SDL_Event event{};
    while (SDL_PollEvent(&event))
        if (!ui.event(event, session, host))
            return false;
    return true;
}
void key(SDL_Keycode code, app::Session &session, app::EditorUI &ui, app::Host &host,
         SDL_Keymod mod = 0) {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = code;
    event.key.down = true;
    event.key.mod = mod;
    app::require_sdl(SDL_PushEvent(&event), "Queue smoke key");
    if (!events(session, ui, host))
        throw std::runtime_error("Unexpected close during smoke");
}
void expect(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void smoke(app::Session &session, app::Host &host, app::EditorUI &ui) {
    const auto draw = [&](const char *capture = nullptr) {
        checked(session.draw(host.canvas_extent(), renderer::render, [&](const renderer::Frame &f) {
            return host.present(
                f, capture, false,
                [&](SDL_Renderer *r) {
                    ui.paint(r, session, host.extent(), SDL_GetWindowDisplayScale(host.window()));
                },
                {session.editing().view().center, session.editing().view().pixels_per_meter});
        }));
    };
    events(session, ui, host);
    checked(session.resize(host.canvas_extent()));
    const auto mouse = [&](SDL_EventType type, double px, double py) {
        int w = 0, h = 0;
        app::require_sdl(SDL_GetWindowSize(host.window(), &w, &h), "Smoke window size");
        const auto extent = host.extent();
        SDL_Event e{};
        e.type = type;
        const float x = static_cast<float>(px * w / extent.width),
                    y = static_cast<float>(py * h / extent.height);
        if (type == SDL_EVENT_MOUSE_MOTION) {
            e.motion.x = x;
            e.motion.y = y;
        } else {
            e.button.button = SDL_BUTTON_LEFT;
            e.button.x = x;
            e.button.y = y;
            e.button.down = type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        }
        app::require_sdl(SDL_PushEvent(&e), "Queue editor pointer");
        events(session, ui, host);
    };
    const auto view = session.editing().view();
    const auto layout = host.workspace();
    const double x = layout.x + double(view.width) / 2 - 240,
                 y = layout.y + double(view.height) / 2 - 80;
    mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    expect(session.editing().selected() == core::EntityId{1}, "Editor selection failed");
    mouse(SDL_EVENT_MOUSE_MOTION, x + 40, y - 20);
    mouse(SDL_EVENT_MOUSE_BUTTON_UP, x + 40, y - 20);
    expect(session.editing().document().particles()[0].fields.initial_position ==
               math::Vec2{-2.5, 1.25},
           "Editor drag failed");
    draw("smoke-editor.bmp");
    key(SDLK_Z, session, ui, host, SDL_KMOD_CTRL);
    expect(session.editing().document().particles()[0].fields.initial_position == math::Vec2{-3, 1},
           "Undo failed");
    key(SDLK_Y, session, ui, host, SDL_KMOD_CTRL);
    key(SDLK_Z, session, ui, host, SDL_KMOD_CTRL);
    mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, 40 * layout.scale,
          (layout.property_y() + 14) * layout.scale);
    SDL_Event text{};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.text = "2.5";
    app::require_sdl(SDL_PushEvent(&text), "Queue property value");
    events(session, ui, host);
    key(SDLK_RETURN, session, ui, host);
    expect(session.editing().document().particles()[0].fields.mass == 2.5, "Property entry failed");
    key(SDLK_Z, session, ui, host, SDL_KMOD_CTRL);
    key(SDLK_TAB, session, ui, host);
    for (int i = 0; i < 7; ++i)
        key(SDLK_TAB, session, ui, host);
    SDL_Event gravity{};
    gravity.type = SDL_EVENT_TEXT_INPUT;
    gravity.text.text = "-5";
    app::require_sdl(SDL_PushEvent(&gravity), "Queue gravity value");
    events(session, ui, host);
    key(SDLK_RETURN, session, ui, host);
    expect(session.editing().document().gravity().y == -5, "Scrolled gravity property failed");
    draw("smoke-properties.bmp");
    key(SDLK_Z, session, ui, host, SDL_KMOD_CTRL);
    key(SDLK_N, session, ui, host);
    expect(session.editing().document().particles().size() == 4, "Create failed");
    key(SDLK_DELETE, session, ui, host);
    expect(session.editing().document().particles().size() == 3, "Delete failed");
    if (layout.left > 0) {
        mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, 40 * layout.scale, 214 * layout.scale);
        expect(session.editing().selected() == core::EntityId{2}, "Object list selection failed");
    }
    mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, (12 + 3.5 * layout.button_width()) * layout.scale,
          60 * layout.scale);
    expect(session.editing().document().particles().size() == 4, "Toolbar Add failed");
    mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, (12 + 4.5 * layout.button_width()) * layout.scale,
          60 * layout.scale);
    expect(session.editing().document().particles().size() == 3, "Toolbar Delete failed");
    key(SDLK_F1, session, ui, host);
    draw("smoke-help.bmp");
    key(SDLK_ESCAPE, session, ui, host);
    draw("smoke-workspace.bmp");
    // Exercise a different raster density without changing the user's desktop settings.
    checked(session.draw(host.canvas_extent(), renderer::render, [&](const renderer::Frame &f) {
        return host.present(f, "smoke-font-150.bmp", false,
                            [&](SDL_Renderer *r) { ui.paint(r, session, host.extent(), 1.5f); });
    }));
    draw("smoke-font-return.bmp");
    session = *app::Session::create().value();
    ui = app::EditorUI{};
    checked(session.resize(host.canvas_extent()));
    draw("smoke-initial.bmp");
    key(SDLK_SPACE, session, ui, host);
    checked(session.tick(1.0 / 128));
    key(SDLK_SPACE, session, ui, host);
    checked(session.tick(1));
    expect(session.snapshot().time() == 1.0 / 128, "Pause advanced time");
    key(SDLK_RIGHT, session, ui, host);
    expect(session.snapshot().time() == 2.0 / 128, "Single step failed");
    key(SDLK_R, session, ui, host);
    expect(session.snapshot().time() == 0 && !session.running(), "Reset failed");
    key(SDLK_SPACE, session, ui, host);
    for (int i = 0; i < 128; ++i)
        checked(session.tick(1.0 / 128));
    key(SDLK_SPACE, session, ui, host);
    expect(session.snapshot().time() == 1, "Run did not reach one second");
    draw("smoke-advanced.bmp");
    const auto before = session.snapshot();
    app::require_sdl(SDL_SetWindowSize(host.window(), 800, 600), "Resize smoke window");
    app::require_sdl(SDL_SyncWindow(host.window()), "Wait for resize");
    SDL_PumpEvents();
    draw("smoke-resized.bmp");
    const auto resized = host.extent();
    expect(resized.width > 0 && resized.height > 0, "Resize produced empty output");
    app::require_sdl(SDL_MinimizeWindow(host.window()), "Minimize smoke window");
    app::require_sdl(SDL_SyncWindow(host.window()), "Wait for minimize");
    SDL_PumpEvents();
    expect(host.extent() == renderer::Extent{0, 0}, "Minimize not observed");
    draw();
    app::require_sdl(SDL_RestoreWindow(host.window()), "Restore smoke window");
    app::require_sdl(SDL_SyncWindow(host.window()), "Wait for restore");
    SDL_PumpEvents();
    draw();
    expect(session.snapshot() == before, "Window lifecycle changed world");
    int count = 0;
    SDL_DisplayID *displays = SDL_GetDisplays(&count);
    app::require_sdl(displays != nullptr, "Enumerate displays");
    std::vector<SDL_DisplayID> ids(displays, displays + count);
    SDL_free(displays);
    for (const auto id : ids) {
        app::require_sdl(SDL_SetWindowPosition(host.window(), SDL_WINDOWPOS_CENTERED_DISPLAY(id),
                                               SDL_WINDOWPOS_CENTERED_DISPLAY(id)),
                         "Move between displays");
        app::require_sdl(SDL_SyncWindow(host.window()), "Wait for move");
        SDL_PumpEvents();
        draw();
        const auto extent = host.extent();
        std::cout << "Display " << id << " scale=" << SDL_GetWindowDisplayScale(host.window())
                  << " output=" << extent.width << "x" << extent.height << '\n';
    }
    expect(session.snapshot() == before, "Display move changed world");
    key(SDLK_R, session, ui, host);
    const auto compact = host.workspace();
    if (compact.left > 0) {
        mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, 40 * compact.scale, 214 * compact.scale);
        expect(session.editing().selected() == core::EntityId{2},
               "Compact object tab selection failed");
        for (int i = 0; i < 30; ++i)
            key(SDLK_N, session, ui, host);
        SDL_Event wheel{};
        wheel.type = SDL_EVENT_MOUSE_WHEEL;
        wheel.wheel.y = -1000;
        app::require_sdl(SDL_PushEvent(&wheel), "Queue object list scroll");
        events(session, ui, host);
        mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, 40 * compact.scale,
              (172 + compact.list_height() - 3) * compact.scale);
        expect(session.editing().selected() == session.editing().document().particles().back().id,
               "Scrolled object list did not reach final object");
        draw("smoke-compact.bmp");
    }
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    app::require_sdl(SDL_PushEvent(&quit), "Queue close");
    expect(!events(session, ui, host), "Close event ignored");
    std::cout << "PASS: editor selection/drag/history/property/create/delete, native upload, "
                 "simulation controls, resize, minimize/restore, display moves, close\n";
}
void verify_imported_cradle(const scene::Mechanism &definition) {
    std::vector<scene::DistanceLink> links;
    for (const auto &j : definition.links)
        if (!j.body_b.valid() && std::hypot(j.local_a.x, j.local_a.y) < 1e-6)
            links.push_back(j);
    std::sort(links.begin(), links.end(),
              [](const auto &a, const auto &b) { return a.local_b.x < b.local_b.x; });
    expect(links.size() >= 5, "Cradle regression requires five ground suspensions");
    links.erase(links.begin(), links.end() - 5);
    const auto body = [&](core::EntityId id) -> const scene::CircleBody & {
        return *std::find_if(definition.bodies.begin(), definition.bodies.end(),
                             [&](const auto &b) { return b.id == id; });
    };
    for (unsigned count : {1u, 2u}) {
        auto result = physics::Mechanism::create(definition);
        if (result.error())
            throw std::runtime_error(result.error()->message);
        auto world = *result.value();
        double initial_energy = 0;
        for (unsigned i = 5 - count; i < 5; ++i) {
            const auto &j = links[i];
            checked(world->relocate(j.body_a, {j.local_b.x + j.length * std::sin(.5),
                                               j.local_b.y - j.length * std::cos(.5)}));
            initial_energy +=
                body(j.body_a).mass * (-definition.gravity.y) * j.length * (1 - std::cos(.5));
        }
        double outgoing[5]{}, returning[5]{}, max_energy = 0, max_overlap = 0;
        for (unsigned step = 0; step < 300; ++step) {
            checked(world->step());
            const auto state = world->snapshot();
            double energy = 0;
            math::Vec2 previous{};
            double previous_radius = 0;
            for (unsigned i = 0; i < 5; ++i) {
                const auto &j = links[i];
                const auto &b = *std::find_if(state.bodies.begin(), state.bodies.end(),
                                              [&](const auto &v) { return v.id == j.body_a; });
                const auto &d = body(j.body_a);
                const double dx = b.center.x - j.local_b.x;
                if (state.time < 2.15)
                    outgoing[i] = std::max(outgoing[i], -dx / j.length);
                if (state.time > 2.15)
                    returning[i] = std::max(returning[i], dx / j.length);
                expect(std::abs(std::hypot(dx, b.center.y - j.local_b.y) - j.length) < .005,
                       "Imported suspension length drift");
                energy +=
                    .5 * d.mass * math::dot(b.velocity, b.velocity) +
                    d.mass * (-definition.gravity.y) * (b.center.y - (j.local_b.y - j.length));
                if (i)
                    max_overlap = std::max(max_overlap, previous_radius + d.radius -
                                                            std::hypot(b.center.x - previous.x,
                                                                       b.center.y - previous.y));
                previous = b.center;
                previous_radius = d.radius;
            }
            max_energy = std::max(max_energy, energy);
        }
        double min_output = 1, min_return = 1, interior = 0;
        for (unsigned i = 0; i < 5; ++i) {
            if (i < count)
                min_output = std::min(min_output, outgoing[i]);
            else
                interior = std::max(interior, outgoing[i]);
            if (i >= 5 - count)
                min_return = std::min(min_return, returning[i]);
        }
        std::cout << "CRADLE released=" << count << " outgoing_sin=" << min_output
                  << " interior_sin=" << interior << " return_sin=" << min_return
                  << " energy_ratio=" << max_energy / initial_energy << " overlap_m=" << max_overlap
                  << '\n';
        expect(min_output > .3, "Imported cradle failed released-ball-count transfer");
        expect(interior < .12, "Imported cradle scattered motion into stationary balls");
        expect(min_return > .22, "Imported cradle failed return swing");
        expect(max_energy < initial_energy * 1.04 && max_overlap < .005,
               "Imported collision energy/overlap invalid");
    }
}
} // namespace
int main(int argc, char **argv) {
    std::string open_path, smoke_source;
    bool benchmark = false, benchmark_legacy = false;
    bool smoke_mechanics = false, smoke_rigid = false;
    bool smoke_mode = false, fail_window = false, fail_texture = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if ((arg == "--check-mechanics" || arg == "--audit-mechanics-directory") && i + 1 < argc) {
            const auto check = [](const std::filesystem::path &path) {
                try {
                    auto parsed = app::read_project(path);
                    if (parsed.error())
                        throw std::runtime_error(parsed.error()->message);
                    auto prepared = compat::MechanicalSource::create(*parsed.value());
                    if (prepared.error())
                        throw std::runtime_error(prepared.error()->message);
                    auto source = *prepared.value();
                    auto runtime = physics::Mechanism::create(source.definition());
                    if (runtime.error())
                        throw std::runtime_error(runtime.error()->message);
                    auto world = *runtime.value();
                    const auto initial = world->snapshot();
                    for (int step = 0; step < 600; ++step) {
                        auto c = source.controls(world->snapshot());
                        if (c.error())
                            throw std::runtime_error(c.error()->message);
                        checked(world->forces(c.value()->forces));
                        for (const auto &[id, mu] : c.value()->friction)
                            checked(world->friction(id, mu));
                        checked(world->step());
                    }
                    double travel = 0;
                    auto final = world->snapshot();
                    for (std::size_t i = 0; i < final.bodies.size(); ++i)
                        travel = std::max(
                            travel,
                            std::hypot(final.bodies[i].center.x - initial.bodies[i].center.x,
                                       final.bodies[i].center.y - initial.bodies[i].center.y));
                    checked(world->reset());
                    expect(world->snapshot().time == 0, "Rigid reset failed");
                    std::cout << path.filename().string()
                              << " | RUN | bodies=" << initial.bodies.size() << " fixtures=";
                    std::size_t fixtures = 0;
                    for (const auto &b : source.definition().bodies)
                        fixtures += b.fixtures.size();
                    std::cout << fixtures << " hinges=" << source.definition().hinges.size()
                              << " windings=" << source.definition().windings.size()
                              << " time=" << final.time << " travel=" << travel << '\n';
                    return true;
                } catch (const std::exception &e) {
                    std::cout << path.filename().string() << " | UNSUPPORTED | " << e.what()
                              << '\n';
                    return false;
                }
            };
            const auto path = app::utf8_path(argv[++i]);
            if (arg == "--check-mechanics")
                return check(path) ? 0 : 1;
            unsigned total = 0, supported = 0;
            for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
                if (entry.is_regular_file() && entry.path().extension() == ".ssim") {
                    ++total;
                    if (check(entry.path()))
                        ++supported;
                }
            std::cout << "Audited " << total << " files; completed playback " << supported << '\n';
            return 0;
        }
        if ((arg == "--inspect" || arg == "--inspect-directory") && i + 1 < argc) {
            const auto inspect = [](const std::filesystem::path &path) {
                auto parsed = app::read_project(path);
                if (parsed.error()) {
                    std::cerr << path.filename().string() << ": " << parsed.error()->message
                              << '\n';
                    return false;
                }
                const auto &p = *parsed.value();
                std::cout << path.filename().string() << " version=" << p.version
                          << " members=" << p.members.size() << " outlines=" << p.outlines.size()
                          << " scripts=" << p.scripts << '\n';
                return true;
            };
            const auto path = app::utf8_path(argv[++i]);
            if (arg == "--inspect")
                return inspect(path) ? 0 : 1;
            unsigned count = 0, failed = 0;
            for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
                if (entry.is_regular_file() && entry.path().extension() == ".ssim") {
                    ++count;
                    if (!inspect(entry.path()))
                        ++failed;
                }
            std::cout << "Archives=" << count << " rejected=" << failed << '\n';
            return count && !failed ? 0 : 1;
        } else if (arg == "--open" && i + 1 < argc)
            open_path = argv[++i];
        else if (arg == "--smoke-rigid" && i + 1 < argc) {
            smoke_source = argv[++i];
            smoke_rigid = true;
        } else if (arg == "--smoke-mechanism" && i + 1 < argc) {
            smoke_source = argv[++i];
            smoke_mechanics = true;
        } else if (arg == "--smoke-source" && i + 1 < argc)
            smoke_source = argv[++i];
        else if ((arg == "--benchmark-mechanism" || arg == "--benchmark-legacy") && i + 1 < argc) {
            open_path = argv[++i];
            benchmark = true;
            benchmark_legacy = arg == "--benchmark-legacy";
        } else if (arg == "--smoke")
            smoke_mode = true;
        else if (arg == "--fail-window")
            fail_window = true;
        else if (arg == "--fail-texture")
            fail_texture = true;
        else {
            std::cerr << "Usage: opensim_demo [--smoke | --fail-window | --fail-texture]\n";
            return 2;
        }
    }
    try {
        const auto initial = app::Session::create();
        if (initial.error())
            throw std::runtime_error(initial.error()->message);
        auto session = *initial.value();
        app::Host host(fail_window);
        app::EditorUI ui;
        if (!open_path.empty() && !ui.open_file(open_path, session))
            throw std::runtime_error(ui.message());
        if (!smoke_source.empty()) {
            SDL_Event dropped{};
            dropped.type = SDL_EVENT_DROP_FILE;
            dropped.drop.data = smoke_source.c_str();
            expect(ui.event(dropped, session, host) && ui.source_open(),
                   "Real source file-drop open failed");
            const auto title = ui.source_title();
            const auto corrupt_path =
                std::filesystem::temp_directory_path() /
                ("opensim-smoke-invalid-" + std::to_string(SDL_GetTicksNS()) + ".ssim");
            {
                std::ofstream bad(corrupt_path, std::ios::binary);
                bad << "not a ZIP archive";
            }
            const auto corrupt_u8 = corrupt_path.u8string();
            const std::string corrupt(reinterpret_cast<const char *>(corrupt_u8.data()),
                                      corrupt_u8.size());
            expect(!ui.open_file(corrupt, session), "Corrupt ZIP accepted");
            std::filesystem::remove(corrupt_path);
            expect(ui.source_title() == title, "Corrupt open changed source");
            const auto retained = session.editing().document();
            expect(!ui.open_file(smoke_source + ".missing", session), "Missing project accepted");
            expect(ui.source_title() == title && session.editing().document() == retained,
                   "Failed open changed source or scene");
            checked(session.draw(
                host.canvas_extent(), renderer::render, [&](const renderer::Frame &frame) {
                    return host.present(frame, "smoke-source.bmp", false, [&](SDL_Renderer *r) {
                        ui.paint(r, session, host.extent(),
                                 SDL_GetWindowDisplayScale(host.window()));
                    });
                }));
            if (smoke_rigid) {
                auto *view = ui.mechanical();
                expect(view != nullptr, "Required rigid source profile unavailable");
                // Exercise actual source controls before requiring dynamic motion: a friction demo
                // may start in equilibrium.
                const auto control_layout = host.workspace();
                int control_width = 0, control_height = 0;
                app::require_sdl(SDL_GetWindowSize(host.window(), &control_width, &control_height),
                                 "Rigid control coordinates");
                for (const auto &widget : view->widgets())
                    if (widget.slider) {
                        const double fraction = widget.maximum > 1 ? .95 : .1;
                        const double cw =
                            double(control_layout.canvas.width) / control_layout.scale;
                        const double ch =
                            double(control_layout.canvas.height) / control_layout.scale;
                        const double px =
                            control_layout.x + (std::clamp(widget.position.x, 8.0,
                                                           std::max(8.0, cw - widget.size.x - 8)) +
                                                widget.size.x * fraction) *
                                                   control_layout.scale;
                        const double py =
                            control_layout.y + (std::clamp(widget.position.y, 8.0,
                                                           std::max(8.0, ch - widget.size.y - 8)) +
                                                widget.size.y * .5) *
                                                   control_layout.scale;
                        SDL_Event control{};
                        control.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                        control.button.button = SDL_BUTTON_LEFT;
                        control.button.x = float(px * control_width / host.extent().width);
                        control.button.y = float(py * control_height / host.extent().height);
                        expect(ui.event(control, session, host), "Initial rigid slider failed");
                        control.type = SDL_EVENT_MOUSE_BUTTON_UP;
                        expect(ui.event(control, session, host),
                               "Initial rigid slider release failed");
                        expect(std::abs(widget.value -
                                        (widget.minimum +
                                         fraction * (widget.maximum - widget.minimum))) < .001,
                               "Initial slider value not applied");
                    }
                const auto rigid_initial = view->snapshot();
                key(SDLK_SPACE, session, ui, host);
                for (int i = 0; i < 180; ++i)
                    ui.tick(view->definition().fixed_dt);
                expect(view->snapshot().time > 2.9, "Rigid playback stopped");
                double movement = 0;
                auto final = view->snapshot();
                for (std::size_t i = 0; i < rigid_initial.bodies.size(); ++i)
                    movement = std::max(
                        movement,
                        std::hypot(final.bodies[i].center.x - rigid_initial.bodies[i].center.x,
                                   final.bodies[i].center.y - rigid_initial.bodies[i].center.y));
                expect(movement > .01, "Rigid source did not move");
                checked(host.present_overlay(
                    [&](SDL_Renderer *r) {
                        ui.paint(r, session, host.extent(),
                                 SDL_GetWindowDisplayScale(host.window()));
                    },
                    "smoke-rigid.bmp"));
                key(SDLK_R, session, ui, host);
                expect(view->snapshot().time == 0, "Rigid reset failed");
                const auto l = host.workspace();
                int ww = 0, wh = 0;
                app::require_sdl(SDL_GetWindowSize(host.window(), &ww, &wh), "Rigid pointer size");
                unsigned sliders = 0, buttons = 0;
                for (const auto &w : view->widgets())
                    if (w.slider || w.button) {
                        const double vw = double(l.canvas.width) / l.scale,
                                     vh = double(l.canvas.height) / l.scale;
                        const double x =
                            l.x + (std::clamp(w.position.x, 8.0, std::max(8.0, vw - w.size.x - 8)) +
                                   w.size.x * .8) *
                                      l.scale;
                        const double y =
                            l.y + (std::clamp(w.position.y, 8.0, std::max(8.0, vh - w.size.y - 8)) +
                                   w.size.y * .5) *
                                      l.scale;
                        SDL_Event e{};
                        e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                        e.button.button = SDL_BUTTON_LEFT;
                        e.button.x = float(x * ww / host.extent().width);
                        e.button.y = float(y * wh / host.extent().height);
                        expect(ui.event(e, session, host), "Rigid control failed");
                        e.type = SDL_EVENT_MOUSE_BUTTON_UP;
                        expect(ui.event(e, session, host), "Rigid release failed");
                        if (w.slider) {
                            ++sliders;
                            expect(std::abs(w.value - (w.minimum + .8 * (w.maximum - w.minimum))) <
                                       .001,
                                   "Slider value was not applied");
                        } else {
                            ++buttons;
                            expect(view->snapshot().time == 0, "Script reset did not reconstruct");
                        }
                    }
                key(SDLK_SPACE, session, ui, host);
                for (int i = 0; i < 60; ++i)
                    ui.tick(view->definition().fixed_dt);
                expect(view->snapshot().time > .99, "Changed-control playback stopped");
                std::cout << "PASS: rigid playback, movement=" << movement
                          << ", sliders=" << sliders << ", callbacks=" << buttons
                          << ", reset and changed-control playback\n";
            }
            if (auto *view = ui.mechanical(); view && smoke_mechanics) {
                expect(!view->definition().bodies.empty(), "Empty mechanical source");
                verify_imported_cradle(view->definition());
                key(SDLK_SPACE, session, ui, host);
                ui.tick(view->definition().fixed_dt);
                expect(view->snapshot().time > 0, "Imported Play did not advance");
                key(SDLK_R, session, ui, host);
                expect(view->snapshot().time == 0 && !view->running(), "Imported Reset failed");
                if (!view->definition().links.empty()) {
                    const auto &joint = view->definition().links.back();
                    if (!joint.body_b.valid()) {
                        const auto cradle_initial = view->snapshot();
                        const auto body = *std::find_if(
                            cradle_initial.bodies.begin(), cradle_initial.bodies.end(),
                            [&](const auto &b) { return b.id == joint.body_a; });
                        const auto l = host.workspace();
                        const auto pixel = [&](math::Vec2 p) {
                            return math::Vec2{l.x + double(l.canvas.width) / 2 +
                                                  (p.x - view->camera().x) * view->zoom() * l.scale,
                                              l.y + double(l.canvas.height) / 2 -
                                                  (p.y - view->camera().y) * view->zoom() *
                                                      l.scale};
                        };
                        const auto mouse = [&](SDL_EventType type, math::Vec2 position) {
                            int ww = 0, wh = 0;
                            app::require_sdl(SDL_GetWindowSize(host.window(), &ww, &wh),
                                             "Mechanism smoke coordinates");
                            const auto screen = host.extent();
                            SDL_Event e{};
                            e.type = type;
                            const float x = float(position.x * ww / screen.width),
                                        y = float(position.y * wh / screen.height);
                            if (type == SDL_EVENT_MOUSE_MOTION) {
                                e.motion.x = x;
                                e.motion.y = y;
                            } else {
                                e.button.x = x;
                                e.button.y = y;
                                e.button.button = SDL_BUTTON_LEFT;
                                e.button.down = type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                            }
                            expect(ui.event(e, session, host),
                                   "Imported pointer unexpectedly closed");
                        };
                        mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, pixel(body.center));
                        mouse(SDL_EVENT_MOUSE_MOTION, pixel({body.center.x + joint.length * .5,
                                                             body.center.y + joint.length * .15}));
                        mouse(SDL_EVENT_MOUSE_BUTTON_UP,
                              pixel({body.center.x + joint.length * .5,
                                     body.center.y + joint.length * .15}));
                        const auto raised = view->snapshot();
                        const auto moved =
                            *std::find_if(raised.bodies.begin(), raised.bodies.end(),
                                          [&](const auto &b) { return b.id == body.id; });
                        expect(moved.center.y > body.center.y + .1,
                               "Imported pendulum drag failed");
                        key(SDLK_SPACE, session, ui, host);
                        double other_motion = 0;
                        for (int i = 0; i < 240; ++i) {
                            ui.tick(view->definition().fixed_dt);
                            const auto state = view->snapshot();
                            for (const auto &b : state.bodies)
                                if (b.id != body.id)
                                    for (const auto &old : cradle_initial.bodies)
                                        if (old.id == b.id && std::abs(old.center.y) < 10)
                                            other_motion = std::max(
                                                other_motion, std::abs(b.center.x - old.center.x));
                        }
                        expect(other_motion > .1,
                               "Imported contact transfer did not move another suspended ball");
                        key(SDLK_SPACE, session, ui, host);
                        checked(session.draw(
                            host.canvas_extent(), renderer::render, [&](const renderer::Frame &f) {
                                return host.present(
                                    f, "smoke-mechanism.bmp", false, [&](SDL_Renderer *r) {
                                        ui.paint(r, session, host.extent(),
                                                 SDL_GetWindowDisplayScale(host.window()));
                                    });
                            }));
                        for (const auto &widget : view->widgets())
                            if (widget.button) {
                                const float left = float(l.x) / l.scale, top = float(l.y) / l.scale,
                                            vw = float(l.canvas.width) / l.scale,
                                            vh = float(l.canvas.height) / l.scale;
                                const double x =
                                    (left +
                                     std::clamp(float(widget.position.x), 8.0f,
                                                std::max(8.0f, vw - float(widget.size.x) - 8)) +
                                     10) *
                                    l.scale;
                                const double y =
                                    (top +
                                     std::clamp(float(widget.position.y), 8.0f,
                                                std::max(8.0f, vh - float(widget.size.y) - 8)) +
                                     10) *
                                    l.scale;
                                mouse(SDL_EVENT_MOUSE_BUTTON_DOWN, {x, y});
                                expect(view->snapshot().time == 0,
                                       "Imported script button did not reset");
                                break;
                            }
                        std::cout << "PASS: imported Play/Reset, pendulum drag, contact transfer "
                                     "and embedded button\n";
                    }
                }
            }
            expect(!smoke_mechanics || ui.mechanical(),
                   "Requested mechanical source profile unavailable");
            key(SDLK_ESCAPE, session, ui, host);
            expect(!ui.source_open(), "Could not return to authored scene");
            std::cout
                << "PASS: real source preview, failed open retention, return to authored scene\n";
            return 0;
        }
        if (smoke_mode) {
            smoke(session, host, ui);
            return 0;
        }
        if (benchmark) {
            expect(ui.mechanical() != nullptr, "Benchmark requires supported mechanism");
            key(SDLK_SPACE, session, ui, host);
            std::vector<double> frames, physics_times, render_times;
            const auto millis = [](auto a, auto b) {
                return std::chrono::duration<double, std::milli>(b - a).count();
            };
            for (int i = 0; i < 100; ++i) {
                const auto begin = std::chrono::steady_clock::now();
                ui.tick(ui.mechanical()->definition().fixed_dt);
                const auto stepped = std::chrono::steady_clock::now();
                const auto paint = [&](SDL_Renderer *r) {
                    ui.paint(r, session, host.extent(), SDL_GetWindowDisplayScale(host.window()));
                };
                if (benchmark_legacy)
                    checked(session.draw(host.canvas_extent(), renderer::render,
                                         [&](const renderer::Frame &f) {
                                             return host.present(f, nullptr, false, paint);
                                         }));
                else
                    checked(host.present_overlay(paint));
                const auto end = std::chrono::steady_clock::now();
                if (i >= 20) {
                    frames.push_back(millis(begin, end));
                    physics_times.push_back(millis(begin, stepped));
                    render_times.push_back(millis(stepped, end));
                }
            }
            const auto mean = [](const auto &v) {
                double sum = 0;
                for (auto x : v)
                    sum += x;
                return sum / double(v.size());
            };
            std::sort(frames.begin(), frames.end());
            std::cout << "BENCHMARK frames=" << frames.size() << " frame_ms=" << mean(frames)
                      << " p95_ms=" << frames[frames.size() * 95 / 100]
                      << " physics_ms=" << mean(physics_times)
                      << " render_ms=" << mean(render_times)
                      << " uncapped_fps=" << 1000 / mean(frames) << '\n';
            return 0;
        }
        auto previous = std::chrono::steady_clock::now();
        std::string last_error;
        while (events(session, ui, host)) {
            ui.poll_open(session, host);
            const auto now = std::chrono::steady_clock::now();
            checked(session.tick(std::chrono::duration<double>(now - previous).count()));
            ui.tick(std::chrono::duration<double>(now - previous).count());
            previous = now;
            const auto paint = [&](SDL_Renderer *r) {
                ui.paint(r, session, host.extent(), SDL_GetWindowDisplayScale(host.window()));
            };
            const auto result =
                ui.mechanical()
                    ? host.present_overlay(paint, nullptr, fail_texture)
                    : session.draw(host.canvas_extent(), renderer::render,
                                   [&](const renderer::Frame &f) {
                                       return host.present(
                                           f, nullptr, fail_texture,
                                           [&](SDL_Renderer *r) {
                                               ui.paint(r, session, host.extent(),
                                                        SDL_GetWindowDisplayScale(host.window()));
                                           },
                                           {session.editing().view().center,
                                            session.editing().view().pixels_per_meter});
                                   });
            if (result.error()) {
                if (fail_texture) {
                    std::cerr << result.error()->message << '\n';
                    return 1;
                }
                if (last_error != result.error()->message)
                    std::cerr << result.error()->message << '\n';
                last_error = result.error()->message;
            } else
                last_error.clear();
            std::ostringstream title;
            title << "Open Simphy - "
                  << (ui.source_open()
                          ? ui.source_title() + (ui.mechanical() ? " | Experimental mechanics"
                                                                 : " | SSIM source preview")
                          : "Untitled scene")
                  << " | "
                  << ((ui.mechanical() ? ui.mechanical()->running() : session.running()) ? "Running"
                                                                                         : "Paused")
                  << " | t=" << std::fixed << std::setprecision(3)
                  << (ui.mechanical() ? ui.mechanical()->snapshot().time
                                      : session.snapshot().time())
                  << " s | Workspace preview";
            if (!last_error.empty())
                title << " | " << last_error;
            host.title(title.str());
            SDL_Delay(8);
        }
    } catch (const std::exception &error) {
        std::cerr << "Open Simphy: " << error.what() << '\n';
        if (!smoke_mode && smoke_source.empty() && !fail_window && !fail_texture)
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Open Simphy", error.what(), nullptr);
        return 1;
    }
    return 0;
}
