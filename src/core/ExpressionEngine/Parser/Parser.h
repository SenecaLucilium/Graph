#pragma once

#include "ExpressionNode.h"
#include "Token.h"
#include "common/Diagnostics/Result.h"

#include <vector>

namespace src::core::ExpressionEngine::Parser
{

using ParseResult = src::common::Diagnostics::Result<std::unique_ptr<ExpressionTree>>;

ParseResult parse(std::vector<std::unique_ptr<src::core::ExpressionEngine::Lexer::Token>>& tokens);

}
