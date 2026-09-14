#include <gtest/gtest.h>

#include "Lexer.h"
#include "Parser.h"
#include "Evaluator.h"

#include <cmath>
#include <limits>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <string>

namespace lexer = src::core::ExpressionEngine::Lexer;
namespace parser = src::core::ExpressionEngine::Parser;
namespace evaluator = src::core::ExpressionEngine::Evaluator;
namespace diagnostics = src::common::Diagnostics;

namespace
{
    std::unique_ptr<parser::ExpressionTree> parseSource(const std::string& source)
    {
        lexer::Lexer lexer(source);
        auto tokenizeResult = lexer.tokenize();
        if (!tokenizeResult) throw std::runtime_error(tokenizeResult.error().message);

        auto tokens = std::move(tokenizeResult).value();
        auto parseResult = parser::parse(tokens);
        if (!parseResult) throw std::runtime_error(parseResult.error().message);

        return std::move(parseResult).value();
    }

    evaluator::EvaluationResult evaluate(const std::string& source, double x = 0.0)
    {
        return evaluator::evaluate(*parseSource(source), x);
    }

    void expectValue(const std::string& source, double x, double expected)
    {
        auto result = evaluate(source, x);

        ASSERT_TRUE(result);
        EXPECT_NEAR(result.value(), expected, 1e-12);
    }

    void expectError(const std::string& source, double x, diagnostics::ErrorCode expectedCode)
    {
        auto result = evaluate(source, x);

        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().domain, diagnostics::ErrorDomain::Evaluation);
        EXPECT_EQ(result.error().code, expectedCode);
    }
}

TEST(EvaluationTests, EvaluatesNumber)
{
    expectValue("42", 123.0, 42.0);
}

TEST(EvaluationTests, EvaluatesVariable)
{
    expectValue("x", -3.5, -3.5);
}

TEST(EvaluationTests, EvaluatesConstants)
{
    expectValue("pi", 0.0, std::numbers::pi);
    expectValue("e", 0.0, std::numbers::e);
}

TEST(EvaluationTests, EvaluatesArithmeticAndPriority)
{
    expectValue("1 + 2 * 3", 0.0, 7.0);
    expectValue("(1 + 2) * 3", 0.0, 9.0);
    expectValue("8 / 2", 0.0, 4.0);
    expectValue("2 ^ 3 ^ 2", 0.0, 512.0);
}

TEST(EvaluationTests, EvaluatesUnaryOperations)
{
    expectValue("--x", -4.0, -4.0);
    expectValue("-(x + 2)", 3.0, -5.0);
}

TEST(EvaluationTests, EvaluatesFunctions)
{
    expectValue("sin(0)", 0.0, 0.0);
    expectValue("cos(0)", 0.0, 1.0);
    expectValue("tg(0)", 0.0, 0.0);
    expectValue("ctg(pi / 4)", 0.0, 1.0);
    expectValue("sqrt(9)", 0.0, 3.0);
}

TEST(EvaluationTests, EvaluatesNestedFunctionsAndImplicitMultiplication)
{
    expectValue("2sin(cos(x))", 0.5, 2.0 * std::sin(std::cos(0.5)));
    expectValue("(x + 1)(x - 1)", 3.0, 8.0);
}

TEST(EvaluationErrors, ReportsDivisionByZero)
{
    expectError("1 / 0", 0.0, diagnostics::ErrorCode::DivisionByZero);
}

TEST(EvaluationErrors, ReportsUndefinedCotangent)
{
    expectError("ctg(0)", 0.0, diagnostics::ErrorCode::DivisionByZero);
}

TEST(EvaluationErrors, ReportsInvalidSquareRootArgument)
{
    expectError("sqrt(-1)", 0.0, diagnostics::ErrorCode::InvalidArgument);
}

TEST(EvaluationErrors, ReportsInvalidPowerArgument)
{
    expectError("(-1) ^ 0.5", 0.0, diagnostics::ErrorCode::InvalidArgument);
}

TEST(EvaluationErrors, ReportsNonFinitePowerResult)
{
    expectError("10 ^ 1000", 0.0, diagnostics::ErrorCode::NonFiniteResult);
}

TEST(EvaluationErrors, PropagatesChildError)
{
    expectError("1 + sqrt(-1)", 0.0, diagnostics::ErrorCode::InvalidArgument);
}

TEST(EvaluationErrors, ReportsNonFiniteVariable)
{
    expectError("x", std::numeric_limits<double>::infinity(), diagnostics::ErrorCode::NonFiniteResult);
}
