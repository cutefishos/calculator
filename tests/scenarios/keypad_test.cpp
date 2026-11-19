// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "scenario.h"

#include "ui/label.h"
#include "ui/theme.h"

#include <gtest/gtest.h>

using namespace cutefish::ui;

static size_t historyCount(TestDriver& driver)
{
    return driver.findAll("history:entry").size();
}

static std::string lastHistoryEntry(TestDriver& driver)
{
    const std::vector<Label*> entries = driver.findAll<Label>("history:entry");
    return entries.empty() ? std::string() : entries.back()->text();
}

TEST(KeypadTest, KeypadCalculates)
{
    Scenario scenario;
    TestDriver& driver = scenario.driver;
    for (const char* key : {"key:7", "key:×", "key:8"})
        driver.click(key);
    EXPECT_EQ(driver.text("expression"), "7×8");
    driver.click("key:=");
    EXPECT_EQ(driver.text("expression"), "56");
    EXPECT_EQ(historyCount(driver), 1u);
    EXPECT_EQ(lastHistoryEntry(driver), "7×8 = 56");

    driver.click("key:−");
    driver.click("key:6");
    driver.click("key:%");
    driver.click("key:=");
    EXPECT_EQ(driver.text("expression"), "52.64");

    driver.click("key:BACKSPACE");
    EXPECT_EQ(driver.text("expression"), "52.6");
    driver.click("key:÷");
    driver.click("key:BACKSPACE");
    EXPECT_EQ(driver.text("expression"), "52.6");

    // C empties the expression and turns into AC, which empties the history.
    EXPECT_EQ(driver.text("key:CLEAR"), "C");
    driver.click("key:CLEAR");
    EXPECT_EQ(driver.text("expression"), "");
    EXPECT_EQ(historyCount(driver), 2u);
    EXPECT_EQ(driver.text("key:CLEAR"), "AC");
    driver.click("key:CLEAR");
    EXPECT_EQ(historyCount(driver), 0u);
    EXPECT_EQ(driver.text("key:CLEAR"), "AC");
    driver.click("key:4");
    EXPECT_EQ(driver.text("key:CLEAR"), "C");
    driver.click("key:BACKSPACE");
    EXPECT_EQ(driver.text("key:CLEAR"), "AC");

    // An expression with no result stays as it is.
    for (const char* key : {"key:1", "key:÷", "key:0", "key:="})
        driver.click(key);
    EXPECT_EQ(driver.text("expression"), "1÷0");
    EXPECT_EQ(historyCount(driver), 0u);
}

TEST(KeypadTest, DoubleZeroKey)
{
    Scenario scenario;
    TestDriver& driver = scenario.driver;
    for (const char* key : {"key:3", "key:00", "key:−", "key:1", "key:="})
        driver.click(key);
    EXPECT_EQ(lastHistoryEntry(driver), "300−1 = 299");
    EXPECT_EQ(driver.text("expression"), "299");
}

TEST(KeypadTest, KeypadWorksInEitherTheme)
{
    for (const bool dark : {false, true}) {
        Theme::setCurrent(dark ? Theme::dark() : Theme::light());
        Scenario scenario(1.5);
        scenario.driver.click("key:1");
        EXPECT_EQ(scenario.driver.text("expression"), "1") << dark;
    }
    Theme::setCurrent(Theme::light());
}
