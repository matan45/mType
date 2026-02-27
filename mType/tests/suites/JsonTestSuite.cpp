#include "JsonTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void JsonTestSuite::setupTests()
    {
        // === SERIALIZATION TESTS ===

        addOutputVerificationTest("Serialize Primitives",
                        passPath + "serializePrimitives.mt");
        addOutputVerificationTest("Serialize Object",
                        passPath + "serializeObject.mt");
        addOutputVerificationTest("Serialize Nested Objects",
                        passPath + "serializeNested.mt");
        addOutputVerificationTest("Serialize Arrays",
                        passPath + "serializeArray.mt");
        addOutputVerificationTest("Serialize Generics",
                        passPath + "serializeGenerics.mt");
        addOutputVerificationTest("Serialize Static Fields",
                        passPath + "serializeStatic.mt");

        // === DESERIALIZATION TESTS ===

        addOutputVerificationTest("Deserialize Object",
                        passPath + "deserializeObject.mt");
        addOutputVerificationTest("Deserialize Nested Objects",
                        passPath + "deserializeNested.mt");
        addOutputVerificationTest("Deserialize Arrays",
                        passPath + "deserializeArray.mt");

        // === ROUNDTRIP TESTS ===

        addOutputVerificationTest("Roundtrip",
                        passPath + "roundTrip.mt");

        // === ERROR TESTS ===

        addTestFromFile("Invalid JSON",
                        errorPath + "invalidJson.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Unknown Class",
                        errorPath + "unknownClass.mt",
                        TestType::ERROR_EXPECTED);
    }
}
