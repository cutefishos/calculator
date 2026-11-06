// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculator.h"
#include "evaluator.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

using Error = Evaluator::Error;

// The result as the calculator shows it.
static std::string shown(Evaluator& evaluator, const std::string& expression)
{
    const Evaluator::Result result = evaluator.evaluate(expression);
    if (!result.value)
        return "error";
    return Calculator::format(*result.value);
}

static void expectResults(const std::vector<std::pair<const char*, const char*>>& cases)
{
    Evaluator evaluator;
    for (const auto& [expression, expected] : cases)
        EXPECT_EQ(shown(evaluator, expression), expected) << expression;
}

static void expectError(const std::string& expression, Error error)
{
    Evaluator evaluator;
    EXPECT_EQ(evaluator.evaluate(expression).error, error) << expression;
}

TEST(EvaluatorTest, Arithmetic)
{
    expectResults({
        {"1+2", "3"},
        {"2+3×4", "14"},
        {"10÷4", "2.5"},
        {"1÷3", "0.3333333333"},
        {"2÷3", "0.6666666667"},
        {"0.1+0.2", "0.3"},
        {"1-0.9", "0.1"},
        {"0.1×3", "0.3"},
        {"1÷3×3", "1"},
        {"10-2-3", "5"},
        {"100÷10÷5", "2"},
        {"8÷2×4", "16"},
        {"-5+2", "-3"},
        {"--5", "5"},
        {"+5", "5"},
        {"3×-2", "-6"},
        {"123456789×987654321", "121932631112635269"},
        {".5+.5", "1"},
        {"5.+1", "6"},
        {"007", "7"},
        {"300-1", "299"},
    });
}

TEST(EvaluatorTest, Notation)
{
    expectResults({
        {"6/3*2", "4"},
        {"5−3", "2"},
        {"  1 +\t2  ", "3"},
    });
}

TEST(EvaluatorTest, Percent)
{
    expectResults({
        {"50%", "0.5"},
        {"200+10%", "220"},
        {"200-10%", "180"},
        {"200×10%", "20"},
        {"10%×200", "20"},
        {"200÷50%", "400"},
        {"50%%", "0.005"},
        {"100+-10%", "90"},
        {"100+50+10%", "165"},
        {"10%+5", "5.1"},
    });
}

TEST(EvaluatorTest, Errors)
{
    expectError("", Error::Empty);
    expectError("   ", Error::Empty);
    expectError("1+", Error::Syntax);
    expectError("×2", Error::Syntax);
    expectError("2 3", Error::Syntax);
    expectError("1..2", Error::Syntax);
    expectError(".", Error::Syntax);
    expectError("%", Error::Syntax);
    expectError("(1+2)", Error::Syntax);
    expectError("2^3", Error::Syntax);
    expectError("1e3", Error::Syntax);
    expectError("1÷0", Error::DivisionByZero);
    expectError("5÷(0)", Error::Syntax);
    expectError("1" + std::string(1000, '0'), Error::Overflow);

    Evaluator evaluator;
    const Evaluator::Result result = evaluator.evaluate("1 + $");
    EXPECT_EQ(result.error, Error::Syntax);
    EXPECT_EQ(result.position, 4u);
    EXPECT_EQ(evaluator.evaluate("4÷0").position, 1u);
}

TEST(EvaluatorTest, Limits)
{
    Evaluator evaluator;
    const std::string largest(1000, '9');
    EXPECT_EQ(shown(evaluator, largest), largest);
    EXPECT_EQ(evaluator.evaluate(largest + "+1").error, Error::Overflow);
    EXPECT_EQ(evaluator.evaluate(largest + "×10").error, Error::Overflow);
    // Whole numbers keep every digit up to the limit.
    EXPECT_EQ(shown(evaluator, largest + "-" + largest + "+1"), "1");
    EXPECT_EQ(shown(evaluator, "1" + std::string(998, '0') + "+1-1" + std::string(998, '0')), "1");
    // Below 10^-1000 is zero.
    EXPECT_EQ(shown(evaluator, "0." + std::string(1000, '0') + "1"), "0");
}

TEST(EvaluatorTest, Ans)
{
    Evaluator evaluator;
    EXPECT_EQ(shown(evaluator, "ans"), "0");
    EXPECT_EQ(shown(evaluator, "1÷3"), "0.3333333333");
    EXPECT_EQ(shown(evaluator, "ans×3"), "1");
    // A failed evaluation keeps ans.
    EXPECT_EQ(shown(evaluator, "1÷0"), "error");
    EXPECT_EQ(shown(evaluator, "ans+1"), "2");
    EXPECT_EQ(evaluator.ans().toString(), "2");
}
