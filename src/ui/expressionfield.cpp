// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "expressionfield.h"

#include "ui/text_field.h"
#include "ui/theme.h"

using namespace cutefish::ui;

ExpressionField::ExpressionField()
    : m_field(addChild<TextField>())
{
    setName("expressionRow");
    m_field->setName("expression");
    m_field->setFrameVisible(false);
    m_field->setFontSize(FontSize);
    m_field->setRightAligned(true);
}

void ExpressionField::layout()
{
    const double margin = Theme::current().largeSpacing;
    m_field->setTextMargins(margin, margin);
    m_field->setGeometry(localRect());
}
