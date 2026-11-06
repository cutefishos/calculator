// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "decimal.h"

#include <algorithm>
#include <cstdio>

static constexpr uint32_t kPowersOfTen[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};

static unsigned limbDigits(uint32_t limb)
{
    unsigned digits = 1;
    while (digits < 9 && limb >= kPowersOfTen[digits])
        ++digits;
    return digits;
}

Natural::Natural(uint64_t value)
{
    while (value) {
        m_limbs.push_back(uint32_t(value % Base));
        value /= Base;
    }
}

std::optional<Natural> Natural::fromDigits(std::string_view digits)
{
    if (digits.empty())
        return std::nullopt;
    Natural result;
    result.m_limbs.reserve(digits.size() / 9 + 1);
    size_t end = digits.size();
    while (end > 0) {
        const size_t start = end >= 9 ? end - 9 : 0;
        uint32_t limb = 0;
        for (size_t i = start; i < end; ++i) {
            const char c = digits[i];
            if (c < '0' || c > '9')
                return std::nullopt;
            limb = limb * 10 + uint32_t(c - '0');
        }
        result.m_limbs.push_back(limb);
        end = start;
    }
    result.trim();
    return result;
}

Natural Natural::powerOfTen(unsigned exponent)
{
    return Natural(1).shifted(exponent);
}

unsigned Natural::digitCount() const
{
    if (m_limbs.empty())
        return 0;
    return unsigned(m_limbs.size() - 1) * 9 + limbDigits(m_limbs.back());
}

unsigned Natural::trailingZeros() const
{
    unsigned zeros = 0;
    for (const uint32_t limb : m_limbs) {
        if (limb == 0) {
            zeros += 9;
            continue;
        }
        uint32_t rest = limb;
        while (rest % 10 == 0) {
            rest /= 10;
            ++zeros;
        }
        return zeros;
    }
    return 0;
}

std::string Natural::toString() const
{
    if (m_limbs.empty())
        return "0";
    std::string text = std::to_string(m_limbs.back());
    char buffer[16];
    for (size_t i = m_limbs.size() - 1; i-- > 0;) {
        std::snprintf(buffer, sizeof(buffer), "%09u", m_limbs[i]);
        text += buffer;
    }
    return text;
}

std::strong_ordering Natural::operator<=>(const Natural& other) const
{
    if (m_limbs.size() != other.m_limbs.size())
        return m_limbs.size() <=> other.m_limbs.size();
    for (size_t i = m_limbs.size(); i-- > 0;) {
        if (m_limbs[i] != other.m_limbs[i])
            return m_limbs[i] <=> other.m_limbs[i];
    }
    return std::strong_ordering::equal;
}

Natural Natural::operator+(const Natural& other) const
{
    const std::vector<uint32_t>& longer = m_limbs.size() >= other.m_limbs.size() ? m_limbs : other.m_limbs;
    const std::vector<uint32_t>& shorter = m_limbs.size() >= other.m_limbs.size() ? other.m_limbs : m_limbs;
    Natural sum;
    sum.m_limbs.resize(longer.size() + 1);
    uint32_t carry = 0;
    for (size_t i = 0; i < longer.size(); ++i) {
        uint32_t limb = longer[i] + carry + (i < shorter.size() ? shorter[i] : 0);
        carry = limb >= Base;
        if (carry)
            limb -= Base;
        sum.m_limbs[i] = limb;
    }
    sum.m_limbs[longer.size()] = carry;
    sum.trim();
    return sum;
}

Natural Natural::operator-(const Natural& other) const
{
    Natural difference = *this;
    int64_t borrow = 0;
    for (size_t i = 0; i < difference.m_limbs.size(); ++i) {
        int64_t limb = int64_t(difference.m_limbs[i]) - borrow - (i < other.m_limbs.size() ? other.m_limbs[i] : 0);
        borrow = limb < 0;
        if (borrow)
            limb += Base;
        difference.m_limbs[i] = uint32_t(limb);
    }
    difference.trim();
    return difference;
}

Natural Natural::operator*(const Natural& other) const
{
    if (isZero() || other.isZero())
        return {};
    std::vector<uint64_t> sums(m_limbs.size() + other.m_limbs.size() + 1, 0);
    for (size_t i = 0; i < m_limbs.size(); ++i) {
        uint64_t carry = 0;
        const uint64_t factor = m_limbs[i];
        for (size_t j = 0; j < other.m_limbs.size(); ++j) {
            const uint64_t value = sums[i + j] + factor * other.m_limbs[j] + carry;
            sums[i + j] = value % Base;
            carry = value / Base;
        }
        size_t k = i + other.m_limbs.size();
        while (carry) {
            const uint64_t value = sums[k] + carry;
            sums[k] = value % Base;
            carry = value / Base;
            ++k;
        }
    }
    Natural product;
    product.m_limbs.assign(sums.begin(), sums.end());
    product.trim();
    return product;
}

