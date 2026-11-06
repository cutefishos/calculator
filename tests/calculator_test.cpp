// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculator.h"

#include <gtest/gtest.h>

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
