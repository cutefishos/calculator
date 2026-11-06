// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculatorwindow.h"

#include "colors.h"
#include "expressionfield.h"
#include "historyview.h"
#include "keypad.h"

#include "base/i18n.h"
#include "ui/about_dialog.h"
#include "ui/application.h"
#include "ui/clipboard.h"
#include "ui/keys.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/window_header.h"

#include <algorithm>
#include <cmath>

using namespace cutefish::ui;
using cutefish::i18n::tr;

// The keypad key a keyboard key stands for; empty for none.
static std::string keypadKey(const KeyEvent& event)
{
    if (event.modifiers & (ControlModifier | AltModifier | SuperModifier))
        return {};
    switch (event.key) {
    case key::Return:
    case key::KeypadEnter:
        return "=";
    case key::BackSpace:
        return Keypad::Backspace;
    case key::Escape:
    case key::Delete:
        return Keypad::Clear;
    default:
        break;
    }
    const std::string& text = event.text;
    if (text.size() == 1 && text[0] >= '0' && text[0] <= '9')
        return text;
    if (text == "." || text == ",")
        return ".";
    if (text == "*" || text == "x" || text == "X")
        return "×";
    if (text == "/")
        return "÷";
    if (text == "-")
        return "−";
    if (text == "+" || text == "%" || text == "=")
        return text;
    return {};
}

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
    bool acceptsFocus() const override { return true; }

    std::function<bool(const KeyEvent&)> onKeyPress;
    std::function<bool(const KeyEvent&)> onKeyRelease;

    WindowHeader* header;
    HistoryView* history;
    ExpressionField* expression;
    Keypad* keypad;

protected:
    bool keyPress(const KeyEvent& event) override { return onKeyPress && onKeyPress(event); }
    bool keyRelease(const KeyEvent& event) override { return onKeyRelease && onKeyRelease(event); }

    void layout() override
    {
        const double width = geometry().width;
        const double height = std::max(0.0, geometry().height - HeaderHeight);
        const double keys = std::min(height, Keypad::fittingHeight(width));
        const double display = height - keys;
        header->setGeometry({0, 0, width, HeaderHeight});
        history->setGeometry({0, HeaderHeight, width, std::max(0.0, display - ExpressionHeight)});
        expression->setGeometry({0, HeaderHeight + display - ExpressionHeight, width, ExpressionHeight});
        keypad->setGeometry({0, HeaderHeight + display, width, keys});

        if (Window* window = this->window())
            window->setBackgroundBlur(localRect(), Theme::current().windowRadius);
    }
};

SizeF CalculatorWindow::size()
{
    const double history = HistoryView::VisibleRows * HistoryView::rowHeight();
    return {Width, std::ceil(HeaderHeight + history + ExpressionHeight + Keypad::fittingHeight(Width))};
}

CalculatorWindow::CalculatorWindow()
{
    const SizeF size = CalculatorWindow::size();
    m_window.resize(size);
    m_window.setMinimumSize(size);
    m_window.setMaximumSize(size);
    m_window.setUserResizable(false);
    m_window.setBackgroundColor(CalculatorColor::WindowBackground);
    auto root = std::make_unique<Root>();
    m_root = root.get();
    m_window.setContentView(std::move(root));

    m_root->keypad->onPressed = [this](const std::string& key) { press(key); };
    m_root->onKeyPress = [this](const KeyEvent& event) { return keyPress(event); };
    m_root->onKeyRelease = [this](const KeyEvent& event) { return keyRelease(event); };
    // A key let go in another window never comes back as a release here.
    m_window.onActiveChanged = [this] {
        if (!m_window.isActive())
            releaseKeys();
    };
    m_root->setFocus();

    // The main menu's key equivalents.
    m_window.addShortcut(ControlModifier, key::letter('q'), [] { Application::instance()->quit(); });
    m_window.addShortcut(ControlModifier, key::letter('c'), [this] { copy(); });
    m_window.addShortcut(ControlModifier, key::letter('v'), [this] { pasteClipboard(); });

    retranslate();
    m_languageChanged = Application::instance()->languageChanged.connect([this] { retranslate(); });
}

CalculatorWindow::~CalculatorWindow() = default;

const std::string& CalculatorWindow::expression() const
{
    return m_root->expression->text();
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
    m_calculator.press(key);
    update();
    // A click on the keypad keeps the keyboard here.
    m_root->setFocus();
}

void CalculatorWindow::copy()
{
    if (!m_calculator.expression().empty())
        Clipboard::setText(m_calculator.expression());
}

void CalculatorWindow::paste(const std::string& text)
{
    for (size_t i = 0; i < text.size();) {
        size_t length = 1;
        const auto lead = static_cast<unsigned char>(text[i]);
        if (lead >= 0xf0)
            length = 4;
        else if (lead >= 0xe0)
            length = 3;
        else if (lead >= 0xc0)
            length = 2;
        const std::string character = text.substr(i, length);
        i += length;
        std::string key;
        if (character.size() == 1 && character[0] >= '0' && character[0] <= '9')
            key = character;
        else if (character == ".")
            key = ".";
        else if (character == "+")
            key = "+";
        else if (character == "-" || character == "−")
            key = "−";
        else if (character == "*" || character == "x" || character == "X" || character == "×")
            key = "×";
        else if (character == "/" || character == "÷")
            key = "÷";
        else if (character == "%")
            key = "%";
        if (!key.empty())
            m_calculator.press(key);
    }
    update();
}

