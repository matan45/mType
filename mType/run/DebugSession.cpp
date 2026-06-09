#include "DebugSession.hpp"
#include "../debugger/DebugContext.hpp"
#include "../debugger/DebugHookHelper.hpp"
#include "../debugger/DebugProtocol.hpp"
#include "../gc/GC.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../json/JsonNatives.hpp"
// Socket transport for --debug-port attach. Pick the platform implementation;
// the Win/Posix .cpp files are mutually excluded by the build (see premake), so
// referencing the wrong one would fail to link (mac/Linux CI).
#ifdef _WIN32
#   include "../net/WinSocket.hpp"
    using DebugSocket       = net::WinSocket;
    using DebugSocketServer = net::WinSocketServer;
#else
#   include "../net/PosixSocket.hpp"
    using DebugSocket       = net::PosixSocket;
    using DebugSocketServer = net::PosixSocketServer;
#endif

#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

using namespace services;
using namespace environment;

// RAII guard to restore std::cout and clear the protocol stream on any exit path
struct StdoutGuard {
    std::streambuf* original;
    StdoutGuard(std::streambuf* buf) : original(buf) {}
    ~StdoutGuard() {
        debugger::DebugProtocol::setProtocolStream(nullptr);
        std::cout.rdbuf(original);
    }
};

namespace
{
    // Shared debug session body: performs the entry handshake, runs the script
    // (pausing at breakpoints), and tears down the debug server. The transport
    // (stdin/stdout or socket) is set up by the caller; stopTransport is invoked
    // just before joining the server thread to unblock any blocking read.
    void driveDebugSession(debugger::DebugServer& debugServer,
                           std::thread& serverThread,
                           ScriptInterpreter& interpreter,
                           const std::string& filename,
                           const std::function<void()>& stopTransport)
    {
        auto& debugCtx = debugger::DebugContext::getInstance();

        // Notify debugger that script is starting
        debugger::DebugHookHelper::notifyScriptStart(filename);

        // Push main script frame before pausing so VSCode has a frame to show
        errors::SourceLocation mainFrameLoc(filename, 1, 1);
        debugger::DebugHookHelper::enterFunctionHook("<main>", mainFrameLoc);

        // Pause at entry to allow debugger to set breakpoints
        debugCtx.pause();

        // Send STOPPED event with reason "entry" to notify the client
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

        auto shutdownServer = [&]()
        {
            debugServer.stop();
            if (stopTransport) { stopTransport(); }
            if (serverThread.joinable()) { serverThread.join(); }
        };

        try
        {
            // Run the script (will pause at breakpoints)
            interpreter.runScript(filename);

            // Pop main script frame after completion
            debugger::DebugHookHelper::exitFunctionHook("<main>");

            // Notify debugger that script completed
            debugger::DebugHookHelper::notifyScriptComplete(filename);
        }
        catch (...)
        {
            shutdownServer();
            throw;
        }

        shutdownServer();
    }
}

void runInDebugMode(const std::string& filename,
                    constants::ExecutionMode execMode)
{
    // Redirect std::cout to stderr so that print() output doesn't corrupt
    // the debug protocol on stdout. Protocol messages use a dedicated stream
    // backed by the original stdout.
    auto* originalStdoutBuf = std::cout.rdbuf();
    std::cout.rdbuf(std::cerr.rdbuf());
    std::ostream protocolStream(originalStdoutBuf);
    debugger::DebugProtocol::setProtocolStream(&protocolStream);
    StdoutGuard stdoutGuard(originalStdoutBuf);

    try
    {
        // Initialize debug context
        debugger::DebugContext::initialize();

        // Create interpreter and enable debugging
        ScriptInterpreter interpreter(execMode);
        interpreter.enableDebugging();

        // Start debug server in a separate thread, reading commands from stdin
        debugger::DebugServer debugServer;
        debugServer.setEnvironment(interpreter.getEnvironment());

        std::thread serverThread([&debugServer]()
        {
            debugServer.run();
        });

        driveDebugSession(debugServer, serverThread, interpreter, filename, nullptr);

        // Shutdown debug context
        debugger::DebugContext::shutdown();
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
    // StdoutGuard restores std::cout and clears protocol stream on all exit paths
}

void runInDebugModePort(const std::string& filename,
                        constants::ExecutionMode execMode,
                        int port)
{
    try
    {
        // Initialize debug context
        debugger::DebugContext::initialize();

        // Create interpreter and enable debugging
        ScriptInterpreter interpreter(execMode);
        interpreter.enableDebugging();

        debugger::DebugServer debugServer;
        debugServer.setEnvironment(interpreter.getEnvironment());

        // Listen for a single attaching debug client.
        DebugSocketServer server;
        std::promise<uintptr_t> clientPromise;
        std::future<uintptr_t> clientFuture = clientPromise.get_future();
        std::atomic<bool> accepted{false};

        server.start(port,
            [&](uintptr_t fd)
            {
                bool expected = false;
                if (accepted.compare_exchange_strong(expected, true))
                {
                    clientPromise.set_value(fd);
                }
                // Additional connections are ignored: one debug client at a time.
            },
            [](const std::string& err)
            {
                std::cerr << "Debug socket error: " << err << std::endl;
            });

        std::cerr << "mType debug server listening on port " << port
                  << " - waiting for a debugger to attach..." << std::endl;

        // Block until a client connects (the engine/host equivalent of stdin).
        uintptr_t clientFd = clientFuture.get();
#ifdef _WIN32
        std::shared_ptr<net::ISocket> sock = std::make_shared<DebugSocket>(clientFd);
#else
        std::shared_ptr<net::ISocket> sock = std::make_shared<DebugSocket>(static_cast<int>(clientFd));
#endif

        // Route protocol output to the socket; read commands back from it.
        debugger::DebugProtocol::setProtocolWriter(
            [sock](const std::string& line)
            {
                try { sock->send(line + "\n"); }
                catch (...) { /* client gone; the read side will detect EOF */ }
            });

        // Buffer partial reads and hand whole lines to the server. Returns false
        // on EOF or socket error, which ends the server loop.
        auto buffer = std::make_shared<std::string>();
        auto readLine = [sock, buffer](std::string& line) -> bool
        {
            std::string::size_type nl;
            while ((nl = buffer->find('\n')) == std::string::npos)
            {
                std::string chunk;
                try { chunk = sock->recv(4096); }
                catch (...) { return false; }
                if (chunk.empty()) { return false; }
                *buffer += chunk;
            }
            line = buffer->substr(0, nl);
            if (!line.empty() && line.back() == '\r') { line.pop_back(); }
            buffer->erase(0, nl + 1);
            return true;
        };

        std::thread serverThread([&debugServer, readLine]()
        {
            debugServer.run(readLine);
        });

        // Closing the socket unblocks the server thread's blocking recv() so the
        // join in driveDebugSession can complete.
        driveDebugSession(debugServer, serverThread, interpreter, filename,
            [sock, &server]()
            {
                server.stop();
                sock->close();
            });

        debugger::DebugProtocol::setProtocolWriter(nullptr);
        debugger::DebugContext::shutdown();
    }
    catch (const std::exception& e)
    {
        if (debugger::DebugHookHelper::isDebuggingEnabled())
        {
            debugger::DebugHookHelper::exitFunctionHook("<main>");
        }
        std::cerr << "Debug session error: " << e.what() << std::endl;
        debugger::DebugProtocol::setProtocolWriter(nullptr);
        debugger::DebugContext::shutdown();
    }
}
