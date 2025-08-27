#pragma once
#include <string>
#include <vector>
#include <memory>
#include "TestRunner.hpp"

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
    
        void addTestFromFile(const std::string& name, const std::string& filePath, TestType type = TestType::NORMAL);
    
    }; 
}

