// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculatorwindow.h"
#include "colors.h"
#include "expressionfield.h"
#include "historyview.h"
#include "icons.h"
#include "keypad.h"

#include "ui/application.h"
#include "ui/icon.h"
#include "ui/keys.h"
#include "ui/label.h"
#include "ui/test_input.h"
#include "ui/text_field.h"
#include "ui/theme.h"

#include <gtest/gtest.h>

#include <cmath>

using namespace cutefish::ui;

static RectF windowRect(const View* view)
{
    const PointF topLeft = view->mapToWindow({});
    return {topLeft.x, topLeft.y, view->geometry().width, view->geometry().height};
}

static PointF center(const View* view)
{
    const RectF rect = windowRect(view);
    return {rect.x + rect.width / 2, rect.y + rect.height / 2};
}

namespace {

struct Fixture
{
    explicit Fixture(double scale = 1, SizeF size = CalculatorWindow::Size)
    {
        window.window().setScale(scale);
        window.window().resize(size);
        window.window().show();
        layout();
    }

    void layout() { window.window().grab(); }

    void click(const std::string& key)
    {
        KeypadButton* button = window.keypad()->button(key);
        ASSERT_NE(button, nullptr) << key;
        TestInput::click(window.window(), center(button));
        layout();
    }

    std::string text() const { return window.expression()->text(); }

    CalculatorWindow window;
};

} // namespace

TEST(UiTest, Layout)
{
    for (const double scale : {1.0, 1.5, 2.0}) {
        for (const SizeF size : {CalculatorWindow::Size}) {
            Fixture fixture(scale, size);
            const double contentHeight = size.height - CalculatorWindow::HeaderHeight;

            const RectF keypad = windowRect(fixture.window.keypad());
            EXPECT_EQ(keypad.bottom(), size.height);
            EXPECT_EQ(keypad.width, size.width);
            EXPECT_LE(keypad.height, std::round(contentHeight * 0.72));

            const RectF expression = windowRect(fixture.window.expressionField());
            EXPECT_EQ(expression.bottom(), keypad.y);
            EXPECT_EQ(expression.height, 40);
            EXPECT_EQ(expression.width, size.width);
            const RectF history = windowRect(fixture.window.history());
            EXPECT_EQ(history.y, CalculatorWindow::HeaderHeight);
            EXPECT_EQ(history.bottom(), expression.y);

            // Four columns and five rows of round keys, evenly spaced and centred.
            const auto& buttons = fixture.window.keypad()->buttons();
            ASSERT_EQ(buttons.size(), 20u);
            const RectF first = windowRect(buttons[0]);
            const RectF last = windowRect(buttons[19]);
            const double gap = windowRect(buttons[1]).x - first.right();
            EXPECT_GT(gap, 0);
            for (size_t i = 0; i < buttons.size(); ++i) {
                const RectF cell = windowRect(buttons[i]);
                EXPECT_DOUBLE_EQ(cell.width, first.width) << i;
                EXPECT_DOUBLE_EQ(cell.height, cell.width) << i;
                if (i % 4 != 0)
                    EXPECT_NEAR(cell.x - windowRect(buttons[i - 1]).right(), gap, 1e-9) << i;
                if (i >= 4)
                    EXPECT_NEAR(cell.y - windowRect(buttons[i - 4]).bottom(), gap, 1e-9) << i;
            }
            EXPECT_NEAR(first.x - keypad.x, keypad.right() - last.right(), 1e-9);
            EXPECT_NEAR(first.y - keypad.y, keypad.bottom() - last.bottom(), 1e-9);
            EXPECT_GE(first.x - keypad.x, gap);
            EXPECT_GE(first.y - keypad.y, gap);

            // The plates, as painted, differ by at most a device pixel.
            int smallest = INT32_MAX;
            int largest = 0;
            for (KeypadButton* button : buttons) {
                const Rect ink = TestInput::inkRect(*button, scale);
                smallest = std::min(smallest, ink.width);
                largest = std::max(largest, ink.width);
            }
            EXPECT_LE(largest - smallest, 1) << scale;
        }
    }
}

