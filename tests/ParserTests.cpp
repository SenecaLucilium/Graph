#include <gtest/gtest.h>

#include "Lexer.h"
#include "Parser.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace lexer = src::core::ExpressionEngine::Lexer;
namespace parser = src::core::ExpressionEngine::Parser;
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

    parser::ParseResult parseResult(const std::string& source)
    {
        lexer::Lexer lexer(source);
        auto tokenizeResult = lexer.tokenize();
        if (!tokenizeResult) throw std::runtime_error(tokenizeResult.error().message);
        auto tokens = std::move(tokenizeResult).value();
        return parser::parse(tokens);
    }

    const parser::NumberNode* expectNumberNode(const parser::ExpressionNode* node, double value)
    {
        auto* numberNode = dynamic_cast<const parser::NumberNode*>(node);

        EXPECT_NE(numberNode, nullptr);
        if (!numberNode) return nullptr;

        EXPECT_DOUBLE_EQ(numberNode->getNumber(), value);
        return numberNode;
    }

    void expectVariableNode(const parser::ExpressionNode* node)
    {
        EXPECT_NE(dynamic_cast<const parser::VariableNode*>(node), nullptr);
    }

    void expectConstantNode(const parser::ExpressionNode* node, parser::ConstantNode::ConstantType type)
    {
        auto* constantNode = dynamic_cast<const parser::ConstantNode*>(node);

        ASSERT_NE(constantNode, nullptr);
        EXPECT_EQ(constantNode->getType(), type);
    }

    const parser::UnaryOperationNode* expectUnaryNode(const parser::ExpressionNode* node, parser::UnaryOperationNode::UnaryType type)
    {
        auto* unaryNode = dynamic_cast<const parser::UnaryOperationNode*>(node);

        EXPECT_NE(unaryNode, nullptr);
        if (!unaryNode) return nullptr;

        EXPECT_EQ(unaryNode->getType(), type);
        return unaryNode;
    }

    const parser::BinaryOperationNode* expectBinaryNode(const parser::ExpressionNode* node, parser::BinaryOperationNode::BinaryType type
    )
    {
        auto* binaryNode = dynamic_cast<const parser::BinaryOperationNode*>(node);

        EXPECT_NE(binaryNode, nullptr);
        if (!binaryNode) return nullptr;

        EXPECT_EQ(binaryNode->getType(), type);
        return binaryNode;
    }

    const parser::FunctionNode* expectFunctionNode(const parser::ExpressionNode* node, parser::FunctionNode::FunctionType type
    )
    {
        auto* functionNode = dynamic_cast<const parser::FunctionNode*>(node);

        EXPECT_NE(functionNode, nullptr);
        if (!functionNode) return nullptr;

        EXPECT_EQ(functionNode->getType(), type);
        return functionNode;
    }
}

TEST(ParserTreeTests, BuildsNumberNode)
{
    auto tree = parseSource("42");

    ASSERT_NE(tree, nullptr);
    expectNumberNode(tree->getRoot(), 42.0);
}

TEST(ParserTreeTests, BuildsVariableNode)
{
    auto tree = parseSource("x");

    ASSERT_NE(tree, nullptr);
    expectVariableNode(tree->getRoot());
}

TEST(ParserTreeTests, BuildsPiConstantNode)
{
    auto tree = parseSource("pi");

    ASSERT_NE(tree, nullptr);
    expectConstantNode(tree->getRoot(), parser::ConstantNode::ConstantType::Pi);
}

TEST(ParserTreeTests, BuildsEConstantNode)
{
    auto tree = parseSource("e");

    ASSERT_NE(tree, nullptr);
    expectConstantNode(tree->getRoot(), parser::ConstantNode::ConstantType::E);
}

TEST(ParserTreeTests, BuildsAdditionNode)
{
    auto tree = parseSource("2 + 3");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Plus
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 2.0);
    expectNumberNode(root->getRightChild(), 3.0);
}

