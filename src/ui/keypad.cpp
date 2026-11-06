// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "keypad.h"

#include "colors.h"

#include "ui/icon.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/path.h"
#include "ui/window.h"

#include <algorithm>
#include <cmath>

using namespace cutefish::ui;

static constexpr int kColumns = 4;
static constexpr int kRows = 5;
// The gap between keys and the margin around them, as shares of a key's diameter.
static constexpr double kKeySpacing = 31.0 / 216;
static constexpr double kKeypadMargin = 58.0 / 216;
static constexpr double kRimWidth = 4.0 / 216;

namespace {

struct KeyInfo
{
    const char* key;
    // A label, or the name of a glyph in icons/.
    const char* label;
    const char* glyph;
    KeypadButton::Kind kind;
};

} // namespace

static const KeyInfo kKeys[] = {
    {Keypad::Clear, "AC", nullptr, KeypadButton::Kind::Operator},
    {"%", "%", nullptr, KeypadButton::Kind::Operator},
    {Keypad::Backspace, nullptr, "backspace", KeypadButton::Kind::Operator},
    {"÷", nullptr, "divide", KeypadButton::Kind::Operator},
    {"7", "7", nullptr, KeypadButton::Kind::Digit},
    {"8", "8", nullptr, KeypadButton::Kind::Digit},
    {"9", "9", nullptr, KeypadButton::Kind::Digit},
    {"×", nullptr, "multiply", KeypadButton::Kind::Operator},
    {"4", "4", nullptr, KeypadButton::Kind::Digit},
    {"5", "5", nullptr, KeypadButton::Kind::Digit},
    {"6", "6", nullptr, KeypadButton::Kind::Digit},
    {"−", nullptr, "minus", KeypadButton::Kind::Operator},
    {"1", "1", nullptr, KeypadButton::Kind::Digit},
    {"2", "2", nullptr, KeypadButton::Kind::Digit},
    {"3", "3", nullptr, KeypadButton::Kind::Digit},
    {"+", nullptr, "plus", KeypadButton::Kind::Operator},
    {"00", "00", nullptr, KeypadButton::Kind::Digit},
    {"0", "0", nullptr, KeypadButton::Kind::Digit},
    {".", ".", nullptr, KeypadButton::Kind::Digit},
    {"=", nullptr, "equals", KeypadButton::Kind::Accent},
};

// The accent shows only while the window is active, as the window controls'.
static bool isWindowActive(const View* view)
{
    const Window* window = view->window();
    return !window || window->isActive();
}

static double diameterForWidth(double width)
{
    return width / (kColumns + (kColumns - 1) * kKeySpacing + 2 * kKeypadMargin);
}

static double diameterForHeight(double height)
{
    return height / (kRows + (kRows - 1) * kKeySpacing + 2 * kKeypadMargin);
}

KeypadButton::KeypadButton(const std::string& text, Kind kind, const std::string& glyph)
    : m_kind(kind)
{
    if (!glyph.empty()) {
        m_icon = addChild<Icon>(glyph);
        if (glyph == "backspace")
            m_glyphScale = 0.38;
        m_icon->setColor(kind == Kind::Accent ? ColorId::HighlightedText : ColorId::Text);
        return;
    }
    m_label = addChild<Label>(text);
    m_label->setTextAlignment(Label::TextAlignment::Center);
    m_label->setVerticallyCentered(true);
    m_label->setColor(kind == Kind::Accent ? ColorId::HighlightedText : ColorId::Text);
}

void KeypadButton::layout()
{
    const double height = geometry().height;
    if (m_label) {
        m_label->setFontSize(std::round(height * 0.36));
        m_label->setGeometry(localRect());
    }
    if (m_icon) {
        const double size = height * m_glyphScale;
        m_icon->setSize(size);
        m_icon->setGeometry({(geometry().width - size) / 2, (height - size) / 2, size, size});
    }
}

