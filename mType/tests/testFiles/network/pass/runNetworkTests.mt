// Top-level runner for the network library test suites. Run with:
//   mType C:\matan\mType\mType\tests\testFiles\network\pass\runNetworkTests.mt
import * from "./HttpResponseTest.mt";
import * from "./NetworkExceptionsTest.mt";
import * from "./JsonApiTest.mt";
import * from "./TcpEchoTest.mt";

TestRunner runner = new TestRunner();
runner.addClass("HttpResponseTest");
runner.addClass("NetworkExceptionsTest");
runner.addClass("JsonApiTest");
runner.addClass("TcpEchoTest");
runner.run();
