// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "colors.h"

#include "ui/theme.h"

using namespace cutefish::ui;

void addCalculatorColors(ColorMixer& mixer, const Theme& theme)
{
    const bool dark = theme.darkMode;
    mixer[CalculatorColor::WindowBackground] = (dark ? Color::rgb(0x000000) : Color::rgb(0xE8E8E8)).withAlpha(0.95);

    const Color key = dark ? Color::rgb(0x5A5A5A) : Color::rgb(0xF7F7F7);
    mixer[CalculatorColor::Key] = key;
    mixer[CalculatorColor::KeyRim] = dark ? Color::rgb(0x7A7A7A) : Color::rgb(0xFCFCFC);
    mixer[CalculatorColor::KeyHovered] = dark ? key.lighter(1.12) : key.darker(1.03);
    mixer[CalculatorColor::KeyPressed] = dark ? key.lighter(1.3) : key.darker(1.08);

    const Color operatorKey = dark ? Color::rgb(0x272727) : Color::rgb(0xEBEBEB);
    mixer[CalculatorColor::OperatorKey] = operatorKey;
    mixer[CalculatorColor::OperatorKeyRim] = dark ? Color::rgb(0x3A3A3A) : Color::rgb(0xF5F5F5);
    mixer[CalculatorColor::OperatorKeyHovered] = dark ? Color::rgb(0x323232) : operatorKey.darker(1.03);
    mixer[CalculatorColor::OperatorKeyPressed] = dark ? Color::rgb(0x404040) : operatorKey.darker(1.08);

    mixer[CalculatorColor::AccentKey] = ColorId::Highlight;
    mixer[CalculatorColor::AccentKeyRim] = lighter(ColorId::Highlight, 1.2);
    mixer[CalculatorColor::AccentKeyHovered] = dark ? lighter(ColorId::Highlight, 1.06) : darker(ColorId::Highlight, 1.05);
    mixer[CalculatorColor::AccentKeyPressed] = dark ? lighter(ColorId::Highlight, 1.14) : darker(ColorId::Highlight, 1.14);

    mixer[CalculatorColor::InactiveAccentKey] = dark ? Color::rgb(0x6E6E6E) : Color::rgb(0xB8B8B8);
    mixer[CalculatorColor::InactiveAccentKeyRim] = dark ? Color::rgb(0x8A8A8A) : Color::rgb(0xCDCDCD);

    mixer[CalculatorColor::KeyShadow] = dark ? Color::rgb(0x000000).withAlpha(0) : Color::rgb(0x000000).withAlpha(0.07);
    mixer[CalculatorColor::AccentKeyShadow] = withAlpha(ColorId::Highlight, dark ? 0.3 : 0.35);
}
