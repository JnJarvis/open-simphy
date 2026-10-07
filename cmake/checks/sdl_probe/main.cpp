#include <SDL3/SDL_version.h>
#include <iostream>

int main() {
    const int version = SDL_GetVersion();
    std::cout << "SDL runtime version: " << version << '\n';
    return version == SDL_VERSIONNUM(3, 2, 28) ? 0 : 1;
}
