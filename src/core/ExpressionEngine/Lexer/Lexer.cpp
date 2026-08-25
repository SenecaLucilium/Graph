#include "Lexer.h"

namespace
{
    bool isparentheses(char ch)
    {
        return (ch == '(' || ch == ')') ? true : false;
    }

    bool issymbol(char ch)
    {
        return (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^') ? true : false;
    }
}

namespace src::core::ExpressionEngine::Lexer
{

void Lexer::skipWhitespace()
{
    this->currentPosition++;
    return;
}

std::unique_ptr<Token> Lexer::readParentheses()
{
    std::unique_ptr<Token> token = std::make_unique<Token>(
        (this->source[this->currentPosition] == '(') ? Token::TokenType::LeftParen : Token::TokenType::RightParen,
        std::string(1, this->source[this->currentPosition]),
        this->currentPosition
    );

    this->currentPosition++;

    return token;
}

std::unique_ptr<Token> Lexer::readMathSymbol()
{
    std::unique_ptr<Token> token = std::make_unique<Token>(
        Token::TokenType::Invalid,
        std::string(1, this->source[this->currentPosition]),
        this->currentPosition
    );

    switch (this->source[this->currentPosition]) {
        case '+':
            token->type = Token::TokenType::Plus;
            break;
        case '-':
            token->type = Token::TokenType::Minus;
            break;
        case '*':
            token->type = Token::TokenType::Star;
            break;
        case '/':
            token->type = Token::TokenType::Slash;
            break;
        case '^':
            token->type = Token::TokenType::Caret;
            break;
        default:
            break;
    }

    this->currentPosition++;

    return token;
}

std::unique_ptr<NumberToken> Lexer::readNumber()
{
    std::string number = "";
    int pos = this->currentPosition;

    while (this->currentPosition < this->source.size() && (isdigit(this->source[this->currentPosition]) || this->source[this->currentPosition] == '.'))
    {
        number += this->source[this->currentPosition];
        this->currentPosition++;
    }

    return std::make_unique<NumberToken> (number, pos, std::stod(number));
}

std::unique_ptr<IdentifierToken> Lexer::readIdentifier()
{
    std::string identifier = "";
    int pos = this->currentPosition;
    IdentifierToken::IdentifierType type = IdentifierToken::IdentifierType::Invalid;

    while (this->currentPosition < this->source.size() && isalpha(this->source[this->currentPosition]))
    {
        identifier += this->source[this->currentPosition];
        this->currentPosition++;

        if (IdentifierToken::isIdentifier(identifier)) break;
    }

    return std::make_unique<IdentifierToken> (identifier, pos, IdentifierToken::whichIdentifier(identifier));
}

std::vector<std::unique_ptr<Token>> Lexer::tokenize()
{
    std::vector<std::unique_ptr<Token>> tokenList = {};

    for (this->currentPosition; this->currentPosition < this->source.size();)
    {
        char currCh = this->source[this->currentPosition];

        if (isspace(currCh)) {
            // If whitespace
            this->skipWhitespace();
            continue;
        }
        else if (isparentheses(currCh)) {
            // If parentheses
            tokenList.push_back(this->readParentheses());
        }
        else if (issymbol(currCh)) {
            // If math symbol
            tokenList.push_back(this->readMathSymbol());
        }
        else if (isdigit(currCh)) {
            // If number
            tokenList.push_back(this->readNumber());
        }
        else if (isalpha(currCh)) {
            // If alphabet
            tokenList.push_back(this->readIdentifier());
        }
    }

    return tokenList;
}

}