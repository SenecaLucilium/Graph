#include <gtest/gtest.h>

#include "Lexer.h"

#include <stdexcept>

using namespace src::core::ExpressionEngine::Lexer;
namespace diagnostics = src::common::Diagnostics;

namespace
{
    std::vector<std::unique_ptr<Token>> tokenize(const std::string& source)
    {
        Lexer lexer(source);
        auto result = lexer.tokenize();
        if (!result) throw std::runtime_error(result.error().message);
        return std::move(result).value();
    }

    void expectToken(const std::unique_ptr<Token>& token, Token::TokenType type, const std::string& text, int position)
    {
        EXPECT_EQ(token->type, type);
        EXPECT_EQ(token->text, text);
        EXPECT_EQ(token->position, position);
    }

    void expectLexingError(const std::string& source, diagnostics::ErrorCode code)
    {
        Lexer lexer(source);
        auto result = lexer.tokenize();

        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().domain, diagnostics::ErrorDomain::Lexer);
        EXPECT_EQ(result.error().code, code);
    }

    void expectNumber(const std::unique_ptr<Token>& token, const std::string& text, int position, double value
    )
    {
        expectToken(token, Token::TokenType::Number, text, position);

        auto* numberToken = dynamic_cast<NumberToken*>(token.get());
        ASSERT_NE(numberToken, nullptr);
        EXPECT_DOUBLE_EQ(numberToken->number, value);
    }

    void expectIdentifier(const std::unique_ptr<Token>& token, const std::string& text, int position, IdentifierToken::IdentifierType identifierType)
    {
        expectToken(token, Token::TokenType::Identifier, text, position);

        auto* identifierToken = dynamic_cast<IdentifierToken*>(token.get());
        ASSERT_NE(identifierToken, nullptr);
        EXPECT_EQ(identifierToken->IType, identifierType);
    }
}

TEST(LexerTests, TokenizesExpressionFromMain)
{
    auto tokens = tokenize("xsin(x^2 + 12 + 2.411 / cos(x)) * e * pi");

    ASSERT_EQ(tokens.size(), 21);

    expectIdentifier(tokens[0], "x", 0, IdentifierToken::IdentifierType::Variable);
    expectIdentifier(tokens[1], "sin", 1, IdentifierToken::IdentifierType::Sin);
    expectToken(tokens[2], Token::TokenType::LeftParen, "(", 4);
    expectIdentifier(tokens[3], "x", 5, IdentifierToken::IdentifierType::Variable);
    expectToken(tokens[4], Token::TokenType::Caret, "^", 6);
    expectNumber(tokens[5], "2", 7, 2.0);
    expectToken(tokens[6], Token::TokenType::Plus, "+", 9);
    expectNumber(tokens[7], "12", 11, 12.0);
    expectToken(tokens[8], Token::TokenType::Plus, "+", 14);
    expectNumber(tokens[9], "2.411", 16, 2.411);
    expectToken(tokens[10], Token::TokenType::Slash, "/", 22);
    expectIdentifier(tokens[11], "cos", 24, IdentifierToken::IdentifierType::Cos);
    expectToken(tokens[12], Token::TokenType::LeftParen, "(", 27);
    expectIdentifier(tokens[13], "x", 28, IdentifierToken::IdentifierType::Variable);
    expectToken(tokens[14], Token::TokenType::RightParen, ")", 29);
    expectToken(tokens[15], Token::TokenType::RightParen, ")", 30);
    expectToken(tokens[16], Token::TokenType::Star, "*", 32);
    expectIdentifier(tokens[17], "e", 34, IdentifierToken::IdentifierType::E);
    expectToken(tokens[18], Token::TokenType::Star, "*", 36);
    expectIdentifier(tokens[19], "pi", 38, IdentifierToken::IdentifierType::Pi);
    expectToken(tokens[20], Token::TokenType::End, "", 40);
}

TEST(LexerTests, TokenizesSupportedIdentifiers)
{
    auto tokens = tokenize("x sin cos tg ctg sqrt pi e");

    ASSERT_EQ(tokens.size(), 9);

    expectIdentifier(tokens[0], "x", 0, IdentifierToken::IdentifierType::Variable);
    expectIdentifier(tokens[1], "sin", 2, IdentifierToken::IdentifierType::Sin);
    expectIdentifier(tokens[2], "cos", 6, IdentifierToken::IdentifierType::Cos);
    expectIdentifier(tokens[3], "tg", 10, IdentifierToken::IdentifierType::Tg);
    expectIdentifier(tokens[4], "ctg", 13, IdentifierToken::IdentifierType::Ctg);
    expectIdentifier(tokens[5], "sqrt", 17, IdentifierToken::IdentifierType::Sqrt);
    expectIdentifier(tokens[6], "pi", 22, IdentifierToken::IdentifierType::Pi);
    expectIdentifier(tokens[7], "e", 25, IdentifierToken::IdentifierType::E);
    expectToken(tokens[8], Token::TokenType::End, "", 26);
}

TEST(LexerTests, TokenizesOperatorsParenthesesAndComma)
{
    auto tokens = tokenize("()+-*/^");

    ASSERT_EQ(tokens.size(), 8);

    expectToken(tokens[0], Token::TokenType::LeftParen, "(", 0);
    expectToken(tokens[1], Token::TokenType::RightParen, ")", 1);
    expectToken(tokens[2], Token::TokenType::Plus, "+", 2);
    expectToken(tokens[3], Token::TokenType::Minus, "-", 3);
    expectToken(tokens[4], Token::TokenType::Star, "*", 4);
    expectToken(tokens[5], Token::TokenType::Slash, "/", 5);
    expectToken(tokens[6], Token::TokenType::Caret, "^", 6);
    expectToken(tokens[7], Token::TokenType::End, "", 7);
}

TEST(LexerTests, TokenizesIntegerAndDecimalNumbers)
{
    auto tokens = tokenize("0 12 2.411 12.");

    ASSERT_EQ(tokens.size(), 5);

    expectNumber(tokens[0], "0", 0, 0.0);
    expectNumber(tokens[1], "12", 2, 12.0);
    expectNumber(tokens[2], "2.411", 5, 2.411);
    expectNumber(tokens[3], "12.", 11, 12.0);
    expectToken(tokens[4], Token::TokenType::End, "", 14);
}

TEST(LexerTests, DoesNotTreatLeadingDotAsNumber)
{
    expectLexingError(".5", diagnostics::ErrorCode::UnexpectedCharacter);
}

TEST(LexerTests, ReportsNumberWithSecondDotAsInvalid)
{
    expectLexingError("2.4.1", diagnostics::ErrorCode::InvalidNumber);
}

TEST(LexerTests, ReportsUnknownCharactersAsError)
{
    expectLexingError("x @ 2", diagnostics::ErrorCode::UnexpectedCharacter);
}

TEST(LexerTests, MarksUnsupportedIdentifierAsInvalidIdentifier)
{
    expectLexingError("abc", diagnostics::ErrorCode::InvalidIdentifier);
}

TEST(LexerTests, DoesNotSupportExpIdentifier)
{
    expectLexingError("exp(x)", diagnostics::ErrorCode::InvalidIdentifier);
}

TEST(LexerTests, AddsEndTokenForEmptyInput)
{
    auto tokens = tokenize("");

    ASSERT_EQ(tokens.size(), 1);
    expectToken(tokens[0], Token::TokenType::End, "", 0);
}
