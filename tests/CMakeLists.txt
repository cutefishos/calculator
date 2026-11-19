# SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

find_package(GTest REQUIRED)
include(GoogleTest)

# The model alone: no toolkit, so no window can come in.
add_executable(calculator-logic-tests
    logic/main.cpp
    logic/calculator_test.cpp
    logic/decimal_test.cpp
    logic/evaluator_test.cpp
)
target_link_libraries(calculator-logic-tests PRIVATE calculator-engine GTest::gtest)
gtest_discover_tests(calculator-logic-tests PROPERTIES LABELS logic)

add_executable(calculator-scenario-tests
    scenarios/main.cpp
    scenarios/keyboard_test.cpp
    scenarios/keypad_test.cpp
    scenarios/menu_test.cpp
)
target_link_libraries(calculator-scenario-tests PRIVATE calculator-ui GTest::gtest)
# The application tests group every running copy of these binaries.
gtest_discover_tests(calculator-scenario-tests PROPERTIES LABELS scenario RESOURCE_LOCK calculator-ui)

add_executable(calculator-layout-tests layout/layout_test.cpp)
target_link_libraries(calculator-layout-tests PRIVATE calculator-ui GTest::gtest)
gtest_discover_tests(calculator-layout-tests PROPERTIES LABELS layout RESOURCE_LOCK calculator-ui)

add_executable(calculator-capture layout/capture.cpp)
target_link_libraries(calculator-capture PRIVATE calculator-ui)
