#include "Parser.h"

#include <stdexcept>

using namespace src::core::ExpressionEngine::Lexer;
namespace diagnostics = src::common::Diagnostics;

namespace src::core::ExpressionEngine::Parser
{
namespace
{
using diagnostics::ErrorCode;
using Error = diagnostics::Errors::Error;

class ParserException final : public std::runtime_error
{
public:
    explicit ParserException(Error error_)
        : std::runtime_error(error_.message), error(std::move(error_)) {}

    Error error;
};

[[noreturn]] void fail(ErrorCode code, std::string message, const Token* token = nullptr)
{
    Error error;
    error.domain = diagnostics::ErrorDomain::Parser;
    error.code = code;
    error.errorLevel = diagnostics::ErrorLevel::Error;
    error.message = std::move(message);

    if (token)
    {
        error.location = diagnostics::SourceLocation{
            .position = static_cast<std::size_t>(token->position),
            .length = token->text.empty() ? 1 : token->text.size()
        };
        error.context["token"] = token->text;
    }

    throw ParserException(std::move(error));
}

const Token* getCurrentToken(const std::vector<std::unique_ptr<Token>>& tokens, size_t position)
{
    return position < tokens.size() ? tokens[position].get() : nullptr;
}

const Token& requireCurrentToken(const std::vector<std::unique_ptr<Token>>& tokens, size_t position)
{
    const Token* token = getCurrentToken(tokens, position);
    if (!token) fail(ErrorCode::UnexpectedEndOfExpression, "Unexpected end of expression");
    return *token;
}

std::unique_ptr<ExpressionNode> parseExpression(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseAdditive(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseMultiplicative(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parsePower(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseUnary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parsePrimary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseFunctionCall(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);

void validateTokens(const std::vector<std::unique_ptr<Token>>& tokens)
{
    if (tokens.empty()) fail(ErrorCode::ExpectedExpression, "Cannot parse empty token list");

    bool endFound = false;

    for (size_t position = 0; position < tokens.size(); position++)
    {
        if (!tokens[position]) fail(ErrorCode::UnexpectedToken, "Token is empty");

        if (tokens[position]->type == Token::TokenType::Invalid)
            fail(ErrorCode::UnexpectedToken, "Invalid token: " + tokens[position]->text, tokens[position].get());

        if (tokens[position]->type == Token::TokenType::End)
        {
            if (position != tokens.size() - 1 || endFound)
                fail(ErrorCode::UnexpectedToken, "End token must be the last token", tokens[position].get());
            endFound = true;
        }
    }

    if (!endFound) fail(ErrorCode::UnexpectedToken, "End token not found");
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

        switch (requireCurrentToken(tokens, currentPos).type)
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

        switch (requireCurrentToken(tokens, currentPos).type)
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
            if (requireCurrentToken(tokens, currentPos).type == Token::TokenType::Number ||
                requireCurrentToken(tokens, currentPos).type == Token::TokenType::Identifier ||
                requireCurrentToken(tokens, currentPos).type == Token::TokenType::LeftParen)
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

    if (requireCurrentToken(tokens, currentPos).type == Token::TokenType::Caret)
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
    switch(requireCurrentToken(tokens, currentPos).type)
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
    const Token* token = &requireCurrentToken(tokens, currentPos);

    switch (token->type)
    {
    case Token::TokenType::Number:
    {
        auto* numberToken = dynamic_cast<const NumberToken*>(token);

        if (!numberToken) fail(ErrorCode::UnexpectedToken, "Invalid number token", token);

        currentPos++;
        return std::make_unique<NumberNode>(numberToken->number);
    }
    case Token::TokenType::Identifier:
    {
        auto* identifierToken = dynamic_cast<const IdentifierToken*>(token);

        if (!identifierToken) fail(ErrorCode::UnexpectedToken, "Invalid identifier token", token);

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
            fail(ErrorCode::UnexpectedToken, "Invalid identifier: " + identifierToken->text, token);
        }
        
        throw std::logic_error("Unknown identifier type");
    }
    case Token::TokenType::LeftParen:
    {
        currentPos++;
        std::unique_ptr<ExpressionNode> expression = parseExpression(tokens, currentPos);

        if (requireCurrentToken(tokens, currentPos).type != Token::TokenType::RightParen)
            fail(ErrorCode::MissingRightParenthesis, "Right parenthesis expected", getCurrentToken(tokens, currentPos));
        
        currentPos++;
        return expression;
    }
    default:
        fail(ErrorCode::UnexpectedToken, "Unexpected token: " + token->text, token);
    }
}

std::unique_ptr<ExpressionNode> parseFunctionCall(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    const Token* token = getCurrentToken(tokens, currentPos);
    auto* identifierToken = dynamic_cast<const IdentifierToken*>(token);

    if (!identifierToken) fail(ErrorCode::UnexpectedToken, "Function identifier expected", getCurrentToken(tokens, currentPos));

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
        fail(ErrorCode::UnexpectedToken, "Identifier is not a function", getCurrentToken(tokens, currentPos));
    }

    currentPos++;

    if (requireCurrentToken(tokens, currentPos).type != Token::TokenType::LeftParen)
        fail(ErrorCode::MissingFunctionParenthesis, "Left parenthesis expected after function", getCurrentToken(tokens, currentPos));

    currentPos++;

    std::unique_ptr<ExpressionNode> child = parseExpression(tokens, currentPos);

    if (requireCurrentToken(tokens, currentPos).type != Token::TokenType::RightParen)
        fail(ErrorCode::MissingRightParenthesis, "Right parenthesis expected after function argument", getCurrentToken(tokens, currentPos));

    currentPos++;

    return std::make_unique<FunctionNode>(functionType, std::move(child));
}

}

ParseResult parse(std::vector<std::unique_ptr<Token>>& tokens)
{
    try
    {
        validateTokens(tokens);

        size_t currentPos = 0;
        std::unique_ptr<ExpressionNode> root = parseExpression(tokens, currentPos);
        const Token* token = getCurrentToken(tokens, currentPos);
        if (!token || token->type != Token::TokenType::End)
            fail(ErrorCode::TrailingTokens, "Unexpected token after expression", token);

        return ParseResult::success(std::make_unique<ExpressionTree>(std::move(root)));
    }
    catch (const ParserException& exception)
    {
        return ParseResult::failure(exception.error);
    }
}

}
