#include "MainExceptionHandlers.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <typeinfo>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#endif

namespace
{
    // MYT-251: route std::terminate through our printer. The previous
    // attempt at the inlining real-fix exited with `0xE06D7363` (MSVC
    // C++ exception SEH magic) and zero output — neither the SEH filter
    // below nor the OSR catch handlers in OSRManager::executeOSRLoop
    // saw it because the C++ unwind path runs `std::terminate()` before
    // the OS surfaces the SEH event. This handler prints the typed
    // exception name + message before aborting so future failures
    // surface a usable diagnostic.
    [[noreturn]] void mtype_terminate_handler() noexcept
    {
        std::cerr.flush();
        std::cerr << "\nFATAL: std::terminate invoked"
#ifdef _WIN32
                  << " (likely C++ exception escape — exit code 0xE06D7363 family)"
#endif
                  << ".\n";
        if (auto eptr = std::current_exception())
        {
            try {
                std::rethrow_exception(eptr);
            }
            catch (const std::exception& e) {
                std::cerr << "  active exception: " << typeid(e).name()
                          << ": " << e.what() << "\n";
            }
            catch (...) {
                std::cerr << "  active exception: <non-std type, unknown>\n";
            }
        }
        else
        {
            std::cerr << "  no active exception object (called directly).\n";
        }
        std::cerr.flush();
        std::abort();
    }
}

#ifdef _WIN32
// MYT-248/249/250: SEH top-level filter so JIT-emitted code that triggers a
// structured exception (access violation from a stale IC pointer, JIT helper
// returning bad memory, etc.) prints a diagnostic instead of silently
// terminating. With MSVC's default /EHsc, catch(...) does NOT catch SEH —
// without this filter the process disappears with no output.
namespace
{
    LONG WINAPI mtype_seh_filter(EXCEPTION_POINTERS* ep)
    {
        std::cerr.flush();
        std::cerr << "\nFATAL: structured exception 0x"
                  << std::hex
                  << (ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionCode : 0u)
                  << " at addr 0x"
                  << static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(
                       ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionAddress : nullptr))
                  << std::dec
                  << " — likely JIT-emitted code dereferenced freed/invalid memory.\n";
        std::cerr.flush();
        return EXCEPTION_CONTINUE_SEARCH;  // Let the OS terminate normally
                                           // so post-mortem debuggers still attach.
    }

    // MYT-251: vectored exception handler. Fires BEFORE the normal SEH
    // dispatcher walk, which means it sees C++ exceptions (0xE06D7363)
    // even when the unwinder cannot traverse asmjit JIT-emitted frames
    // (no PE x64 unwind data registered for the JIT region) — the path
    // that bypasses both `set_terminate` and `SetUnhandledExceptionFilter`
    // and produces a silent exit on stream_pipeline_hot / reflection_lookup_hot.
    // Returns EXCEPTION_CONTINUE_SEARCH so normal handling still runs (this
    // is a diagnostic, not a handler). Gated by MTYPE_TRACE_VEH=1 so the
    // VEH does not interfere with normal program operation when not
    // diagnosing a silent crash.
    LONG WINAPI mtype_veh_diagnostic(EXCEPTION_POINTERS* ep)
    {
        if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
        const DWORD code = ep->ExceptionRecord->ExceptionCode;

        // Filter out the noise: ignore everything except first-chance
        // events that historically silently terminate. 0xE06D7363 is MSVC
        // C++ throw raise; 0xC0000005 is access violation; 0xC0000409 is
        // /GS cookie failure (__fastfail); 0xC0000374 is heap corruption.
        // Page faults during normal operation (0x80000003 single-step etc.)
        // are filtered out.
        const bool isInteresting =
            code == 0xE06D7363u || code == 0xC0000005u ||
            code == 0xC0000409u || code == 0xC0000374u ||
            code == 0xC00000FDu;  // stack overflow
        if (!isInteresting) return EXCEPTION_CONTINUE_SEARCH;

        std::cerr.flush();
        std::cerr << "[VEH] first-chance exception code=0x"
                  << std::hex << code
                  << " at addr=0x"
                  << static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(
                       ep->ExceptionRecord->ExceptionAddress))
                  << std::dec;
        if (code == 0xE06D7363u && ep->ExceptionRecord->NumberParameters >= 3)
        {
            // Parameter[2] is a pointer to the ThrowInfo descriptor;
            // dereference cautiously inside __try / SEH not C++ try.
            std::cerr << " (C++ throw, ThrowInfo=0x"
                      << std::hex
                      << static_cast<std::uint64_t>(
                             ep->ExceptionRecord->ExceptionInformation[2])
                      << std::dec << ")";
        }
        std::cerr << "\n";
        std::cerr.flush();
        return EXCEPTION_CONTINUE_SEARCH;
    }
}
#endif

namespace runMain
{
    void installCrashHandlers()
    {
        // MYT-251: install the std::terminate handler before any user code runs
        // (cross-platform, complementary to the Windows SEH filter below). The
        // previous inlining attempt exited silently with 0xE06D7363; this is
        // the diagnostic that catches that path next time.
        std::set_terminate(mtype_terminate_handler);

#ifdef _WIN32
        // MYT-248/249/250: install before any JIT compilation runs so silent SEH
        // failures inside the JIT or its native helpers (the symptom of these
        // three bugs) at least print a diagnostic line.
        SetUnhandledExceptionFilter(mtype_seh_filter);

        // MYT-251: optional vectored exception handler — set MTYPE_TRACE_VEH=1
        // to install. Fires on first-chance exceptions before the normal
        // dispatch walk, so it sees throws even when the C++ unwind cannot
        // traverse asmjit JIT regions (no PE x64 unwind info registered).
        // This is the path that produces silent 0xE06D7363 exits on the
        // stream_pipeline_hot / reflection_lookup_hot benchmarks.
        if (const char* v = std::getenv("MTYPE_TRACE_VEH");
            v && v[0] == '1' && v[1] == '\0')
        {
            AddVectoredExceptionHandler(/*first=*/1, mtype_veh_diagnostic);
            std::cerr << "[VEH] installed vectored exception diagnostic handler\n";
            std::cerr.flush();
        }
#endif
    }
}
