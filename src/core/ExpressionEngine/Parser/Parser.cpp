#include "Parser.h"

#include <stdexcept>

using namespace src::core::ExpressionEngine::Lexer;

namespace src::core::ExpressionEngine::Parser
{
namespace
{
std::unique_ptr<ExpressionNode> parseExpression(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseAdditive(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseMultiplicative(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parsePower(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseUnary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parsePrimary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseFunctionCall(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);

void validateTokens(const std::vector<std::unique_ptr<Token>>& tokens)
{
    if (tokens.empty()) throw std::invalid_argument("Cannot parse empty token list");

    bool endFound = false;

    for (size_t position = 0; position < tokens.size(); position++)
    {
        if (!tokens[position]) throw std::invalid_argument("Token is empty");

        if (tokens[position]->type == Token::TokenType::End)
        {
            if (position != tokens.size() - 1 || endFound) throw std::invalid_argument("End token must be the last token");
            endFound = true;
        }
    }

    if (!endFound) throw std::invalid_argument("End token not found");
}

std::unique_ptr<ExpressionNode> parseExpression(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    return parseAdditive(tokens, currentPos);
}
std::unique_ptr<ExpressionNode> parseAdditive(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    std::unique_ptr<ExpressionNode> leftChild = parseMultiplicative(tokens, currentPos);

    while (true)
    {
        BinaryOperationNode::BinaryType operationType;

        switch (tokens[currentPos]->type)
        {
        case Token::TokenType::Plus:
            operationType = BinaryOperationNode::BinaryType::Plus;
            break;
        case Token::TokenType::Minus:
            operationType = BinaryOperationNode::BinaryType::Minus;
            break;
        default:
            return leftChild;
        }

        currentPos++;

        std::unique_ptr<ExpressionNode> rightChild = parseMultiplicative(tokens, currentPos);
        leftChild = std::make_unique<BinaryOperationNode>(operationType, std::move(leftChild), std::move(rightChild));
    }
}

std::unique_ptr<ExpressionNode> parseMultiplicative(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    std::unique_ptr<ExpressionNode> leftChild = parsePower(tokens, currentPos);

    while (true)
    {
        BinaryOperationNode::BinaryType operationType;
        bool explicitOperator = false;

        switch (tokens[currentPos]->type)
        {
        case Token::TokenType::Star:
            operationType = BinaryOperationNode::BinaryType::Star;
            explicitOperator = true;
            break;

        case Token::TokenType::Slash:
            operationType = BinaryOperationNode::BinaryType::Slash;
            explicitOperator = true;
            break;

        default:
            if (tokens[currentPos]->type == Token::TokenType::Number ||
                tokens[currentPos]->type == Token::TokenType::Identifier ||
                tokens[currentPos]->type == Token::TokenType::LeftParen)
            {
                operationType = BinaryOperationNode::BinaryType::Star;
            }
            else return leftChild;
        }

        if (explicitOperator) currentPos++;

        std::unique_ptr<ExpressionNode> rightChild = parsePower(tokens, currentPos);
        leftChild = std::make_unique<BinaryOperationNode>(operationType, std::move(leftChild), std::move(rightChild));
    }
}

std::unique_ptr<ExpressionNode> parsePower(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    std::unique_ptr<ExpressionNode> leftChild = parseUnary(tokens, currentPos);

    if (tokens[currentPos]->type == Token::TokenType::Caret)
    {
        currentPos++;

        std::unique_ptr<ExpressionNode> rightChild = parsePower(tokens, currentPos);

        return std::make_unique<BinaryOperationNode>(BinaryOperationNode::BinaryType::Caret, std::move(leftChild), std::move(rightChild));
    }

    return leftChild;
}

std::unique_ptr<ExpressionNode> parseUnary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    UnaryOperationNode::UnaryType operationType;
    switch(tokens[currentPos]->type)
    {
    case Token::TokenType::Plus:
    {
        currentPos++;
        operationType = UnaryOperationNode::UnaryType::Plus;
        break;
    }
    case Token::TokenType::Minus:
    {
        currentPos++;
        operationType = UnaryOperationNode::UnaryType::Minus;
        break;
    }
    default:
        return parsePrimary(tokens, currentPos);
    }

    std::unique_ptr<ExpressionNode> child = parseUnary(tokens, currentPos);
    return std::make_unique<UnaryOperationNode>(operationType, std::move(child));
}

std::unique_ptr<ExpressionNode> parsePrimary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    Token* currentToken = tokens[currentPos].get();

    switch (currentToken->type)
    {
    case Token::TokenType::Number:
    {
        auto* numberToken = dynamic_cast<NumberToken*>(currentToken);

        if (!numberToken) throw std::invalid_argument("Invalid number token");

        currentPos++;
        return std::make_unique<NumberNode>(numberToken->number);
    }
    case Token::TokenType::Identifier:
    {
        auto* identifierToken = dynamic_cast<IdentifierToken*>(currentToken);

        if (!identifierToken) throw std::invalid_argument("Invalid identifier token");

        switch (identifierToken->IType)
        {
        case IdentifierToken::IdentifierType::Variable:
            currentPos++;
            return std::make_unique<VariableNode>();

        case IdentifierToken::IdentifierType::Pi:
            currentPos++;
            return std::make_unique<ConstantNode>(ConstantNode::ConstantType::Pi);

        case IdentifierToken::IdentifierType::E:
            currentPos++;
            return std::make_unique<ConstantNode>(ConstantNode::ConstantType::E);

        case IdentifierToken::IdentifierType::Sin:
        case IdentifierToken::IdentifierType::Cos:
        case IdentifierToken::IdentifierType::Tg:
        case IdentifierToken::IdentifierType::Ctg:
        case IdentifierToken::IdentifierType::Sqrt:
            return parseFunctionCall(tokens, currentPos);

        case IdentifierToken::IdentifierType::Invalid:
            throw std::invalid_argument("Invalid identifier: " + identifierToken->text);
        }
        
        throw std::logic_error("Unknown identifier type");
    }
    case Token::TokenType::LeftParen:
    {
        currentPos++;
        std::unique_ptr<ExpressionNode> expression = parseExpression(tokens, currentPos);

        if (tokens[currentPos]->type != Token::TokenType::RightParen) throw std::invalid_argument("Right parentheses expected");
        
        currentPos++;
        return expression;
    }
    default:
        throw std::invalid_argument("Unexpected token: " + currentToken->text);
    }
}

std::unique_ptr<ExpressionNode> parseFunctionCall(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    auto* identifierToken = dynamic_cast<IdentifierToken*>(tokens[currentPos].get());

    if (!identifierToken) throw std::invalid_argument("Function identifier expected");

    FunctionNode::FunctionType functionType;

    switch (identifierToken->IType)
    {
    case IdentifierToken::IdentifierType::Sin:
        functionType = FunctionNode::FunctionType::Sin;
        break;

    case IdentifierToken::IdentifierType::Cos:
        functionType = FunctionNode::FunctionType::Cos;
        break;

    case IdentifierToken::IdentifierType::Tg:
        functionType = FunctionNode::FunctionType::Tg;
        break;

    case IdentifierToken::IdentifierType::Ctg:
        functionType = FunctionNode::FunctionType::Ctg;
        break;

    case IdentifierToken::IdentifierType::Sqrt:
        functionType = FunctionNode::FunctionType::Sqrt;
        break;

    default:
        throw std::invalid_argument("Identifier is not a function");
    }

    currentPos++;

    if (tokens[currentPos]->type != Token::TokenType::LeftParen) throw std::invalid_argument("Left parenthesis expected after function");

    currentPos++;

    std::unique_ptr<ExpressionNode> child = parseExpression(tokens, currentPos);

    if (tokens[currentPos]->type != Token::TokenType::RightParen) throw std::invalid_argument("Right parenthesis expected after function argument");

    currentPos++;

    return std::make_unique<FunctionNode>(functionType, std::move(child));
}

}

std::unique_ptr<ExpressionTree> parse(std::vector<std::unique_ptr<Token>>& tokens)
{
    validateTokens(tokens);

    size_t currentPos = 0;

    std::unique_ptr<ExpressionNode> root = parseExpression(tokens, currentPos);
    if (currentPos >= tokens.size() || tokens[currentPos]->type != Token::TokenType::End) throw std::invalid_argument("Unexpected token after expression");
    
    return std::make_unique<ExpressionTree>(std::move(root));
}

}