TEST(UiTest, LabelsCentredOnKeys)
{
    for (const double scale : {1.0, 1.5}) {
        Fixture fixture(scale);
        for (KeypadButton* button : fixture.window.keypad()->buttons()) {
            const RectF cell = windowRect(button);
            ASSERT_EQ(button->children().size(), 1u);
            View* content = button->children().front().get();
            const RectF rect = windowRect(content);
            EXPECT_NEAR(rect.x + rect.width / 2, cell.x + cell.width / 2, 0.01) << button->name();
            EXPECT_NEAR(rect.y + rect.height / 2, cell.y + cell.height / 2, 0.01) << button->name();

            // The glyphs' ink, not only their box, within a device pixel of the middle
            // across; digits down as well, the other signs sitting where the font puts them.
            const Rect ink = TestInput::inkRect(*content, scale);
            ASSERT_GT(ink.width, 0) << button->name();
            const std::string& name = button->name();
            // The font draws a 1 and AC left of the middle of their advance.
            if (name != "key 1" && name != "key AC")
                EXPECT_NEAR((ink.x + ink.width / 2.0) / scale, rect.width / 2, 1 / scale) << name;
            if (name.size() == 5 && name[4] >= '0' && name[4] <= '9')
                EXPECT_NEAR((ink.y + ink.height / 2.0) / scale, rect.height / 2, 1) << name;
        }
    }
}

TEST(UiTest, KeypadCalculates)
{
    Fixture fixture;
    for (const char* key : {"7", "×", "8"})
        fixture.click(key);
    EXPECT_EQ(fixture.text(), "7×8");
    fixture.click("=");
    EXPECT_EQ(fixture.text(), "56");
    EXPECT_EQ(fixture.window.history()->count(), 1u);
    EXPECT_EQ(fixture.window.calculator().history().back(), "7×8 = 56");

    fixture.click("-");
    fixture.click("6");
    fixture.click("%");
    fixture.click("=");
    EXPECT_EQ(fixture.text(), "52.64");

    fixture.click(Keypad::Backspace);
    EXPECT_EQ(fixture.text(), "52.6");
    fixture.click("÷");
    fixture.click(Keypad::Backspace);
    EXPECT_EQ(fixture.text(), "52.6");

    // C empties the expression, then the history.
    fixture.click("AC");
    EXPECT_EQ(fixture.text(), "");
    EXPECT_EQ(fixture.window.history()->count(), 2u);
    fixture.click("AC");
    EXPECT_EQ(fixture.window.history()->count(), 0u);

    // An expression with no result stays as it is.
    for (const char* key : {"1", "÷", "0", "="})
        fixture.click(key);
    EXPECT_EQ(fixture.text(), "1÷0");
    EXPECT_EQ(fixture.window.history()->count(), 0u);
}

TEST(UiTest, DoubleZeroKey)
{
    Fixture fixture;
    for (const char* key : {"3", "00", "-", "1", "="})
        fixture.click(key);
    EXPECT_EQ(fixture.window.calculator().history().back(), "300-1 = 299");
    EXPECT_EQ(fixture.text(), "299");
}

TEST(UiTest, KeysKeepTheFocusAndTheCursor)
{
    Fixture fixture;
    TextField* field = fixture.window.expression();
    EXPECT_TRUE(field->hasFocus());
    fixture.click("1");
    fixture.click("3");
    field->setCursorPosition(1);
    fixture.click("2");
    EXPECT_EQ(fixture.text(), "123");
    EXPECT_EQ(field->cursorPosition(), 2u);
    EXPECT_TRUE(field->hasFocus());
    field->select(0, 2);
    fixture.click("9");
    EXPECT_EQ(fixture.text(), "93");
    field->select(2, 1);
    fixture.click(Keypad::Backspace);
    EXPECT_EQ(fixture.text(), "9");
}

TEST(UiTest, Typing)
{
    Fixture fixture;
    Window& window = fixture.window.window();
    TestInput::setKeyboardFocus(window, true);
    TestInput::typeText(window, "2^10");
    TestInput::typeKey(window, key::Return);
    fixture.layout();
    EXPECT_EQ(fixture.text(), "1024");
    TestInput::typeText(window, "/4");
    TestInput::typeKey(window, key::KeypadEnter);
    fixture.layout();
    EXPECT_EQ(fixture.text(), "256");
    EXPECT_EQ(fixture.window.history()->count(), 2u);
}

