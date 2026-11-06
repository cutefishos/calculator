// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculatorwindow.h"
#include "colors.h"
#include "expressionfield.h"
#include "historyview.h"
#include "icons.h"
#include "keypad.h"

#include "ui/application.h"
#include "ui/clipboard.h"
#include "ui/icon.h"
#include "ui/keys.h"
#include "ui/label.h"
#include "ui/menu.h"
#include "ui/test_input.h"
#include "ui/theme.h"

#include <gtest/gtest.h>

#include <cmath>
#include <tuple>

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
    explicit Fixture(double scale = 1, SizeF size = CalculatorWindow::size())
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

    std::string text() const { return window.expression(); }

    CalculatorWindow window;
};

} // namespace

TEST(UiTest, Layout)
{
    for (const double scale : {1.0, 1.5, 2.0}) {
        for (const SizeF size : {CalculatorWindow::size()}) {
            Fixture fixture(scale, size);
            const double contentHeight = size.height - CalculatorWindow::HeaderHeight;

            const RectF keypad = windowRect(fixture.window.keypad());
            EXPECT_EQ(keypad.bottom(), size.height);
            EXPECT_EQ(keypad.width, size.width);
            EXPECT_DOUBLE_EQ(keypad.height, Keypad::fittingHeight(size.width));

            const RectF expression = windowRect(fixture.window.expressionField());
            EXPECT_EQ(expression.bottom(), keypad.y);
            EXPECT_EQ(expression.height, CalculatorWindow::ExpressionHeight);
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
            if (name != "key 1" && name != "key CLEAR")
                EXPECT_NEAR((ink.x + ink.width / 2.0) / scale, rect.width / 2, 1 / scale) << name;
            if (name.size() == 5 && name[4] >= '0' && name[4] <= '9')
                EXPECT_NEAR((ink.y + ink.height / 2.0) / scale, rect.height / 2, 1) << name;
        }

        // C, once there is an expression to clear: centred both ways, as a capital.
        fixture.click("1");
        View* clear = fixture.window.keypad()->button(Keypad::Clear)->children().front().get();
        const Rect ink = TestInput::inkRect(*clear, scale);
        const RectF rect = clear->geometry();
        EXPECT_NEAR((ink.x + ink.width / 2.0) / scale, rect.width / 2, 1 / scale) << scale;
        EXPECT_NEAR((ink.y + ink.height / 2.0) / scale, rect.height / 2, 1) << scale;
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

    fixture.click("−");
    fixture.click("6");
    fixture.click("%");
    fixture.click("=");
    EXPECT_EQ(fixture.text(), "52.64");

    fixture.click(Keypad::Backspace);
    EXPECT_EQ(fixture.text(), "52.6");
    fixture.click("÷");
    fixture.click(Keypad::Backspace);
    EXPECT_EQ(fixture.text(), "52.6");

    // C empties the expression and turns into AC, which empties the history.
    const auto clearLabel = [&] {
        return static_cast<Label*>(fixture.window.keypad()->button(Keypad::Clear)->children().front().get())->text();
    };
    EXPECT_EQ(clearLabel(), "C");
    fixture.click(Keypad::Clear);
    EXPECT_EQ(fixture.text(), "");
    EXPECT_EQ(fixture.window.history()->count(), 2u);
    EXPECT_EQ(clearLabel(), "AC");
    fixture.click(Keypad::Clear);
    EXPECT_EQ(fixture.window.history()->count(), 0u);
    EXPECT_EQ(clearLabel(), "AC");
    fixture.click("4");
    EXPECT_EQ(clearLabel(), "C");
    fixture.click(Keypad::Backspace);
    EXPECT_EQ(clearLabel(), "AC");

    // An expression with no result stays as it is.
    for (const char* key : {"1", "÷", "0", "="})
        fixture.click(key);
    EXPECT_EQ(fixture.text(), "1÷0");
    EXPECT_EQ(fixture.window.history()->count(), 0u);
}

TEST(UiTest, DoubleZeroKey)
{
    Fixture fixture;
    for (const char* key : {"3", "00", "−", "1", "="})
        fixture.click(key);
    EXPECT_EQ(fixture.window.calculator().history().back(), "300−1 = 299");
    EXPECT_EQ(fixture.text(), "299");
}

static KeyEvent keyEvent(uint32_t keysym, uint32_t keycode, const std::string& text = {})
{
    KeyEvent event;
    event.key = keysym;
    event.keycode = keycode;
    event.text = text;
    return event;
}

TEST(UiTest, KeyboardHoldsTheKeypadKey)
{
    Fixture fixture;
    Window& window = fixture.window.window();
    TestInput::setKeyboardFocus(window, true);
    Keypad* keypad = fixture.window.keypad();

    // Down on the press, up on the release; the key counts once, on the press.
    const KeyEvent seven = keyEvent('7', 16, "7");
    TestInput::pressKey(window, seven);
    fixture.layout();
    EXPECT_TRUE(keypad->button("7")->isPressed());
    EXPECT_EQ(fixture.text(), "7");
    KeyEvent sevenAgain = seven;
    sevenAgain.repeat = true;
    TestInput::pressKey(window, sevenAgain);
    EXPECT_EQ(fixture.text(), "7");
    TestInput::releaseKey(window, keyEvent('7', 16));
    fixture.layout();
    EXPECT_FALSE(keypad->button("7")->isPressed());
    EXPECT_EQ(fixture.text(), "7");

    // * and / stand for the signs; Return for =.
    for (const auto& [keysym, keycode, text] : {std::tuple{uint32_t('*'), 55u, "*"}, std::tuple{uint32_t('6'), 15u, "6"}}) {
        TestInput::pressKey(window, keyEvent(keysym, keycode, text));
        TestInput::releaseKey(window, keyEvent(keysym, keycode));
    }
    EXPECT_EQ(fixture.text(), "7×6");
    TestInput::pressKey(window, keyEvent(key::Return, 28, "\r"));
    fixture.layout();
    EXPECT_TRUE(keypad->button("=")->isPressed());
    EXPECT_EQ(fixture.text(), "42");
    TestInput::releaseKey(window, keyEvent(key::Return, 28));
    EXPECT_FALSE(keypad->button("=")->isPressed());

    // A held backspace repeats and stays down.
    TestInput::pressKey(window, keyEvent(key::BackSpace, 14));
    KeyEvent repeat = keyEvent(key::BackSpace, 14);
    repeat.repeat = true;
    TestInput::pressKey(window, repeat);
    EXPECT_TRUE(keypad->button(Keypad::Backspace)->isPressed());
    EXPECT_EQ(fixture.text(), "");
    TestInput::releaseKey(window, keyEvent(key::BackSpace, 14));
    EXPECT_FALSE(keypad->button(Keypad::Backspace)->isPressed());

    // Escape clears; shortcuts and other text change nothing.
    TestInput::pressKey(window, keyEvent('5', 6, "5"));
    TestInput::releaseKey(window, keyEvent('5', 6));
    KeyEvent shortcut = keyEvent('1', 2, "1");
    shortcut.modifiers = ControlModifier;
    TestInput::pressKey(window, shortcut);
    TestInput::releaseKey(window, shortcut);
    TestInput::typeText(window, "a(");
    EXPECT_EQ(fixture.text(), "5");
    EXPECT_FALSE(keypad->button("1")->isPressed());
    TestInput::pressKey(window, keyEvent(key::Escape, 1));
    TestInput::releaseKey(window, keyEvent(key::Escape, 1));
    EXPECT_EQ(fixture.text(), "");

    // A key still down when the window is deactivated comes up.
    const int width = int(window.size().width);
    const int height = int(window.size().height);
    TestInput::configure(window, width, height, {.activated = true});
    TestInput::pressKey(window, keyEvent('8', 9, "8"));
    EXPECT_TRUE(keypad->button("8")->isPressed());
    TestInput::configure(window, width, height, {.activated = false});
    fixture.layout();
    EXPECT_FALSE(keypad->button("8")->isPressed());
}

TEST(UiTest, KeypadClicksKeepTheKeyboard)
{
    Fixture fixture;
    Window& window = fixture.window.window();
    TestInput::setKeyboardFocus(window, true);
    fixture.click("1");
    TestInput::typeText(window, "2");
    EXPECT_EQ(fixture.text(), "12");
}

TEST(UiTest, Typing)
{
    Fixture fixture;
    Window& window = fixture.window.window();
    TestInput::setKeyboardFocus(window, true);
    TestInput::typeText(window, "12*3+4");
    TestInput::typeKey(window, key::Return);
    fixture.layout();
    EXPECT_EQ(fixture.text(), "40");
    TestInput::typeText(window, "/4");
    TestInput::typeKey(window, key::KeypadEnter);
    fixture.layout();
    EXPECT_EQ(fixture.text(), "10");
    EXPECT_EQ(fixture.window.history()->count(), 2u);
}

TEST(UiTest, ExpressionStaysOnTheRightEdge)
{
    for (const double scale : {1.0, 1.5, 1.75}) {
        Fixture fixture(scale);
        const RectF row = windowRect(fixture.window.expressionField());
        View* label = fixture.window.expressionField()->label();
        const double end = row.right() - Theme::current().largeSpacing;

        // The text grows to the left; its end stays where it is.
        for (const char* key : {"1", "2", "3", "+", "4", "5", "×", "6"}) {
            fixture.click(key);
            EXPECT_DOUBLE_EQ(windowRect(label).right(), end) << scale << " " << fixture.text();
            const Rect ink = TestInput::inkRect(*label, scale);
            EXPECT_NEAR((ink.x + ink.width) / scale, label->geometry().width, 2) << scale << " " << fixture.text();
        }

        // A long expression slides out at the left margin, its end in sight.
        for (int i = 0; i < 40; ++i)
            fixture.click("7");
        EXPECT_DOUBLE_EQ(windowRect(label).right(), end);
        EXPECT_LT(windowRect(label).x, row.x);
        const Image frame = fixture.window.window().grab();
        const double margin = Theme::current().largeSpacing;
        const auto inked = [&](double x) {
            for (int y = int(row.y * scale); y < int(row.bottom() * scale); ++y) {
                if (frame.pixel(int(x * scale), y) != frame.pixel(1, y))
                    return true;
            }
            return false;
        };
        for (double x = row.x + 1; x < row.x + margin - 1; x += 1 / scale)
            EXPECT_FALSE(inked(x)) << scale << " " << x;
        EXPECT_TRUE(inked(row.x + margin + 2)) << scale;
    }
}

TEST(UiTest, SignsKeepTheTextOnItsLine)
{
    // × and ÷ come from a fallback font with taller metrics; the digits stay put.
    for (const double scale : {1.0, 1.5, 1.75}) {
        Fixture fixture(scale);
        View* label = fixture.window.expressionField()->label();
        const auto inkTop = [&] {
            const Rect ink = TestInput::inkRect(*label, scale);
            return windowRect(label).y * scale + ink.y;
        };
        const auto inkBottom = [&] {
            const Rect ink = TestInput::inkRect(*label, scale);
            return windowRect(label).y * scale + ink.y + ink.height;
        };
        fixture.click("1");
        fixture.click("2");
        const double top = inkTop();
        const double bottom = inkBottom();
        for (const char* key : {"×", "÷", "+", "−"}) {
            fixture.click(key);
            EXPECT_NEAR(inkTop(), top, 0.01) << scale << " " << key;
            EXPECT_NEAR(inkBottom(), bottom, 0.01) << scale << " " << key;
            fixture.click(Keypad::Backspace);
        }

        // History rows with and without the signs: one height, the text at one place in them.
        for (const char* key : {"+", "3", "=", "×", "2", "=", "÷", "4", "="})
            fixture.click(key);
        View* rows = fixture.window.history()->contentView();
        ASSERT_EQ(rows->children().size(), 3u);
        std::vector<double> offsets;
        for (const auto& child : rows->children()) {
            const Rect ink = TestInput::inkRect(*child, scale);
            offsets.push_back(child->geometry().y * scale + ink.y + ink.height);
        }
        const double pitch = HistoryView::rowHeight() * scale;
        EXPECT_NEAR(offsets[1] - offsets[0], pitch, 0.01) << scale;
        EXPECT_NEAR(offsets[2] - offsets[1], pitch, 0.01) << scale;
    }
}

TEST(UiTest, HistoryScrollsToTheLatest)
{
    Fixture fixture;
    for (int i = 1; i <= 12; ++i) {
        fixture.click(std::to_string(i % 10));
        fixture.click("+");
        fixture.click("1");
        fixture.click("=");
        fixture.click(Keypad::Clear);
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

TEST(UiTest, HistoryShowsTwoWholeRows)
{
    Fixture fixture(1.5);
    for (int i = 1; i <= 5; ++i) {
        fixture.click(std::to_string(i));
        fixture.click("+");
        fixture.click("1");
        fixture.click("=");
        fixture.click(Keypad::Clear);
    }
    HistoryView* history = fixture.window.history();
    const RectF area = windowRect(history);
    View* rows = history->contentView();
    ASSERT_EQ(rows->children().size(), 5u);
    // The last rows' text lies inside the area, the ones before them above it.
    const double spacing = Theme::current().largeSpacing;
    int inside = 0;
    for (const auto& child : rows->children()) {
        const RectF entry = windowRect(child.get());
        if (entry.y >= area.y && entry.bottom() <= area.bottom())
            ++inside;
        else
            EXPECT_LE(entry.bottom(), area.y + spacing);
    }
    EXPECT_EQ(inside, HistoryView::VisibleRows);
    EXPECT_GE(windowRect(rows->children()[3].get()).y - area.y, spacing - 1);
}

TEST(UiTest, EqualsKeyGreysWhenInactive)
{
    Fixture fixture(1.5);
    Window& window = fixture.window.window();
    const int width = int(window.size().width);
    const int height = int(window.size().height);
    // Inside a plate, clear of its rim and its label; and an empty spot above the history.
    const auto plate = [&](const std::string& key) {
        const RectF rect = windowRect(fixture.window.keypad()->button(key));
        return window.grab().pixel(int((rect.x + rect.width * 0.3) * 1.5), int((rect.y + rect.height * 0.7) * 1.5));
    };
    const auto background = [&] { return window.grab().pixel(30, 90); };

    TestInput::configure(window, width, height, {.activated = true});
    fixture.layout();
    EXPECT_EQ(plate("7"), color(CalculatorColor::Key));
    EXPECT_EQ(plate("×"), color(CalculatorColor::OperatorKey));
    EXPECT_EQ(plate("="), color(CalculatorColor::AccentKey));
    const Color active = background();

    TestInput::configure(window, width, height, {.activated = false});
    fixture.layout();
    EXPECT_EQ(plate("7"), color(CalculatorColor::Key));
    EXPECT_EQ(plate("×"), color(CalculatorColor::OperatorKey));
    EXPECT_EQ(plate("="), color(CalculatorColor::InactiveAccentKey));
    EXPECT_EQ(background(), active);

    TestInput::configure(window, width, height, {.activated = true});
    fixture.layout();
    EXPECT_EQ(plate("7"), color(CalculatorColor::Key));
    EXPECT_EQ(plate("="), color(CalculatorColor::AccentKey));
}

TEST(UiTest, MainMenu)
{
    Fixture fixture;
    const Menu* menu = fixture.window.window().mainMenu();
    ASSERT_NE(menu, nullptr);
    EXPECT_EQ(menu->submenu(0)->role(), MenuRole::Application);
    ASSERT_NE(menu->submenu(1), nullptr);
    EXPECT_EQ(menu->submenu(2)->role(), MenuRole::Windows);
}

TEST(UiTest, CopyPasteAndClearHistory)
{
    Fixture fixture;
    for (const char* key : {"1", "2", "×", "3"})
        fixture.click(key);
    fixture.window.copy();
    std::string copied;
    Clipboard::requestText([&](std::string text) { copied = std::move(text); });
    EXPECT_EQ(copied, "12×3");

    // Pasted text goes in as typed, skipping what the keypad does not have.
    fixture.click(Keypad::Clear);
    fixture.window.paste("1,234.5 + 10% =");
    EXPECT_EQ(fixture.text(), "1234.5+10%");
    fixture.window.paste("*2-1/4");
    EXPECT_EQ(fixture.text(), "1234.5+10%×2−1÷4");
    fixture.click(Keypad::Clear);
    fixture.window.paste("..5x+−3");
    EXPECT_EQ(fixture.text(), "0.5−3");

    // The same from the keyboard.
    Window& window = fixture.window.window();
    TestInput::setKeyboardFocus(window, true);
    KeyEvent copyKey = keyEvent(key::letter('c'), 46);
    copyKey.modifiers = ControlModifier;
    TestInput::pressKey(window, copyKey);
    TestInput::releaseKey(window, copyKey);
    Clipboard::requestText([&](std::string text) { copied = std::move(text); });
    EXPECT_EQ(copied, "0.5−3");
    fixture.click(Keypad::Clear);
    KeyEvent pasteKey = keyEvent(key::letter('v'), 47);
    pasteKey.modifiers = ControlModifier;
    TestInput::pressKey(window, pasteKey);
    TestInput::releaseKey(window, pasteKey);
    EXPECT_EQ(fixture.text(), "0.5−3");

    fixture.click("=");
    EXPECT_EQ(fixture.window.history()->count(), 1u);
    fixture.window.clearHistory();
    EXPECT_EQ(fixture.window.history()->count(), 0u);
    EXPECT_EQ(fixture.text(), "-2.5");
}

TEST(UiTest, LayoutIsStable)
{
    TestInput::setVerifiesLayout(true);
    {
        Fixture fixture(1.25);
        for (const char* key : {"1", "+", "2", "=", Keypad::Clear})
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
