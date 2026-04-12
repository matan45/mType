#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include "DebugContext.hpp"

namespace debugger {

    /**
     * Simple line-based protocol for debug communication
     *
     * Protocol format (each message is one line):
     * COMMAND key1=value1 key2=value2 ...
     *
     * Commands from client:
     * - SETBREAKPOINT file=path.mt line=10 [condition="x>5"] [logMessage="Value is {x}"]
     * - CLEARBREAKPOINT file=path.mt line=10
     * - CLEARALL
     * - CONTINUE
     * - STEPINTO
     * - STEPOVER
     * - STEPOUT
     * - GETSTACKTRACE
     * - GETVARIABLES scope=local
     * - GETVARIABLES scope=global
     * - EXPANDVARIABLE ref=123
     * - SETEXCEPTIONBREAKPOINTS filter0=all filter1=uncaught
     * - EVALUATE expr="x+1" frame=0
     * - STOP
     *
     * Events from server:
     * - STOPPED reason=breakpoint file=path.mt line=10
     * - STOPPED reason=step file=path.mt line=11
     * - STOPPED reason=exception message="error" file=path.mt line=12
     * - STARTED file=path.mt
     * - TERMINATED
     * - OUTPUT text="Hello World" category=stdout
     *
     * Responses:
     * - STACKTRACE frame0="main@path.mt:10" frame1="foo@path.mt:5"
     * - VARIABLES var0="x=42:int:0" var1="obj={MyClass}:MyClass:123"
     * - EXPANDEDVAR child0="field1=10:int:0" child1="field2=hello:string:0"
     * - RESULT value="43" type="int"
     * - OK
     * - ERROR message="Invalid command"
     */
    class DebugProtocol {
    public:
        /**
         * Set a dedicated output stream for protocol messages.
         * When set, send() uses this stream instead of std::cout.
         * This allows redirecting std::cout (for print()) to stderr
         * while keeping protocol messages on the original stdout.
         */
        static void setProtocolStream(std::ostream* stream);


        struct Message {
            std::string command;
            std::map<std::string, std::string> parameters;

            Message() = default;
            Message(const std::string& cmd) : command(cmd) {}

            void addParameter(const std::string& key, const std::string& value) {
                parameters[key] = value;
            }

            void addParameter(const std::string& key, int value) {
                parameters[key] = std::to_string(value);
            }

            void addParameter(const std::string& key, int64_t value) {
                parameters[key] = std::to_string(value);
            }

            std::string getParameter(const std::string& key, const std::string& defaultValue = "") const {
                auto it = parameters.find(key);
                return (it != parameters.end()) ? it->second : defaultValue;
            }

            int getIntParameter(const std::string& key, int defaultValue = 0) const {
                auto it = parameters.find(key);
                if (it != parameters.end()) {
                    try {
                        return std::stoi(it->second);
                    } catch (...) {
                        return defaultValue;
                    }
                }
                return defaultValue;
            }

            std::string serialize() const {
                std::stringstream ss;
                ss << command;
                for (const auto& [key, value] : parameters) {
                    ss << " " << key << "=" << escapeValue(value);
                }
                return ss.str();
            }

        private:
            static std::string escapeValue(const std::string& value) {
                if (value.find(' ') != std::string::npos ||
                    value.find('=') != std::string::npos ||
                    value.find('\n') != std::string::npos) {
                    // Quote values containing special characters
                    std::string escaped = "\"";
                    for (char c : value) {
                        if (c == '"' || c == '\\') {
                            escaped += '\\';
                        }
                        escaped += c;
                    }
                    escaped += "\"";
                    return escaped;
                }
                return value;
            }
        };

        /**
         * Parse a protocol message from a line of text
         */
        static Message parse(const std::string& line);

        /**
         * Send a message to stdout
         */
        static void send(const Message& message);

        /**
         * Send a simple OK response
         */
        static void sendOK();

        /**
         * Send an error response
         */
        static void sendError(const std::string& message);

        /**
         * Send a stopped event
         */
        static void sendStoppedEvent(const std::string& reason, const SourceLocation& location);

        /**
         * Send a stack trace response
         */
        static void sendStackTrace(const std::vector<CallFrame>& frames);

        /**
         * Send output event
         */
        static void sendOutput(const std::string& text, const std::string& category = "stdout");

        /**
         * Send variables response
         */
        static void sendVariables(const std::vector<std::tuple<std::string, std::string, std::string, int64_t>>& vars);

        /**
         * Send expanded variable response
         */
        static void sendExpandedVariable(const std::vector<std::tuple<std::string, std::string, std::string, int64_t>>& children);

        /**
         * Send evaluate result response
         */
        static void sendEvaluateResult(const std::string& result, const std::string& type, int64_t refId = 0);

    private:
        static std::string unescapeValue(const std::string& value);
        static std::ostream* protocolOutputStream;
    };

    // Forward declarations
    class VariableInspector;
    class VMVariableInspector;

} // namespace debugger

// Forward declarations
namespace environment {
    class Environment;
}

namespace vm::runtime {
    class VirtualMachine;
}

namespace debugger {

    /**
     * Debug server that reads commands from stdin and sends responses/events to stdout
     */
    class DebugServer {
    private:
        bool running;
        DebugContext* debugContext;
        std::unique_ptr<VariableInspector> variableInspector;
        std::unique_ptr<VMVariableInspector> vmVariableInspector;
        std::shared_ptr<environment::Environment> currentEnvironment;
        std::shared_ptr<vm::runtime::VirtualMachine> currentVM;

    public:
        DebugServer();
        ~DebugServer();

        /**
         * Set the current environment for variable inspection (AST mode)
         */
        void setEnvironment(std::shared_ptr<environment::Environment> env);

        /**
         * Set the current VM for variable inspection (bytecode mode)
         */
        void setVM(std::shared_ptr<vm::runtime::VirtualMachine> vm);

        /**
         * Start the debug server loop
         * This will block and process commands from stdin
         */
        void run();

        /**
         * Stop the debug server
         */
        void stop();

    private:
        void processCommand(const DebugProtocol::Message& message);
        void handleSetBreakpoint(const DebugProtocol::Message& message);
        void handleClearBreakpoint(const DebugProtocol::Message& message);
        void handleClearFile(const DebugProtocol::Message& message);
        void handleContinue();
        void handleStepInto();
        void handleStepOver();
        void handleStepOut();
        void handleGetStackTrace();
        void handleGetVariables(const DebugProtocol::Message& message);
        void handleExpandVariable(const DebugProtocol::Message& message);
        void handleSetExceptionBreakpoints(const DebugProtocol::Message& message);
        void handleEvaluate(const DebugProtocol::Message& message);
        void handleStop();
    };

} // namespace debugger
