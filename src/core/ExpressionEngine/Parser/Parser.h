#pragma once

#include "ExpressionNode.h"
#include "Token.h"

#include <vector>

using namespace src::core::ExpressionEngine::Lexer;

namespace src::core::ExpressionEngine::Parser
{

std::unique_ptr<ExpressionNode> parseExpression(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseAdditive(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseMultiplicative(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parsePower(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseUnary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parsePrimary(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);
std::unique_ptr<ExpressionNode> parseFunctionCall(std::vector<std::unique_ptr<Token>>& tokens, size_t& currentPos);

std::unique_ptr<ExpressionTree> parse(std::vector<std::unique_ptr<Token>> tokens);

}