// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "expressionfield.h"

#include "ui/label.h"
#include "ui/theme.h"

#include <algorithm>

using namespace cutefish::ui;

ExpressionField::ExpressionField()
    : m_clip(addChild<View>())
    , m_label(m_clip->addChild<Label>())
{
    setName("expressionRow");
    m_clip->setClipsChildren(true);
    m_label->setName("expression");
    m_label->setFontSize(FontSize);
    m_label->setTextAlignment(Label::TextAlignment::Right);
    m_label->setVerticallyCentered(true);
}

const std::string& ExpressionField::text() const
{
    return m_label->text();
}

void ExpressionField::setText(const std::string& text)
{
    m_label->setText(text);
    requestLayout();
}

void ExpressionField::layout()
{
    // The label ends at the right margin and grows past the left one, its
    // text right-aligned, so the end of the text never moves.
    const double margin = Theme::current().largeSpacing;
    const double available = std::max(0.0, geometry().width - 2 * margin);
    const double width = std::max(available, m_label->sizeHint().width);
    m_clip->setGeometry({margin, 0, available, geometry().height});
    m_label->setGeometry({available - width, 0, width, geometry().height});
}
