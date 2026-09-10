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

    diagnostics::Errors::Error makeLexerError(diagnostics::ErrorCode code, std::string message, std::size_t position, std::size_t length, diagnostics::Context context = {})
    {
        diagnostics::Errors::Error error;

        error.domain = diagnostics::ErrorDomain::Lexer;
        error.code = code;
        error.errorLevel = diagnostics::ErrorLevel::Error;
        error.message = std::move(message);
        error.location = diagnostics::SourceLocation{.position = position, .length = length, .col = 0, .row = 0};
        error.context = std::move(context);

        return error;
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

diagnostics::Result<std::unique_ptr<NumberToken>> Lexer::readNumber()
{
    std::string number;
    const std::size_t position = this->currentPosition;
    bool hasDot = false;

    while (this->currentPosition < this->source.size() &&
        (
            std::isdigit(static_cast<unsigned char>(this->source[this->currentPosition])) ||
            this->source[this->currentPosition] == '.'
        )
    )
    {
        if (this->source[this->currentPosition] == '.')
        {
            if (hasDot)
            {
                diagnostics::Context context;
                context["number"] = number;

                return diagnostics::Result<std::unique_ptr<NumberToken>>::failure
                (
                    makeLexerError
                    (
                        diagnostics::ErrorCode::InvalidNumber,
                        "Number contains more than one dot",
                        this->currentPosition,
                        1,
                        std::move(context)
                    )
                );
            }

            hasDot = true;
        }
        number += this->source[this->currentPosition];
        this->currentPosition++;
    }

    return diagnostics::Result<std::unique_ptr<NumberToken>>::success
    (
        std::make_unique<NumberToken>
        (
            number,
            static_cast<int>(position),
            std::stod(number)
        )
    );
}

diagnostics::Result<std::unique_ptr<IdentifierToken>> Lexer::readIdentifier()
{
    std::string identifier;
    const std::size_t position = this->currentPosition;

    while (
        this->currentPosition < this->source.size() &&
        std::isalpha(static_cast<unsigned char>(this->source[this->currentPosition]))
    )
    {
        identifier += this->source[this->currentPosition];
        this->currentPosition++;

        if (IdentifierToken::isIdentifier(identifier)) break;
    }

    const IdentifierToken::IdentifierType type = IdentifierToken::whichIdentifier(identifier);

    if (type == IdentifierToken::IdentifierType::Invalid)
    {
        diagnostics::Context context;
        context["identifier"] = identifier;

        return diagnostics::Result<std::unique_ptr<IdentifierToken>>::failure(
            makeLexerError
            (
                diagnostics::ErrorCode::InvalidIdentifier,
                "Unknown identifier",
                position,
                identifier.size(),
                std::move(context)
            )
        );
    }

    return diagnostics::Result<std::unique_ptr<IdentifierToken>>::success(
        std::make_unique<IdentifierToken>
        (
            identifier,
            static_cast<int>(position),
            type
        )
    );
}

diagnostics::Result<std::vector<std::unique_ptr<Token>>> Lexer::tokenize()
{
    std::vector<std::unique_ptr<Token>> tokenList;

    while (this->currentPosition < this->source.size())
    {
        const char currentCharacter = this->source[this->currentPosition];

        if (std::isspace(static_cast<unsigned char>(currentCharacter)))
        {
            this->skipWhitespace();
            continue;
        }

        if (isparentheses(currentCharacter))
        {
            tokenList.push_back(this->readParentheses());
            continue;
        }

        if (issymbol(currentCharacter))
        {
            tokenList.push_back(this->readMathSymbol());
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(currentCharacter)))
        {
            auto numberResult = this->readNumber();

            if (!numberResult)
            {
                return diagnostics::Result<std::vector<std::unique_ptr<Token>>>::failure(numberResult.error());
            }

            std::unique_ptr<NumberToken> numberToken = std::move(numberResult).value();

            tokenList.push_back(std::move(numberToken));
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(currentCharacter)))
        {
            auto identifierResult = this->readIdentifier();

            if (!identifierResult)
            {
                return diagnostics::Result<std::vector<std::unique_ptr<Token>>>::failure(identifierResult.error());
            }

            std::unique_ptr<IdentifierToken> identifierToken = std::move(identifierResult).value();

            tokenList.push_back(std::move(identifierToken));
            continue;
        }

        diagnostics::Context context;
        context["character"] = std::string(1, currentCharacter);

        return diagnostics::Result<std::vector<std::unique_ptr<Token>>>::failure(
            makeLexerError
            (
                diagnostics::ErrorCode::UnexpectedCharacter,
                "Unexpected character",
                this->currentPosition,
                1,
                std::move(context)
            )
        );
    }

    tokenList.push_back
    (
        std::make_unique<Token>
        (
            Token::TokenType::End,
            "",
            static_cast<int>(this->currentPosition)
        )
    );

    return diagnostics::Result<std::vector<std::unique_ptr<Token>>>::success(std::move(tokenList));
}

}