#pragma once
#include <string>
#include <chrono>
#include <functional>
#include "TestTypeEnum.hpp"
#include "../../constants/ExecutionMode.hpp"

namespace services { class ScriptAPI; class ScriptInterpreter; }

namespace tests::testFramework
{
    enum class TestStatus
    {
        NOT_RUN,
        PASSED,
        FAILED,
        ERROR,
        SKIPPED
    };

    /**
     * Callback signature for NATIVE_CALLBACK tests (MYT-42).
     *
     * The callback receives a ready-to-use ScriptAPI bound to a fresh
     * ScriptInterpreter. The `bootstrapSource` passed to the TestCase
     * is executed first (so the callback can reference classes and
     * variables it declares), then the callback runs and any uncaught
     * exception fails the test.
     */
    using NativeCallback = std::function<void(services::ScriptAPI&)>;

    /**
     * Interpreter-level callback variant. The callback receives the
     * test's single ScriptInterpreter directly, so it can drive paths
     * the ScriptAPI surface doesn't expose — e.g. resetForRebuild() +
     * re-parse to simulate the editor's Build Scripts cycle. Using the
     * one harness interpreter avoids the global-singleton contention a
     * second coexisting interpreter would cause.
     */
    using InterpreterCallback = std::function<void(services::ScriptInterpreter&)>;

    class TestCase
    {
    private:
        std::string name;
        std::string filePath;
        TestType type;
        TestStatus status;
        std::string errorMessage;
        std::chrono::milliseconds executionTime;
        std::string output;
        constants::ExecutionMode executionMode;
        // MYT-259: per-test JIT toggle so the suite runner can run the same
        // tests with both JIT on (default) and `--no-jit` to catch JIT vs
        // interpreter divergence in CI.
        bool jitEnabled = true;

        // MYT-38 — optional substring required to appear inside the thrown
        // exception's `what()` for an ERROR_EXPECTED test to pass. Empty
        // means "any throw is acceptable" (legacy behavior). When non-empty,
        // a thrown exception whose message does not contain this substring
        // demotes the test to FAILED with a "Expected error containing 'X'
        // but got 'Y'" message — so an ERROR_EXPECTED test can pin the
        // *reason* it's supposed to fail, not just "did fail somehow."
        std::string expectedErrorSubstring;

        // NATIVE_CALLBACK state — the callback itself. The bootstrap
        // is taken from `filePath` (a real committed .mt file in the
        // test tree), so relative imports resolve the same way they do
        // for normal tests.
        NativeCallback nativeCallback;

        // Interpreter-level callback (see InterpreterCallback). When set,
        // executeNativeCallback hands the callback the bootstrap interpreter
        // instead of building a ScriptAPI around it.
        InterpreterCallback interpreterCallback;

        // Helper methods
        bool verifyOutputAgainstExpected() const;
        void executeNativeCallback();
        // MYT-326 — useGui=true selects the windowed-subsystem
        // mtype-launcher-gui binary so the EXE_GUI_TEST path actually exercises
        // it. Output capture works the same: when a WindowedApp child is
        // launched from a console parent (the test harness), it inherits the
        // parent's stdio handles, so print() still flows back through _popen.
        void executeExeTest(bool useGui = false);
        // MYT-38 — after an ERROR_EXPECTED catch block has provisionally set
        // status=PASSED, demote to FAILED if expectedErrorSubstring is set
        // and the thrown exception's message does not contain it.
        void applyExpectedErrorFilter(const std::string& errWhat);

    public:
        TestCase(const std::string& testName, const std::string& testFilePath, TestType testType = TestType::NORMAL);

        // MYT-38 — overload that pins an expected substring of the thrown
        // error's message. Only meaningful for ERROR_EXPECTED tests.
        TestCase(const std::string& testName,
                 const std::string& testFilePath,
                 TestType testType,
                 const std::string& expectedErrorSubstr);

        // Skip-at-setup entry. execute() short-circuits with SKIPPED status
        // and the provided reason. Used when a feature is unavailable in the
        // current build so the suite reports the skip explicitly instead of
        // silently dropping the tests.
        static TestCase skipped(const std::string& testName, const std::string& reason);

        // NATIVE_CALLBACK constructor — runs `bootstrapFilePath` first
        // (so the callback can reference classes / globals it declares),
        // then invokes the C++ callback with a ScriptAPI bound to the
        // same interpreter. `bootstrapFilePath` may be empty if the
        // callback doesn't need any mt-side setup.
        TestCase(const std::string& testName,
                 const std::string& bootstrapFilePath,
                 NativeCallback callback);

        // Interpreter-level NATIVE_CALLBACK constructor — like the
        // ScriptAPI variant, but the callback drives the bootstrap
        // ScriptInterpreter directly.
        TestCase(const std::string& testName,
                 const std::string& bootstrapFilePath,
                 InterpreterCallback callback);

        void execute();

        // Getters
        const std::string& getName() const { return name; }
        const std::string& getFilePath() const { return filePath; }
        TestType getType() const { return type; }
        TestStatus getStatus() const { return status; }
        const std::string& getErrorMessage() const { return errorMessage; }
        std::chrono::milliseconds getExecutionTime() const { return executionTime; }
        const std::string& getOutput() const { return output; }
        
        // MYT-259: control whether this test runs with JIT enabled. Defaults
        // to true; flip to false via the suite's setJitEnabledForAll to drive
        // a `--no-jit --tests` pass.
        void setJitEnabled(bool enabled) { jitEnabled = enabled; }
        bool isJitEnabled() const { return jitEnabled; }

        // Status string conversion
        static std::string statusToString(TestStatus status);
        static std::string typeToString(TestType type);
    };
}
