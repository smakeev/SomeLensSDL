#include "somelens/glass_lens.hpp"

#include "generated/refraction.msl.h"
#include "generated/refraction.spv.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace somelens {
namespace {

/* What shows where the lens reaches past the snapshot: orange at 0.3. */
constexpr SDL_Color kUncovered{255, 149, 0, 77};
/* The snapshot is cropped 1.5 points lower than the lens, as the reference does. */
constexpr float kVerticalCorrection = 1.5f;

struct RefractionUniforms {
    float size_center[4];
    float params[4];
};

/** @brief The outline's outward normal at each point, scaled so an offset keeps its width at corners. */
std::vector<SDL_FPoint> outward_normals(const std::vector<SDL_FPoint>& points)
{
    const std::size_t n = points.size();
    double area = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const SDL_FPoint& a = points[i];
        const SDL_FPoint& b = points[(i + 1) % n];
        area += static_cast<double>(a.x) * b.y - static_cast<double>(b.x) * a.y;
    }
    const float sign = area > 0.0 ? 1.0f : -1.0f;
    auto edge_normal = [&](std::size_t i) {
        const SDL_FPoint& a = points[i];
        const SDL_FPoint& b = points[(i + 1) % n];
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        const float length = std::max(std::hypot(dx, dy), 1e-6f);
        return SDL_FPoint{sign * dy / length, -sign * dx / length};
    };
    std::vector<SDL_FPoint> normals(n);
    for (std::size_t i = 0; i < n; ++i) {
        const SDL_FPoint before = edge_normal((i + n - 1) % n);
        const SDL_FPoint after = edge_normal(i);
        SDL_FPoint mid{before.x + after.x, before.y + after.y};
        const float length = std::hypot(mid.x, mid.y);
        if (length < 1e-6f) {
            mid = after;
        } else {
            mid = SDL_FPoint{mid.x / length, mid.y / length};
        }
        const float cosine = std::max(mid.x * after.x + mid.y * after.y, 0.25f);
        normals[i] = SDL_FPoint{mid.x / cosine, mid.y / cosine};
    }
    return normals;
}

float cross(SDL_FPoint a, SDL_FPoint b, SDL_FPoint c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/** @brief Triangles covering a simple polygon, by clipping ears: indices into `polygon`, three per triangle. */
std::vector<int> triangulate(const std::vector<SDL_FPoint>& polygon)
{
    std::vector<int> result;
    std::vector<int> left(polygon.size());
    for (std::size_t i = 0; i < left.size(); ++i) {
        left[i] = static_cast<int>(i);
    }
    double area = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const SDL_FPoint& a = polygon[i];
        const SDL_FPoint& b = polygon[(i + 1) % polygon.size()];
        area += static_cast<double>(a.x) * b.y - static_cast<double>(b.x) * a.y;
    }
    const float turn = area > 0.0 ? 1.0f : -1.0f;
    std::size_t misses = 0;
    std::size_t i = 0;
    while (left.size() > 3) {
        const std::size_t m = left.size();
        const int ia = left[(i + m - 1) % m];
        const int ib = left[i % m];
        const int ic = left[(i + 1) % m];
        const SDL_FPoint& a = polygon[static_cast<std::size_t>(ia)];
        const SDL_FPoint& b = polygon[static_cast<std::size_t>(ib)];
        const SDL_FPoint& c = polygon[static_cast<std::size_t>(ic)];
        const float corner = cross(a, b, c) * turn;
        bool ear = corner > 0.0f;
        for (std::size_t k = 0; ear && k < m; ++k) {
            const int ip = left[k];
            if (ip == ia || ip == ib || ip == ic) {
                continue;
            }
            const SDL_FPoint& p = polygon[static_cast<std::size_t>(ip)];
            if (cross(a, b, p) * turn >= 0.0f && cross(b, c, p) * turn >= 0.0f && cross(c, a, p) * turn >= 0.0f) {
                ear = false;
            }
        }
        // A straight or degenerate corner adds nothing: dropped. A polygon with no ear left (self-touching) is
        // finished as a fan, so something is always drawn.
        if (ear || corner == 0.0f || misses > m) {
            if (corner != 0.0f) {
                result.insert(result.end(), {ia, ib, ic});
            }
            left.erase(left.begin() + static_cast<std::ptrdiff_t>(i % m));
            misses = 0;
            i = i % m;
            if (i >= left.size()) {
                i = 0;
            }
            continue;
        }
        ++misses;
        i = (i + 1) % m;
    }
    if (left.size() == 3) {
        result.insert(result.end(), {left[0], left[1], left[2]});
    }
    return result;
}

