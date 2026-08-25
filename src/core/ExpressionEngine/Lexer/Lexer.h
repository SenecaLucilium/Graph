#pragma once

#include "Token.h"

#include <vector>
#include <memory>

namespace src::core::ExpressionEngine::Lexer
{

class Lexer
{
private:
    std::string source;
    int currentPosition;

    void skipWhitespace();
    std::unique_ptr<Token> readParentheses();
    std::unique_ptr<Token> readMathSymbol();
    std::unique_ptr<NumberToken> readNumber();
    std::unique_ptr<IdentifierToken> readIdentifier();

public:
    Lexer (std::string source_) : source(source_), currentPosition(0) {}

    std::vector<std::unique_ptr<Token>> tokenize();
};

}