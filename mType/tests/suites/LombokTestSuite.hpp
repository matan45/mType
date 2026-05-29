#pragma once
#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    // Exercises the Lombok-style synthesis annotations (@Data, @Getter, @Setter,
    // @ToString, @NoArgsConstructor, @AllArgsConstructor, @Builder) end-to-end
    // through the LombokSynthesisPass + downstream StructuralEqualitySynthesis.
    class LombokTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/lombok/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/lombok/error/";
    public:
        explicit LombokTestSuite() : TestSuite("Lombok Synthesis Test Suite") {}
        void setupTests() override;
    };
}
