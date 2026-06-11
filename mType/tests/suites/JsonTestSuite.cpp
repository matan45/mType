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
        addOutputVerificationTest("Null Fields",
                        passPath + "nullFields.mt");
        addOutputVerificationTest("Subclass Deserialization",
                        passPath + "subclass.mt");

        // === ROUNDTRIP TESTS ===

        addOutputVerificationTest("Roundtrip",
                        passPath + "roundTrip.mt");
        addOutputVerificationTest("File I/O",
                        passPath + "fileIO.mt");

        // === COLLECTION TESTS ===

        addOutputVerificationTest("Serialize ArrayList",
                        passPath + "serializeArrayList.mt");
        addOutputVerificationTest("Serialize LinkedList",
                        passPath + "serializeLinkedList.mt");
        addOutputVerificationTest("Serialize Stack",
                        passPath + "serializeStack.mt");
        addOutputVerificationTest("Serialize ArrayQueue",
                        passPath + "serializeQueue.mt");
        addOutputVerificationTest("Serialize HashMap",
                        passPath + "serializeHashMap.mt");
        addOutputVerificationTest("Serialize HashSet",
                        passPath + "serializeHashSet.mt");

        // === FORMAT TEST ===

        addOutputVerificationTest("Format JSON",
                        passPath + "format.mt");

        // === ERROR TESTS ===

        addTestFromFile("Invalid JSON",
                        errorPath + "invalidJson.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Unknown Class",
                        errorPath + "unknownClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Empty JSON",
                        errorPath + "emptyJson.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Truncated JSON",
                        errorPath + "truncatedJson.mt",
                        TestType::ERROR_EXPECTED,
                        "JSON parse error");

        // === ROUND-TRIP EDGE CASES ===
        addOutputVerificationTest("Deep Nesting Round Trip (50 levels)",
                                  passPath + "jsonDeepNestingRoundTrip.mt");

        addOutputVerificationTest("Unicode And Escapes Round Trip",
                                  passPath + "jsonUnicodeRoundTrip.mt");

        // === CANARIES (MYT-388: DESERIALIZATION IGNORES FIELD TYPES) ===
        // JsonDeserializer sets fields directly from the parsed value without
        // checking the declared type — a JSON string lands intact in an
        // int-typed field and no error fires. This ERROR_EXPECTED test fails
        // today (nothing throws) and stays failing until MYT-388 lands
        // (memory: feedback_keep_failing_canary_tests).
        addTestFromFile("CANARY Type Mismatch Raises Error",
                        errorPath + "typeMismatchJson.mt",
                        TestType::ERROR_EXPECTED,
                        "JSON deserialization type mismatch");
    }
}
