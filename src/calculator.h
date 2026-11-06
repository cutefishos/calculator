// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "evaluator.h"

#include <optional>
#include <string>
#include <vector>

// The calculator apart from its window: what = does, the history of
// calculations, and what backspace does to the expression.
class Calculator
{
public:
    // Results show at most this many decimals.
    static constexpr int DisplayDecimals = 10;

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
    Evaluator m_evaluator;
    Evaluator::Result m_lastResult;
    std::vector<std::string> m_history;
    // The last result as shown, and so as it starts an expression that goes on from it.
    std::string m_shownResult;
};
