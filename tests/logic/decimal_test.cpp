// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "decimal.h"

#include <gtest/gtest.h>

#include <random>

static Decimal parsed(const char* text)
{
    const std::optional<Decimal> value = Decimal::parse(text);
    EXPECT_TRUE(value) << text;
    return value.value_or(Decimal());
}

TEST(NaturalTest, DigitsRoundTrip)
{
    for (const char* digits : {"0", "1", "999999999", "1000000000", "123456789012345678901234567890"}) {
        const std::optional<Natural> value = Natural::fromDigits(digits);
        ASSERT_TRUE(value);
        EXPECT_EQ(value->toString(), digits);
    }
    EXPECT_EQ(Natural::fromDigits("000123")->toString(), "123");
    EXPECT_FALSE(Natural::fromDigits(""));
    EXPECT_FALSE(Natural::fromDigits("12a"));
}

TEST(NaturalTest, Counts)
{
    EXPECT_EQ(Natural().digitCount(), 0u);
    EXPECT_EQ(Natural(7).digitCount(), 1u);
    EXPECT_EQ(Natural(1000000000).digitCount(), 10u);
    EXPECT_EQ(Natural::powerOfTen(25).digitCount(), 26u);
    EXPECT_EQ(Natural::powerOfTen(25).trailingZeros(), 25u);
    EXPECT_EQ(Natural(120).trailingZeros(), 1u);
}

TEST(NaturalTest, Arithmetic)
{
    const Natural a = *Natural::fromDigits("123456789123456789123456789");
    const Natural b = *Natural::fromDigits("987654321987654321");
    EXPECT_EQ((a + b).toString(), "123456790111111111111111110");
    EXPECT_EQ((a - b).toString(), "123456788135802467135802468");
    EXPECT_EQ((a * b).toString(), "121932631356500531469135800347203169112635269");
    EXPECT_EQ((a * 1000u).toString(), "123456789123456789123456789000");
    EXPECT_EQ(a.shifted(11).toString(), "12345678912345678912345678900000000000");
    Natural quotient;
    Natural remainder;
    Natural::divide(a, b, &quotient, &remainder);
    EXPECT_EQ(quotient.toString(), "124999998");
    EXPECT_EQ(remainder.toString(), "850308642973765431");
    Natural small = a;
    EXPECT_EQ(small.divide(1000u), 789u);
    EXPECT_EQ(small.toString(), "123456789123456789123456");
}

// Every quotient and remainder put back together gives the dividend.
TEST(NaturalTest, DivisionReconstructs)
{
    std::mt19937_64 random(20261023);
    const auto randomNatural = [&random](int limbs) {
        Natural value;
        for (int i = 0; i < limbs; ++i)
            value = value.shifted(9) + Natural(random() % Natural::Base);
        return value;
    };
    for (int round = 0; round < 3000; ++round) {
        const Natural dividend = randomNatural(1 + int(random() % 12));
        Natural divisor = randomNatural(1 + int(random() % 6));
        // Divisors whose top limb is small or large test the normalisation.
        if (round % 3 == 0)
            divisor = divisor.shifted(unsigned(random() % 9)) + Natural(1);
        if (divisor.isZero())
            continue;
        Natural quotient;
        Natural remainder;
        Natural::divide(dividend, divisor, &quotient, &remainder);
        ASSERT_LT(remainder, divisor);
        ASSERT_EQ(quotient * divisor + remainder, dividend) << dividend.toString() << " / " << divisor.toString();
    }
}

TEST(DecimalTest, ParseAndPrint)
{
    EXPECT_EQ(parsed("123").toString(), "123");
    EXPECT_EQ(parsed("-1.50").toString(), "-1.5");
    EXPECT_EQ(parsed(".5").toString(), "0.5");
    EXPECT_EQ(parsed("2.").toString(), "2");
    EXPECT_EQ(parsed("1e-3").toString(), "0.001");
    EXPECT_EQ(parsed("6.02E23").toString(), "602000000000000000000000");
    EXPECT_EQ(parsed("0.000").toString(), "0");
    EXPECT_EQ(parsed("-0").toString(), "0");
    EXPECT_FALSE(parsed("-0").isNegative());
    for (const char* bad : {"", ".", "e5", "1e", "1e+", "1.2.3", "1x", "--1"})
        EXPECT_FALSE(Decimal::parse(bad)) << bad;
}

