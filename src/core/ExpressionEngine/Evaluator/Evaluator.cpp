#include "Evaluator.h"

#include <cmath>
#include <numbers>
#include <string>
#include <utility>

namespace diagnostics = src::common::Diagnostics;
namespace parser = src::core::ExpressionEngine::Parser;

namespace src::core::ExpressionEngine::Evaluator
{
namespace
{
using diagnostics::ErrorCode;

EvaluationResult evaluationError(
    ErrorCode code,
    std::string message,
    diagnostics::Context context = {}
)
{
    diagnostics::Errors::Error error;
    error.domain = diagnostics::ErrorDomain::Evaluation;
    error.code = code;
    error.errorLevel = diagnostics::ErrorLevel::Error;
    error.message = std::move(message);
    error.context = std::move(context);

    return EvaluationResult::failure(std::move(error));
}

EvaluationResult internalError(std::string message)
{
    diagnostics::Errors::Error error;
    error.domain = diagnostics::ErrorDomain::Internal;
    error.code = ErrorCode::Unknown;
    error.errorLevel = diagnostics::ErrorLevel::Error;
    error.message = std::move(message);

    return EvaluationResult::failure(std::move(error));
}

EvaluationResult propagate(const EvaluationResult& result)
{
    return EvaluationResult::failure(result.error());
}

EvaluationResult finiteResult(
    double value,
    std::string operation,
    ErrorCode nanCode = ErrorCode::NonFiniteResult
)
{
    if (std::isnan(value))
    {
        diagnostics::Context context;
        context["operation"] = std::move(operation);
        return evaluationError(nanCode, "Evaluation produced NaN", std::move(context));
    }

    if (std::isinf(value))
    {
        diagnostics::Context context;
        context["operation"] = std::move(operation);
        return evaluationError(
            ErrorCode::NonFiniteResult,
            "Evaluation produced an infinite result",
            std::move(context)
        );
    }

    return EvaluationResult::success(value);
}

EvaluationResult evaluateNode(const parser::ExpressionNode& node, double x)
{
    if (const auto* number = dynamic_cast<const parser::NumberNode*>(&node))
        return finiteResult(number->getNumber(), "number literal");

    if (dynamic_cast<const parser::VariableNode*>(&node))
        return finiteResult(x, "variable x");

    if (const auto* constant = dynamic_cast<const parser::ConstantNode*>(&node))
    {
        switch (constant->getType())
        {
        case parser::ConstantNode::ConstantType::Pi:
            return EvaluationResult::success(std::numbers::pi);
        case parser::ConstantNode::ConstantType::E:
            return EvaluationResult::success(std::numbers::e);
        }

        return internalError("Unknown constant type");
    }

    if (const auto* unary = dynamic_cast<const parser::UnaryOperationNode*>(&node))
    {
        const auto* child = unary->getChild();
        if (!child) return internalError("Unary operation has no child");

        const EvaluationResult childResult = evaluateNode(*child, x);
        if (!childResult) return propagate(childResult);

        switch (unary->getType())
        {
        case parser::UnaryOperationNode::UnaryType::Plus:
            return EvaluationResult::success(childResult.value());
        case parser::UnaryOperationNode::UnaryType::Minus:
            return finiteResult(-childResult.value(), "unary operation");
        }

        return internalError("Unknown unary operation type");
    }

    if (const auto* binary = dynamic_cast<const parser::BinaryOperationNode*>(&node))
    {
        const auto* leftChild = binary->getLeftChild();
        const auto* rightChild = binary->getRightChild();
        if (!leftChild || !rightChild)
            return internalError("Binary operation has a missing child");

        const EvaluationResult leftResult = evaluateNode(*leftChild, x);
        if (!leftResult) return propagate(leftResult);

        const EvaluationResult rightResult = evaluateNode(*rightChild, x);
        if (!rightResult) return propagate(rightResult);

        const double leftValue = leftResult.value();
        const double rightValue = rightResult.value();

        switch (binary->getType())
        {
        case parser::BinaryOperationNode::BinaryType::Plus:
            return finiteResult(leftValue + rightValue, "addition");
        case parser::BinaryOperationNode::BinaryType::Minus:
            return finiteResult(leftValue - rightValue, "subtraction");
        case parser::BinaryOperationNode::BinaryType::Star:
            return finiteResult(leftValue * rightValue, "multiplication");
        case parser::BinaryOperationNode::BinaryType::Slash:
            if (rightValue == 0.0)
            {
                diagnostics::Context context;
                context["dividend"] = leftValue;
                context["divisor"] = rightValue;
                return evaluationError(ErrorCode::DivisionByZero, "Division by zero", std::move(context));
            }
            return finiteResult(leftValue / rightValue, "division");
        case parser::BinaryOperationNode::BinaryType::Caret:
        {
            const double result = std::pow(leftValue, rightValue);
            const ErrorCode nanCode = std::isnan(result)
                ? ErrorCode::InvalidArgument
                : ErrorCode::NonFiniteResult;
            return finiteResult(result, "power", nanCode);
        }
        }

        return internalError("Unknown binary operation type");
    }

    if (const auto* function = dynamic_cast<const parser::FunctionNode*>(&node))
    {
        const auto* child = function->getChild();
        if (!child) return internalError("Function has no argument");

        const EvaluationResult childResult = evaluateNode(*child, x);
        if (!childResult) return propagate(childResult);

        const double value = childResult.value();

        switch (function->getType())
        {
        case parser::FunctionNode::FunctionType::Sin:
            return finiteResult(std::sin(value), "sin");
        case parser::FunctionNode::FunctionType::Cos:
            return finiteResult(std::cos(value), "cos");
        case parser::FunctionNode::FunctionType::Tg:
            return finiteResult(std::tan(value), "tg");
        case parser::FunctionNode::FunctionType::Ctg:
        {
            const double tangent = std::tan(value);
            if (tangent == 0.0)
                return evaluationError(ErrorCode::DivisionByZero, "Cotangent is undefined for this argument");
            return finiteResult(1.0 / tangent, "ctg");
        }
        case parser::FunctionNode::FunctionType::Sqrt:
            if (value < 0.0)
            {
                diagnostics::Context context;
                context["argument"] = value;
                return evaluationError(
                    ErrorCode::InvalidArgument,
                    "Square root argument must be non-negative",
                    std::move(context)
                );
            }
            return finiteResult(std::sqrt(value), "sqrt");
        }

        return internalError("Unknown function type");
    }

    return internalError("Unknown expression node type");
}

} // namespace

EvaluationResult evaluate(const parser::ExpressionTree& tree, double x)
{
    const auto* root = tree.getRoot();
    if (!root) return internalError("Expression tree has no root");

    return evaluateNode(*root, x);
}

} // namespace src::core::ExpressionEngine::Evaluator
