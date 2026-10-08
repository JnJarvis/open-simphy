#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace opensim::app {
struct GridTick {
    double pixel, world;
    bool major;
};
// Bounded presentation-only ticks; no physics snapping or document mutation.
inline std::vector<GridTick> grid_ticks(double center, double scale, unsigned pixels) {
    std::vector<GridTick> ticks;
    if (!std::isfinite(center) || !std::isfinite(scale) || scale <= 0 || !pixels)
        return ticks;
    const double desired = 80 / scale;
    const double decade = std::pow(10.0, std::floor(std::log10(desired)));
    const double ratio = desired / decade;
    const double major = decade * (ratio <= 1 ? 1 : ratio <= 2 ? 2 : ratio <= 5 ? 5 : 10);
    const double step = major / 5;
    const double low = center - double(pixels) / (2 * scale);
    const double first = std::ceil(low / step);
    if (!std::isfinite(first) || std::abs(first) > 1e12 || !std::isfinite(step) || step <= 0)
        return ticks;
    for (unsigned i = 0; i < 512; ++i) {
        const double index = first + i, world = index * step;
        const double pixel = (world - center) * scale + double(pixels) / 2;
        if (!std::isfinite(pixel) || pixel >= pixels)
            break;
        if (pixel >= 0)
            ticks.push_back({pixel, world, std::fmod(std::abs(index), 5) == 0});
    }
    return ticks;
}
// The renderer contract fixes the clear color. Decorate only clear pixels in
// the host upload copy; immutable renderer frames and object pixels stay intact.
inline void grid_pixel(std::uint8_t *pixel, bool major) {
    if (pixel[0] == 16 && pixel[1] == 20 && pixel[2] == 28 && pixel[3] == 255) {
        pixel[0] = pixel[1] = pixel[2] = major ? 76 : 49;
    }
}
} // namespace opensim::app
