// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ui/view.h"

namespace cutefish::ui {
class TextField;
}

// The expression being typed, in large text against the right edge: a text
// field without a frame across the row.
class ExpressionField : public cutefish::ui::View
{
public:
    static constexpr double FontSize = 20;

    ExpressionField();

    cutefish::ui::TextField* textField() const { return m_field; }

protected:
    void layout() override;

private:
    cutefish::ui::TextField* m_field;
};