/** @brief Builds a lens's triangles: the fill (textured) and the ring, around one outline in window pixels. */
class Mesh {
public:
    Mesh(const std::vector<SDL_FPoint>& points, const std::vector<SDL_FPoint>& normals)
        : points_(points), normals_(normals)
    {
    }

    /** @brief A band between two offsets of the outline, its alpha going from `alpha_in` to `alpha_out`. */
    void band(float inner, float outer, float alpha_in, float alpha_out, const SDL_FRect* uv_rect)
    {
        const std::size_t n = points_.size();
        const auto base = static_cast<int>(vertices_.size());
        for (std::size_t i = 0; i < n; ++i) {
            vertices_.push_back(vertex(i, inner, alpha_in, uv_rect));
            vertices_.push_back(vertex(i, outer, alpha_out, uv_rect));
        }
        for (std::size_t i = 0; i < n; ++i) {
            const int a = base + static_cast<int>(i) * 2;
            const int b = base + static_cast<int>((i + 1) % n) * 2;
            for (const int index : {a, a + 1, b + 1, a, b + 1, b}) {
                indices_.push_back(index);
            }
        }
    }

    /** @brief A soft line from `inner` to `outer` at `alpha`, its edges smoothed over `feather` pixels. */
    void soft_band(float inner, float outer, float alpha, float feather)
    {
        const float half = feather / 2.0f;
        if (outer - inner <= feather) {
            const float middle = (inner + outer) / 2.0f;
            const float peak = alpha * std::max(outer - inner, 0.0f) / feather;
            band(middle - half, middle, 0.0f, peak, nullptr);
            band(middle, middle + half, peak, 0.0f, nullptr);
            return;
        }
        band(inner - half, inner + half, 0.0f, alpha, nullptr);
        band(inner + half, outer - half, alpha, alpha, nullptr);
        band(outer - half, outer + half, alpha, 0.0f, nullptr);
    }

    /** @brief The inside of the outline, shrunk by `inset`; any simple outline, convex or not. */
    void fill(float inset, const SDL_FRect& uv_rect)
    {
        const std::size_t n = points_.size();
        const auto base = static_cast<int>(vertices_.size());
        std::vector<SDL_FPoint> shrunk;
        shrunk.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            vertices_.push_back(vertex(i, -inset, 1.0f, &uv_rect));
            shrunk.push_back(vertices_.back().position);
        }
        for (const int index : triangulate(shrunk)) {
            indices_.push_back(base + index);
        }
    }

    void draw(SDL_Renderer* renderer, SDL_Texture* texture)
    {
        if (!indices_.empty()) {
            SDL_RenderGeometry(renderer, texture, vertices_.data(), static_cast<int>(vertices_.size()), indices_.data(),
                               static_cast<int>(indices_.size()));
        }
        vertices_.clear();
        indices_.clear();
    }

