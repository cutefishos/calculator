// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ui/scroll_view.h"

#include <string>
#include <vector>

class HistoryRows;

// The calculations so far, one a row, right-aligned and dimmed, kept
// scrolled to the latest.
class HistoryView : public cutefish::ui::ScrollView
{
public:
    // Rows the window keeps in sight above the expression.
    static constexpr int VisibleRows = 2;

    HistoryView();

    // A row's height: its text with the large spacing above and below.
    static double rowHeight();

    void setEntries(const std::vector<std::string>& entries);
    size_t count() const;

protected:
    void layout() override;

private:
    HistoryRows* m_rows;
    bool m_scrollToEnd = false;
};
