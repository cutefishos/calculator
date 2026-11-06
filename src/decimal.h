// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// A whole number of any size, in limbs of nine decimal digits, least
// significant first, with no zero limbs at the top.
class Natural
{
public:
    static constexpr uint32_t Base = 1000000000;

    Natural() = default;
    Natural(uint64_t value);

    // Decimal digits only; nullopt for anything else or an empty string.
    static std::optional<Natural> fromDigits(std::string_view digits);
    static Natural powerOfTen(unsigned exponent);

    bool isZero() const { return m_limbs.empty(); }
    bool isOdd() const { return !m_limbs.empty() && (m_limbs[0] & 1); }
    // How many decimal digits it has; 0 has none.
    unsigned digitCount() const;
    // The trailing decimal zeros; 0 has none.
    unsigned trailingZeros() const;
    std::string toString() const;

    std::strong_ordering operator<=>(const Natural& other) const;
    bool operator==(const Natural& other) const = default;

    Natural operator+(const Natural& other) const;
    // The other must not be larger.
    Natural operator-(const Natural& other) const;
    Natural operator*(const Natural& other) const;
    Natural operator*(uint32_t factor) const;
    // Times ten to the power.
    Natural shifted(unsigned exponent) const;
    // The quotient and the remainder; the divisor must not be zero.
    static void divide(const Natural& dividend, const Natural& divisor, Natural* quotient, Natural* remainder);
    // Returns the remainder.
    uint32_t divide(uint32_t divisor);

private:
    void trim();

    std::vector<uint32_t> m_limbs;
};

// A signed decimal number: coefficient × 10^exponent. Every operation is
// exact or rounds half to even to a number of significant digits.
class Decimal
{
public:
    // The significant digits arithmetic keeps unless told otherwise.
    static constexpr int Precision = 160;

    Decimal() = default;
    Decimal(int64_t value);
    Decimal(Natural coefficient, int64_t exponent, bool negative = false);

    // "123", "-1.5", ".5", "2.", "1e-3", "6.02E23"; nullopt for anything else.
    static std::optional<Decimal> parse(std::string_view text);

    bool isZero() const { return m_coefficient.isZero(); }
    bool isNegative() const { return m_negative; }
    bool isInteger() const;
    // The exponent of the leading digit: 2 for 123, -3 for 0.00456; 0 for zero.
    int64_t adjustedExponent() const;
    const Natural& coefficient() const { return m_coefficient; }
    int64_t exponent() const { return m_exponent; }

    // Plain digits, as many as there are; no exponent.
    std::string toString() const;
    // At most the given decimals, rounded half away from zero, trailing zeros
    // and a bare point dropped; never "-0".
    std::string toFixed(int decimals) const;

    Decimal operator-() const;
    std::strong_ordering operator<=>(const Decimal& other) const;
    bool operator==(const Decimal& other) const;

    static Decimal add(const Decimal& a, const Decimal& b, int digits = Precision);
    static Decimal subtract(const Decimal& a, const Decimal& b, int digits = Precision);
    static Decimal multiply(const Decimal& a, const Decimal& b, int digits = Precision);
    // The divisor must not be zero.
    static Decimal divide(const Decimal& a, const Decimal& b, int digits = Precision);

    Decimal operator+(const Decimal& other) const { return add(*this, other); }
    Decimal operator-(const Decimal& other) const { return subtract(*this, other); }
    Decimal operator*(const Decimal& other) const { return multiply(*this, other); }
    Decimal operator/(const Decimal& other) const { return divide(*this, other); }

    // Rounded half to even to the significant digits.
    Decimal rounded(int digits) const;

private:
    // Rounded half away from zero to the decimals after the point.
    Decimal roundedToDecimals(int64_t decimals) const;
    // Drops trailing zeros of the coefficient into the exponent.
    void normalize();

    Natural m_coefficient;
    int64_t m_exponent = 0;
    bool m_negative = false;
};