private:
    static SDL_FPoint uv(SDL_FPoint p, const SDL_FRect* rect)
    {
        if (rect == nullptr) {
            return SDL_FPoint{0.0f, 0.0f};
        }
        return SDL_FPoint{(p.x - rect->x) / rect->w, (p.y - rect->y) / rect->h};
    }

    SDL_Vertex vertex(std::size_t i, float offset, float alpha, const SDL_FRect* uv_rect) const
    {
        const SDL_FPoint p{points_[i].x + normals_[i].x * offset, points_[i].y + normals_[i].y * offset};
        return SDL_Vertex{p, SDL_FColor{1.0f, 1.0f, 1.0f, alpha}, uv(p, uv_rect)};
    }

    const std::vector<SDL_FPoint>& points_;
    const std::vector<SDL_FPoint>& normals_;
    std::vector<SDL_Vertex> vertices_;
    std::vector<int> indices_;
};

bool inside(const std::vector<SDL_FPoint>& polygon, SDL_FPoint p)
{
    bool in = false;
    const std::size_t n = polygon.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const SDL_FPoint& a = polygon[i];
        const SDL_FPoint& b = polygon[j];
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x) {
            in = !in;
        }
    }
    return in;
}

} // namespace

struct GlassLens::Resources {
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* ping = nullptr;
    SDL_Texture* pong = nullptr;
    int width = 0;
    int height = 0;
    std::vector<ShaderProgram> programs;

    ~Resources() { release_textures(); }

    void release_textures()
    {
        for (SDL_Texture** texture : {&ping, &pong}) {
            if (*texture != nullptr) {
                SDL_DestroyTexture(*texture);
                *texture = nullptr;
            }
        }
        width = 0;
        height = 0;
    }

    bool ensure_textures(SDL_Renderer* to, int w, int h)
    {
        if (renderer != to) {
            release_textures();
            programs.clear();
            renderer = to;
        }
        if (ping != nullptr && w == width && h == height) {
            return true;
        }
        release_textures();
        for (SDL_Texture** texture : {&ping, &pong}) {
            *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
            if (*texture == nullptr) {
                return false;
            }
            SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_NONE);
        }
        width = w;
        height = h;
        return true;
    }

    /** @brief The render state for step `index` of the chain: one per step, so each keeps its own uniforms. */
    ShaderProgram* program(std::size_t index, const ShaderBinary& binary)
    {
        if (programs.size() <= index) {
            programs.resize(index + 1);
        }
        ShaderProgram& program = programs[index];
        if (program.state() == nullptr && !program.create(renderer, binary)) {
            SDL_Log("somelens: shader failed: %s", SDL_GetError());
            return nullptr;
        }
        return &program;
    }
};

GlassLens::GlassLens() : r_(std::make_unique<Resources>()) {}

GlassLens::~GlassLens() = default;

void GlassLens::release()
{
    r_ = std::make_unique<Resources>();
}

bool GlassLens::contains(SDL_FPoint centre, const LensSettings& settings, SDL_FPoint point) const
{
    const SDL_FRect rect{centre.x - settings.width / 2.0f, centre.y - settings.height / 2.0f, settings.width,
                         settings.height};
    return inside(settings.path(rect), point);
}

