#include "ExpressionNode.h"

#include <numbers>
#include <stdexcept>

namespace src::core::ExpressionEngine::Parser
{

double NumberNode::calculate([[maybe_unused]] double x) { return this->number; }

double VariableNode::calculate(double x) { return x; }

double ConstantNode::calculate([[maybe_unused]] double x) {
    switch (this->type)
    {
    case ConstantType::Pi:
        return std::numbers::pi;
    case ConstantType::E:
        return std::numbers::e;
    }

    throw std::logic_error("Unknown constant type");
}

double UnaryOperationNode::calculate(double x) {
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

}