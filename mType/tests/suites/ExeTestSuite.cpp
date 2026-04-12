#include "ExeTestSuite.hpp"

namespace tests::testSuite
{
    void ExeTestSuite::setupTests()
    {
        // Existing basic exe test
        addExeTest("Exe - Basic Hello World",
                   basePath + "ExeTest.mtproj");

        // ===== GENERICS =====
        addExeTest("Exe - Generics",
                   basePath + "generics/GenericExe.mtproj");

        // ===== CLASSES & INHERITANCE =====
        addExeTest("Exe - Classes",
                   basePath + "classes/ClassExe.mtproj");

        // ===== ASYNC/AWAIT =====
        addExeTest("Exe - Async/Await",
                   basePath + "async/AsyncExe.mtproj");

        // ===== INTERFACES =====
        addExeTest("Exe - Interfaces",
                   basePath + "interfaces/InterfaceExe.mtproj");

        // ===== LAMBDA & CLOSURES =====
        addExeTest("Exe - Lambdas",
                   basePath + "lambda/LambdaExe.mtproj");

        // ===== isClassOf OPERATOR =====
        addExeTest("Exe - TypeOf/isClassOf",
                   basePath + "typeof/TypeOfExe.mtproj");

        // ===== TRY/CATCH =====
        addExeTest("Exe - Try/Catch",
                   basePath + "trycatch/TryCatchExe.mtproj");

        // ===== NULL SAFETY =====
        addExeTest("Exe - Null Safety",
                   basePath + "nullsafety/NullSafetyExe.mtproj");

        // ===== ARRAYS =====
        addExeTest("Exe - Arrays",
                   basePath + "arrays/ArrayExe.mtproj");

        // ===== IMPORTS =====
        addExeTest("Exe - Imports",
                   basePath + "imports/ImportExe.mtproj");

        // ===== COLLECTIONS =====
        addExeTest("Exe - Collections",
                   basePath + "collections/CollectionExe.mtproj");

        // ===== STREAMS =====
        addExeTest("Exe - Streams",
                   basePath + "streams/StreamExe.mtproj");

        // ===== CAST =====
        addExeTest("Exe - Cast",
                   basePath + "cast/CastExe.mtproj");
    }
}
