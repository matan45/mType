#include "TestCase.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/ParseException.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include "../../services/ScriptAPI.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../runtime/EventLoop.hpp"
#include "../../reflection/ReflectionNatives.hpp"
#include "../../json/JsonNatives.hpp"
#include "../../gc/GC.hpp"
#include "../../project/ProjectConfigParser.hpp"
#include "../../project/ProjectBuilder.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstdio>

namespace tests::testFramework
{
    using namespace services;

    TestCase::TestCase(const std::string& testName, const std::string& testFilePath, TestType testType)
        : name(testName), filePath(testFilePath), type(testType), status(TestStatus::NOT_RUN),
          executionTime(0), executionMode(constants::ExecutionMode::BYTECODE_VM)
    {
    }

    TestCase::TestCase(const std::string& testName,
                       const std::string& testFilePath,
                       TestType testType,
                       const std::string& expectedErrorSubstr)
        : name(testName), filePath(testFilePath), type(testType), status(TestStatus::NOT_RUN),
          executionTime(0), executionMode(constants::ExecutionMode::BYTECODE_VM),
          expectedErrorSubstring(expectedErrorSubstr)
    {
    }

    TestCase::TestCase(const std::string& testName,
                       const std::string& bootstrapFilePath,
                       NativeCallback callback)
        : name(testName), filePath(bootstrapFilePath), type(TestType::NATIVE_CALLBACK),
          status(TestStatus::NOT_RUN), executionTime(0),
          executionMode(constants::ExecutionMode::BYTECODE_VM),
          nativeCallback(std::move(callback))
    {
    }

    TestCase TestCase::skipped(const std::string& testName, const std::string& reason)
    {
        TestCase tc(testName, "", TestType::NORMAL);
        tc.status = TestStatus::SKIPPED;
        tc.errorMessage = reason;
        return tc;
    }

