// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "evaluator.h"

#include <string>
#include <vector>

// The significant digits of anything not a whole number.
static constexpr int kDigits = Decimal::Precision;
// Results below 10^1000.
static constexpr int64_t kLargestExponent = 999;
// Enough for every whole number below the limit, and a digit to round on.
static constexpr int kExactDigits = kLargestExponent + 2;
// Smaller magnitudes are zero.
static constexpr int64_t kSmallestExponent = -1000;

namespace {

enum class TokenKind {
    Number,
    Ans,
    Plus,
    Minus,
    Times,
    Divide,
    Percent,
    End,
};

struct Token
{
    TokenKind kind;
    size_t position = 0;
    Decimal value;
};

// An operand and whether it ended in %, for a ± b%.
struct Operand
{
    Decimal value;
    bool percent = false;
    // The value before the last %.
    Decimal percentBase;
};

} // namespace

struct Symbol
{
    const char* text;
    TokenKind kind;
};

static const Symbol kSymbols[] = {
    {"+", TokenKind::Plus},  {"-", TokenKind::Minus},  {"−", TokenKind::Minus},  {"×", TokenKind::Times},
    {"*", TokenKind::Times}, {"÷", TokenKind::Divide}, {"/", TokenKind::Divide}, {"%", TokenKind::Percent},
};

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static std::optional<std::vector<Token>> tokenize(std::string_view text, size_t* errorPosition)
{
    std::vector<Token> tokens;
    size_t i = 0;
    while (i < text.size()) {
        const char c = text[i];
        if (c == ' ' || c == '\t') {
            ++i;
            continue;
        }
        if (isDigit(c) || c == '.') {
            const size_t start = i;
            bool point = false;
            while (i < text.size() && (isDigit(text[i]) || text[i] == '.')) {
                if (text[i] == '.') {
                    if (point)
                        break;
                    point = true;
                }
                ++i;
            }
            const std::optional<Decimal> value = Decimal::parse(text.substr(start, i - start));
            if (!value) {
                *errorPosition = start;
                return std::nullopt;
            }
            tokens.push_back({TokenKind::Number, start, *value});
            continue;
        }
        if (text.substr(i).starts_with("ans")) {
            tokens.push_back({TokenKind::Ans, i, {}});
            i += 3;
            continue;
        }
        bool matched = false;
        for (const Symbol& symbol : kSymbols) {
            if (text.substr(i).starts_with(symbol.text)) {
                tokens.push_back({symbol.kind, i, {}});
                i += std::string_view(symbol.text).size();
                matched = true;
                break;
            }
        }
        if (!matched) {
            *errorPosition = i;
            return std::nullopt;
        }
    }
    tokens.push_back({TokenKind::End, text.size(), {}});
    return tokens;
}

struct Evaluator::Parser
{
    const Evaluator& evaluator;
    std::vector<Token> tokens;
    size_t index = 0;
    Result failure;

    const Token& peek() const { return tokens[index]; }
    const Token& next() { return tokens[index++]; }

    bool failed() const { return failure.error != Error::None; }

    std::nullopt_t fail(Error error, size_t position = 0)
    {
        if (!failed()) {
            failure.error = error;
            failure.position = position;
        }
        return std::nullopt;
    }

    // Whole numbers keep every digit through +, − and ×; anything else keeps the precision.
    static int digitsFor(const Decimal& a, const Decimal& b)
    {
        return a.isInteger() && b.isInteger() ? kExactDigits : kDigits;
    }

    std::optional<Decimal> checked(const Decimal& value)
    {
        if (value.isZero())
            return value;
        const int64_t exponent = value.adjustedExponent();
        if (exponent > kLargestExponent)
            return fail(Error::Overflow);
        if (exponent < kSmallestExponent)
            return Decimal();
        return value;
    }

    std::optional<Operand> expression()
    {
        std::optional<Operand> left = term();
        if (!left)
            return std::nullopt;
        while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus) {
            const bool plus = next().kind == TokenKind::Plus;
            const std::optional<Operand> right = term();
            if (!right)
                return std::nullopt;
            Decimal amount = right->value;
            if (right->percent)
                amount = Decimal::divide(left->value * right->percentBase, Decimal(100));
            const int digits = digitsFor(left->value, amount);
            const std::optional<Decimal> sum = checked(plus ? Decimal::add(left->value, amount, digits)
                                                            : Decimal::subtract(left->value, amount, digits));
            if (!sum)
                return std::nullopt;
            left = Operand{*sum};
        }
        return left;
    }

    std::optional<Operand> term()
    {
        std::optional<Operand> left = unary();
        if (!left)
            return std::nullopt;
        while (peek().kind == TokenKind::Times || peek().kind == TokenKind::Divide) {
            const Token& op = next();
            const std::optional<Operand> right = unary();
            if (!right)
                return std::nullopt;
            std::optional<Decimal> result;
            if (op.kind == TokenKind::Times) {
                result = checked(Decimal::multiply(left->value, right->value, digitsFor(left->value, right->value)));
            } else {
                if (right->value.isZero())
                    return fail(Error::DivisionByZero, op.position);
                result = checked(left->value / right->value);
            }
            if (!result)
                return std::nullopt;
            left = Operand{*result};
        }
        return left;
    }

    std::optional<Operand> unary()
    {
        if (peek().kind == TokenKind::Plus) {
            next();
            return unary();
        }
        if (peek().kind == TokenKind::Minus) {
            next();
            std::optional<Operand> operand = unary();
            if (operand) {
                operand->value = -operand->value;
                operand->percentBase = -operand->percentBase;
            }
            return operand;
        }
        return postfix();
    }

    std::optional<Operand> postfix()
    {
        std::optional<Operand> operand = primary();
        if (!operand)
            return std::nullopt;
        while (peek().kind == TokenKind::Percent) {
            next();
            operand->percentBase = operand->value;
            operand->value = Decimal::divide(operand->value, Decimal(100));
            operand->percent = true;
        }
        return operand;
    }

    std::optional<Operand> primary()
    {
        const Token& token = next();
        if (token.kind == TokenKind::Number) {
            const std::optional<Decimal> value = checked(token.value);
            if (!value)
                return std::nullopt;
            return Operand{*value};
        }
        if (token.kind == TokenKind::Ans)
            return Operand{evaluator.m_ans};
        return fail(Error::Syntax, token.position);
    }
};

Evaluator::Result Evaluator::evaluate(std::string_view expression)
{
    Result result;
    size_t errorPosition = 0;
    std::optional<std::vector<Token>> tokens = tokenize(expression, &errorPosition);
    if (!tokens) {
        result.error = Error::Syntax;
        result.position = errorPosition;
        return result;
    }
    if (tokens->size() == 1) {
        result.error = Error::Empty;
        return result;
    }

    Parser parser{*this, std::move(*tokens)};
    const std::optional<Operand> value = parser.expression();
    if (value && parser.peek().kind != TokenKind::End)
        parser.fail(Error::Syntax, parser.peek().position);
    if (parser.failed())
        return parser.failure;

    result.value = value->value;
    m_ans = *result.value;
    return result;
}
