#include "somelens/lens_settings.hpp"

#include <algorithm>
#include <cmath>

namespace somelens {
namespace {

constexpr int kEllipsePoints = 128;
constexpr int kCubicSteps = 24;
constexpr int kQuarterArcSteps = 16;
constexpr float kPi = 3.14159265359f;

} // namespace

void PathBuilder::add(SDL_FPoint p)
{
    if (!points_.empty() && std::fabs(points_.back().x - p.x) < 1e-4f && std::fabs(points_.back().y - p.y) < 1e-4f) {
        return;
    }
    points_.push_back(p);
}

void PathBuilder::move_to(SDL_FPoint p)
{
    add(p);
}

void PathBuilder::line_to(SDL_FPoint p)
{
    add(p);
}

void PathBuilder::cubic_to(SDL_FPoint control1, SDL_FPoint control2, SDL_FPoint p)
{
    const SDL_FPoint p0 = points_.empty() ? control1 : points_.back();
    for (int i = 1; i <= kCubicSteps; ++i) {
        const float t = static_cast<float>(i) / kCubicSteps;
        const float u = 1.0f - t;
        const float a = u * u * u;
        const float b = 3.0f * u * u * t;
        const float c = 3.0f * u * t * t;
        const float d = t * t * t;
        add(SDL_FPoint{a * p0.x + b * control1.x + c * control2.x + d * p.x,
                       a * p0.y + b * control1.y + c * control2.y + d * p.y});
    }
}

void PathBuilder::arc(SDL_FPoint centre, float radius, float start, float end)
{
    const float quarters = std::fabs(end - start) / (kPi / 2.0f);
    const int steps = std::max(2, static_cast<int>(std::ceil(quarters * kQuarterArcSteps)));
    for (int i = 0; i <= steps; ++i) {
        const float angle = start + (end - start) * static_cast<float>(i) / static_cast<float>(steps);
        add(SDL_FPoint{centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius});
    }
}

void PathBuilder::add_ellipse(const SDL_FRect& rect)
{
    const float cx = rect.x + rect.w / 2.0f;
    const float cy = rect.y + rect.h / 2.0f;
    for (int i = 0; i < kEllipsePoints; ++i) {
        const float angle = static_cast<float>(i) / kEllipsePoints * 2.0f * kPi;
        add(SDL_FPoint{cx + std::cos(angle) * rect.w / 2.0f, cy + std::sin(angle) * rect.h / 2.0f});
    }
}

void PathBuilder::add_rectangle(const SDL_FRect& rect)
{
    add(SDL_FPoint{rect.x, rect.y});
    add(SDL_FPoint{rect.x + rect.w, rect.y});
    add(SDL_FPoint{rect.x + rect.w, rect.y + rect.h});
    add(SDL_FPoint{rect.x, rect.y + rect.h});
}

void PathBuilder::add_rounded_rectangle(const SDL_FRect& rect, float radius)
{
    const float r = std::clamp(radius, 0.0f, std::min(rect.w, rect.h) / 2.0f);
    if (r <= 0.0f) {
        add_rectangle(rect);
        return;
    }
    const float left = rect.x + r;
    const float right = rect.x + rect.w - r;
    const float top = rect.y + r;
    const float bottom = rect.y + rect.h - r;
    arc(SDL_FPoint{right, top}, r, -kPi / 2.0f, 0.0f);
    arc(SDL_FPoint{right, bottom}, r, 0.0f, kPi / 2.0f);
    arc(SDL_FPoint{left, bottom}, r, kPi / 2.0f, kPi);
    arc(SDL_FPoint{left, top}, r, kPi, kPi * 1.5f);
}

std::vector<SDL_FPoint> PathBuilder::points() const
{
    std::vector<SDL_FPoint> points = points_;
    while (points.size() > 1 && std::fabs(points.back().x - points.front().x) < 1e-4f
           && std::fabs(points.back().y - points.front().y) < 1e-4f) {
        points.pop_back();
    }
    return points;
}

LensPath ellipse_path()
{
    return [](const SDL_FRect& rect) {
        PathBuilder path;
        path.add_ellipse(rect);
        return path.points();
    };
}

LensPath rectangle_path()
{
    return [](const SDL_FRect& rect) {
        PathBuilder path;
        path.add_rectangle(rect);
        return path.points();
    };
}

LensPath rounded_rectangle_path(float fraction)
{
    return [fraction](const SDL_FRect& rect) {
        PathBuilder path;
        path.add_rounded_rectangle(rect, std::min(rect.w, rect.h) * fraction);
        return path.points();
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
