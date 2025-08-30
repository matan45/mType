#pragma once

#include "../testFramework/TestSuite.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include "../../value/ValueType.hpp"

namespace tests::testSuite
{
    class NativeTest : public testFramework::TestSuite
    {
    private:
        std::unique_ptr<services::ScriptInterpreter> interpreter;
        
        // Native test methods
        void testCallInterpreterFunction();
        void testCallInterpreterMethod();
        void testCallInterpreterStaticMethod();
        void testReturnObjectFromNative();
        void testNativeFunctionRegistration();
        void testNativeClassIntegration();
        void testObjectCreationFromNative();
        void testStaticFieldAccess();
        void testQualifiedMethodCalls();
        void testNamespacedObjectCreation();
        
        // Helper methods for creating test values
        value::Value createTestObject();
        void setupTestEnvironment();
        void runNativeTests();
        
    public:
        explicit NativeTest() : TestSuite("Native C++ Integration Test Suite") {}
        void setupTests() override;
        void run();
    };
}