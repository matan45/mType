#pragma once
#include <string>
#include <chrono>
#include <functional>
#include "TestTypeEnum.hpp"
#include "../../constants/ExecutionMode.hpp"

namespace services { class ScriptAPI; }

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

        // Helper methods
        bool verifyOutputAgainstExpected() const;
        void executeNativeCallback();
        void executeExeTest();
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

        void execute();

        // Getters
        const std::string& getName() const { return name; }
        const std::string& getFilePath() const { return filePath; }
        TestType getType() const { return type; }
        TestStatus getStatus() const { return status; }
        const std::string& getErrorMessage() const { return errorMessage; }
        std::chrono::milliseconds getExecutionTime() const { return executionTime; }
        const std::string& getOutput() const { return output; }
        
        // Status string conversion
        static std::string statusToString(TestStatus status);
        static std::string typeToString(TestType type);
    };
}
