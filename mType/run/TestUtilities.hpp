#pragma once

#include "../tests/testFramework/TestSuite.hpp"
#include "../constants/ExecutionMode.hpp"
#include <memory>
#include <string>

std::unique_ptr<tests::testFramework::TestSuite> createTestSuite(const std::string& suiteName);
void printAvailableTestSuites();
void runSpecificTestSuite(const std::string& suiteName, constants::ExecutionMode execMode);
void runAllTests(constants::ExecutionMode execMode);
