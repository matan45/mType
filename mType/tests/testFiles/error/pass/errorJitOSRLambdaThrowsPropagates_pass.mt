// MYT-254 regression: a lambda invoked inside an OSR-compiled hot loop
// throws on a sentinel iteration. The throw must propagate to the
// surrounding try/catch instead of being silently swallowed by the JIT
// helper's pendingException early-return — without the back-edge
// pendingException check in emitOSRCodegenLoop + the OSRManager rethrow,
// every CALL_METHOD after the first throw becomes a no-op and the loop
// spins forever. The iteration count is sized to comfortably exceed the
// OSR threshold so the loop body is JIT-compiled before the sentinel
// hits.
import * from "../../lib/exceptions/Exception.mt";

interface IntFunction { function apply(int x): int; }

function main(): void {
    IntFunction bomb = x -> {
        if (x == 5000) {
            throw new Exception("bomb at 5000");
        }
        return x + 1;
    };

    int caught = 0;
    int lastSeen = -1;
    int N = 50000;
    try {
        for (int i = 0; i < N; i = i + 1) {
            lastSeen = bomb.apply(i);
        }
    } catch (Exception e) {
        caught = 1;
    }

    print("propagated=" + caught + " lastSeen=" + lastSeen);
}

main();
