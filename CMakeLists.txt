# SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(cutefish-calculator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CTest)

find_package(CutefishToolkit REQUIRED)

add_library(calculator-engine STATIC
    src/calculator.cpp
    src/decimal.cpp
    src/evaluator.cpp
)
target_include_directories(calculator-engine PUBLIC src)

# The glyphs, built in.
file(GLOB CALCULATOR_ICONS CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/icons/*.svg)
set(CALCULATOR_EMBEDDED ${CMAKE_CURRENT_BINARY_DIR}/icons.cpp)
# A list would be split into separate arguments on the way to the script.
string(REPLACE ";" "|" CALCULATOR_ICON_LIST "${CALCULATOR_ICONS}")
add_custom_command(OUTPUT ${CALCULATOR_EMBEDDED}
    COMMAND ${CMAKE_COMMAND} -DOUTPUT=${CALCULATOR_EMBEDDED}
            -DFILES=${CALCULATOR_ICON_LIST} -P ${PROJECT_SOURCE_DIR}/cmake/EmbedIcons.cmake
    DEPENDS ${CALCULATOR_ICONS} ${PROJECT_SOURCE_DIR}/cmake/EmbedIcons.cmake
    VERBATIM)

add_library(calculator-ui STATIC
    src/ui/calculatorwindow.cpp
    src/ui/colors.cpp
    src/ui/expressionfield.cpp
    src/ui/historyview.cpp
    src/ui/keypad.cpp
    ${CALCULATOR_EMBEDDED}
)
target_include_directories(calculator-ui PUBLIC src src/ui)
target_link_libraries(calculator-ui PUBLIC calculator-engine Cutefish::Toolkit)

add_executable(${PROJECT_NAME} src/main.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE CALCULATOR_VERSION="${PROJECT_VERSION}")
target_link_libraries(${PROJECT_NAME} PRIVATE calculator-ui)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

cutefish_add_translations(cutefish-calculator SOURCES src DESKTOP_FILES cutefish-calculator.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cutefish-calculator.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
