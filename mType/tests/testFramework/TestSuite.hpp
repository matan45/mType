#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "TestRunner.hpp"
#include "TestCase.hpp"
#include "TestTypeEnum.hpp"
#include "../../constants/ExecutionMode.hpp"

namespace tests::testFramework
{
    class TestSuite
    {
    protected:
        std::string suiteName;
        std::unique_ptr<TestRunner> runner;
        std::vector<TestCase> testCases;
    
    public:
        explicit TestSuite(const std::string& name);
        virtual ~TestSuite() = default;

        virtual void setupTests() = 0;

        void run();
        void generateReport();

        const std::string& getName() const { return suiteName; }

        void addTestFromFile(const std::string& name, const std::string& filePath, TestType type = TestType::NORMAL);
        // MYT-38 — overload that pins the substring required to appear in
        // the thrown exception's message for an ERROR_EXPECTED test to pass.
        void addTestFromFile(const std::string& name,
                             const std::string& filePath,
                             TestType type,
                             const std::string& expectedErrorSubstring);
        void addOutputVerificationTest(const std::string& name, const std::string& filePath);
        void addInteropTest(const std::string& name, const std::string& filePath);

        // MYT-42 — register a C++-driven test that exercises ScriptAPI
        // directly. `bootstrapFilePath` is an optional mt file executed
        // before the callback runs (so the callback can reference
        // classes / globals it declares). Any uncaught exception thrown
        // from the callback fails the test. Pass an empty string for
        // callbacks that don't need any mt-side setup.
        void addCallbackTest(const std::string& name,
                             const std::string& bootstrapFilePath,
                             NativeCallback callback);

        void setExecutionModeForAll(constants::ExecutionMode mode);

    private:
        void generateHtmlReport();
    
    }; 
}

