#pragma once

#include <memory>

namespace src::core::ExpressionEngine::Parser
{
class ExpressionNode
{
public:
    ExpressionNode() = default;
    virtual ~ExpressionNode() = default;

    virtual double calculate(double x) = 0;
};

class NumberNode : public ExpressionNode
{
private:
    double number;

public:
    NumberNode(double number_) : number(number_) {};

    double calculate([[maybe_unused]] double x) final;
};

class VariableNode : public ExpressionNode
{
public:
    VariableNode() {};

    double calculate(double x) final;
};

class ConstantNode : public ExpressionNode
{
public:
    enum class ConstantType {Pi, E};

    ConstantNode(ConstantType type_) : type(type_) {};

    double calculate([[maybe_unused]] double x) final;

private:
    ConstantType type;
};

class UnaryOperationNode : public ExpressionNode
{
public:
    enum class UnaryType {Plus, Minus};

    UnaryOperationNode(UnaryType type_, std::unique_ptr<ExpressionNode> child_) : type(type_), child(std::move(child_)) {};

    double calculate(double x) final;

private:
    UnaryType type;
    std::unique_ptr<ExpressionNode> child;
};

}