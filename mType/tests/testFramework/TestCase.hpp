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

        // NATIVE_CALLBACK state — bootstrap mt source to execute before
        // the callback runs, plus the callback itself.
        std::string bootstrapSource;
        NativeCallback nativeCallback;

        // Helper methods
        bool verifyOutputAgainstExpected() const;
        void executeNativeCallback();

    public:
        TestCase(const std::string& testName, const std::string& testFilePath, TestType testType = TestType::NORMAL);

        // NATIVE_CALLBACK constructor — no file path, runs a C++ callback
        // against a ScriptInterpreter pre-seeded with `bootstrapSource`.
        TestCase(const std::string& testName,
                 std::string bootstrapSource,
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