void GlassLens::draw(LensContainer& container, SDL_FPoint centre, const LensSettings& settings)
{
    SDL_Renderer* renderer = container.renderer();
    SDL_Texture* snapshot = container.snapshot();
    // Hidden until the first snapshot.
    if (renderer == nullptr || snapshot == nullptr) {
        return;
    }
    const float scale = container.pixel_scale();
    const float lens_w = std::max(settings.width, 1.0f);
    const float lens_h = std::max(settings.height, 1.0f);
    const int tex_w = std::max(1, static_cast<int>(std::lround(lens_w * scale)));
    const int tex_h = std::max(1, static_cast<int>(std::lround(lens_h * scale)));
    // The lens sits on whole pixels, so the crop is the snapshot one to one.
    const float origin_x = std::round((centre.x - lens_w / 2.0f) * scale);
    const float origin_y = std::round((centre.y - lens_h / 2.0f) * scale);
    if (!r_->ensure_textures(renderer, tex_w, tex_h)) {
        return;
    }

    SDL_Texture* const previous_target = SDL_GetRenderTarget(renderer);

    // 1. The part of the snapshot under the lens, over the colour of what is not covered.
    SDL_SetRenderTarget(renderer, r_->ping);
    SDL_SetRenderDrawColor(renderer, kUncovered.r, kUncovered.g, kUncovered.b, kUncovered.a);
    SDL_RenderClear(renderer);
    float snap_w = 0.0f;
    float snap_h = 0.0f;
    SDL_GetTextureSize(snapshot, &snap_w, &snap_h);
    const SDL_FRect shifted{-origin_x, -(origin_y + kVerticalCorrection * scale), snap_w, snap_h};
    SDL_SetTextureBlendMode(snapshot, SDL_BLENDMODE_NONE);
    SDL_RenderTexture(renderer, snapshot, nullptr, &shifted);

    // 2. The shader chain: each step reads what the one before wrote.
    const auto size_x = static_cast<float>(tex_w);
    const auto size_y = static_cast<float>(tex_h);
    const float radius = std::hypot(lens_w, lens_h) / 2.0f * scale;
    for (std::size_t i = 0; i < settings.shaders.size(); ++i) {
        ShaderProgram* program = nullptr;
        std::visit(
            [&](const auto& shader) {
                using Shader = std::decay_t<decltype(shader)>;
                if constexpr (std::is_same_v<Shader, RefractionShader>) {
                    program = r_->program(i, ShaderBinary{refraction_spv, sizeof refraction_spv, refraction_msl,
                                                          sizeof refraction_msl, 1});
                    if (program != nullptr) {
                        const RefractionUniforms uniforms{
                            {size_x, size_y, size_x / 2.0f, size_y / 2.0f},
                            {radius, shader.refraction, shader.edge_reflection, 0.0f}};
                        program->set_uniforms(&uniforms, sizeof uniforms);
                    }
                }
            },
            settings.shaders[i]);
        if (program == nullptr) {
            continue;
        }
        SDL_SetRenderTarget(renderer, r_->pong);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetGPURenderState(renderer, program->state());
        SDL_RenderTexture(renderer, r_->ping, nullptr, nullptr);
        SDL_SetGPURenderState(renderer, nullptr);
        std::swap(r_->ping, r_->pong);
    }

    // 3. The result cut to the outline, with a pixel of soft edge, then the ring along the outline.
    SDL_SetRenderTarget(renderer, previous_target);
    const SDL_FRect lens_rect{origin_x / scale, origin_y / scale, lens_w, lens_h};
    std::vector<SDL_FPoint> outline = settings.path(lens_rect);
    if (outline.size() < 3) {
        return;
    }
    for (SDL_FPoint& p : outline) {
        p = SDL_FPoint{p.x * scale, p.y * scale};
    }
    const std::vector<SDL_FPoint> normals = outward_normals(outline);
    const SDL_FRect uv_rect{origin_x, origin_y, size_x, size_y};
    Mesh mesh(outline, normals);
    mesh.fill(0.5f, uv_rect);
    mesh.band(-0.5f, 0.5f, 1.0f, 0.0f, &uv_rect);
    SDL_SetTextureBlendMode(r_->ping, SDL_BLENDMODE_BLEND);
    mesh.draw(renderer, r_->ping);
    SDL_SetTextureBlendMode(r_->ping, SDL_BLENDMODE_NONE);

    // The ring: white at 0.45, ring width centred on the outline, slightly blurred; and a faint 0.5 point line
    // just inside it, white at 0.12.
    const float ring = settings.ring_width * scale;
    mesh.soft_band(-ring / 2.0f, ring / 2.0f, 0.45f, 1.0f + 0.4f * scale);
    mesh.soft_band(-0.5f * scale, 0.0f, 0.12f, 1.0f);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    mesh.draw(renderer, nullptr);
}

} // namespace somelens