TEST(DecimalTest, FixedFormat)
{
    EXPECT_EQ(parsed("0.33333333333333").toFixed(10), "0.3333333333");
    EXPECT_EQ(parsed("0.66666666666666").toFixed(10), "0.6666666667");
    EXPECT_EQ(parsed("2.5").toFixed(0), "3");
    EXPECT_EQ(parsed("-2.5").toFixed(0), "-3");
    EXPECT_EQ(parsed("1.10").toFixed(10), "1.1");
    EXPECT_EQ(parsed("-0.00000000001").toFixed(10), "0");
    EXPECT_EQ(parsed("0.99999999999").toFixed(10), "1");
    EXPECT_EQ(parsed("1e-1000").toFixed(10), "0");
    EXPECT_EQ(parsed("1e12").toFixed(10), "1000000000000");
}

TEST(DecimalTest, Arithmetic)
{
    EXPECT_EQ((parsed("0.1") + parsed("0.2")).toString(), "0.3");
    EXPECT_EQ((parsed("1") - parsed("0.9")).toString(), "0.1");
    EXPECT_EQ((parsed("1.5") * parsed("-4")).toString(), "-6");
    EXPECT_EQ((parsed("1") / parsed("8")).toString(), "0.125");
    EXPECT_EQ((parsed("1e20") + parsed("1")).toString(), "100000000000000000001");
    EXPECT_EQ((parsed("5") - parsed("5")).toString(), "0");
    EXPECT_EQ(Decimal::divide(Decimal(2), Decimal(3), 5).toString(), "0.66667");
    EXPECT_EQ(Decimal::divide(Decimal(1), Decimal(3), 160).coefficient().digitCount(), 160u);
    // A term far under the precision still rounds the sum the right way.
    EXPECT_EQ(Decimal::add(parsed("1"), parsed("1e-500"), 5).toString(), "1");
    EXPECT_EQ(Decimal::subtract(parsed("1"), parsed("1e-500"), 5).toString(), "1");
    EXPECT_EQ(Decimal::add(parsed("1.000005"), parsed("1e-500"), 6).toString(), "1.00001");
    EXPECT_EQ(Decimal::add(parsed("1.000005"), parsed("-1e-500"), 6).toString(), "1");
}

TEST(DecimalTest, Rounding)
{
    EXPECT_EQ(parsed("12345").rounded(3).toString(), "12300");
    EXPECT_EQ(parsed("12350").rounded(3).toString(), "12400");
    EXPECT_EQ(parsed("12450").rounded(3).toString(), "12400");
    EXPECT_EQ(parsed("99999").rounded(3).toString(), "100000");
}

TEST(DecimalTest, Comparison)
{
    EXPECT_LT(parsed("-2"), parsed("-1"));
    EXPECT_LT(parsed("-1"), parsed("0"));
    EXPECT_LT(parsed("0"), parsed("0.0001"));
    EXPECT_LT(parsed("0.0999"), parsed("0.1"));
    EXPECT_GT(parsed("1e10"), parsed("9999999999.99"));
    EXPECT_EQ(parsed("1.000"), parsed("1"));
    EXPECT_EQ(parsed("10").coefficient(), Natural(1));
    EXPECT_EQ(parsed("10").exponent(), 1);
}

TEST(DecimalTest, Exponents)
{
    EXPECT_EQ(parsed("123.45").adjustedExponent(), 2);
    EXPECT_EQ(parsed("0.00456").adjustedExponent(), -3);
    EXPECT_TRUE(parsed("1e3").isInteger());
    EXPECT_FALSE(parsed("0.5").isInteger());
}