TEST(UiTest, ExpressionHugsTheRightEdge)
{
    Fixture fixture(1.5);
    const RectF row = windowRect(fixture.window.expressionField());
    TextField* field = fixture.window.expression();
    RectF rect = windowRect(field);
    EXPECT_EQ(rect.right(), row.right());
    const double empty = rect.width;
    EXPECT_LT(empty, 30);

    for (const char* key : {"1", "2", "3"})
        fixture.click(key);
    rect = windowRect(field);
    EXPECT_EQ(rect.right(), row.right());
    EXPECT_GT(rect.width, empty + 20);
    EXPECT_LT(rect.width, row.width / 2);

    // A long expression fills the row and scrolls.
    for (int i = 0; i < 40; ++i)
        fixture.click("7");
    rect = windowRect(field);
    EXPECT_EQ(rect.x, row.x);
    EXPECT_EQ(rect.width, row.width);

    // A press in the empty part puts the cursor at the start.
    fixture.click("AC");
    fixture.click("5");
    TestInput::click(fixture.window.window(), {row.x + 20, row.y + row.height / 2});
    fixture.layout();
    EXPECT_EQ(field->cursorPosition(), 0u);
    EXPECT_TRUE(field->hasFocus());
}

TEST(UiTest, HistoryScrollsToTheLatest)
{
    Fixture fixture;
    for (int i = 1; i <= 12; ++i) {
        fixture.click(std::to_string(i % 10));
        fixture.click("+");
        fixture.click("1");
        fixture.click("=");
        fixture.click("AC");
    }
    HistoryView* history = fixture.window.history();
    EXPECT_EQ(history->count(), 12u);
    EXPECT_GT(history->contentSize().height, history->geometry().height);
    EXPECT_DOUBLE_EQ(history->scrollPosition().y, history->contentSize().height - history->geometry().height);
}

TEST(UiTest, HistorySitsOnTheExpression)
{
    Fixture fixture;
    for (const char* key : {"1", "+", "2", "="})
        fixture.click(key);
    HistoryView* history = fixture.window.history();
    ASSERT_EQ(history->count(), 1u);
    View* rows = history->contentView();
    ASSERT_EQ(rows->children().size(), 1u);
    const View* entry = rows->children().front().get();
    // The row's text, the large spacing under it, then the expression.
    const double spacing = Theme::current().largeSpacing;
    EXPECT_DOUBLE_EQ(windowRect(entry).bottom() + spacing, windowRect(history).bottom());
    EXPECT_GT(windowRect(entry).y, windowRect(history).y + 20);
    EXPECT_EQ(history->scrollPosition().y, 0);

    for (const char* key : {"3", "+", "4", "="})
        fixture.click(key);
    ASSERT_EQ(rows->children().size(), 2u);
    EXPECT_DOUBLE_EQ(windowRect(rows->children().back().get()).bottom() + spacing, windowRect(history).bottom());
    EXPECT_DOUBLE_EQ(windowRect(rows->children().back().get()).y - windowRect(rows->children().front().get()).bottom(),
                     2 * spacing);
    EXPECT_LT(windowRect(rows->children().front().get()).y, windowRect(rows->children().back().get()).y);
}

TEST(UiTest, LayoutIsStable)
{
    TestInput::setVerifiesLayout(true);
    {
        Fixture fixture(1.25);
        for (const char* key : {"1", "+", "2", "=", "AC"})
            fixture.click(key);
    }
    EXPECT_EQ(TestInput::layoutMismatches(), 0);
    TestInput::setVerifiesLayout(false);
}

TEST(UiTest, Themes)
{
    for (const bool dark : {false, true}) {
        Theme::setCurrent(dark ? Theme::dark() : Theme::light());
        const Color background = color(CalculatorColor::WindowBackground);
        EXPECT_EQ(background, (dark ? Color{0, 0, 0, 242} : Color{232, 232, 232, 242}));
        EXPECT_NE(color(CalculatorColor::KeyHovered), color(CalculatorColor::Key));
        EXPECT_NE(color(CalculatorColor::KeyPressed), color(CalculatorColor::KeyHovered));
        EXPECT_NE(color(CalculatorColor::OperatorKeyHovered), color(CalculatorColor::OperatorKey));
        EXPECT_NE(color(CalculatorColor::OperatorKeyPressed), color(CalculatorColor::OperatorKeyHovered));
        EXPECT_NE(color(CalculatorColor::AccentKeyPressed), color(CalculatorColor::AccentKeyHovered));
        Fixture fixture(1.5);
        fixture.click("1");
        EXPECT_EQ(fixture.text(), "1");
    }
    Theme::setCurrent(Theme::light());
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    Application application(argc, argv, Application::Platform::Headless);
    for (const auto& [name, svg] : embeddedIcons())
        Icon::registerGlyph(std::string(name), std::string(svg));
    return RUN_ALL_TESTS();
}
