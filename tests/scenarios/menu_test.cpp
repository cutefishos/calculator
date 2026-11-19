// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "scenario.h"

#include "ui/clipboard.h"
#include "ui/keys.h"
#include "ui/menu.h"
#include "ui/window.h"

#include <gtest/gtest.h>

using namespace cutefish::ui;

static std::string clipboardText()
{
    std::string text;
    Clipboard::requestText([&](std::string copied) { text = std::move(copied); });
    return text;
}

static void pasteText(TestDriver& driver, const std::string& text)
{
    Clipboard::setText(text);
    driver.chooseMenuItem("Edit", "Paste");
}

static KeyEvent shortcut(char letter, uint32_t keycode)
{
    KeyEvent event;
    event.key = key::letter(letter);
    event.keycode = keycode;
    event.modifiers = ControlModifier;
    return event;
}

TEST(MenuTest, MainMenu)
{
    Scenario scenario;
    const Menu* menu = scenario.driver.window().mainMenu();
    ASSERT_NE(menu, nullptr);
    EXPECT_EQ(menu->submenu(0)->role(), MenuRole::Application);
    ASSERT_NE(menu->submenu(1), nullptr);
    EXPECT_EQ(menu->submenu(2)->role(), MenuRole::Windows);
}

TEST(MenuTest, CopyPasteAndClearHistory)
{
    Scenario scenario;
    TestDriver& driver = scenario.driver;
    for (const char* key : {"key:1", "key:2", "key:×", "key:3"})
        driver.click(key);
    driver.chooseMenuItem("Edit", "Copy");
    EXPECT_EQ(clipboardText(), "12×3");

    // Pasted text goes in as typed, skipping what the keypad does not have.
    driver.click("key:CLEAR");
    pasteText(driver, "1,234.5 + 10% =");
    EXPECT_EQ(driver.text("expression"), "1234.5+10%");
    pasteText(driver, "*2-1/4");
    EXPECT_EQ(driver.text("expression"), "1234.5+10%×2−1÷4");
    driver.click("key:CLEAR");
    pasteText(driver, "..5x+−3");
    EXPECT_EQ(driver.text("expression"), "0.5−3");

    // The same from the keyboard.
    driver.setKeyboardFocus(true);
    driver.pressKey(shortcut('c', 46));
    driver.releaseKey(shortcut('c', 46));
    EXPECT_EQ(clipboardText(), "0.5−3");
    driver.click("key:CLEAR");
    driver.pressKey(shortcut('v', 47));
    driver.releaseKey(shortcut('v', 47));
    EXPECT_EQ(driver.text("expression"), "0.5−3");

    driver.click("key:=");
    EXPECT_EQ(driver.findAll("history:entry").size(), 1u);
    driver.chooseMenuItem("Edit", "Clear History");
    EXPECT_EQ(driver.findAll("history:entry").size(), 0u);
    EXPECT_EQ(driver.text("expression"), "-2.5");
}
