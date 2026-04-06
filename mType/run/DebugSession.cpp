#include "DebugSession.hpp"
#include "../debugger/DebugContext.hpp"
#include "../debugger/DebugHookHelper.hpp"
#include "../debugger/DebugProtocol.hpp"
#include "../gc/GC.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../json/JsonNatives.hpp"

#include <iostream>
#include <thread>

using namespace services;
using namespace environment;

void runInDebugMode(const std::string& filename,
                    constants::ExecutionMode execMode)
{
    // Output debug banner to stderr so it doesn't interfere with debug protocol on stdout
    std::cerr << "\n" << std::string(80, '=') << "\n";
    std::cerr << "mType Debugger Mode\n";
    std::cerr << std::string(80, '=') << "\n";
    std::cerr << "Debug protocol: stdin/stdout\n";
    std::cerr << "Script file: " << filename << "\n";
    std::cerr << std::string(80, '=') << "\n\n";

    try
    {
        // Initialize debug context
        debugger::DebugContext::initialize();
        auto& debugCtx = debugger::DebugContext::getInstance();

        // Create interpreter and enable debugging
        ScriptInterpreter interpreter(execMode);
        interpreter.enableDebugging();

        // Start debug server in a separate thread
        debugger::DebugServer debugServer;

        // Set the environment for variable inspection
        debugServer.setEnvironment(interpreter.getEnvironment());

        std::thread serverThread([&debugServer]()
        {
            debugServer.run();
        });

        // Notify debugger that script is starting
        debugger::DebugHookHelper::notifyScriptStart(filename);

        // Push main script frame before pausing so VSCode has a frame to show
        errors::SourceLocation mainFrameLoc(filename, 1, 1);
        debugger::DebugHookHelper::enterFunctionHook("<main>", mainFrameLoc);

        // Pause at entry to allow debugger to set breakpoints
        debugCtx.pause();
        std::cerr << "Paused at entry. Waiting for debugger commands...\n";

        // Send STOPPED event with reason "entry" to notify VSCode
        errors::SourceLocation entryLocation(filename, 1, 1);
        debugger::DebugProtocol::sendStoppedEvent("entry", entryLocation);

        // Wait for debugger to set breakpoints and send CONTINUE
        debugCtx.waitForResume();

        // In bytecode mode, update DebugServer to use VM for variable inspection
        auto vm = interpreter.getVM();
        if (vm)
        {
            debugServer.setVM(vm);
        }

        // Run the script (will pause at breakpoints)
        interpreter.runScript(filename);

        // Pop main script frame after completion
        debugger::DebugHookHelper::exitFunctionHook("<main>");

        // Notify debugger that script completed
        debugger::DebugHookHelper::notifyScriptComplete(filename);

        // Stop debug server
        debugServer.stop();
        if (serverThread.joinable())
        {
            serverThread.join();
        }

        // Shutdown debug context
        debugger::DebugContext::shutdown();

        std::cerr << "\n" << std::string(80, '=') << "\n";
        std::cerr << "Debug session ended\n";
        std::cerr << std::string(80, '=') << "\n";
    }
    catch (const std::exception& e)
    {
        // Pop main script frame on exception
        if (debugger::DebugHookHelper::isDebuggingEnabled())
        {
            debugger::DebugHookHelper::exitFunctionHook("<main>");
        }
        std::cerr << "Debug session error: " << e.what() << std::endl;
        debugger::DebugContext::shutdown();
    }
}
