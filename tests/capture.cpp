// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

// Renders the window headless with a few calculations, with its layout:
//   calculator-capture DIR [WIDTH HEIGHT SCALE [dark]]
#include "calculatorwindow.h"
#include "icons.h"

#include "ui/application.h"
#include "ui/icon.h"
#include "ui/theme.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using namespace cutefish;

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s DIR [WIDTH HEIGHT SCALE [dark]]\n", argv[0]);
        return 1;
    }
    const std::string directory = argv[1];
    const double width = argc > 2 ? std::atof(argv[2]) : 280;
    const double height = argc > 3 ? std::atof(argv[3]) : 512;
    const double scale = argc > 4 ? std::atof(argv[4]) : 1.5;
    const bool dark = argc > 5 && std::string(argv[5]) == "dark";
    std::filesystem::create_directories(directory);

    ui::Application app(argc, argv, ui::Application::Platform::Headless);
    for (const auto& [name, svg] : embeddedIcons())
        ui::Icon::registerGlyph(std::string(name), std::string(svg));
    ui::Theme::setCurrent(dark ? ui::Theme::dark() : ui::Theme::light());
    CalculatorWindow window;
    window.window().setScale(scale);
    window.window().resize({width, height});
    window.window().show();

    const std::string name = directory + "/calculator" + (dark ? "-dark" : "");
    window.window().grab().savePng(name + "-empty.png");
    for (const char* key : {"1", "2", "×", "3", "=", "AC", "1", "÷", "3", "=", "-", "1", "0", "%", "="})
        window.press(key);
    for (const char* key : {"2", "+", "3"})
        window.press(key);
    window.window().grab().savePng(name + ".png");
    std::ofstream(name + ".txt") << window.window().layoutDump();
    return 0;
}