    void TestCase::execute()
    {
        if (status == TestStatus::SKIPPED)
        {
            return;
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        // Clear interface validation cache to prevent contamination between tests
        ScriptInterpreter interpreter;
        auto env = interpreter.getEnvironment();
        if (env) {
            auto interfaceRegistry = env->getInterfaceRegistry();
            if (interfaceRegistry) {
                interfaceRegistry->clearValidationCache();
            }
        }

        // Clear reflection handle registry to prevent stale handles between tests
        reflection::ReflectionNatives::cleanup();
        json::JsonNatives::cleanup();

        // Reset GC state to prevent cross-test contamination
        gc::GC::reset();

        if (type == TestType::NATIVE_CALLBACK)
        {
            executeNativeCallback();
            auto endTime = std::chrono::high_resolution_clock::now();
            executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            return;
        }

        if (type == TestType::EXE_TEST)
        {
            executeExeTest();
            auto endTime = std::chrono::high_resolution_clock::now();
            executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            return;
        }

        try
        {
            // Check if file exists
            if (!std::filesystem::exists(filePath))
            {
                status = TestStatus::ERROR;
                errorMessage = "Test file not found: " + filePath;
                auto endTime = std::chrono::high_resolution_clock::now();
                executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                return;
            }

            // Redirect stdout to capture output
            std::stringstream outputBuffer;
            std::streambuf* oldCout = std::cout.rdbuf();
            std::cout.rdbuf(outputBuffer.rdbuf());

            try
            {
                // Always use ScriptInterpreter with bytecode mode
                ScriptInterpreter testInterpreter(executionMode);
                // MYT-259: JIT defaults on; suite runner can flip this off
                // via setJitEnabledForAll for a `--no-jit --tests` pass.
                testInterpreter.getVM()->setJitEnabled(jitEnabled);

                if (type == TestType::SCRIPT_INTEROP)
                {
                    // C++ interop path: parse classes, create object, call onStart
                    testInterpreter.parseAndRegisterClasses(filePath);

                    auto env = testInterpreter.getEnvironment();
                    auto classRegistry = env->getClassRegistry();
                    auto allClassNames = classRegistry->getAllItemNames();

                    for (const auto& className : allClassNames)
                    {
                        auto classDef = classRegistry->findClass(className);
                        if (classDef && classDef->hasAnnotation("Script"))
                        {
                            std::vector<value::Value> noArgs;
                            value::Value obj = testInterpreter.createObject(className);
                            testInterpreter.callMethod(obj, "onStart", noArgs);

                            // If the class has onInteropTest(string), call it with C++ strings
                            // This tests native std::string vs InternedString comparison
                            if (classDef->findMethod("onInteropTest", 1))
                            {
                                std::vector<value::Value> stringArgs;
                                stringArgs.push_back(value::Value(std::string("NewGameBtn")));
                                testInterpreter.callMethod(obj, "onInteropTest", stringArgs);

                                stringArgs.clear();
                                stringArgs.push_back(value::Value(std::string("SettingsBtn")));
                                testInterpreter.callMethod(obj, "onInteropTest", stringArgs);

                                stringArgs.clear();
                                stringArgs.push_back(value::Value(std::string("UnknownBtn")));
                                testInterpreter.callMethod(obj, "onInteropTest", stringArgs);
                            }
                        }
                    }

                    // Tick event loop to complete any async tasks scheduled by callMethod
                    auto vmPtr = testInterpreter.getVM();
                    if (vmPtr)
                    {
                        auto* eventLoop = vmPtr->getEventLoop();
                        if (eventLoop)
                        {
                            while (eventLoop->tick()) {}
                        }
                    }
                }
                else
                {
                    testInterpreter.runScript(filePath);
                }

                // Restore stdout
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();

                // Determine test result based on type
                if (type == TestType::ERROR_EXPECTED)
                {
                    // This test was expected to fail but passed
                    status = TestStatus::FAILED;
                    errorMessage = "Expected error but test passed";
                }
                else if (type == TestType::OUTPUT_EXPECTED || type == TestType::SCRIPT_INTEROP)
                {
                    // Verify output against expected file
                    if (verifyOutputAgainstExpected())
                    {
                        status = TestStatus::PASSED;
                    }
                    else
                    {
                        status = TestStatus::FAILED;
                        errorMessage = "Output does not match expected result";
                    }
                }
                else
                {
                    // Normal test passed
                    status = TestStatus::PASSED;
                }
            }
            catch (const errors::ParseException& e)
            {
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();

                if (type == TestType::ERROR_EXPECTED)
                {
                    status = TestStatus::PASSED;
                    errorMessage = "Expected parse error: " + std::string(e.what());
                    applyExpectedErrorFilter(e.what());
                }
                else
                {
                    status = TestStatus::FAILED;
                    errorMessage = "Parse error: " + std::string(e.what());
                }
            }
            catch (const errors::TypeException& e)
            {
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();

                if (type == TestType::ERROR_EXPECTED)
                {
                    status = TestStatus::PASSED;
                    errorMessage = "Expected type error: " + std::string(e.what());
                    applyExpectedErrorFilter(e.what());
                }
                else
                {
                    status = TestStatus::FAILED;
                    errorMessage = "Type error: " + std::string(e.what());
                }
            }
            catch (const errors::UndefinedException& e)
            {
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();

                if (type == TestType::ERROR_EXPECTED)
                {
                    status = TestStatus::PASSED;
                    errorMessage = "Expected undefined error: " + std::string(e.what());
                    applyExpectedErrorFilter(e.what());
                }
                else
                {
                    status = TestStatus::FAILED;
                    errorMessage = "Undefined error: " + std::string(e.what());
                }
            }
            catch (const std::exception& e)
            {
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();

                if (type == TestType::ERROR_EXPECTED)
                {
                    status = TestStatus::PASSED;
                    errorMessage = "Expected error: " + std::string(e.what());
                    applyExpectedErrorFilter(e.what());
                }
                else
                {
                    status = TestStatus::FAILED;
                    errorMessage = "Runtime error: " + std::string(e.what());
                }
            }
        }
        catch (const std::exception& e)
        {
            status = TestStatus::ERROR;
            errorMessage = "Test execution error: " + std::string(e.what());
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    }

    std::string TestCase::statusToString(TestStatus status)
    {
        switch (status)
        {
        case TestStatus::NOT_RUN:
            return "NOT RUN";
        case TestStatus::PASSED:
            return "PASSED";
        case TestStatus::FAILED:
            return "FAILED";
        case TestStatus::ERROR:
            return "ERROR";
        case TestStatus::SKIPPED:
            return "SKIPPED";
        default:
            return "UNKNOWN";
        }
    }

    std::string TestCase::typeToString(TestType type)
    {
        switch (type)
        {
        case TestType::NORMAL:
            return "NORMAL";
        case TestType::ERROR_EXPECTED:
            return "ERROR_EXPECTED";
        case TestType::OUTPUT_EXPECTED:
            return "OUTPUT_EXPECTED";
        case TestType::PERFORMANCE:
            return "PERFORMANCE";
        case TestType::SCRIPT_INTEROP:
            return "SCRIPT_INTEROP";
        case TestType::NATIVE_CALLBACK:
            return "NATIVE_CALLBACK";
        case TestType::EXE_TEST:
            return "EXE_TEST";
        default:
            return "UNKNOWN";
        }
    }

    void TestCase::executeNativeCallback()
    {
        if (!nativeCallback)
        {
            status = TestStatus::ERROR;
            errorMessage = "NATIVE_CALLBACK test has no callback set";
            return;
        }

        // Redirect stdout to capture any output the bootstrap or callback
        // produces — we discard it unless the test fails, in which case the
        // captured output is attached to aid debugging.
        std::stringstream outputBuffer;
        std::streambuf* oldCout = std::cout.rdbuf(outputBuffer.rdbuf());

        try
        {
            ScriptInterpreter testInterpreter(executionMode);
            testInterpreter.getVM()->setJitEnabled(jitEnabled);

            // Parse-and-register the bootstrap file WITHOUT executing its
            // top-level code (same mechanism SCRIPT_INTEROP uses, so the
            // VM is left in a clean "ready for interop" state after).
            // `filePath` stores the bootstrap file here. The bootstrap is
            // required to contain only class/function declarations —
            // callbacks drive execution via ScriptAPI::callFunction.
            if (!filePath.empty())
            {
                if (!std::filesystem::exists(filePath))
                {
                    std::cout.rdbuf(oldCout);
                    status = TestStatus::ERROR;
                    errorMessage = "Bootstrap file not found: " + filePath;
                    return;
                }
                testInterpreter.parseAndRegisterClasses(filePath);
            }

            // Build a ScriptAPI bound to the interpreter's VM + bytecode
            // program and hand it to the callback. Any exception thrown
            // from the callback fails the test via the catch below.
            auto vmShared = testInterpreter.getVM();
            vm::runtime::VirtualMachine* vmPtr = vmShared.get();
            const vm::bytecode::BytecodeProgram* program = vmPtr ? vmPtr->getProgram() : nullptr;
            services::ScriptAPI api(testInterpreter.getEnvironment(), vmPtr, program);

            nativeCallback(api);

            std::cout.rdbuf(oldCout);
            output = outputBuffer.str();
            status = TestStatus::PASSED;
        }
        catch (const std::exception& e)
        {
            std::cout.rdbuf(oldCout);
            output = outputBuffer.str();
            status = TestStatus::FAILED;
            errorMessage = std::string("Native callback failed: ") + e.what();
        }
        catch (...)
        {
            std::cout.rdbuf(oldCout);
            output = outputBuffer.str();
            status = TestStatus::FAILED;
            errorMessage = "Native callback failed: unknown exception";
        }
    }

    void TestCase::executeExeTest()
    {
        try
        {
            // Step 1: Locate the launcher binary in the build output directory
#ifdef _WIN32
            std::string launcherName = "mtype-launcher.exe";
#else
            std::string launcherName = "mtype-launcher";
#endif
            // Try known build output paths relative to working directory
            std::string launcherPath;
            std::vector<std::string> searchPaths = {
                "bin/mType/Debug/x64/" + launcherName,
                "bin/mType/Release/x64/" + launcherName,
                "bin/mtype-launcher/Debug/x64/" + launcherName,
                "bin/mtype-launcher/Release/x64/" + launcherName
            };
            for (const auto& candidate : searchPaths)
            {
                if (std::filesystem::exists(candidate))
                {
                    launcherPath = candidate;
                    break;
                }
            }
            if (launcherPath.empty())
            {
                status = TestStatus::SKIPPED;
                errorMessage = "Launcher binary not found in any search path"
                               " (build mtype-launcher project first)";
                return;
            }

            // Step 2: Parse the .mtproj file
            if (!std::filesystem::exists(filePath))
            {
                status = TestStatus::ERROR;
                errorMessage = "Project file not found: " + filePath;
                return;
            }

            project::ProjectConfigParser configParser;
            auto config = configParser.parse(filePath);

            // Step 3: Build the exe
            std::filesystem::path outputDir =
                std::filesystem::path(config->projectRoot) / config->output.directory;
#ifdef _WIN32
            std::string exeName = config->name + ".exe";
#else
            std::string exeName = config->name;
#endif
            std::string exePath = (outputDir / exeName).string();

            project::ProjectBuilder builder;
            auto buildResult = builder.buildExecutable(*config, exePath, launcherPath);

            if (!buildResult.success)
            {
                status = TestStatus::FAILED;
                std::string errors;
                for (const auto& err : buildResult.errors)
                {
                    if (!errors.empty()) errors += "; ";
                    errors += err;
                }
                errorMessage = "Exe build failed: " + errors;
                return;
            }

            // Step 4: Run the exe as a subprocess and capture stdout
            std::string command = "\"" + exePath + "\" 2>&1";
            FILE* pipe = _popen(command.c_str(), "r");
            if (!pipe)
            {
                status = TestStatus::ERROR;
                errorMessage = "Failed to run built exe: " + exePath;
                return;
            }

            char buffer[256];
            output.clear();
            while (fgets(buffer, sizeof(buffer), pipe))
            {
                output += buffer;
            }
            int exitCode = _pclose(pipe);

            // Step 5: Verify output against .expected file
            if (verifyOutputAgainstExpected())
            {
                status = TestStatus::PASSED;
            }
            else
            {
                status = TestStatus::FAILED;
                if (errorMessage.empty())
                {
                    errorMessage = "Output does not match expected result";
                }
                if (exitCode != 0)
                {
                    errorMessage += " (exit code: " + std::to_string(exitCode) + ")";
                }
            }

            // Step 6: Cleanup - remove the built exe and build directory
            try
            {
                if (std::filesystem::exists(outputDir))
                {
                    std::filesystem::remove_all(outputDir);
                }
            }
            catch (...)
            {
                // Cleanup failure is non-fatal
            }
        }
        catch (const std::exception& e)
        {
            status = TestStatus::ERROR;
            errorMessage = "Exe test error: " + std::string(e.what());
        }
    }

    bool TestCase::verifyOutputAgainstExpected() const
    {
        // Construct expected output file path by replacing .mt with .expected
        std::string expectedFilePath = filePath;
        size_t pos = expectedFilePath.find_last_of('.');
        if (pos != std::string::npos) {
            expectedFilePath = expectedFilePath.substr(0, pos) + ".expected";
        } else {
            expectedFilePath += ".expected";
        }

        // Check if expected file exists
        if (!std::filesystem::exists(expectedFilePath)) {
            return false; // Cannot verify without expected file
        }

        // Read expected output
        std::ifstream expectedFile(expectedFilePath);
        if (!expectedFile.is_open()) {
            return false;
        }

        std::string expectedOutput((std::istreambuf_iterator<char>(expectedFile)),
                                   std::istreambuf_iterator<char>());
        expectedFile.close();

        // Normalize line endings and whitespace for comparison
        auto normalize = [](std::string str) {
            // Remove trailing whitespace from each line
            std::stringstream ss(str);
            std::string line;
            std::string result;
            while (std::getline(ss, line)) {
                // Remove trailing whitespace
                while (!line.empty() && std::isspace(line.back())) {
                    line.pop_back();
                }
                if (!result.empty()) {
                    result += "\n";
                }
                result += line;
            }
            return result;
        };

        std::string normalizedOutput = normalize(output);
        std::string normalizedExpected = normalize(expectedOutput);

        // Validate line count first
        auto countNonEmptyLines = [](const std::string& str) {
            std::stringstream ss(str);
            std::string line;
            int count = 0;
            while (std::getline(ss, line)) {
                // Count lines that have content after trimming whitespace
                std::string trimmed = line;
                // Remove leading and trailing whitespace
                size_t first = trimmed.find_first_not_of(" \t\r\n");
                if (first == std::string::npos) {
                    continue; // Skip empty lines
                }
                size_t last = trimmed.find_last_not_of(" \t\r\n");
                trimmed = trimmed.substr(first, (last - first + 1));
                if (!trimmed.empty()) {
                    count++;
                }
            }
            return count;
        };

        int actualLines = countNonEmptyLines(normalizedOutput);
        int expectedLines = countNonEmptyLines(normalizedExpected);

        // If line counts don't match, log the discrepancy and fail
        if (actualLines != expectedLines) {
            std::cerr << "Line count mismatch in " << filePath << ":\n";
            std::cerr << "  Expected: " << expectedLines << " lines\n";
            std::cerr << "  Actual: " << actualLines << " lines\n";
            std::cerr << "  Difference: " << (actualLines - expectedLines) << "\n";
            return false;
        }

        // Then validate content
        return normalizedOutput == normalizedExpected;
    }

    void TestCase::applyExpectedErrorFilter(const std::string& errWhat)
    {
        if (status != TestStatus::PASSED || expectedErrorSubstring.empty())
        {
            return;
        }
        if (errWhat.find(expectedErrorSubstring) == std::string::npos)
        {
            status = TestStatus::FAILED;
            errorMessage = "Expected error containing '" + expectedErrorSubstring +
                           "' but got: " + errWhat;
        }
    }
}
