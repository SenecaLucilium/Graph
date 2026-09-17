#include "ExpressionEngine.h"

#include "Lexer/Lexer.h"

#include <string>
#include <utility>

namespace src::core::ExpressionEngine
{

CompileResult Engine::compile(std::string_view source)
{
    Lexer::Lexer lexer{std::string(source)};
    auto tokenizeResult = lexer.tokenize();

    if (!tokenizeResult) return CompileResult::failure(tokenizeResult.error());

    auto tokens = std::move(tokenizeResult).value();
    return Parser::parse(tokens);
}

Evaluator::EvaluationResult Engine::evaluate(const Parser::ExpressionTree& tree, double x)
{
    return Evaluator::evaluate(tree, x);
}

Graph::SampleResult Engine::sample(const Parser::ExpressionTree& tree, double from, double to, std::size_t count)
{
    return Graph::sample(tree, from, to, count);
}

}
