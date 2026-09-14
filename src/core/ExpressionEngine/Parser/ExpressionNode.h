#pragma once

#include <memory>

namespace src::core::ExpressionEngine::Parser
{
class ExpressionNode
{
public:
    ExpressionNode() = default;
    virtual ~ExpressionNode() = default;

};

class ExpressionTree
{
public:
    ExpressionTree(std::unique_ptr<ExpressionNode> root_);
    ~ExpressionTree() = default;

    const ExpressionNode* getRoot() const { return this->root.get(); };

private:
    std::unique_ptr<ExpressionNode> root;
};

class NumberNode : public ExpressionNode
{
private:
    double number;

public:
    NumberNode(double number_) : number(number_) {};

    double getNumber() const { return this->number; };

};

class VariableNode : public ExpressionNode
{
public:
    VariableNode() {};

};

class ConstantNode : public ExpressionNode
{
public:
    enum class ConstantType {Pi, E};

    ConstantNode(ConstantType type_) : type(type_) {};

    ConstantType getType() const { return this->type; };

private:
    ConstantType type;
};

class UnaryOperationNode : public ExpressionNode
{
public:
    enum class UnaryType {Plus, Minus};

    UnaryOperationNode(UnaryType type_, std::unique_ptr<ExpressionNode> child_)
        : type(type_), child(std::move(child_)) {};

    UnaryType getType() const { return this->type; };
    const ExpressionNode* getChild() const { return this->child.get(); };

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

    BinaryType getType() const { return this->type; };
    const ExpressionNode* getLeftChild() const { return this->leftChild.get(); };
    const ExpressionNode* getRightChild() const { return this->rightChild.get(); };
    
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

    FunctionType getType() const { return this->type; };
    const ExpressionNode* getChild() const { return this->child.get(); };

private:
    FunctionType type;
    std::unique_ptr<ExpressionNode> child;
};

}
