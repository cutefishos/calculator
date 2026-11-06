// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ui/view.h"

namespace cutefish::ui {
class Label;
class TextField;
}

// The expression being typed, in large text against the right edge: a text
// field without a frame, as wide as its text until it fills the row.
class ExpressionField : public cutefish::ui::View
{
public:
    static constexpr double FontSize = 20;

    ExpressionField();

    cutefish::ui::TextField* textField() const { return m_field; }

    bool acceptsPointer() const override { return true; }

protected:
    void layout() override;
    // A press left of the text puts the cursor at its start.
    void pointerPress(const cutefish::ui::PointerEvent& event) override;

private:
    cutefish::ui::TextField* m_field;
    // Measures the text; never shown.
    cutefish::ui::Label* m_measure;
};
