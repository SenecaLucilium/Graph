#pragma once

#include "Token.h"
#include "common/Diagnostics/Result.h"

namespace diagnostics = src::common::Diagnostics;

#include <vector>
#include <memory>

namespace src::core::ExpressionEngine::Lexer
{

using TokenList = std::vector<std::unique_ptr<Token>>;
using TokenizeResult = diagnostics::Result<TokenList>;

class Lexer
{
private:
    std::string source;
    int currentPosition;

    void skipWhitespace();
    std::unique_ptr<Token> readParentheses();
    std::unique_ptr<Token> readMathSymbol();
    diagnostics::Result<std::unique_ptr<NumberToken>> readNumber();
    diagnostics::Result<std::unique_ptr<IdentifierToken>> readIdentifier();

public:
    Lexer (std::string source_) : source(source_), currentPosition(0) {}

    TokenizeResult tokenize();
};

}
