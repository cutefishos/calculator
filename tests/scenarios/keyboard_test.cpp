// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "keypad.h"
#include "scenario.h"

#include "ui/keys.h"

#include <gtest/gtest.h>

#include <tuple>

using namespace cutefish::ui;

static KeyEvent keyEvent(uint32_t keysym, uint32_t keycode, const std::string& text = {})
{
    KeyEvent event;
    event.key = keysym;
    event.keycode = keycode;
    event.text = text;
    return event;
}

static bool isPressed(TestDriver& driver, const std::string& name)
{
    const KeypadButton* button = driver.find<KeypadButton>(name);
    return button && button->isPressed();
}

TEST(KeyboardTest, KeyboardHoldsTheKeypadKey)
{
    Scenario scenario;
    TestDriver& driver = scenario.driver;
    driver.setKeyboardFocus(true);

    // Down on the press, up on the release; the key counts once, on the press.
    const KeyEvent seven = keyEvent('7', 16, "7");
    driver.pressKey(seven);
    EXPECT_TRUE(isPressed(driver, "key:7"));
    EXPECT_EQ(driver.text("expression"), "7");
    KeyEvent sevenAgain = seven;
    sevenAgain.repeat = true;
    driver.pressKey(sevenAgain);
    EXPECT_EQ(driver.text("expression"), "7");
    driver.releaseKey(keyEvent('7', 16));
    EXPECT_FALSE(isPressed(driver, "key:7"));
    EXPECT_EQ(driver.text("expression"), "7");

    // * and / stand for the signs; Return for =.
    for (const auto& [keysym, keycode, text] : {std::tuple{uint32_t('*'), 55u, "*"}, std::tuple{uint32_t('6'), 15u, "6"}}) {
        driver.pressKey(keyEvent(keysym, keycode, text));
        driver.releaseKey(keyEvent(keysym, keycode));
    }
    EXPECT_EQ(driver.text("expression"), "7×6");
    driver.pressKey(keyEvent(key::Return, 28, "\r"));
    EXPECT_TRUE(isPressed(driver, "key:="));
    EXPECT_EQ(driver.text("expression"), "42");
    driver.releaseKey(keyEvent(key::Return, 28));
    EXPECT_FALSE(isPressed(driver, "key:="));

    // A held backspace repeats and stays down.
    driver.pressKey(keyEvent(key::BackSpace, 14));
    KeyEvent repeat = keyEvent(key::BackSpace, 14);
    repeat.repeat = true;
    driver.pressKey(repeat);
    EXPECT_TRUE(isPressed(driver, "key:BACKSPACE"));
    EXPECT_EQ(driver.text("expression"), "");
    driver.releaseKey(keyEvent(key::BackSpace, 14));
    EXPECT_FALSE(isPressed(driver, "key:BACKSPACE"));

    // Escape clears; shortcuts and other text change nothing.
    driver.pressKey(keyEvent('5', 6, "5"));
    driver.releaseKey(keyEvent('5', 6));
    KeyEvent shortcut = keyEvent('1', 2, "1");
    shortcut.modifiers = ControlModifier;
    driver.pressKey(shortcut);
    driver.releaseKey(shortcut);
    driver.typeText("a(");
    EXPECT_EQ(driver.text("expression"), "5");
    EXPECT_FALSE(isPressed(driver, "key:1"));
    driver.pressKey(keyEvent(key::Escape, 1));
    driver.releaseKey(keyEvent(key::Escape, 1));
    EXPECT_EQ(driver.text("expression"), "");

    // A key still down when the window is deactivated comes up.
    driver.setActive(true);
    driver.pressKey(keyEvent('8', 9, "8"));
    EXPECT_TRUE(isPressed(driver, "key:8"));
    driver.setActive(false);
    EXPECT_FALSE(isPressed(driver, "key:8"));
}

TEST(KeyboardTest, KeypadClicksKeepTheKeyboard)
{
    Scenario scenario;
    TestDriver& driver = scenario.driver;
    driver.setKeyboardFocus(true);
    driver.click("key:1");
    driver.typeText("2");
    EXPECT_EQ(driver.text("expression"), "12");
}

TEST(KeyboardTest, Typing)
{
    Scenario scenario;
    TestDriver& driver = scenario.driver;
    driver.setKeyboardFocus(true);
    driver.typeText("12*3+4");
    driver.typeKey(key::Return);
    EXPECT_EQ(driver.text("expression"), "40");
    driver.typeText("/4");
    driver.typeKey(key::KeypadEnter);
    EXPECT_EQ(driver.text("expression"), "10");
    EXPECT_EQ(driver.findAll("history:entry").size(), 2u);
}
