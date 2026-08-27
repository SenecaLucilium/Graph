#include "ExpressionNode.h"

#include <numbers>
#include <stdexcept>
#include <cmath>

namespace src::core::ExpressionEngine::Parser
{

ExpressionTree::ExpressionTree(std::unique_ptr<ExpressionNode> root_)
{
    if (root_) this->root = std::move(root_);
    else {
        throw std::invalid_argument("Cannot create ExpressionTree with nullptr root");
    }
}

double ExpressionTree::calculate(double x) const
{
    return this->root->calculate(x);
}

double NumberNode::calculate([[maybe_unused]] double x) const { return this->number; }

double VariableNode::calculate(double x) const { return x; }

double ConstantNode::calculate([[maybe_unused]] double x) const
{
    switch (this->type)
    {
    case ConstantType::Pi:
        return std::numbers::pi;
    case ConstantType::E:
        return std::numbers::e;
    }

    throw std::logic_error("Unknown constant type");
}

double UnaryOperationNode::calculate(double x) const
{
    double value = this->child->calculate(x);

    switch (this->type)
    {
    case UnaryType::Plus:
        return value;
    case UnaryType::Minus:
        return -value;
    }

    throw std::logic_error("Unknown unary type");
}

double BinaryOperationNode::calculate(double x) const
{
    double leftValue = this->leftChild->calculate(x);
    double rightValue = this->rightChild->calculate(x);

    switch (this->type)
    {
    case BinaryType::Plus:
        return leftValue + rightValue;
    case BinaryType::Minus:
        return leftValue - rightValue;
    case BinaryType::Star:
        return leftValue * rightValue;
    case BinaryType::Slash:
        return leftValue / rightValue;
    case BinaryType::Caret:
        return std::pow(leftValue, rightValue);
    }

    throw std::logic_error("Unknown binary type");
}

double FunctionNode::calculate(double x) const
{
    double value = this->child->calculate(x);

    switch (this->type)
    {
    case FunctionType::Sin:
        return std::sin(value);
    case FunctionType::Cos:
        return std::cos(value);
    case FunctionType::Tg:
        return std::tan(value);
    case FunctionType::Ctg:
        return 1.0 / std::tan(value);
    case FunctionType::Sqrt:
        return std::sqrt(value);
    }

    throw std::logic_error("Unknown function type");
}
}