Natural Natural::operator*(uint32_t factor) const
{
    if (factor == 0 || isZero())
        return {};
    Natural product;
    product.m_limbs.resize(m_limbs.size() + 1);
    uint64_t carry = 0;
    for (size_t i = 0; i < m_limbs.size(); ++i) {
        const uint64_t value = uint64_t(m_limbs[i]) * factor + carry;
        product.m_limbs[i] = uint32_t(value % Base);
        carry = value / Base;
    }
    product.m_limbs[m_limbs.size()] = uint32_t(carry % Base);
    if (carry >= Base)
        product.m_limbs.push_back(uint32_t(carry / Base));
    product.trim();
    return product;
}

Natural Natural::shifted(unsigned exponent) const
{
    if (isZero())
        return {};
    Natural result = exponent % 9 ? *this * kPowersOfTen[exponent % 9] : *this;
    result.m_limbs.insert(result.m_limbs.begin(), exponent / 9, 0);
    return result;
}

uint32_t Natural::divide(uint32_t divisor)
{
    uint64_t remainder = 0;
    for (size_t i = m_limbs.size(); i-- > 0;) {
        const uint64_t value = remainder * Base + m_limbs[i];
        m_limbs[i] = uint32_t(value / divisor);
        remainder = value % divisor;
    }
    trim();
    return uint32_t(remainder);
}

// Long division in base 10^9 with the divisor normalised so that its top
// limb is at least half the base, which keeps each estimated quotient limb
// at most two too large.
void Natural::divide(const Natural& dividend, const Natural& divisor, Natural* quotient, Natural* remainder)
{
    if (dividend < divisor) {
        if (remainder)
            *remainder = dividend;
        if (quotient)
            *quotient = {};
        return;
    }
    if (divisor.m_limbs.size() == 1) {
        Natural q = dividend;
        const uint32_t r = q.divide(divisor.m_limbs[0]);
        if (remainder)
            *remainder = Natural(r);
        if (quotient)
            *quotient = std::move(q);
        return;
    }

    const size_t n = divisor.m_limbs.size();
    const size_t m = dividend.m_limbs.size() - n;
    const uint32_t scale = uint32_t(Base / (uint64_t(divisor.m_limbs.back()) + 1));
    std::vector<uint32_t> u = (dividend * scale).m_limbs;
    u.resize(dividend.m_limbs.size() + 1, 0);
    std::vector<uint32_t> v = (divisor * scale).m_limbs;
    Natural q;
    q.m_limbs.assign(m + 1, 0);

    for (size_t j = m + 1; j-- > 0;) {
        const uint64_t top = uint64_t(u[j + n]) * Base + u[j + n - 1];
        uint64_t estimate = top / v[n - 1];
        uint64_t rest = top % v[n - 1];
        while (estimate >= Base || estimate * v[n - 2] > rest * Base + u[j + n - 2]) {
            --estimate;
            rest += v[n - 1];
            if (rest >= Base)
                break;
        }

        int64_t borrow = 0;
        uint64_t carry = 0;
        for (size_t i = 0; i < n; ++i) {
            const uint64_t product = estimate * v[i] + carry;
            carry = product / Base;
            int64_t limb = int64_t(u[i + j]) - int64_t(product % Base) - borrow;
            borrow = limb < 0;
            if (borrow)
                limb += Base;
            u[i + j] = uint32_t(limb);
        }
        const int64_t top_limb = int64_t(u[j + n]) - int64_t(carry) - borrow;
        if (top_limb < 0) {
            --estimate;
            uint32_t addCarry = 0;
            for (size_t i = 0; i < n; ++i) {
                uint32_t limb = u[i + j] + v[i] + addCarry;
                addCarry = limb >= Base;
                if (addCarry)
                    limb -= Base;
                u[i + j] = limb;
            }
            u[j + n] = uint32_t(top_limb + addCarry);
        } else {
            u[j + n] = uint32_t(top_limb);
        }
        q.m_limbs[j] = uint32_t(estimate);
    }

    q.trim();
    if (quotient)
        *quotient = std::move(q);
    if (remainder) {
        Natural r;
        r.m_limbs.assign(u.begin(), u.begin() + long(n));
        r.trim();
        r.divide(scale);
        *remainder = std::move(r);
    }
}

