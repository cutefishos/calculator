// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "evaluator.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The calculator apart from its window: the expression the keys build, what
// = does, and the history of calculations.
class Calculator
{
public:
    // Results show at most this many decimals.
    static constexpr int DisplayDecimals = 10;

    // The keys: the digits, "00", ".", the operators + − × ÷, "%", "=" and these.
    static constexpr std::string_view Clear = "CLEAR";
    static constexpr std::string_view Backspace = "BACKSPACE";

    // What a key does to the expression, by what it ends with:
    // - an operator on an empty expression follows a 0, replaces an operator
    //   before it, drops a point before it, and goes on from a result;
    // - a digit replaces a lone 0, and starts a new expression after a result;
    // - 00 is a single 0 where a number starts, and nothing after a lone 0;
    // - a point is 0. where a number starts, and nothing in a number with one;
    // - % follows a number only, once;
    // - = needs an expression that does not end with an operator;
    // - Clear empties the expression, or the history when it is empty.
    // Keys that do nothing there are ignored.
    void press(std::string_view key);
    const std::string& expression() const { return m_expression; }

    // The result as shown, or nothing when the expression has none. Adds
    // "expression = result" to the history unless the two are the same. An
    // expression that starts with the last result shown goes on from its
    // full value, so 1 ÷ 3 = then × 3 gives 1.
    std::optional<std::string> calculate(const std::string& expression);
    const Evaluator::Result& lastResult() const { return m_lastResult; }

    const std::vector<std::string>& history() const { return m_history; }
    void clearHistory();

    static std::string format(const Decimal& value);

    // The text without its last character, however many bytes it takes.
    static std::string backspace(const std::string& text);

private:
    std::string m_expression;
    // The expression is the result of the last =.
    bool m_showingResult = false;
    Evaluator m_evaluator;
    Evaluator::Result m_lastResult;
    std::vector<std::string> m_history;
    // The last result as shown, and so as it starts an expression that goes on from it.
    std::string m_shownResult;
};
