#pragma once

#include "Evaluator/Evaluator.h"
#include "Graph/GraphSampler.h"
#include "Parser/Parser.h"
#include "common/Diagnostics/Result.h"

#include <cstddef>
#include <memory>
#include <string_view>

namespace src::core::ExpressionEngine
{

using CompileResult = src::common::Diagnostics::Result<std::unique_ptr<Parser::ExpressionTree>>;

class Engine
{
public:
    static CompileResult compile(std::string_view source);
    static Evaluator::EvaluationResult evaluate(const Parser::ExpressionTree& tree, double x);
    static Graph::SampleResult sample(const Parser::ExpressionTree& tree, double from, double to, std::size_t count);
};

}
