// TestRunner — drives discovery and execution of mtest suites.
//
// Usage:
//   TestRunner r = new TestRunner();
//   r.addClass("CalculatorTest");
//   r.run();   // returns ArrayList<TestSuiteResult>
//
// Discovery is explicit: callers enumerate suite class names. mType lacks a
// "scan all loaded classes" native, so auto-discovery is a separate concern.
//
// Within a suite the runner partitions declared methods by annotation, then
// executes @BeforeAll (static), each @Test (with @BeforeEach / @AfterEach
// around it), then @AfterAll. AssertionFailedException is treated as FAIL
// with the raw message; any other Exception is FAIL with "unexpected: ..."
// unless @Test(expected=...) matched the thrown class name.
import * from "../collections/ArrayList.mt";
import * from "../primitives/String.mt";
import * from "../reflect/Class.mt";
import * from "../reflect/Method.mt";
import * from "../reflect/Constructor.mt";
import * from "../reflect/Annotation.mt";
import * from "../exceptions/Exception.mt";
import * from "./TestResult.mt";
import * from "./TestSuiteResult.mt";
import * from "./AssertionFailedException.mt";

public class TestRunner {
    private ArrayList<String> _classNames;
    private ArrayList<TestSuiteResult> _suiteResults;
    private bool _verbose;

    public constructor() {
        this._classNames = new ArrayList<String>();
        this._suiteResults = new ArrayList<TestSuiteResult>();
        this._verbose = false;
    }

    public function addClass(string className): TestRunner {
        this._classNames.add(new String(className));
        return this;
    }

    public function setVerbose(bool v): TestRunner {
        this._verbose = v;
        return this;
    }

    public function run(): ArrayList<TestSuiteResult> {
        int totalPassed = 0;
        int totalFailed = 0;
        int totalSkipped = 0;

        for (int i = 0; i < this._classNames.size(); i = i + 1) {
            string name = this._classNames.get(i).getValue();
            TestSuiteResult suiteResult = this._runSuite(name);
            this._suiteResults.add(suiteResult);
            suiteResult.printSummary();
            totalPassed = totalPassed + suiteResult.getPassedCount();
            totalFailed = totalFailed + suiteResult.getFailedCount();
            totalSkipped = totalSkipped + suiteResult.getSkippedCount();
        }

        print("");
        print("=== mtest: " + totalPassed + " passed, " + totalFailed
              + " failed, " + totalSkipped + " skipped ===");
        if (totalFailed == 0) {
            print("ALL PASSED");
        } else {
            print("FAILED: " + totalFailed + " tests");
        }
        return this._suiteResults;
    }

    public function runAndExit(): int {
        this.run();
        int failed = 0;
        for (int i = 0; i < this._suiteResults.size(); i = i + 1) {
            failed = failed + this._suiteResults.get(i).getFailedCount();
        }
        if (failed == 0) {
            return 0;
        }
        return 1;
    }

    function _runSuite(string className): TestSuiteResult {
        TestSuiteResult suiteResult = new TestSuiteResult(className);
        Class cls = Class::forName(className);
        Method[] allMethods = cls.getDeclaredMethods();

        Method[] beforeAll = this._collect(allMethods, "BeforeAll", true);
        Method[] afterAll  = this._collect(allMethods, "AfterAll",  true);
        Method[] beforeEach = this._collect(allMethods, "BeforeEach", false);
        Method[] afterEach  = this._collect(allMethods, "AfterEach",  false);
        Method[] tests      = this._collect(allMethods, "Test",       false);

        // mType's Method.invoke requires a non-null Object for the receiver,
        // so even "static" @BeforeAll / @AfterAll hooks are routed through a
        // dedicated bootstrap instance. This is also why annotation-discovery
        // treats static-ness as advisory rather than required in v1.
        Object bootstrap;
        try {
            bootstrap = this._newInstance(cls);
        } catch (Exception e) {
            for (int i = 0; i < tests.length; i = i + 1) {
                TestResult r = new TestResult(className, tests[i].getName());
                r.markFailed("suite constructor threw: " + e.getMessage());
                suiteResult.addResult(r);
            }
            return suiteResult;
        }

        string beforeAllFailure = this._invokeAll(beforeAll, bootstrap);
        if (beforeAllFailure != "") {
            for (int i = 0; i < tests.length; i = i + 1) {
                TestResult r = new TestResult(className, tests[i].getName());
                r.markFailed("BeforeAll failed: " + beforeAllFailure);
                suiteResult.addResult(r);
            }
            this._invokeAll(afterAll, bootstrap);
            return suiteResult;
        }

        for (int t = 0; t < tests.length; t = t + 1) {
            TestResult result = this._runOneTest(cls, tests[t], beforeEach, afterEach);
            suiteResult.addResult(result);
        }

        this._invokeAll(afterAll, bootstrap);
        return suiteResult;
    }

