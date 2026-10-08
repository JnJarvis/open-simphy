#include "editor_ui.hpp"
#include "sdl_host.hpp"
#include "session.hpp"
#include <SDL3/SDL_main.h>
#include <chrono>
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
} // namespace
int main(int argc, char **argv) {
    std::string open_path, smoke_source;
    bool smoke_mode = false, fail_window = false, fail_texture = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
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
        else if (arg == "--smoke-source" && i + 1 < argc)
            smoke_source = argv[++i];
        else if (arg == "--smoke")
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
        auto previous = std::chrono::steady_clock::now();
        std::string last_error;
        while (events(session, ui, host)) {
            ui.poll_open(session, host);
            const auto now = std::chrono::steady_clock::now();
            checked(session.tick(std::chrono::duration<double>(now - previous).count()));
            previous = now;
            const auto result =
                session.draw(host.canvas_extent(), renderer::render, [&](const renderer::Frame &f) {
                    return host.present(f, nullptr, fail_texture,
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
                  << (ui.source_open() ? ui.source_title() + " | SSIM source preview"
                                       : "Untitled scene")
                  << " | " << (session.running() ? "Running" : "Paused") << " | t=" << std::fixed
                  << std::setprecision(3) << session.snapshot().time() << " s | Workspace preview";
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
