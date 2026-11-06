// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculator.h"

static constexpr std::string_view kOperators[] = {"+", "−", "×", "÷"};

static bool isContinuationByte(char c)
{
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool isOperator(std::string_view key)
{
    for (const std::string_view op : kOperators) {
        if (key == op)
            return true;
    }
    return false;
}

static bool endsWithOperator(const std::string& text)
{
    for (const std::string_view op : kOperators) {
        if (text.ends_with(op))
            return true;
    }
    return false;
}

// The number being typed at the end: its digits and point.
static std::string_view lastNumber(const std::string& text)
{
    size_t start = text.size();
    while (start > 0 && (isDigit(text[start - 1]) || text[start - 1] == '.'))
        --start;
    return std::string_view(text).substr(start);
}

void Calculator::press(std::string_view key)
{
    if (key == Clear) {
        if (m_expression.empty())
            clearHistory();
        m_expression.clear();
        m_showingResult = false;
        return;
    }
    if (key == Backspace) {
        m_expression = backspace(m_expression);
        m_showingResult = false;
        return;
    }
    if (key == "=") {
        if (m_expression.empty() || endsWithOperator(m_expression))
            return;
        if (const std::optional<std::string> result = calculate(m_expression)) {
            m_expression = *result;
            m_showingResult = true;
        }
        return;
    }

    const bool numberKey = key == "." || key == "00" || (key.size() == 1 && isDigit(key[0]));
    if (numberKey && m_showingResult)
        m_expression.clear();
    m_showingResult = false;

    if (isOperator(key)) {
        if (m_expression.empty())
            m_expression = "0";
        else if (endsWithOperator(m_expression))
            m_expression = backspace(m_expression);
        else if (m_expression.back() == '.')
            m_expression.pop_back();
        m_expression += key;
        return;
    }
    if (key == "%") {
        if (m_expression.empty() || endsWithOperator(m_expression) || m_expression.back() == '%')
            return;
        if (m_expression.back() == '.')
            m_expression.pop_back();
        m_expression += key;
        return;
    }
    if (!numberKey || (!m_expression.empty() && m_expression.back() == '%'))
        return;

    const std::string_view number = lastNumber(m_expression);
    if (key == ".") {
        if (number.find('.') != std::string_view::npos)
            return;
        m_expression += number.empty() ? "0." : ".";
    } else if (key == "00") {
        if (number == "0")
            return;
        m_expression += number.empty() ? "0" : "00";
    } else if (number == "0") {
        m_expression.back() = key[0];
    } else {
        m_expression += key;
    }
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