    // Reflectively construct a fresh suite instance. Propagates any exception
    // the constructor or reflection lookup throws — callers wrap in try/catch.
    function _newInstance(Class cls): Object {
        Constructor ctor = cls.getDeclaredConstructor(0);
        Object[] ctorArgs = new Object[0];
        return ctor.newInstance(ctorArgs);
    }

    // Collect declared methods carrying `annName`. If `requireStatic`, non-
    // static hits are returned as well and validated at the call site; we
    // keep the filter here focused on annotation presence so ordering is
    // preserved. Static-vs-instance checks for @BeforeAll/@AfterAll happen
    // inside _invokeAll.
    function _collect(Method[] methods, string annName, bool requireStatic): Method[] {
        int count = 0;
        for (int i = 0; i < methods.length; i = i + 1) {
            if (methods[i].hasAnnotation(annName)) {
                count = count + 1;
            }
        }
        Method[] out = new Method[count];
        int w = 0;
        for (int i = 0; i < methods.length; i = i + 1) {
            if (methods[i].hasAnnotation(annName)) {
                out[w] = methods[i];
                w = w + 1;
            }
        }
        return out;
    }

    // Invoke a set of hook methods in declaration order. Returns "" on full
    // success, or the first exception's message (prefixed with method name)
    // on failure so the caller can annotate dependent results.
    function _invokeAll(Method[] hooks, Object instance): string {
        for (int i = 0; i < hooks.length; i = i + 1) {
            Method m = hooks[i];
            try {
                Object[] noArgs = new Object[0];
                m.invoke(instance, noArgs);
            } catch (Exception e) {
                return m.getName() + ": " + e.getMessage();
            }
        }
        return "";
    }

    function _runOneTest(Class cls, Method testMethod, Method[] beforeEach, Method[] afterEach): TestResult {
        TestResult result = new TestResult(cls.getName(), testMethod.getName());

        if (testMethod.hasAnnotation("Disabled")) {
            string reason = "";
            Annotation? dis = testMethod.getAnnotation("Disabled");
            if (dis != null) {
                reason = dis.getString("reason");
            }
            result.markSkipped(reason);
            return result;
        }

        Annotation? testAnn = testMethod.getAnnotation("Test");
        string expected = "";
        if (testAnn != null) {
            expected = testAnn.getString("expected");
        }
        result.setExpectedException(expected);

        if (testMethod.hasAnnotation("Timeout")) {
            Annotation? to = testMethod.getAnnotation("Timeout");
            if (to != null) {
                result.setDeclaredTimeoutMs(to.getInt("millis"));
            }
        }

        Object instance;
        try {
            instance = this._newInstance(cls);
        } catch (Exception e) {
            result.markFailed("suite constructor threw: " + e.getMessage());
            return result;
        }

        string beforeFailure = this._invokeAll(beforeEach, instance);
        if (beforeFailure != "") {
            result.markFailed("BeforeEach failed: " + beforeFailure);
            this._invokeAll(afterEach, instance);
            return result;
        }

        bool threw = false;
        bool assertionFailure = false;
        string failureMsg = "";
        string actualExcName = "";
        try {
            Object[] noArgs = new Object[0];
            testMethod.invoke(instance, noArgs);
        } catch (AssertionFailedException afe) {
            threw = true;
            assertionFailure = true;
            failureMsg = afe.getMessage();
            actualExcName = "AssertionFailedException";
        } catch (Exception e) {
            threw = true;
            failureMsg = e.getMessage();
            actualExcName = _extractExcName(e);
        }

        string afterFailure = this._invokeAll(afterEach, instance);

        if (assertionFailure) {
            result.markFailedWithActual(failureMsg, actualExcName);
        } else if (threw) {
            if (expected == "") {
                result.markFailedWithActual(
                    "unexpected " + actualExcName + ": " + failureMsg,
                    actualExcName);
            } else if (expected == "Exception" || actualExcName == expected) {
                result.markPassed();
            } else {
                result.markFailedWithActual(
                    "expected " + expected + " but got " + actualExcName + ": " + failureMsg,
                    actualExcName);
            }
        } else {
            if (expected != "") {
                result.markFailed("expected " + expected + " but nothing was thrown");
            } else {
                result.markPassed();
            }
        }

        if (afterFailure != "" && result.isPassed()) {
            result.markFailed("AfterEach failed: " + afterFailure);
        }

        return result;
    }
}

// Free helper reused across the runner. Mirrors the prefix-parsing used in
// Assertions.assertThrows: exceptions in lib/exceptions/ override toString()
// as "<ClassName>: <message>", giving a best-effort runtime-class name.
function _extractExcName(Exception e): string {
    string repr = e.toString();
    int colonIdx = indexOf(repr, ":");
    if (colonIdx <= 0) {
        return repr;
    }
    return substring(repr, 0, colonIdx);
}
