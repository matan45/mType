// Top-level driver for the example suite. Run with:
//   mType C:\matan\mType\mType\tests\testFiles\lib\mtest\examples\runCalculatorTest.mt
import * from "./CalculatorTest.mt";

TestRunner runner = new TestRunner();
runner.addClass("CalculatorTest");
await runner.run();
