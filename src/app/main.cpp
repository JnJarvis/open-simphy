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
bool events(app::Session &session) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            return false;
        if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat)
            continue;
        switch (event.key.key) {
        case SDLK_ESCAPE:
            return false;
        case SDLK_SPACE:
            session.set_running(!session.running());
            break;
        case SDLK_RIGHT:
            session.set_running(false);
            checked(session.advance());
            break;
        case SDLK_R:
            checked(session.reset());
            break;
        default:
            break;
        }
    }
    return true;
}
void key(SDL_Keycode code, app::Session &session) {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = code;
    event.key.down = true;
    app::require_sdl(SDL_PushEvent(&event), "Queue smoke key");
    if (!events(session))
        throw std::runtime_error("Unexpected close during smoke");
}
void expect(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void smoke(app::Session &session, app::Host &host) {
    const auto draw = [&](const char *capture = nullptr) {
        checked(session.draw(host.extent(), renderer::render,
                             [&](const renderer::Frame &f) { return host.present(f, capture); }));
    };
    draw("smoke-initial.bmp");
    key(SDLK_SPACE, session);
    checked(session.tick(1.0 / 128));
    key(SDLK_SPACE, session);
    checked(session.tick(1));
    expect(session.snapshot().time() == 1.0 / 128, "Pause advanced time");
    key(SDLK_RIGHT, session);
    expect(session.snapshot().time() == 2.0 / 128, "Single step failed");
    key(SDLK_R, session);
    expect(session.snapshot().time() == 0 && !session.running(), "Reset failed");
    key(SDLK_SPACE, session);
    for (int i = 0; i < 128; ++i)
        checked(session.tick(1.0 / 128));
    key(SDLK_SPACE, session);
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
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    app::require_sdl(SDL_PushEvent(&quit), "Queue close");
    expect(!events(session), "Close event ignored");
    std::cout << "PASS: native upload, controls, resize, minimize/restore, display moves, close\n";
}
} // namespace
int main(int argc, char **argv) {
    bool smoke_mode = false, fail_window = false, fail_texture = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--smoke")
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
        if (smoke_mode) {
            smoke(session, host);
            return 0;
        }
        auto previous = std::chrono::steady_clock::now();
        std::string last_error;
        while (events(session)) {
            const auto now = std::chrono::steady_clock::now();
            checked(session.tick(std::chrono::duration<double>(now - previous).count()));
            previous = now;
            const auto result =
                session.draw(host.extent(), renderer::render, [&](const renderer::Frame &f) {
                    return host.present(f, nullptr, fail_texture);
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
            title << "Open Simphy | " << (session.running() ? "Running" : "Paused")
                  << " | t=" << std::fixed << std::setprecision(3) << session.snapshot().time()
                  << " s | Space: play/pause | Right: step | R: reset | Esc: exit";
            if (!last_error.empty())
                title << " | " << last_error;
            host.title(title.str());
            SDL_Delay(8);
        }
    } catch (const std::exception &error) {
        std::cerr << "Open Simphy: " << error.what() << '\n';
        if (!smoke_mode && !fail_window && !fail_texture)
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Open Simphy", error.what(), nullptr);
        return 1;
    }
    return 0;
}
