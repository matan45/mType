#pragma once

namespace runMain
{
    // Install std::terminate + (Windows) SEH/VEH diagnostic handlers.
    // Idempotent. Call once at the top of main() before any user code runs.
    void installCrashHandlers();
}
