#pragma once

#include <memory>

namespace src::core::ExpressionEngine::Parser
{
class ExpressionNode
{
public:
    ExpressionNode() = default;
    virtual ~ExpressionNode() = default;

    virtual double calculate(double x) const = 0;
};

class ExpressionTree
{
public:
    ExpressionTree(std::unique_ptr<ExpressionNode> root_);
    ~ExpressionTree() = default;

    double calculate(double x) const;
private:
    std::unique_ptr<ExpressionNode> root;
};

class NumberNode : public ExpressionNode
{
private:
    double number;

public:
    NumberNode(double number_) : number(number_) {};

    double calculate([[maybe_unused]] double x) const final;
};

class VariableNode : public ExpressionNode
{
public:
    VariableNode() {};

    double calculate(double x) const final;
};

class ConstantNode : public ExpressionNode
{
public:
    enum class ConstantType {Pi, E};

    ConstantNode(ConstantType type_) : type(type_) {};

    double calculate([[maybe_unused]] double x) const final;

private:
    ConstantType type;
};

class UnaryOperationNode : public ExpressionNode
{
public:
    enum class UnaryType {Plus, Minus};

    UnaryOperationNode(UnaryType type_, std::unique_ptr<ExpressionNode> child_)
        : type(type_), child(std::move(child_)) {};

    double calculate(double x) const final;

private:
    UnaryType type;
    std::unique_ptr<ExpressionNode> child;
};

class BinaryOperationNode : public ExpressionNode
{
public:
    enum class BinaryType {Plus, Minus, Star, Slash, Caret};

    BinaryOperationNode(BinaryType type_, std::unique_ptr<ExpressionNode> leftChild_, std::unique_ptr<ExpressionNode> rightChild_)
        : type(type_), leftChild(std::move(leftChild_)), rightChild(std::move(rightChild_)) {};
    
    double calculate(double x) const final;

private:
    BinaryType type;
    std::unique_ptr<ExpressionNode> leftChild;
    std::unique_ptr<ExpressionNode> rightChild;
};

class FunctionNode : public ExpressionNode
{
public:
    enum class FunctionType {Sin, Cos, Tg, Ctg, Sqrt};

    FunctionNode(FunctionType type_, std::unique_ptr<ExpressionNode> child_)
        : type(type_), child(std::move(child_)) {};

    double calculate(double x) const final;

private:
    FunctionType type;
    std::unique_ptr<ExpressionNode> child;
};

}