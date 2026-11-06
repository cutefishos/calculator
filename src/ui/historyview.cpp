// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "historyview.h"

#include "ui/label.h"
#include "ui/theme.h"

#include <algorithm>

using namespace cutefish::ui;

// The rows: each its text with the large spacing above and below it;
// together at the bottom, next to the expression.
class HistoryRows : public View
{
public:
    void setEntries(const std::vector<std::string>& entries)
    {
        while (m_labels.size() > entries.size()) {
            takeChild(m_labels.back());
            m_labels.pop_back();
        }
        for (size_t i = 0; i < entries.size(); ++i) {
            if (i < m_labels.size()) {
                m_labels[i]->setText(entries[i]);
                continue;
            }
            Label* label = addChild<Label>(entries[i]);
            label->setName("history entry");
            label->setTextAlignment(Label::TextAlignment::Right);
            label->setElide(Label::Elide::Middle);
            label->setColor(ColorId::DisabledText);
            m_labels.push_back(label);
        }
        requestLayout();
    }

    size_t count() const { return m_labels.size(); }

    double heightForWidth(double) const override
    {
        double height = 0;
        for (const Label* label : m_labels)
            height += rowHeight(label);
        return height;
    }

protected:
    void layout() override
    {
        const double padding = Theme::current().largeSpacing;
        const double width = std::max(0.0, geometry().width - 2 * padding);
        double y = std::max(0.0, geometry().height - heightForWidth(geometry().width));
        for (Label* label : m_labels) {
            label->setGeometry({padding, y + padding, width, label->sizeHint().height});
            y += rowHeight(label);
        }
    }

private:
    static double rowHeight(const Label* label) { return label->sizeHint().height + 2 * Theme::current().largeSpacing; }

    friend class HistoryView;

    std::vector<Label*> m_labels;
};

HistoryView::HistoryView()
    : m_rows(setContent<HistoryRows>())
{
    setName("history");
    m_rows->setLayoutHints({.fillHeight = true});
}

double HistoryView::rowHeight()
{
    const Label label("0");
    return HistoryRows::rowHeight(&label);
}

void HistoryView::setEntries(const std::vector<std::string>& entries)
{
    m_scrollToEnd = entries.size() > m_rows->count();
    m_rows->setEntries(entries);
    requestLayout();
}

size_t HistoryView::count() const
{
    return m_rows->count();
}

void HistoryView::layout()
{
    ScrollView::layout();
    if (m_scrollToEnd) {
        m_scrollToEnd = false;
        setScrollPosition({0, contentSize().height - geometry().height});
    }
}
