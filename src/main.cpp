#include <iostream>
#include <vector>

#include "Lexer.h"

using namespace src::core::ExpressionEngine::Lexer;

std::string tokenTypeToString(Token::TokenType type)
{
    switch (type)
    {
        case Token::TokenType::Number:
            return "Number";
        case Token::TokenType::Identifier:
            return "Identifier";
        case Token::TokenType::Plus:
            return "Plus";
        case Token::TokenType::Minus:
            return "Minus";
        case Token::TokenType::Star:
            return "Star";
        case Token::TokenType::Slash:
            return "Slash";
        case Token::TokenType::Caret:
            return "Caret";
        case Token::TokenType::LeftParen:
            return "LeftParen";
        case Token::TokenType::RightParen:
            return "RightParen";
        case Token::TokenType::End:
            return "End";
        case Token::TokenType::Invalid:
            return "Invalid";
        default:
            return "Unknown";
    }
}

std::string identifierTypeToString(IdentifierToken::IdentifierType type)
{
    switch (type)
    {
        case IdentifierToken::IdentifierType::Variable:
            return "x";
        case IdentifierToken::IdentifierType::Sin:
            return "sin";
        case IdentifierToken::IdentifierType::Cos:
            return "cos";
        case IdentifierToken::IdentifierType::Tg:
            return "tg";
        case IdentifierToken::IdentifierType::Ctg:
            return "ctg";
        case IdentifierToken::IdentifierType::Sqrt:
            return "sqrt";
        case IdentifierToken::IdentifierType::Pi:
            return "pi";
        case IdentifierToken::IdentifierType::E:
            return "e";
        case IdentifierToken::IdentifierType::Invalid:
            return "Invalid";
        default:
            return "Unknown";
    }
}

int main()
{
    Lexer lexer("xsin(x^2 + 12 + 2.411 / cos(x)) * e * pi");

    auto tokenizeResult = lexer.tokenize();
    if (!tokenizeResult)
    {
        std::cerr << "Lexing failed: " << tokenizeResult.error().message << '\n';
        return 1;
    }

    std::vector<std::unique_ptr<Token>> tokens = std::move(tokenizeResult).value();

    for (const std::unique_ptr<Token>& token : tokens)
    {
        std::cout
            << "type: " << tokenTypeToString(token->type)
            << ", text: \"" << token->text << "\""
            << ", position: " << token->position;
        if (auto numberToken = dynamic_cast<NumberToken*>(token.get()))
        {
            std::cout
                << ", number: " << numberToken->number;
        }
        if (auto identifierToken = dynamic_cast<IdentifierToken*>(token.get()))
        {
            std::cout
                << ", identifier: " << identifierTypeToString(identifierToken->IType);
        }
        std::cout
            << '\n';
    }

    return 0;
}
