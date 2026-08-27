#include "Parser.h"

namespace src::core::ExpressionEngine::Parser
{
std::unique_ptr<ExpressionNode> parseExpression(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{

}
std::unique_ptr<ExpressionNode> parseAdditive();

std::unique_ptr<ExpressionNode> parseMultiplicative();

std::unique_ptr<ExpressionNode> parsePower();

std::unique_ptr<ExpressionNode> parseUnary();

std::unique_ptr<ExpressionNode> parsePrimary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    std::unique_ptr<Token> currToken = std::move(tokens[currentPos]);

    switch (currToken->type)
    {
    case Token::TokenType::Number:
        if (auto numberToken = dynamic_cast<NumberToken*>(currToken.get()))
        {
            currentPos++;
            return std::make_unique<NumberNode>(numberToken->number);
        }
        else throw std::invalid_argument("Number token error");
    case Token::TokenType::Identifier:
        if (auto identifierToken = dynamic_cast<IdentifierToken*>(currToken.get()))
        {
            switch (identifierToken->IType)
            {
            case IdentifierToken::IdentifierType::Variable:
                currentPos++;
                return std::make_unique<VariableNode>();
            case IdentifierToken::IdentifierType::Sin:
            case IdentifierToken::IdentifierType::Cos:
            case IdentifierToken::IdentifierType::Tg:
            case IdentifierToken::IdentifierType::Ctg:
            case IdentifierToken::IdentifierType::Sqrt:
                return parseFunctionCall();
            case IdentifierToken::IdentifierType::Pi:
                currentPos++;
                return std::make_unique<ConstantNode>(ConstantNode::ConstantType::Pi);
            case IdentifierToken::IdentifierType::E:
                currentPos++;
                return std::make_unique<ConstantNode>(ConstantNode::ConstantType::E);
            case IdentifierToken::IdentifierType::Invalid:
                throw std::invalid_argument("Identifier token type is Invalid");
            }

            throw std::logic_error("Unknown identifier token type");
        }
        else throw std::invalid_argument("Identifier token error");
    case Token::TokenType::LeftParen:
        currentPos++;
        std::unique_ptr<ExpressionNode> underParentheses = std::move(parseExpression(tokens, currentPos));
        if (tokens[currentPos]->type == Token::TokenType::RightParen)
        {
            currentPos++;
            return underParentheses;
        }
        else throw std::logic_error("Right parentheses not found");
    }

    throw std::logic_error("Unknown token type");
}

std::unique_ptr<ExpressionNode> parseFunctionCall(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos)
{
    std::unique_ptr<Token> currToken = std::move(tokens[currentPos]);

    currentPos++;

    if (currToken->type == Token::TokenType::LeftParen)
    {
        std::unique_ptr<ExpressionNode> child = std::move(parsePrimary(tokens, currentPos));
        
        if (auto identifierToken = dynamic_cast<IdentifierToken*>(currToken.get()))
        {
            FunctionNode::FunctionType type;
            switch(identifierToken->IType)
            {
            case IdentifierToken::IdentifierType::Sin:
                type = FunctionNode::FunctionType::Sin;
            case IdentifierToken::IdentifierType::Cos:
                type = FunctionNode::FunctionType::Cos;
            case IdentifierToken::IdentifierType::Tg:
                type = FunctionNode::FunctionType::Tg;
            case IdentifierToken::IdentifierType::Ctg:
                type = FunctionNode::FunctionType::Ctg;
            case IdentifierToken::IdentifierType::Sqrt:
                type = FunctionNode::FunctionType::Sqrt;
            default:
                throw std::logic_error("Identifier is not a valid function");
            }

            return std::make_unique<FunctionNode>(type, child);
        }
        else std::logic_error("Identifier is not valid");
    }
    else std::logic_error("No parentheses after function token");
}

std::unique_ptr<ExpressionTree> parse(std::vector<std::unique_ptr<Token>> tokens)
{
    size_t currentPos = 0;


}

}