void KeypadButton::paint(Painter& painter)
{
    CalculatorColor base;
    CalculatorColor rim;
    switch (m_kind) {
    case Kind::Digit:
        base = isPressed() ? CalculatorColor::KeyPressed : m_hovered ? CalculatorColor::KeyHovered : CalculatorColor::Key;
        rim = CalculatorColor::KeyRim;
        break;
    case Kind::Operator:
        base = isPressed() ? CalculatorColor::OperatorKeyPressed
               : m_hovered ? CalculatorColor::OperatorKeyHovered
                           : CalculatorColor::OperatorKey;
        rim = CalculatorColor::OperatorKeyRim;
        break;
    case Kind::Accent:
        if (!isWindowActive(this)) {
            base = CalculatorColor::InactiveAccentKey;
            rim = CalculatorColor::InactiveAccentKeyRim;
            break;
        }
        base = isPressed() ? CalculatorColor::AccentKeyPressed
               : m_hovered ? CalculatorColor::AccentKeyHovered
                           : CalculatorColor::AccentKey;
        rim = CalculatorColor::AccentKeyRim;
        break;
    }

    // A square of whole device pixels, so the plate comes out round.
    const double scale = painter.scale();
    const Rect device = painter.snappedRect(localRect());
    const PointF offset = painter.offset();
    const RectF plate{device.x / scale - offset.x, device.y / scale - offset.y, device.width / scale,
                      device.height / scale};

    // The rim shows at the top and the bottom and fades out towards the sides.
    const Color fill = color(base);
    const Color edge = color(rim);
    const LinearGradient gradient{{plate.x, plate.y},
                                  {plate.x, plate.bottom()},
                                  {{0, edge}, {0.3, fill}, {0.7, fill}, {1, edge}}};
    painter.fillRoundedRect(plate, plate.width / 2, gradient);
    const double inset = std::max(1.0, std::round(plate.width * kRimWidth * scale)) / scale;
    painter.fillEllipse(plate.adjusted(inset, inset, -inset, -inset), fill);
}

void KeypadButton::setKeyDown(bool down)
{
    if (down == m_keyDown)
        return;
    m_keyDown = down;
    requestPaint();
}

void KeypadButton::setText(const std::string& text)
{
    if (m_label)
        m_label->setText(text);
}

void KeypadButton::pointerEnter()
{
    m_hovered = true;
    requestPaint();
}

void KeypadButton::pointerLeave()
{
    m_hovered = false;
    requestPaint();
}

void KeypadButton::pointerPress(const PointerEvent& event)
{
    if (event.button != MouseButton::Left)
        return;
    m_pressed = true;
    requestPaint();
}

void KeypadButton::pointerRelease(const PointerEvent& event)
{
    if (!m_pressed)
        return;
    m_pressed = false;
    requestPaint();
    if (localRect().contains(event.position) && onClicked)
        onClicked();
}

Keypad::Keypad()
{
    for (const KeyInfo& info : kKeys) {
        const std::string key = info.key;
        KeypadButton* button = addChild<KeypadButton>(info.label ? info.label : "", info.kind,
                                                      info.glyph ? info.glyph : "");
        button->setName("key " + key);
        button->onClicked = [this, key] {
            if (onPressed)
                onPressed(key);
        };
        m_buttons.push_back(button);
        m_keys.push_back(key);
    }
}

KeypadButton* Keypad::button(const std::string& key) const
{
    for (size_t i = 0; i < m_keys.size(); ++i) {
        if (m_keys[i] == key)
            return m_buttons[i];
    }
    return nullptr;
}

double Keypad::fittingHeight(double width)
{
    return diameterForWidth(width) * (kRows + (kRows - 1) * kKeySpacing + 2 * kKeypadMargin);
}

void Keypad::layout()
{
    // Positions are fractional; the plates snap to the device grid as they paint.
    const double width = geometry().width;
    const double height = geometry().height;
    const double diameter = std::max(0.0, std::min(diameterForWidth(width), diameterForHeight(height)));
    const double pitch = diameter * (1 + kKeySpacing);
    const double gridWidth = kColumns * diameter + (kColumns - 1) * diameter * kKeySpacing;
    const double gridHeight = kRows * diameter + (kRows - 1) * diameter * kKeySpacing;
    const double x = (width - gridWidth) / 2;
    const double y = (height - gridHeight) / 2;
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        const int column = int(i) % kColumns;
        const int row = int(i) / kColumns;
        m_buttons[i]->setGeometry({x + column * pitch, y + row * pitch, diameter, diameter});
    }
}

void Keypad::paint(Painter& painter)
{
    // The shadows reach past the keys, so the keypad draws them under all of them.
    for (const KeypadButton* button : m_buttons) {
        const RectF rect = button->geometry();
        const double diameter = rect.width;
        if (button->kind() == KeypadButton::Kind::Accent && isWindowActive(this))
            painter.drawShadow(rect, diameter / 2, diameter * 0.09, color(CalculatorColor::AccentKeyShadow),
                               {0, diameter * 0.07});
        else
            painter.drawShadow(rect, diameter / 2, diameter * 0.1, color(CalculatorColor::KeyShadow),
                               {0, diameter * 0.04});
    }
}
