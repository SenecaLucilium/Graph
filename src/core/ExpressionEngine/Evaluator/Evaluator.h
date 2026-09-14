#pragma once

#include "ExpressionNode.h"
#include "common/Diagnostics/Result.h"

namespace src::core::ExpressionEngine::Evaluator
{

using EvaluationResult = src::common::Diagnostics::Result<double>;

EvaluationResult evaluate(
    const src::core::ExpressionEngine::Parser::ExpressionTree& tree,
    double x
);

} // namespace src::core::ExpressionEngine::Evaluator
