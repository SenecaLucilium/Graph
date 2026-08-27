#pragma once

#include "ExpressionNode.h"
#include "Token.h"

#include <vector>

namespace src::core::ExpressionEngine::Parser
{

std::unique_ptr<ExpressionTree> parse(std::vector<std::unique_ptr<src::core::ExpressionEngine::Lexer::Token>>& tokens);

}