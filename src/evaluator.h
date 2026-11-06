// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "decimal.h"

#include <optional>
#include <string_view>

// Evaluates what the keypad types: numbers, + − × ÷ and %, as in
// "200 + 10%" or "1.5 × -2". × and ÷ come before + and −; a sign may
// stand before a number. "ans" is the last result.
//
// x% is x / 100, except as the right side of + and −, where a ± b% adds or
// takes away b percent of a.
class Evaluator
{
public:
    enum class Error {
        None,
        // Nothing but white space.
        Empty,
        Syntax,
        DivisionByZero,
        // A magnitude of 10^1000 or more.
        Overflow,
    };

    struct Result
    {
        std::optional<Decimal> value;
        Error error = Error::None;
        // Where in the expression, in bytes, for Syntax.
        size_t position = 0;
    };

    // Sets ans on success.
    Result evaluate(std::string_view expression);

    const Decimal& ans() const { return m_ans; }

private:
    struct Parser;

    Decimal m_ans;
};
