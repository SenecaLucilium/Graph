#include "GraphSampler.h"

#include "Evaluator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace diagnostics = src::common::Diagnostics;
namespace evaluator = src::core::ExpressionEngine::Evaluator;

namespace src::core::ExpressionEngine::Graph
{
namespace
{

constexpr std::size_t maxAdaptiveDepth = 5;
constexpr std::size_t maxSamplePoints = 4096;

SampleResult makeInvalidArgumentError(std::string message, diagnostics::Context context = {})
{
    diagnostics::Errors::Error error;

    error.domain = diagnostics::ErrorDomain::Evaluation;
    error.code = diagnostics::ErrorCode::InvalidArgument;
    error.errorLevel = diagnostics::ErrorLevel::Error;
    error.message = std::move(message);
    error.context = std::move(context);

    return SampleResult::failure(std::move(error));
}

SamplePoint evaluatePoint(const src::core::ExpressionEngine::Parser::ExpressionTree& tree, double x)
{
    SamplePoint point{.x = x};
    auto evaluationResult = evaluator::evaluate(tree, x);

    if (evaluationResult)
    {
        point.y = evaluationResult.value();
    }
    else
    {
        point.error = evaluationResult.error();
    }

    return point;
}

bool needsSubdivision(const SamplePoint& left, const SamplePoint& middle, const SamplePoint& right)
{
    if (!left.y.has_value() || !right.y.has_value()) return false;
    if (!middle.y.has_value()) return true;

    const double scale = std::max({1.0, std::abs(*left.y), std::abs(*middle.y), std::abs(*right.y)});
    const double linearEstimate = (*left.y + *right.y) * 0.5;
    const double curvatureError = std::abs(*middle.y - linearEstimate);
    const double endpointJump = std::abs(*right.y - *left.y);

    return curvatureError > scale * 0.05 || endpointJump > scale * 0.75;
}

void appendAdaptiveInterval(const src::core::ExpressionEngine::Parser::ExpressionTree& tree, const SamplePoint& left, const SamplePoint& right, std::size_t depth, SamplePoints& points)
{
    if (depth >= maxAdaptiveDepth || points.size() >= maxSamplePoints - 1)
    {
        points.push_back(right);
        return;
    }

    const double middleX = left.x + (right.x - left.x) * 0.5;

    if (!(middleX > left.x && middleX < right.x))
    {
        points.push_back(right);
        return;
    }

    const SamplePoint middle = evaluatePoint(tree, middleX);

    if (!needsSubdivision(left, middle, right))
    {
        points.push_back(right);
        return;
    }

    appendAdaptiveInterval(tree, left, middle, depth + 1, points);
    appendAdaptiveInterval(tree, middle, right, depth + 1, points);
}

}

SampleResult sample(const src::core::ExpressionEngine::Parser::ExpressionTree& tree, double from, double to, std::size_t count)
{
    if (count < 2 || count > maxSamplePoints)
    {
        return makeInvalidArgumentError("Sample point count must be between 2 and 4096", {{"count", static_cast<std::int64_t>(count)}});
    }

    if (!std::isfinite(from) || !std::isfinite(to))
    {
        return makeInvalidArgumentError("Sampling range must contain finite values", {{"from", from}, {"to", to}});
    }

    if (from >= to)
    {
        return makeInvalidArgumentError("Sampling range must satisfy from < to", {{"from", from}, {"to", to}});
    }

    SamplePoints basePoints;
    basePoints.reserve(count);

    const double step = (to - from) / static_cast<double>(count - 1);

    for (std::size_t index = 0; index < count; index++)
    {
        const double x = from + step * static_cast<double>(index);
        basePoints.push_back(evaluatePoint(tree, x));
    }

    SamplePoints points;
    points.reserve(std::min(maxSamplePoints, count * 2));
    points.push_back(basePoints.front());

    for (std::size_t index = 1; index < basePoints.size(); index++)
        appendAdaptiveInterval(tree, basePoints[index - 1], basePoints[index], 0, points);

    return SampleResult::success(std::move(points));
}


}
