#pragma once

#include "ExpressionNode.h"
#include "Token.h"

#include <vector>

using namespace src::core::ExpressionEngine::Lexer;

namespace src::core::ExpressionEngine::Parser
{

std::unique_ptr<ExpressionNode> parseExpression();
std::unique_ptr<ExpressionNode> parseAdditive();
std::unique_ptr<ExpressionNode> parseMultiplicative();
std::unique_ptr<ExpressionNode> parsePower();
std::unique_ptr<ExpressionNode> parseUnary();
std::unique_ptr<ExpressionNode> parsePrimary();
std::unique_ptr<ExpressionNode> parseFunctionCall();

std::unique_ptr<ExpressionTree> parse(std::vector<std::unique_ptr<Token>> tokens);

}