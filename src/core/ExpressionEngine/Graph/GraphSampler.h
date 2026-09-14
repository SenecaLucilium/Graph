#pragma once

#include "ExpressionNode.h"
#include "common/Diagnostics/Result.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace src::core::ExpressionEngine::Graph
{

struct SamplePoint
{
    double x;
    std::optional<double> y;
};

using SamplePoints = std::vector<SamplePoint>;
using SampleResult = src::common::Diagnostics::Result<SamplePoints>;

SampleResult sample(const src::core::ExpressionEngine::Parser::ExpressionTree& tree, double from, double to, std::size_t count);

}