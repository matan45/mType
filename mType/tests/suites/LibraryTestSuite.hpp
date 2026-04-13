#pragma once
#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * Test suite for .mtcLib library linking features (MYT-76 through MYT-94).
     * Tests library build, compile-time linking, runtime loading,
     * and all language features across library boundaries.
     */
    class LibraryTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string basePath = "mType/tests/testFiles/library/";
        inline static const std::string passPath = "mType/tests/testFiles/library/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/library/error/";
        inline static const std::string libsPath = "mType/tests/testFiles/library/libs/";

    public:
        explicit LibraryTestSuite() : TestSuite("Library Linking Test Suite") {}
        void setupTests() override;

    private:
        // Unit tests for foundation components
        void setupSerializationTests();
        void setupTypeSignatureTests();
        void setupDependencyResolverTests();
        void setupMtcLibBuilderTests();

        // Integration tests for library build + load pipeline
        void setupBuildTests();
        void setupLinkingTests();
        void setupRuntimeLoadingTests();

        // MYT-100: Transitive dependency loading tests
        void setupTransitiveDependencyTests();

        // MYT-101: Native loadLibrary API tests
        void setupNativeLoadLibraryTests();

        // Feature tests across library boundaries
        void setupClassTests();
        void setupInterfaceTests();
        void setupGenericsTests();
        void setupArrayTests();
        void setupLambdaTests();
        void setupAsyncTests();
        void setupCastTests();
        void setupImportTests();
        void setupCollectionTests();
        void setupReflectionTests();
        void setupValueClassTests();
        void setupForEachTests();
        void setupTryCatchTests();
        void setupStaticTests();
        void setupInheritanceTests();
        void setupModifierTests();
        void setupStreamTests();
        void setupPatternMatchingTests();
    };
}