TEST(ParserTreeTests, BuildsSubtractionNode)
{
    auto tree = parseSource("5 - 2");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Minus
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 5.0);
    expectNumberNode(root->getRightChild(), 2.0);
}

TEST(ParserTreeTests, BuildsMultiplicationNode)
{
    auto tree = parseSource("2 * 3");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Star
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 2.0);
    expectNumberNode(root->getRightChild(), 3.0);
}

TEST(ParserTreeTests, BuildsDivisionNode)
{
    auto tree = parseSource("8 / 2");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Slash
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 8.0);
    expectNumberNode(root->getRightChild(), 2.0);
}

TEST(ParserTreeTests, BuildsPowerNode)
{
    auto tree = parseSource("2 ^ 3");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Caret
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 2.0);
    expectNumberNode(root->getRightChild(), 3.0);
}

TEST(ParserTreeTests, RespectsMultiplicationPriority)
{
    auto tree = parseSource("1 + 2 * 3");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Plus
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 1.0);

    auto* right = expectBinaryNode(
        root->getRightChild(),
        parser::BinaryOperationNode::BinaryType::Star
    );

    ASSERT_NE(right, nullptr);
    expectNumberNode(right->getLeftChild(), 2.0);
    expectNumberNode(right->getRightChild(), 3.0);
}

TEST(ParserTreeTests, RespectsParenthesesPriority)
{
    auto tree = parseSource("(1 + 2) * 3");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Star
    );

    ASSERT_NE(root, nullptr);

    auto* left = expectBinaryNode(
        root->getLeftChild(),
        parser::BinaryOperationNode::BinaryType::Plus
    );

    ASSERT_NE(left, nullptr);
    expectNumberNode(left->getLeftChild(), 1.0);
    expectNumberNode(left->getRightChild(), 2.0);
    expectNumberNode(root->getRightChild(), 3.0);
}

TEST(ParserTreeTests, PowerIsRightAssociative)
{
    auto tree = parseSource("2 ^ 3 ^ 4");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Caret
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 2.0);

    auto* right = expectBinaryNode(
        root->getRightChild(),
        parser::BinaryOperationNode::BinaryType::Caret
    );

    ASSERT_NE(right, nullptr);
    expectNumberNode(right->getLeftChild(), 3.0);
    expectNumberNode(right->getRightChild(), 4.0);
}

TEST(ParserTreeTests, AdditionAndSubtractionAreLeftAssociative)
{
    auto tree = parseSource("1 - 2 + 3");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Plus
    );

    ASSERT_NE(root, nullptr);

    auto* left = expectBinaryNode(
        root->getLeftChild(),
        parser::BinaryOperationNode::BinaryType::Minus
    );

    ASSERT_NE(left, nullptr);
    expectNumberNode(left->getLeftChild(), 1.0);
    expectNumberNode(left->getRightChild(), 2.0);
    expectNumberNode(root->getRightChild(), 3.0);
}

TEST(ParserTreeTests, BuildsUnaryPlusNode)
{
    auto tree = parseSource("+2");
    auto* root = expectUnaryNode(
        tree->getRoot(),
        parser::UnaryOperationNode::UnaryType::Plus
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getChild(), 2.0);
}

TEST(ParserTreeTests, BuildsUnaryMinusNode)
{
    auto tree = parseSource("-2");
    auto* root = expectUnaryNode(
        tree->getRoot(),
        parser::UnaryOperationNode::UnaryType::Minus
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getChild(), 2.0);
}

TEST(ParserTreeTests, BuildsNestedUnaryNodes)
{
    auto tree = parseSource("--x");
    auto* outer = expectUnaryNode(
        tree->getRoot(),
        parser::UnaryOperationNode::UnaryType::Minus
    );

    ASSERT_NE(outer, nullptr);

    auto* inner = expectUnaryNode(
        outer->getChild(),
        parser::UnaryOperationNode::UnaryType::Minus
    );

    ASSERT_NE(inner, nullptr);
    expectVariableNode(inner->getChild());
}

