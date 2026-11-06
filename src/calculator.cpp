// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculator.h"

static bool isContinuationByte(char c)
{
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

std::optional<std::string> Calculator::calculate(const std::string& expression)
{
    // The shown result rounds its value; the full value is in ans. A digit
    // or a point after it would make it part of another number.
    std::string source = expression;
    if (!m_shownResult.empty() && expression.size() > m_shownResult.size() && expression.starts_with(m_shownResult)) {
        const char next = expression[m_shownResult.size()];
        if (!(next >= '0' && next <= '9') && next != '.')
            source = "ans" + expression.substr(m_shownResult.size());
    }

    m_lastResult = m_evaluator.evaluate(source);
    if (!m_lastResult.value)
        return std::nullopt;

    const std::string result = format(*m_lastResult.value);
    if (expression != result)
        m_history.push_back(expression + " = " + result);
    m_shownResult = result;
    return result;
}

void Calculator::clearHistory()
{
    m_history.clear();
}

std::string Calculator::format(const Decimal& value)
{
    return value.toFixed(DisplayDecimals);
}

std::string Calculator::backspace(const std::string& text)
{
    size_t end = text.size();
    if (end == 0)
        return text;
    --end;
    while (end > 0 && isContinuationByte(text[end]))
        --end;
    return text.substr(0, end);
}
