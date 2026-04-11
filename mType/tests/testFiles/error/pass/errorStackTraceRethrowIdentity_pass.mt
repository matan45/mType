// MYT-38: Pin the rethrow stack-trace behavior.
//
// Spec (per MYT-38 AC): rethrow (`throw e;` inside catch) should preserve the
// original stack trace so that the trace tells you where the exception was first
// thrown — not where it was rethrown.
//
// Current implementation: ExceptionExecutor::handleThrow unconditionally
// regenerates the trace from the current call stack on every THROW
// (ExceptionExecutor.cpp:99-135). Rethrow therefore *replaces* the original
// trace with a shorter one captured from the rethrow site.
//
// This test pins the CURRENT behavior (not the spec). When the discrepancy is
// fixed in a follow-up, this test will fail and force a deliberate update of
// the .expected file — at which point the spec and the implementation will be
// in agreement.
import * from "../../lib/exceptions/Exception.mt";

function deepThrow(): void {
    throw new Exception("rethrowIdentity");
}

function main(): void {
    string capturedAtMiddle = "";
    string capturedAtTop = "";

    try {
        try {
            deepThrow();
        } catch (Exception e) {
            capturedAtMiddle = e.getStackTrace();
            throw e;
        }
    } catch (Exception e) {
        capturedAtTop = e.getStackTrace();
    }

    if (capturedAtMiddle == capturedAtTop) {
        print("rethrow preserves trace: true");
    } else {
        print("rethrow preserves trace: false");
    }

    if (capturedAtMiddle != "") {
        print("middle trace non-empty: true");
    } else {
        print("middle trace non-empty: false");
    }

    if (capturedAtTop != "") {
        print("top trace non-empty: true");
    } else {
        print("top trace non-empty: false");
    }
}

main();