TEST(ParserTreeTests, BuildsSinFunctionNode)
{
    auto tree = parseSource("sin(x)");
    auto* root = expectFunctionNode(
        tree->getRoot(),
        parser::FunctionNode::FunctionType::Sin
    );

    ASSERT_NE(root, nullptr);
    expectVariableNode(root->getChild());
}

TEST(ParserTreeTests, BuildsCosFunctionNode)
{
    auto tree = parseSource("cos(2)");
    auto* root = expectFunctionNode(
        tree->getRoot(),
        parser::FunctionNode::FunctionType::Cos
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getChild(), 2.0);
}

TEST(ParserTreeTests, BuildsTgFunctionNode)
{
    auto tree = parseSource("tg(x)");
    auto* root = expectFunctionNode(
        tree->getRoot(),
        parser::FunctionNode::FunctionType::Tg
    );

    ASSERT_NE(root, nullptr);
    expectVariableNode(root->getChild());
}

TEST(ParserTreeTests, BuildsCtgFunctionNode)
{
    auto tree = parseSource("ctg(x)");
    auto* root = expectFunctionNode(
        tree->getRoot(),
        parser::FunctionNode::FunctionType::Ctg
    );

    ASSERT_NE(root, nullptr);
    expectVariableNode(root->getChild());
}

TEST(ParserTreeTests, BuildsSqrtFunctionNode)
{
    auto tree = parseSource("sqrt(9)");
    auto* root = expectFunctionNode(
        tree->getRoot(),
        parser::FunctionNode::FunctionType::Sqrt
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getChild(), 9.0);
}

TEST(ParserTreeTests, BuildsFunctionWithExpressionArgument)
{
    auto tree = parseSource("sin(1 + x)");
    auto* function = expectFunctionNode(
        tree->getRoot(),
        parser::FunctionNode::FunctionType::Sin
    );

    ASSERT_NE(function, nullptr);

    auto* argument = expectBinaryNode(
        function->getChild(),
        parser::BinaryOperationNode::BinaryType::Plus
    );

    ASSERT_NE(argument, nullptr);
    expectNumberNode(argument->getLeftChild(), 1.0);
    expectVariableNode(argument->getRightChild());
}

TEST(ParserTreeTests, BuildsNestedFunctionNodes)
{
    auto tree = parseSource("sqrt(sin(x))");
    auto* outer = expectFunctionNode(
        tree->getRoot(),
        parser::FunctionNode::FunctionType::Sqrt
    );

    ASSERT_NE(outer, nullptr);

    auto* inner = expectFunctionNode(
        outer->getChild(),
        parser::FunctionNode::FunctionType::Sin
    );

    ASSERT_NE(inner, nullptr);
    expectVariableNode(inner->getChild());
}

TEST(ParserTreeTests, BuildsImplicitMultiplicationOfNumberAndVariable)
{
    auto tree = parseSource("2x");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Star
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 2.0);
    expectVariableNode(root->getRightChild());
}

TEST(ParserTreeTests, BuildsImplicitMultiplicationOfVariableAndFunction)
{
    auto tree = parseSource("xsin(x)");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Star
    );

    ASSERT_NE(root, nullptr);
    expectVariableNode(root->getLeftChild());

    auto* function = expectFunctionNode(
        root->getRightChild(),
        parser::FunctionNode::FunctionType::Sin
    );

    ASSERT_NE(function, nullptr);
    expectVariableNode(function->getChild());
}

TEST(ParserTreeTests, BuildsImplicitMultiplicationWithParentheses)
{
    auto tree = parseSource("2(x + 1)");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Star
    );

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 2.0);

    auto* right = expectBinaryNode(
        root->getRightChild(),
        parser::BinaryOperationNode::BinaryType::Plus
    );

    ASSERT_NE(right, nullptr);
    expectVariableNode(right->getLeftChild());
    expectNumberNode(right->getRightChild(), 1.0);
}

