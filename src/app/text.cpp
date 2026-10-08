#include "text.hpp"
#include "sdl_host.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace opensim::app {
namespace {
// Decode complete UTF-8 code points; replace invalid sequences without reading past input.
std::uint32_t next(std::string_view &text) {
    const auto c = static_cast<unsigned char>(text.front());
    text.remove_prefix(1);
    if (c < 128)
        return c;
    unsigned count = c >= 0xc2 && c <= 0xdf   ? 1
                     : c >= 0xe0 && c <= 0xef ? 2
                     : c >= 0xf0 && c <= 0xf4 ? 3
                                              : 0;
    if (!count || text.size() < count)
        return 0xfffd;
    std::uint32_t value = c & ((1u << (6 - count)) - 1);
    for (unsigned i = 0; i < count; ++i) {
        const auto b = static_cast<unsigned char>(text[i]);
        if ((b & 0xc0) != 0x80)
            return 0xfffd;
        value = (value << 6) | (b & 63);
    }
    text.remove_prefix(count);
    if ((count == 1 && value < 128) || (count == 2 && value < 2048) ||
        (count == 3 && value < 65536) || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        return 0xfffd;
    return value;
}
} // namespace
struct TextRenderer::Impl {
    struct Glyph {
        Texture texture{nullptr, SDL_DestroyTexture};
        float left = 0, top = 0, width = 0, height = 0, advance = 0;
        FT_UInt index = 0;
    };
    SDL_Renderer *renderer;
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    std::unique_ptr<void, decltype(&SDL_free)> font_bytes{nullptr, SDL_free};
    unsigned pixels = 0;
    std::map<std::uint32_t, Glyph> glyphs;
    explicit Impl(SDL_Renderer *r) : renderer(r) {
        if (FT_Init_FreeType(&library))
            throw std::runtime_error("Initialize font rasterizer failed");
        const char *base = SDL_GetBasePath();
        std::size_t byte_count = 0;
        if (base)
            font_bytes.reset(
                SDL_LoadFile((std::string(base) + "NotoSans-Regular.ttf").c_str(), &byte_count));
        if (!font_bytes || byte_count > 4 * 1024 * 1024 ||
            FT_New_Memory_Face(library, static_cast<const FT_Byte *>(font_bytes.get()),
                               static_cast<FT_Long>(byte_count), 0, &face)) {
            FT_Done_FreeType(library);
            library = nullptr;
            throw std::runtime_error("Open bundled Noto Sans font failed");
        }
    }
    ~Impl() {
        glyphs.clear();
        if (face)
            FT_Done_Face(face);
        if (library)
            FT_Done_FreeType(library);
    }
    const Glyph &glyph(std::uint32_t code) {
        const auto existing = glyphs.find(code);
        if (existing != glyphs.end())
            return existing->second;
        // Bound source-title/inspector glyph caches without discarding source text.
        if (glyphs.size() >= 2048)
            glyphs.clear();
        if (FT_Load_Char(face, code, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT))
            throw std::runtime_error("Rasterize font glyph failed");
        const auto slot = face->glyph;
        const auto &bitmap = slot->bitmap;
        Glyph g;
        g.left = static_cast<float>(slot->bitmap_left);
        g.top = static_cast<float>(slot->bitmap_top);
        g.width = static_cast<float>(bitmap.width);
        g.height = static_cast<float>(bitmap.rows);
        g.advance = static_cast<float>(slot->advance.x) / 64;
        g.index = FT_Get_Char_Index(face, code);
        if (bitmap.width && bitmap.rows) {
            if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY || bitmap.pitch < 0)
                throw std::runtime_error("Unexpected font bitmap format");
            std::vector<std::uint8_t> rgba(std::size_t(bitmap.width) * bitmap.rows * 4, 255);
            for (unsigned y = 0; y < bitmap.rows; ++y)
                for (unsigned x = 0; x < bitmap.width; ++x)
                    rgba[(std::size_t(y) * bitmap.width + x) * 4 + 3] =
                        bitmap.buffer[std::size_t(y) * static_cast<unsigned>(bitmap.pitch) + x];
            g.texture.reset(
                SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                                  static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows)));
            require_sdl(bool(g.texture), "Create glyph texture");
            require_sdl(SDL_UpdateTexture(g.texture.get(), nullptr, rgba.data(),
                                          static_cast<int>(bitmap.width * 4)),
                        "Upload glyph");
            require_sdl(SDL_SetTextureBlendMode(g.texture.get(), SDL_BLENDMODE_BLEND),
                        "Blend glyph");
            require_sdl(SDL_SetTextureScaleMode(g.texture.get(), SDL_SCALEMODE_LINEAR),
                        "Filter glyph");
        }
        return glyphs.emplace(code, std::move(g)).first->second;
    }
};
TextRenderer::TextRenderer(SDL_Renderer *r) : impl_(std::make_unique<Impl>(r)) {}
TextRenderer::~TextRenderer() = default;
void TextRenderer::draw(float x, float y, std::string_view text, float width) {
    float sx = 1, sy = 1;
    require_sdl(SDL_GetRenderScale(impl_->renderer, &sx, &sy), "Query font scale");
    const auto pixels = static_cast<unsigned>(std::clamp(std::round(13 * sy), 1.0f, 104.0f));
    if (impl_->pixels != pixels) {
        impl_->glyphs.clear();
        if (FT_Set_Pixel_Sizes(impl_->face, 0, pixels))
            throw std::runtime_error("Set font pixel size failed");
        impl_->pixels = pixels;
    }
    Uint8 red = 255, green = 255, blue = 255, alpha = 255;
    require_sdl(SDL_GetRenderDrawColor(impl_->renderer, &red, &green, &blue, &alpha),
                "Query text color");
    const float start = x;
    const float baseline = std::round(y * sy) / sy +
                           static_cast<float>(impl_->face->size->metrics.ascender) / (64 * sy);
    FT_UInt previous = 0;
    // Width clipping uses glyph advances, so UTF-8 titles cannot be split mid-character.
    while (!text.empty()) {
        const auto &g = impl_->glyph(next(text));
        FT_Vector kern{};
        if (previous && g.index && FT_HAS_KERNING(impl_->face))
            FT_Get_Kerning(impl_->face, previous, g.index, FT_KERNING_DEFAULT, &kern);
        x += static_cast<float>(kern.x) / (64 * sx);
        if (x + std::max(g.advance, g.left + g.width) / sx > start + width)
            break;
        if (g.texture) {
            require_sdl(SDL_SetTextureColorMod(g.texture.get(), red, green, blue), "Tint glyph");
            require_sdl(SDL_SetTextureAlphaMod(g.texture.get(), alpha), "Set glyph opacity");
            const SDL_FRect dest{std::round(x * sx + g.left) / sx, baseline - g.top / sy,
                                 g.width / sx, g.height / sy};
            require_sdl(SDL_RenderTexture(impl_->renderer, g.texture.get(), nullptr, &dest),
                        "Draw glyph");
        }
        x += g.advance / sx;
        previous = g.index;
    }
}
} // namespace opensim::app
