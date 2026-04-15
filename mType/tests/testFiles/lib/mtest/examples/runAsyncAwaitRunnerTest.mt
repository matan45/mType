// Top-level driver for the async await regression suite. Run with:
//   mType C:\matan\mType\mType\tests\testFiles\lib\mtest\examples\runAsyncAwaitRunnerTest.mt
import * from "./AsyncAwaitRunnerTest.mt";

TestRunner runner = new TestRunner();
runner.addClass("AsyncAwaitRunnerTest");
await runner.run();
