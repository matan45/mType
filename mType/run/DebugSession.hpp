#pragma once

#include "../constants/ExecutionMode.hpp"
#include <string>

void runInDebugMode(const std::string& filename, constants::ExecutionMode execMode);

// Listen on a TCP port for a single debug client (attach transport). Mirrors
// runInDebugMode but drives the DebugServer over a socket instead of stdin/stdout,
// so a VS Code client can attach to this host. The host keeps running if the
// client detaches.
void runInDebugModePort(const std::string& filename, constants::ExecutionMode execMode, int port);