void Natural::trim()
{
    while (!m_limbs.empty() && m_limbs.back() == 0)
        m_limbs.pop_back();
}

Decimal::Decimal(int64_t value)
    : m_coefficient(value < 0 ? uint64_t(0) - uint64_t(value) : uint64_t(value))
    , m_negative(value < 0)
{
    normalize();
}

Decimal::Decimal(Natural coefficient, int64_t exponent, bool negative)
    : m_coefficient(std::move(coefficient))
    , m_exponent(exponent)
    , m_negative(negative)
{
    normalize();
}

std::optional<Decimal> Decimal::parse(std::string_view text)
{
    bool negative = false;
    if (!text.empty() && (text[0] == '-' || text[0] == '+')) {
        negative = text[0] == '-';
        text.remove_prefix(1);
    }
    std::string digits;
    int64_t exponent = 0;
    size_t i = 0;
    bool point = false;
    for (; i < text.size(); ++i) {
        const char c = text[i];
        if (c >= '0' && c <= '9') {
            digits += c;
            if (point)
                --exponent;
        } else if (c == '.' && !point) {
            point = true;
        } else {
            break;
        }
    }
    if (digits.empty())
        return std::nullopt;
    if (i < text.size()) {
        if (text[i] != 'e' && text[i] != 'E')
            return std::nullopt;
        ++i;
        bool negativeExponent = false;
        if (i < text.size() && (text[i] == '-' || text[i] == '+')) {
            negativeExponent = text[i] == '-';
            ++i;
        }
        if (i == text.size())
            return std::nullopt;
        int64_t value = 0;
        for (; i < text.size(); ++i) {
            const char c = text[i];
            if (c < '0' || c > '9')
                return std::nullopt;
            if (value > 1000000000000000)
                return std::nullopt;
            value = value * 10 + (c - '0');
        }
        exponent += negativeExponent ? -value : value;
    }
    return Decimal(*Natural::fromDigits(digits), exponent, negative);
}

bool Decimal::isInteger() const
{
    return m_exponent >= 0;
}

int64_t Decimal::adjustedExponent() const
{
    if (isZero())
        return 0;
    return m_exponent + int64_t(m_coefficient.digitCount()) - 1;
}

static std::string placePoint(const std::string& digits, int64_t exponent)
{
    if (exponent >= 0)
        return digits + std::string(size_t(exponent), '0');
    const int64_t point = int64_t(digits.size()) + exponent;
    if (point <= 0)
        return "0." + std::string(size_t(-point), '0') + digits;
    return digits.substr(0, size_t(point)) + "." + digits.substr(size_t(point));
}

std::string Decimal::toString() const
{
    if (isZero())
        return "0";
    return (m_negative ? "-" : "") + placePoint(m_coefficient.toString(), m_exponent);
}

std::string Decimal::toFixed(int decimals) const
{
    return roundedToDecimals(decimals).toString();
}

Decimal Decimal::operator-() const
{
    Decimal negated = *this;
    if (!negated.isZero())
        negated.m_negative = !negated.m_negative;
    return negated;
}

static std::strong_ordering compareMagnitudes(const Natural& a, int64_t aExponent, const Natural& b, int64_t bExponent)
{
    const int64_t aAdjusted = aExponent + int64_t(a.digitCount());
    const int64_t bAdjusted = bExponent + int64_t(b.digitCount());
    if (aAdjusted != bAdjusted)
        return aAdjusted <=> bAdjusted;
    if (aExponent > bExponent)
        return a.shifted(unsigned(aExponent - bExponent)) <=> b;
    return a <=> b.shifted(unsigned(bExponent - aExponent));
}

std::strong_ordering Decimal::operator<=>(const Decimal& other) const
{
    if (isZero() && other.isZero())
        return std::strong_ordering::equal;
    if (isZero())
        return other.m_negative ? std::strong_ordering::greater : std::strong_ordering::less;
    if (other.isZero())
        return m_negative ? std::strong_ordering::less : std::strong_ordering::greater;
    if (m_negative != other.m_negative)
        return m_negative ? std::strong_ordering::less : std::strong_ordering::greater;
    const std::strong_ordering magnitude = compareMagnitudes(m_coefficient, m_exponent, other.m_coefficient, other.m_exponent);
    if (!m_negative)
        return magnitude;
    return 0 <=> magnitude;
}

bool Decimal::operator==(const Decimal& other) const
{
    return m_negative == other.m_negative && m_exponent == other.m_exponent && m_coefficient == other.m_coefficient;
}

