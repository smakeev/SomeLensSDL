#include "lens_morph.hpp"

#include "lens_shaders.hpp"

#include <algorithm>
#include <cmath>

namespace somelens {
namespace {

constexpr std::size_t kMorphSamples = 96;
constexpr std::size_t kSignatureSamples = 64;
/* The spring of the reference: response 0.42 s, damping fraction 0.86. */
constexpr double kResponse = 0.42;
constexpr double kDamping = 0.86;

int quantized(float value)
{
    return static_cast<int>(std::lround(static_cast<double>(value) * 10000.0));
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

/** @brief `count` points spread evenly by length along the closed outline. */
std::vector<SDL_FPoint> resampled(const std::vector<SDL_FPoint>& points, std::size_t count)
{
    if (points.empty()) {
        return std::vector<SDL_FPoint>(count, SDL_FPoint{0.0f, 0.0f});
    }
    std::vector<float> lengths(points.size());
    float total = 0.0f;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const SDL_FPoint& a = points[i];
        const SDL_FPoint& b = points[(i + 1) % points.size()];
        lengths[i] = std::hypot(b.x - a.x, b.y - a.y);
        total += lengths[i];
    }
    if (total <= 0.0f) {
        return std::vector<SDL_FPoint>(count, points.front());
    }
    std::vector<SDL_FPoint> result;
    result.reserve(count);
    std::size_t segment = 0;
    float walked = 0.0f;  // length of the outline before `segment`
    for (std::size_t k = 0; k < count; ++k) {
        const float target = total * static_cast<float>(k) / static_cast<float>(count);
        while (segment + 1 < points.size() && walked + lengths[segment] < target) {
            walked += lengths[segment];
            ++segment;
        }
        const SDL_FPoint& a = points[segment];
        const SDL_FPoint& b = points[(segment + 1) % points.size()];
        const float t = lengths[segment] > 0.0f ? (target - walked) / lengths[segment] : 0.0f;
        result.push_back(SDL_FPoint{lerp(a.x, b.x, t), lerp(a.y, b.y, t)});
    }
    return result;
}

std::vector<SDL_FPoint> unit_outline(const LensSettings& settings)
{
    return settings.path ? settings.path(SDL_FRect{0.0f, 0.0f, 1.0f, 1.0f}) : std::vector<SDL_FPoint>{};
}

/** @brief The spring's value at `seconds`: 0 at the start, settling at 1, overshooting a little. */
double spring(double seconds)
{
    const double omega = 2.0 * 3.14159265358979 / kResponse;
    const double decay = kDamping * omega;
    const double omega_d = omega * std::sqrt(1.0 - kDamping * kDamping);
    const double wave = std::cos(omega_d * seconds) + decay / omega_d * std::sin(omega_d * seconds);
    return 1.0 - std::exp(-decay * seconds) * wave;
}

} // namespace

LensMorph::Shape LensMorph::shape_of(const LensSettings& settings)
{
    Shape shape;
    shape.width = settings.width;
    shape.height = settings.height;
    shape.ring_width = settings.ring_width;
    shape.outline =
        std::make_shared<const std::vector<SDL_FPoint>>(resampled(unit_outline(settings), kMorphSamples));
    shape.shaders = settings.shaders;
    return shape;
}

LensMorph::Signature LensMorph::signature_of(const LensSettings& settings)
{
    Signature signature;
    signature.width = quantized(settings.width);
    signature.height = quantized(settings.height);
    signature.ring_width = quantized(settings.ring_width);
    signature.animates = settings.animates_path_changes;
    for (const LensShader& shader : settings.shaders) {
        for (const float value : shader_signature(shader)) {
            signature.shaders.push_back(quantized(value));
        }
    }
    for (const SDL_FPoint& p : resampled(unit_outline(settings), kSignatureSamples)) {
        signature.outline.push_back(quantized(p.x));
        signature.outline.push_back(quantized(p.y));
    }
    return signature;
}

LensMorph::Shape LensMorph::blend(const Shape& from, const Shape& to, float progress)
{
    Shape shape;
    shape.width = lerp(from.width, to.width, progress);
    shape.height = lerp(from.height, to.height, progress);
    shape.ring_width = lerp(from.ring_width, to.ring_width, progress);
    std::vector<SDL_FPoint> outline(kMorphSamples);
    for (std::size_t i = 0; i < kMorphSamples; ++i) {
        const SDL_FPoint& a = (*from.outline)[i];
        const SDL_FPoint& b = (*to.outline)[i];
        outline[i] = SDL_FPoint{lerp(a.x, b.x, progress), lerp(a.y, b.y, progress)};
    }
    shape.outline = std::make_shared<const std::vector<SDL_FPoint>>(std::move(outline));
    if (from.shaders.size() == to.shaders.size()) {
        for (std::size_t i = 0; i < to.shaders.size(); ++i) {
            shape.shaders.push_back(blended(from.shaders[i], to.shaders[i], progress));
        }
    } else {
        shape.shaders = to.shaders;
    }
    return shape;
}

LensMorph::Shape LensMorph::current(Uint64 now_ns) const
{
    if (!animating_) {
        return to_;
    }
    const double seconds = static_cast<double>(now_ns - start_ns_) / 1e9;
    return blend(from_, to_, static_cast<float>(std::clamp(spring(seconds), 0.0, 1.0)));
}

LensSettings LensMorph::settings_of(const Shape& shape, const LensSettings& target)
{
    LensSettings settings = target;
    settings.width = shape.width;
    settings.height = shape.height;
    settings.ring_width = shape.ring_width;
    settings.shaders = shape.shaders;
    settings.path = [outline = shape.outline](const SDL_FRect& rect) {
        std::vector<SDL_FPoint> points;
        points.reserve(outline->size());
        for (const SDL_FPoint& p : *outline) {
            points.push_back(SDL_FPoint{rect.x + p.x * rect.w, rect.y + p.y * rect.h});
        }
        return points;
    };
    return settings;
}

LensSettings LensMorph::shown(const LensSettings& target, Uint64 now_ns)
{
    Signature signature = signature_of(target);
    if (!has_shown_) {
        has_shown_ = true;
        signature_ = std::move(signature);
        to_ = shape_of(target);
        return target;
    }
    if (!(signature == signature_)) {
        signature_ = std::move(signature);
        if (target.animates_path_changes) {
            // From what is on screen now, so a change in the middle of a morph does not jump.
            from_ = current(now_ns);
            to_ = shape_of(target);
            start_ns_ = now_ns;
            animating_ = true;
        } else {
            to_ = shape_of(target);
            animating_ = false;
        }
    }
    if (!animating_) {
        return target;
    }
    const double seconds = static_cast<double>(now_ns - start_ns_) / 1e9;
    const double value = spring(seconds);
    if (seconds > kResponse && std::fabs(1.0 - value) < 1e-3) {
        animating_ = false;
        return target;
    }
    return settings_of(current(now_ns), target);
}

} // namespace somelens
