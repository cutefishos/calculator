// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculator.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <string_view>

TEST(CalculatorTest, ResultsAndHistory)
{
    Calculator calculator;
    EXPECT_EQ(calculator.calculate("1+2"), "3");
    EXPECT_EQ(calculator.calculate("7×8"), "56");
    // The same text as its result adds nothing.
    EXPECT_EQ(calculator.calculate("56"), "56");
    EXPECT_EQ(calculator.calculate("1÷0"), std::nullopt);
    EXPECT_EQ(calculator.lastResult().error, Evaluator::Error::DivisionByZero);
    EXPECT_EQ(calculator.calculate(""), std::nullopt);
    EXPECT_EQ(calculator.history(), (std::vector<std::string>{"1+2 = 3", "7×8 = 56"}));
    calculator.clearHistory();
    EXPECT_TRUE(calculator.history().empty());
}

TEST(CalculatorTest, GoesOnFromTheFullResult)
{
    Calculator calculator;
    EXPECT_EQ(calculator.calculate("1÷3"), "0.3333333333");
    EXPECT_EQ(calculator.calculate("0.3333333333×3"), "1");
    EXPECT_EQ(calculator.history().back(), "0.3333333333×3 = 1");
    // More digits after it make another number.
    EXPECT_EQ(calculator.calculate("2÷3"), "0.6666666667");
    EXPECT_EQ(calculator.calculate("0.66666666671×3"), "2.0000000001");
    // The result typed again by hand elsewhere in the expression is taken as typed.
    EXPECT_EQ(calculator.calculate("1÷3"), "0.3333333333");
    EXPECT_EQ(calculator.calculate("3×0.3333333333"), "0.9999999999");
    // Percent goes on from the full value too.
    EXPECT_EQ(calculator.calculate("2÷3"), "0.6666666667");
    EXPECT_EQ(calculator.calculate("0.6666666667×150%"), "1");
}

TEST(CalculatorTest, Backspace)
{
    EXPECT_EQ(Calculator::backspace("123"), "12");
    EXPECT_EQ(Calculator::backspace("1"), "");
    EXPECT_EQ(Calculator::backspace(""), "");
    // A character of several bytes goes whole.
    EXPECT_EQ(Calculator::backspace("6×"), "6");
    EXPECT_EQ(Calculator::backspace("6÷2"), "6÷");
}

// The expression after pressing each key of the sequence, from empty.
static std::string typed(std::initializer_list<std::string_view> keys)
{
    Calculator calculator;
    for (const std::string_view key : keys)
        calculator.press(key);
    return calculator.expression();
}

TEST(CalculatorTest, Operators)
{
    // An operator first follows a 0.
    EXPECT_EQ(typed({"+"}), "0+");
    EXPECT_EQ(typed({"−", "5"}), "0−5");
    EXPECT_EQ(typed({"×"}), "0×");
    // Another operator replaces it.
    EXPECT_EQ(typed({"7", "+", "×"}), "7×");
    EXPECT_EQ(typed({"7", "×", "−", "÷"}), "7÷");
    // A point before it goes.
    EXPECT_EQ(typed({"5", ".", "+"}), "5+");
    EXPECT_EQ(typed({"5", "%", "+", "2"}), "5%+2");
}

TEST(CalculatorTest, Numbers)
{
    // A lone 0 gives way to the digit; more zeros do nothing.
    EXPECT_EQ(typed({"0", "7"}), "7");
    EXPECT_EQ(typed({"0", "0", "0"}), "0");
    EXPECT_EQ(typed({"1", "+", "0", "5"}), "1+5");
    EXPECT_EQ(typed({"1", "0", "0"}), "100");
    // 00 is a single 0 where a number starts.
    EXPECT_EQ(typed({"00"}), "0");
    EXPECT_EQ(typed({"00", "00"}), "0");
    EXPECT_EQ(typed({"1", "+", "00"}), "1+0");
    EXPECT_EQ(typed({"3", "00"}), "300");
    EXPECT_EQ(typed({"0", ".", "00"}), "0.00");
    // One point to a number, after a 0 where it starts.
    EXPECT_EQ(typed({"."}), "0.");
    EXPECT_EQ(typed({"1", "+", "."}), "1+0.");
    EXPECT_EQ(typed({"1", ".", "5", "."}), "1.5");
    EXPECT_EQ(typed({"1", ".", "5", "+", "2", "."}), "1.5+2.");
    EXPECT_EQ(typed({"0", ".", "0", "5"}), "0.05");
    // Nothing after a %.
    EXPECT_EQ(typed({"5", "%", "3"}), "5%");
    EXPECT_EQ(typed({"5", "%", "."}), "5%");
}

TEST(CalculatorTest, PercentKey)
{
    EXPECT_EQ(typed({"%"}), "");
    EXPECT_EQ(typed({"5", "+", "%"}), "5+");
    EXPECT_EQ(typed({"5", "%", "%"}), "5%");
    EXPECT_EQ(typed({"5", ".", "%"}), "5%");
}

TEST(CalculatorTest, EqualsAndResults)
{
    Calculator calculator;
    for (const std::string_view key : {"=", "7", "+", "="})
        calculator.press(key);
    // Nothing to work out yet.
    EXPECT_EQ(calculator.expression(), "7+");
    EXPECT_TRUE(calculator.history().empty());
    for (const std::string_view key : {"8", "="})
        calculator.press(key);
    EXPECT_EQ(calculator.expression(), "15");
    // An operator goes on from the result, a digit starts again.
    calculator.press("×");
    EXPECT_EQ(calculator.expression(), "15×");
    for (const std::string_view key : {"2", "="})
        calculator.press(key);
    EXPECT_EQ(calculator.expression(), "30");
    calculator.press("4");
    EXPECT_EQ(calculator.expression(), "4");
    for (const std::string_view key : {"=", "."})
        calculator.press(key);
    EXPECT_EQ(calculator.expression(), "0.");
    EXPECT_EQ(calculator.history(), (std::vector<std::string>{"7+8 = 15", "15×2 = 30"}));
    // A result taken back by backspace is a number to type on.
    for (const std::string_view key : std::initializer_list<std::string_view>{Calculator::Clear, "9", "×", "9", "=", Calculator::Backspace, "1"})
        calculator.press(key);
    EXPECT_EQ(calculator.expression(), "81");
}

TEST(CalculatorTest, ClearKey)
{
    Calculator calculator;
    for (const std::string_view key : {"1", "+", "2", "=", "3"})
        calculator.press(key);
    calculator.press(Calculator::Clear);
    EXPECT_EQ(calculator.expression(), "");
    EXPECT_EQ(calculator.history().size(), 1u);
    calculator.press(Calculator::Clear);
    EXPECT_TRUE(calculator.history().empty());
}
