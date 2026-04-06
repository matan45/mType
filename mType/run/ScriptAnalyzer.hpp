#pragma once

#include "../environment/Environment.hpp"
#include "../constants/ExecutionMode.hpp"
#include <memory>
#include <string>

void demonstrateScriptObjectUsage(const std::string& scriptPath, constants::ExecutionMode execMode);
void printScriptAnnotatedClasses(std::shared_ptr<environment::Environment> env);
