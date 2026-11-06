// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "calculator.h"

#include "ui/view.h"

#include <functional>
#include <string>
#include <vector>

namespace cutefish::ui {
class Icon;
class Label;
}

// A key of the keypad: a round plate as large as the key, with a label or a glyph.
class KeypadButton : public cutefish::ui::View
{
public:
    enum class Kind {
        Digit,
        // AC, %, backspace and the operators, on a plate a shade darker.
        Operator,
        // The = key.
        Accent,
    };

    KeypadButton(const std::string& text, Kind kind, const std::string& glyph = {});

    Kind kind() const { return m_kind; }
    bool isPressed() const { return m_pressed || m_keyDown; }
    // Shown pressed while a keyboard key holds it.
    void setKeyDown(bool down);
    void setText(const std::string& text);

    bool acceptsPointer() const override { return true; }

    std::function<void()> onClicked;

protected:
    void layout() override;
    void paint(cutefish::ui::Painter& painter) override;
    void pointerEnter() override;
    void pointerLeave() override;
    void pointerPress(const cutefish::ui::PointerEvent& event) override;
    void pointerRelease(const cutefish::ui::PointerEvent& event) override;

private:
    Kind m_kind;
    cutefish::ui::Label* m_label = nullptr;
    cutefish::ui::Icon* m_icon = nullptr;
    // The glyph's size as a share of the key's.
    double m_glyphScale = 0.29;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_keyDown = false;
};

// The standard keypad: four columns and five rows of round keys, as large as
// the width or the height allows, centred.
class Keypad : public cutefish::ui::View
{
public:
    // What the keys send: the digits and operators as they read, Clear for
    // C/AC and Backspace for the backspace key, as the calculator takes them.
    static constexpr const char* Clear = Calculator::Clear.data();
    static constexpr const char* Backspace = Calculator::Backspace.data();

    Keypad();

    // The height that fits keys as large as the width allows.
    static double fittingHeight(double width);

    const std::vector<KeypadButton*>& buttons() const { return m_buttons; }
    KeypadButton* button(const std::string& key) const;

    std::function<void(const std::string& key)> onPressed;

protected:
    void layout() override;
    void paint(cutefish::ui::Painter& painter) override;

private:
    std::vector<KeypadButton*> m_buttons;
    std::vector<std::string> m_keys;
};
