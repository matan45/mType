// MYT-38: Lock in that stack traces include every call frame and that frames
// are emitted innermost-first. Existing stack-trace tests only check trace != "";
// this test pins the actual frame ordering so future format changes intentionally
// break it.
import * from "../../lib/exceptions/Exception.mt";

function levelD(): void {
    throw new Exception("frameOrderTest");
}

function levelC(): void {
    levelD();
}

function levelB(): void {
    levelC();
}

function levelA(): void {
    levelB();
}

function main(): void {
    try {
        levelA();
    } catch (Exception e) {
        string trace = e.getStackTrace();

        int idxA = indexOf(trace, "levelA");
        int idxB = indexOf(trace, "levelB");
        int idxC = indexOf(trace, "levelC");
        int idxD = indexOf(trace, "levelD");

        if (idxA != -1) { print("contains levelA: true"); } else { print("contains levelA: false"); }
        if (idxB != -1) { print("contains levelB: true"); } else { print("contains levelB: false"); }
        if (idxC != -1) { print("contains levelC: true"); } else { print("contains levelC: false"); }
        if (idxD != -1) { print("contains levelD: true"); } else { print("contains levelD: false"); }

        // Innermost (levelD where throw originates) must appear before its callers.
        if (idxD < idxC) { print("D before C: true"); } else { print("D before C: false"); }
        if (idxC < idxB) { print("C before B: true"); } else { print("C before B: false"); }
        if (idxB < idxA) { print("B before A: true"); } else { print("B before A: false"); }
    }
}

main();
