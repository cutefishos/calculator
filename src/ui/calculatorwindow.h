// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "calculator.h"

#include "base/signal.h"
#include "ui/window.h"

#include <memory>
#include <string>

class ExpressionField;
class HistoryView;
class Keypad;

namespace cutefish::ui {
class TextField;
}

// The calculator's window: the history and the expression above the keypad.
class CalculatorWindow
{
public:
    static constexpr double HeaderHeight = 40;
    static constexpr double Width = 280;
    static constexpr double ExpressionHeight = 40;

    // Fixed: the history's visible rows, the expression and the keypad at the width.
    static cutefish::ui::SizeF size();

    CalculatorWindow();
    ~CalculatorWindow();

    cutefish::ui::Window& window() { return m_window; }
    Calculator& calculator() { return m_calculator; }

    // What a keypad key does to the expression; Keypad::Backspace for the glyph key.
    void press(const std::string& key);

    cutefish::ui::TextField* expression() const;
    ExpressionField* expressionField() const;
    HistoryView* history() const;
    Keypad* keypad() const;

private:
    class Root;

    void edit(const Calculator::Edit& edit);
    void retranslate();

    Calculator m_calculator;
    cutefish::ui::Window m_window;
    Root* m_root;
    cutefish::base::Subscription m_languageChanged;
};
