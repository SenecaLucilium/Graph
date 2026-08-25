#pragma once

#include <string>

namespace src::core::ExpressionEngine::Lexer
{

class Token
{
public:
    enum TokenType
    {
        Number = 1,
        Identifier = 2,

        Plus = 100,
        Minus = 101,
        Star = 102,
        Slash = 103,
        Caret = 104,

        LeftParen = 200,
        RightParen = 201,
        Comma = 202,

        End = 400,
        Invalid = 404
    };

    TokenType type;
    std::string text;
    int position;

    Token (TokenType type_, std::string text_, int position_) : type(type_), text(text_), position(position_) {}

    virtual ~Token() = default;
};

class NumberToken : public Token
{
public:
    double number;

    NumberToken (std::string text_, int position_, double number_) : Token(TokenType::Number, std::move(text_), position_), number(number_) {}
};

class IdentifierToken : public Token
{
public:
    enum IdentifierType
    {
        Variable = 'x',

        Sin = 100,
        Cos = 101,
        Tg = 102,
        Ctg = 103,

        Sqrt = 200,

        Pi = 300,
        E = 301,

        Invalid = 404
    };

    IdentifierType IType;

    IdentifierToken (std::string text_, int position_, IdentifierType IType_) : Token(TokenType::Identifier, std::move(text_), position_), IType(IType_) {}

    static bool isIdentifier(std::string str)
    {
        if (str == "x") return true;
        else if (str == "sin") return true;
        else if (str == "cos") return true;
        else if (str == "tg") return true;
        else if (str == "ctg") return true;
        else if (str == "sqrt") return true;
        else if (str == "pi") return true;
        else if (str == "e") return true;
        else return false;
    }

    static IdentifierType whichIdentifier(std::string str)
    {
        if (str == "x") return IdentifierToken::IdentifierType::Variable;
        else if (str == "sin") return IdentifierToken::IdentifierType::Sin;
        else if (str == "cos") return IdentifierToken::IdentifierType::Cos;
        else if (str == "tg") return IdentifierToken::IdentifierType::Tg;
        else if (str == "ctg") return IdentifierToken::IdentifierType::Ctg;
        else if (str == "sqrt") return IdentifierToken::IdentifierType::Sqrt;
        else if (str == "pi") return IdentifierToken::IdentifierType::Pi;
        else if (str == "e") return IdentifierToken::IdentifierType::E;
        else return IdentifierToken::IdentifierType::Invalid;
    }
};

}