#pragma once
#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class AnnotationTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/annotation/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/annotation/error/";
    public:
        explicit AnnotationTestSuite() : TestSuite("Annotation Test Suite") {}
        void setupTests() override;
    };
}
