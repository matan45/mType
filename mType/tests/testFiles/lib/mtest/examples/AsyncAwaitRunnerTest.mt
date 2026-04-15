// Regression suite for MYT-113: exercises the TestRunner's async await path.
//
// testAsyncPasses: a resolving async @Test is only marked PASSED after its
// promise settles (post-await assertion observed).
//
// testAsyncThrowsReachesRunner: an Exception thrown AFTER an await is
// propagated through the rejected promise and caught by the runner,
// matching @Test(expected=...). Pre-fix the runner ignored the returned
// Promise, the throw happened only on the detached task, and the runner
// recorded FAIL "nothing was thrown".
import * from "../Mtest.mt";
import * from "../../primitives/Int.mt";

public class AsyncAwaitRunnerTest extends TestSuite {
    public constructor() : super() { }

    private function async _resolveOne(): Promise<Int> {
        return new Int(1);
    }

    @Test
    public function async testAsyncPasses(): Promise<void> {
        Int v = await this._resolveOne();
        assertEqual(v.getValue(), 1);
    }

    @Test(expected = "Exception")
    public function async testAsyncThrowsReachesRunner(): Promise<void> {
        await this._resolveOne();
        throw new Exception("post-await throw");
    }
}
