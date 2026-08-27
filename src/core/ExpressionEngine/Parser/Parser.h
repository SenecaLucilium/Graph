#pragma once

#include "ExpressionNode.h"
#include "Token.h"

#include <vector>

using namespace src::core::ExpressionEngine::Lexer;

namespace src::core::ExpressionEngine::Parser
{
namespace
{
void validateTokens(const std::vector<std::unique_ptr<Token>>& tokens);

std::unique_ptr<ExpressionNode> parseExpression(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseAdditive(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseMultiplicative(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parsePower(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseUnary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parsePrimary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseFunctionCall(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
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