TEST(ParserTreeTests, BuildsImplicitMultiplicationOfParenthesizedExpressions)
{
    auto tree = parseSource("(x + 1)(x - 1)");
    auto* root = expectBinaryNode(
        tree->getRoot(),
        parser::BinaryOperationNode::BinaryType::Star
    );

    ASSERT_NE(root, nullptr);

    auto* left = expectBinaryNode(
        root->getLeftChild(),
        parser::BinaryOperationNode::BinaryType::Plus
    );

    ASSERT_NE(left, nullptr);
    expectVariableNode(left->getLeftChild());
    expectNumberNode(left->getRightChild(), 1.0);

    auto* right = expectBinaryNode(
        root->getRightChild(),
        parser::BinaryOperationNode::BinaryType::Minus
    );

    ASSERT_NE(right, nullptr);
    expectVariableNode(right->getLeftChild());
    expectNumberNode(right->getRightChild(), 1.0);
}

TEST(ParserTreeTests, RespectsDivisionPriority)
{
    auto tree = parseSource("1 + 8 / 2");
    auto* root = expectBinaryNode(tree->getRoot(), parser::BinaryOperationNode::BinaryType::Plus);

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 1.0);

    auto* division = expectBinaryNode(root->getRightChild(), parser::BinaryOperationNode::BinaryType::Slash);
    ASSERT_NE(division, nullptr);
    expectNumberNode(division->getLeftChild(), 8.0);
    expectNumberNode(division->getRightChild(), 2.0);
}

TEST(ParserTreeTests, BuildsUnaryOperationInsidePower)
{
    auto tree = parseSource("2 ^ -x");
    auto* root = expectBinaryNode(tree->getRoot(), parser::BinaryOperationNode::BinaryType::Caret);

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 2.0);

    auto* unary = expectUnaryNode(root->getRightChild(), parser::UnaryOperationNode::UnaryType::Minus);
    ASSERT_NE(unary, nullptr);
    expectVariableNode(unary->getChild());
}

TEST(ParserTreeTests, BuildsNestedFunctionsWithImplicitMultiplication)
{
    auto tree = parseSource("2sin(cos(x))");
    auto* root = expectBinaryNode(tree->getRoot(), parser::BinaryOperationNode::BinaryType::Star);

    ASSERT_NE(root, nullptr);
    expectNumberNode(root->getLeftChild(), 2.0);

    auto* sin = expectFunctionNode(root->getRightChild(), parser::FunctionNode::FunctionType::Sin);
    ASSERT_NE(sin, nullptr);
    auto* cos = expectFunctionNode(sin->getChild(), parser::FunctionNode::FunctionType::Cos);
    ASSERT_NE(cos, nullptr);
    expectVariableNode(cos->getChild());
}

TEST(ParserTreeTests, BuildsSeveralNestedBinaryOperations)
{
    auto tree = parseSource("(x + 1) * (2 - (x / 3))");
    auto* root = expectBinaryNode(tree->getRoot(), parser::BinaryOperationNode::BinaryType::Star);

    ASSERT_NE(root, nullptr);
    auto* left = expectBinaryNode(root->getLeftChild(), parser::BinaryOperationNode::BinaryType::Plus);
    ASSERT_NE(left, nullptr);
    expectVariableNode(left->getLeftChild());
    expectNumberNode(left->getRightChild(), 1.0);

    auto* right = expectBinaryNode(root->getRightChild(), parser::BinaryOperationNode::BinaryType::Minus);
    ASSERT_NE(right, nullptr);
    expectNumberNode(right->getLeftChild(), 2.0);
    auto* division = expectBinaryNode(right->getRightChild(), parser::BinaryOperationNode::BinaryType::Slash);
    ASSERT_NE(division, nullptr);
    expectVariableNode(division->getLeftChild());
    expectNumberNode(division->getRightChild(), 3.0);
}

