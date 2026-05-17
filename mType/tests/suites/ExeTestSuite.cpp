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

        // ===== MYT-304: mt_modules alias resolution =====
        addExeTest("Exe - mt_modules Basic Resolution",
                   basePath + "imports/mtmodules-basic/MtModulesBasic.mtproj");
        addExeTest("Exe - mt_modules Project Alias Beats Module",
                   basePath + "imports/mtmodules-precedence/MtModulesPrecedence.mtproj");

        // ===== MYT-310: direct `mType.exe script.mt` discovers ambient .mtproj =====
        // Reuses the mtmodules-basic fixture but runs the .mt file directly
        // (no --build) so the test exercises the discover+merge codepath in
        // ScriptInterpreter::tryLoadAmbientProject. Output must match the
        // sibling Main.expected (identical to MtModulesBasic.expected).
        addDirectScriptWithProjectTest(
            "Exe - mt_modules Direct Script Discovers .mtproj (MYT-310)",
            basePath + "imports/mtmodules-basic/DirectMain.mt");

        // ===== COLLECTIONS =====
        addExeTest("Exe - Collections",
                   basePath + "collections/CollectionExe.mtproj");

        // ===== STREAMS =====
        addExeTest("Exe - Streams",
                   basePath + "streams/StreamExe.mtproj");

        // ===== CAST =====
        addExeTest("Exe - Cast",
                   basePath + "cast/CastExe.mtproj");

        // ===== REGRESSION: MYT-290 - static initializers before entry point =====
        addExeTest("Exe - Regression: Static Initializers (MYT-290)",
                   basePath + "static-init/StaticInitExe.mtproj");

        // ===== REGRESSION: MYT-326 - --gui builds windowed-subsystem launcher =====
        // Builds with mtype-launcher-gui (WindowedApp + /SUBSYSTEM:WINDOWS on
        // Windows). The harness is a console parent, so the child inherits
        // stdio and print() still flows back through _popen — output match
        // proves the GUI launcher is wired and functional.
        addExeGuiTest("Exe - GUI Launcher Subsystem (MYT-326)",
                      basePath + "gui-launcher/GuiLauncherExe.mtproj");

        // ===== REGRESSION: MYT-63 — parent class linking =====
        addExeTest("Exe - Regression: Exception Catch Dispatch (MYT-63)",
                   basePath + "regression-catch/RegressionCatchExe.mtproj");

        addExeTest("Exe - Regression: Inherited Method Resolution (MYT-63)",
                   basePath + "regression-method/RegressionMethodExe.mtproj");
    }
}
