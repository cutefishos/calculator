// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "icons.h"

#include "ui/application.h"
#include "ui/icon.h"

#include <gtest/gtest.h>

#include <string>

using namespace cutefish::ui;

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    Application application(argc, argv, Application::Platform::Headless);
    for (const auto& [name, svg] : embeddedIcons())
        Icon::registerGlyph(std::string(name), std::string(svg));
    return RUN_ALL_TESTS();
}
