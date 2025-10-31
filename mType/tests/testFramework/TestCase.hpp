#pragma once
#include <string>
#include <chrono>
#include "TestTypeEnum.hpp"
#include "../../constants/ExecutionMode.hpp"

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

        // Helper methods
        bool verifyOutputAgainstExpected() const;

    public:
        TestCase(const std::string& testName, const std::string& testFilePath, TestType testType = TestType::NORMAL);

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
