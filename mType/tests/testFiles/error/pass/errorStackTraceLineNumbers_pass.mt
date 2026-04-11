// MYT-38: Lock in that stack-trace frames include source line numbers and the
// source filename. The throw is intentionally placed on a known line; the
// assertions search for ":<line>:" in the trace, so any edit to this file
// that shifts line 19 will break the test.
//
// DO NOT EDIT — line 19 is part of the assertion below.
import * from "../../lib/exceptions/Exception.mt";

function lineThrowingFunction(): void {
    // Padding so that the throw lands on line 19. Adding or removing any line
    // above the throw shifts it and breaks the assertion. The assertion looks
    // for ":19:" (line 19, followed by ":<column>") in the trace.
    //
    //
    //
    //
    //
    //
    throw new Exception("lineNumberTest");
}

function main(): void {
    try {
        lineThrowingFunction();
    } catch (Exception e) {
        string trace = e.getStackTrace();

        int idxLine = indexOf(trace, ":19:");
        int idxFile = indexOf(trace, "errorStackTraceLineNumbers_pass.mt");

        if (idxLine != -1) { print("contains line 19: true"); } else { print("contains line 19: false"); }
        if (idxFile != -1) { print("contains filename: true"); } else { print("contains filename: false"); }
    }
}

main();
