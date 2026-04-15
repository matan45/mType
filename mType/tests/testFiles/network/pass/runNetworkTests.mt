// Top-level runner for the network library mtest suites. Run with:
//   mType C:\matan\mType\mType\tests\testFiles\network\pass\runNetworkTests.mt
//
// Note: mtest v1 invokes test methods via Method.invoke, which returns the
// Promise of an async test without awaiting it — so async network I/O can't
// be exercised through mtest yet. The TCP loopback integration lives in
// tcpEchoIntegration.mt and is run as a standalone async script.
import * from "./HttpResponseTest.mt";
import * from "./NetworkExceptionsTest.mt";
import * from "./JsonApiTest.mt";

TestRunner runner = new TestRunner();
runner.addClass("HttpResponseTest");
runner.addClass("NetworkExceptionsTest");
runner.addClass("JsonApiTest");
runner.run();