struct InvalidExpressionCase
{
    const char* source;
};

class InvalidExpressionTest
    : public ::testing::TestWithParam<InvalidExpressionCase>
{
};

TEST_P(InvalidExpressionTest, RejectsExpression)
{
    auto result = parseResult(GetParam().source);
    EXPECT_FALSE(result);
}

INSTANTIATE_TEST_SUITE_P(
    Parser,
    InvalidExpressionTest,
    ::testing::Values(
        InvalidExpressionCase{""},
        InvalidExpressionCase{"sin"},
        InvalidExpressionCase{"sinx"},
        InvalidExpressionCase{"sin()"},
        InvalidExpressionCase{"sin(1"},
        InvalidExpressionCase{"1 +"},
        InvalidExpressionCase{"1 *"},
        InvalidExpressionCase{"2 ^"},
        InvalidExpressionCase{"2 ** 3"},
        InvalidExpressionCase{"(1 + 2"},
        InvalidExpressionCase{"1 + 2)"},
        InvalidExpressionCase{"()"},
        InvalidExpressionCase{"1)"}
    )
);

TEST(ParserTokenValidation, RejectsEmptyList)
{
    std::vector<std::unique_ptr<lexer::Token>> tokens;

    auto result = parser::parse(tokens);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, diagnostics::ErrorCode::ExpectedExpression);
}

TEST(ParserTokenValidation, RejectsEmptyToken)
{
    std::vector<std::unique_ptr<lexer::Token>> tokens;

    tokens.push_back(nullptr);
    tokens.push_back(std::make_unique<lexer::Token>(
        lexer::Token::TokenType::End,
        "",
        0
    ));

    auto result = parser::parse(tokens);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, diagnostics::ErrorCode::UnexpectedToken);
}

TEST(ParserTokenValidation, RejectsMissingEndToken)
{
    std::vector<std::unique_ptr<lexer::Token>> tokens;

    tokens.push_back(std::make_unique<lexer::NumberToken>(
        "2",
        0,
        2.0
    ));

    auto result = parser::parse(tokens);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, diagnostics::ErrorCode::UnexpectedToken);
}

TEST(ParserTokenValidation, RejectsEndInTheMiddle)
{
    std::vector<std::unique_ptr<lexer::Token>> tokens;

    tokens.push_back(std::make_unique<lexer::Token>(
        lexer::Token::TokenType::End,
        "",
        0
    ));

    tokens.push_back(std::make_unique<lexer::NumberToken>(
        "2",
        1,
        2.0
    ));

    auto result = parser::parse(tokens);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, diagnostics::ErrorCode::UnexpectedToken);
}

TEST(ParserTokenValidation, RejectsInvalidToken)
{
    std::vector<std::unique_ptr<lexer::Token>> tokens;

    tokens.push_back(std::make_unique<lexer::Token>(
        lexer::Token::TokenType::Invalid,
        "@",
        0
    ));

    tokens.push_back(std::make_unique<lexer::Token>(
        lexer::Token::TokenType::End,
        "",
        1
    ));

    auto result = parser::parse(tokens);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, diagnostics::ErrorCode::UnexpectedToken);
}

TEST(ParserErrors, ReportsMissingFunctionParenthesis)
{
    auto result = parseResult("sin x");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, diagnostics::ErrorCode::MissingFunctionParenthesis);
    EXPECT_EQ(result.error().domain, diagnostics::ErrorDomain::Parser);
    EXPECT_NE(result.error().message.find("parenthesis"), std::string::npos);
}

TEST(ParserErrors, ReportsMissingRightParenthesis)
{
    auto result = parseResult("(x + 1");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, diagnostics::ErrorCode::MissingRightParenthesis);
    EXPECT_EQ(result.error().domain, diagnostics::ErrorDomain::Parser);
}

TEST(ParserErrors, ReportsTrailingTokens)
{
    auto result = parseResult("1)");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, diagnostics::ErrorCode::TrailingTokens);
}
