#include "somelens/lens_settings.hpp"

#include <algorithm>
#include <cmath>

namespace somelens {
namespace {

constexpr int kEllipsePoints = 128;

} // namespace

LensPath ellipse_path()
{
    return [](const SDL_FRect& rect) {
        std::vector<SDL_FPoint> points;
        points.reserve(kEllipsePoints);
        const float cx = rect.x + rect.w / 2.0f;
        const float cy = rect.y + rect.h / 2.0f;
        for (int i = 0; i < kEllipsePoints; ++i) {
            const float angle = static_cast<float>(i) / kEllipsePoints * 6.28318530718f;
            points.push_back(SDL_FPoint{cx + std::cos(angle) * rect.w / 2.0f, cy + std::sin(angle) * rect.h / 2.0f});
        }
        return points;
    };
}

LensSettings LensSettings::circle(float diameter)
{
    LensSettings settings;
    settings.width = diameter;
    settings.height = diameter;
    settings.path = ellipse_path();
    return settings;
}

std::optional<double> SnapshotRefreshRate::interval() const
{
    switch (kind) {
    case Kind::kNever:
        return std::nullopt;
    case Kind::kAutomatic:
        return 0.2;
    case Kind::kFast:
        return 1.0 / 30.0;
    case Kind::kBalanced:
        return 0.1;
    case Kind::kRelaxed:
        return 0.25;
    case Kind::kSlow:
        return 0.5;
    case Kind::kCustom:
        return static_cast<double>(std::max(milliseconds, 1)) / 1000.0;
    }
    return std::nullopt;
}

} // namespace somelens
