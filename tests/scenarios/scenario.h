// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "calculatorwindow.h"

#include "ui/test_driver.h"

#include <gtest/gtest.h>

#include <string>

// The window at its size, shown, and worked only through the driver.
class Scenario
{
    CalculatorWindow m_window;

public:
    explicit Scenario(double scale = 1)
    {
        m_window.window().setScale(scale);
        m_window.window().resize(CalculatorWindow::size());
        m_window.window().show();
        driver.layout();
    }

    cutefish::ui::TestDriver driver{m_window.window(), [](const std::string& message) { ADD_FAILURE() << message; }};
};
