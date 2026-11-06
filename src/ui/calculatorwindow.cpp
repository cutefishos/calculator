// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculatorwindow.h"

#include "colors.h"
#include "expressionfield.h"
#include "historyview.h"
#include "keypad.h"

#include "base/i18n.h"
#include "ui/application.h"
#include "ui/text_field.h"
#include "ui/theme.h"
#include "ui/window_header.h"

#include <algorithm>
#include <cmath>

using namespace cutefish::ui;
using cutefish::i18n::tr;

// The keypad takes at most this share of the height under the header.
static constexpr double kKeypadShare = 0.72;
static constexpr double kExpressionHeight = 40;

class CalculatorWindow::Root : public View
{
public:
    Root()
        : header(addChild<WindowHeader>(HeaderHeight))
        , history(addChild<HistoryView>())
        , expression(addChild<ExpressionField>())
        , keypad(addChild<Keypad>())
    {
        header->setMaximizeEnabled(false);
    }

    bool acceptsPointer() const override { return false; }

    WindowHeader* header;
    HistoryView* history;
    ExpressionField* expression;
    Keypad* keypad;

protected:
    void layout() override
    {
        const double width = geometry().width;
        const double height = std::max(0.0, geometry().height - HeaderHeight);
        const double keys = std::round(std::min(Keypad::fittingHeight(width), height * kKeypadShare));
        const double display = height - keys;
        header->setGeometry({0, 0, width, HeaderHeight});
        history->setGeometry({0, HeaderHeight, width, std::max(0.0, display - kExpressionHeight)});
        expression->setGeometry({0, HeaderHeight + display - kExpressionHeight, width, kExpressionHeight});
        keypad->setGeometry({0, HeaderHeight + display, width, keys});

        if (Window* window = this->window())
            window->setBackgroundBlur(localRect(), Theme::current().windowRadius);
    }
};

CalculatorWindow::CalculatorWindow()
{
    m_window.resize(Size);
    m_window.setMinimumSize(Size);
    m_window.setMaximumSize(Size);
    m_window.setUserResizable(false);
    m_window.setBackgroundColor(CalculatorColor::WindowBackground);
    auto root = std::make_unique<Root>();
    m_root = root.get();
    m_window.setContentView(std::move(root));

    m_root->keypad->onPressed = [this](const std::string& key) { press(key); };
    TextField* field = expression();
    field->onAccepted = [this] { press("="); };
    field->setFocus();

    retranslate();
    m_languageChanged = Application::instance()->languageChanged.connect([this] { retranslate(); });
}

CalculatorWindow::~CalculatorWindow() = default;

TextField* CalculatorWindow::expression() const
{
    return m_root->expression->textField();
}

ExpressionField* CalculatorWindow::expressionField() const
{
    return m_root->expression;
}

HistoryView* CalculatorWindow::history() const
{
    return m_root->history;
}

Keypad* CalculatorWindow::keypad() const
{
    return m_root->keypad;
}

void CalculatorWindow::press(const std::string& key)
{
    TextField* field = expression();
    const std::string& text = field->text();
    const size_t cursor = field->cursorPosition();
    // The other end of the selection: the selected text lies on one side of the cursor.
    size_t anchor = cursor;
    if (field->hasSelection()) {
        const std::string selected = field->selectedText();
        anchor = text.compare(cursor, selected.size(), selected) == 0 ? cursor + selected.size() : cursor - selected.size();
    }

    if (key == "=") {
        if (const std::optional<std::string> result = m_calculator.calculate(text)) {
            field->setText(*result);
            m_root->history->setEntries(m_calculator.history());
        }
    } else if (key == "AC") {
        if (!text.empty()) {
            field->clear();
        } else {
            m_calculator.clearHistory();
            m_root->history->setEntries(m_calculator.history());
        }
    } else if (key == Keypad::Backspace) {
        edit(Calculator::backspace(text, anchor, cursor));
    } else {
        edit(Calculator::insert(text, anchor, cursor, key));
    }
    field->setFocus();
}

void CalculatorWindow::edit(const Calculator::Edit& edit)
{
    TextField* field = expression();
    field->setText(edit.text);
    field->setCursorPosition(edit.cursor);
}

void CalculatorWindow::retranslate()
{
    m_window.setTitle(tr("Calculator"));
}
