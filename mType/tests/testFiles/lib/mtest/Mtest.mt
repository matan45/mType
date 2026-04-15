// mtest — pure-mType unit-testing library (MYT-59).
//
// Canonical entry point: users import this single file.
//
//   import * from "<relative>/lib/mtest/Mtest.mt";
//
//   class MyTest extends TestSuite {
//       @Test
//       public function testSomething(): void {
//           assertEqual(1 + 1, 2);
//       }
//   }
//
//   TestRunner r = new TestRunner();
//   r.addClass("MyTest");
//   r.run();
//
// v1 known limitations (track in follow-ups):
//   * @Test(expected=...) does NOT work — exceptions thrown from a
//     reflectively-invoked test body are swallowed by the VM's state
//     restore in Method.invoke (the handler's call-stack unwind is
//     overwritten when invokeMethod restores its savedCallStack). Use
//     assertThrows(name, () -> { ... }) instead; the lambda body runs
//     inside the same reflective frame, so its try/catch works.
//   * @Timeout is parsed but not enforced — no timing native yet.
//   * assertThrows matches by parsing the thrown exception's toString()
//     prefix (`ClassName: msg` convention used by lib/exceptions/).
//     Subclass matching is not supported; pass "Exception" for a
//     permissive any-exception match.
//   * Tests run sequentially; no parallelism. Order follows reflection
//     order, not source order.
//   * runAndExit() returns 0/1 but does NOT terminate — mType has no
//     exit() native. Output includes a FAILED/ALL-PASSED summary line.
//   * Test discovery is explicit via addClass(). No auto-scan.
//   * @BeforeAll/@AfterAll run on a dedicated bootstrap instance (not a
//     true static context) because Method.invoke requires a non-null
//     receiver. Treat them as "shared-instance" hooks in v1.

import * from "../annotations/Retention.mt";
import * from "../annotations/Targets.mt";
import * from "../collections/ArrayList.mt";
import * from "../exceptions/Exception.mt";
import * from "../primitives/String.mt";
import * from "../reflect/Class.mt";
import * from "../reflect/Method.mt";
import * from "../reflect/Annotation.mt";
import * from "../reflect/Constructor.mt";

import * from "./annotations/Test.mt";
import * from "./annotations/Lifecycle.mt";
import * from "./TestSuite.mt";
import * from "./ThrowingRunnable.mt";
import * from "./AssertionFailedException.mt";
import * from "./TestResult.mt";
import * from "./TestSuiteResult.mt";
import * from "./Assertions.mt";
import * from "./TestRunner.mt";
