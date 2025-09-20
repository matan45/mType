#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class SerializationTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/complie/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/complie/error/";

    public:
        explicit SerializationTestSuite() : TestSuite("Serialization Test Suite") {}
        void setupTests() override;
        void run(); // Override run to use ScriptInterpreter directly
    };
}