void CalculatorWindow::pasteClipboard()
{
    Clipboard::requestText([this](std::string text) { paste(text); });
}

void CalculatorWindow::clearHistory()
{
    m_calculator.clearHistory();
    update();
}

void CalculatorWindow::update()
{
    const std::string& expression = m_calculator.expression();
    m_root->expression->setText(expression);
    if (m_root->history->count() != m_calculator.history().size())
        m_root->history->setEntries(m_calculator.history());
    // C clears the expression; AC, on an empty one, the history.
    m_root->keypad->button(Keypad::Clear)->setText(expression.empty() ? "AC" : "C");
}

// The physical key when known, so the release finds the press under any layout or modifier.
static uint64_t heldKeyId(const KeyEvent& event)
{
    return event.keycode ? event.keycode : (uint64_t(1) << 32) | event.key;
}

bool CalculatorWindow::keyPress(const KeyEvent& event)
{
    const uint64_t id = heldKeyId(event);
    if (event.repeat) {
        // Only a held backspace repeats; the other keys count once.
        const auto held = m_heldKeys.find(id);
        if (held == m_heldKeys.end())
            return false;
        if (held->second == Keypad::Backspace)
            press(held->second);
        return true;
    }
    const std::string key = keypadKey(event);
    KeypadButton* button = key.empty() ? nullptr : m_root->keypad->button(key);
    if (!button)
        return false;
    m_heldKeys[id] = key;
    button->setKeyDown(true);
    press(key);
    return true;
}

bool CalculatorWindow::keyRelease(const KeyEvent& event)
{
    const auto held = m_heldKeys.find(heldKeyId(event));
    if (held == m_heldKeys.end())
        return false;
    const std::string key = held->second;
    m_heldKeys.erase(held);
    // Another keyboard key may still hold the same button, as Return and the keypad's Enter do.
    for (const auto& [id, other] : m_heldKeys) {
        if (other == key)
            return true;
    }
    m_root->keypad->button(key)->setKeyDown(false);
    return true;
}

void CalculatorWindow::releaseKeys()
{
    for (const auto& [id, key] : m_heldKeys)
        m_root->keypad->button(key)->setKeyDown(false);
    m_heldKeys.clear();
}

void CalculatorWindow::buildMainMenu()
{
    m_window.setMainMenu(nullptr);
    m_mainMenu = std::make_unique<Menu>();

    Menu* application = m_mainMenu->addSubmenu(tr("Calculator"));
    application->setRole(MenuRole::Application);
    application->addItem(tr("About Calculator"), [this] { openAbout(); });
    application->addSeparator();
    application->addItem(tr("Hide Calculator"), [this] { m_window.hideApplication(); });
    application->addSeparator();
    const size_t quit = application->addItem(tr("Quit Calculator"), [] { Application::instance()->quit(); });
    application->setItemKeyEquivalent(quit, "q", ControlModifier);

    Menu* edit = m_mainMenu->addSubmenu(tr("Edit"));
    const size_t copyRow = edit->addItem(tr("Copy"), [this] { copy(); });
    edit->setItemKeyEquivalent(copyRow, "c", ControlModifier);
    const size_t pasteRow = edit->addItem(tr("Paste"), [this] { pasteClipboard(); });
    edit->setItemKeyEquivalent(pasteRow, "v", ControlModifier);
    edit->addSeparator();
    const size_t clearRow = edit->addItem(tr("Clear History"), [this] { clearHistory(); });
    edit->onAboutToShow = [this, edit, copyRow, pasteRow, clearRow] {
        edit->setItemEnabled(copyRow, !m_calculator.expression().empty());
        edit->setItemEnabled(pasteRow, Clipboard::hasText());
        edit->setItemEnabled(clearRow, !m_calculator.history().empty());
    };

    Menu* windowMenu = m_mainMenu->addSubmenu(tr("Window"));
    windowMenu->setRole(MenuRole::Windows);
    windowMenu->addItem(tr("Minimize"), [this] { m_window.minimize(); });

    m_window.setMainMenu(m_mainMenu.get());
}

void CalculatorWindow::openAbout()
{
    if (!m_about) {
        m_about = std::make_unique<AboutDialog>(&m_window);
        m_about->setName(tr("Calculator"));
        m_about->setDescription(tr("A simple calculator"));
        m_about->setIconSource("accessories-calculator");
    }
    m_about->show();
}

// The menu and an open About dialog are made again in the new language.
void CalculatorWindow::retranslate()
{
    m_window.setTitle(tr("Calculator"));
    buildMainMenu();
    const bool about = m_about && m_about->isShown();
    m_about.reset();
    if (about)
        openAbout();
}
