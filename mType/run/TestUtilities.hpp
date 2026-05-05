#pragma once

#include "../tests/testFramework/TestSuite.hpp"
#include "../constants/ExecutionMode.hpp"
#include <memory>
#include <string>

std::unique_ptr<tests::testFramework::TestSuite> createTestSuite(const std::string& suiteName);
void printAvailableTestSuites();
// MYT-259: `jitEnabled` lets the caller drive both a JIT-on (default) and
// `--no-jit` pass over the same test suites. Defaults preserve the
// pre-existing JIT-on behavior for any caller that didn't pass it.
// Returns the number of failed + errored tests so callers can use it as a
// process exit code (0 = all green).
int runSpecificTestSuite(const std::string& suiteName,
                         constants::ExecutionMode execMode,
                         bool jitEnabled = true);
int runAllTests(constants::ExecutionMode execMode,
                bool jitEnabled = true);