Decimal Decimal::add(const Decimal& a, const Decimal& b, int digits)
{
    if (a.isZero())
        return b.rounded(digits);
    if (b.isZero())
        return a.rounded(digits);

    // A term far below the rounding position only nudges the result; a unit
    // just below that position rounds the same way and keeps the sum short.
    const Decimal& larger = a.adjustedExponent() >= b.adjustedExponent() ? a : b;
    const Decimal* smaller = &larger == &a ? &b : &a;
    const int64_t floor = larger.adjustedExponent() - digits - 3;
    Decimal nudge;
    if (smaller->adjustedExponent() < floor) {
        nudge = Decimal(Natural(1), floor, smaller->m_negative);
        smaller = &nudge;
    }

    const int64_t exponent = std::min(larger.m_exponent, smaller->m_exponent);
    const Natural x = larger.m_coefficient.shifted(unsigned(larger.m_exponent - exponent));
    const Natural y = smaller->m_coefficient.shifted(unsigned(smaller->m_exponent - exponent));
    Decimal sum;
    if (larger.m_negative == smaller->m_negative)
        sum = Decimal(x + y, exponent, larger.m_negative);
    else if (x >= y)
        sum = Decimal(x - y, exponent, larger.m_negative);
    else
        sum = Decimal(y - x, exponent, smaller->m_negative);
    return sum.rounded(digits);
}

Decimal Decimal::subtract(const Decimal& a, const Decimal& b, int digits)
{
    return add(a, -b, digits);
}

Decimal Decimal::multiply(const Decimal& a, const Decimal& b, int digits)
{
    if (a.isZero() || b.isZero())
        return {};
    return Decimal(a.m_coefficient * b.m_coefficient, a.m_exponent + b.m_exponent, a.m_negative != b.m_negative)
        .rounded(digits);
}

Decimal Decimal::divide(const Decimal& a, const Decimal& b, int digits)
{
    if (a.isZero())
        return {};
    const int64_t shift =
        std::max<int64_t>(0, int64_t(digits) + 2 + int64_t(b.m_coefficient.digitCount()) - int64_t(a.m_coefficient.digitCount()));
    Natural quotient;
    Natural remainder;
    Natural::divide(a.m_coefficient.shifted(unsigned(shift)), b.m_coefficient, &quotient, &remainder);
    int64_t exponent = a.m_exponent - b.m_exponent - shift;
    if (!remainder.isZero()) {
        // A sticky digit: the quotient is not exact, so it never rounds as a tie.
        quotient = quotient * 10 + Natural(1);
        --exponent;
    }
    return Decimal(std::move(quotient), exponent, a.m_negative != b.m_negative).rounded(digits);
}

Decimal Decimal::rounded(int digits) const
{
    const unsigned count = m_coefficient.digitCount();
    if (int(count) <= digits)
        return *this;
    const unsigned drop = count - unsigned(digits);
    Natural quotient;
    Natural remainder;
    const Natural unit = Natural::powerOfTen(drop);
    Natural::divide(m_coefficient, unit, &quotient, &remainder);
    const std::strong_ordering half = remainder * 2 <=> unit;
    if (half > 0 || (half == 0 && quotient.isOdd()))
        quotient = quotient + Natural(1);
    return Decimal(std::move(quotient), m_exponent + drop, m_negative);
}

Decimal Decimal::roundedToDecimals(int64_t decimals) const
{
    if (isZero() || m_exponent >= -decimals)
        return *this;
    const int64_t drop = -decimals - m_exponent;
    Natural quotient;
    bool upperHalf = false;
    if (drop > int64_t(m_coefficient.digitCount()) + 1) {
        // Well under half a unit.
    } else {
        Natural remainder;
        const Natural unit = Natural::powerOfTen(unsigned(drop));
        Natural::divide(m_coefficient, unit, &quotient, &remainder);
        upperHalf = remainder * 2 >= unit;
    }
    if (upperHalf)
        quotient = quotient + Natural(1);
    return Decimal(std::move(quotient), -decimals, m_negative);
}

void Decimal::normalize()
{
    if (m_coefficient.isZero()) {
        m_exponent = 0;
        m_negative = false;
        return;
    }
    const unsigned zeros = m_coefficient.trailingZeros();
    if (zeros == 0)
        return;
    Natural quotient;
    Natural::divide(m_coefficient, Natural::powerOfTen(zeros), &quotient, nullptr);
    m_coefficient = std::move(quotient);
    m_exponent += zeros;
}
