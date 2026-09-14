#include "GraphSampler.h"

#include "Evaluator.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

namespace diagnostics = src::common::Diagnostics;
namespace evaluator = src::core::ExpressionEngine::Evaluator;

namespace src::core::ExpressionEngine::Graph
{
namespace
{

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

}

SampleResult sample(const src::core::ExpressionEngine::Parser::ExpressionTree& tree, double from, double to, std::size_t count)
{
    if (count < 2)
    {
        return makeInvalidArgumentError("At least two sample points are required", {{"count", static_cast<std::int64_t>(count)}});
    }

    if (!std::isfinite(from) || !std::isfinite(to))
    {
        return makeInvalidArgumentError("Sampling range must contain finite values", {{"from", from}, {"to", to}});
    }

    if (from >= to)
    {
        return makeInvalidArgumentError("Sampling range must satisfy from < to", {{"from", from}, {"to", to}});
    }

    SamplePoints points;
    points.reserve(count);

    const double step = (to - from) / static_cast<double>(count - 1);

    for (std::size_t index = 0; index < count; index++)
    {
        const double x = from + step * static_cast<double>(index);
        auto evaluationResult = evaluator::evaluate(tree, x);

        SamplePoint point;
        point.x = x;

        if (evaluationResult) point.y = evaluationResult.value();
        else point.y = std::nullopt;

        points.push_back(std::move(point));
    }

    return SampleResult::success(std::move(points));
}


}