#include "DebugProtocol.hpp"
#include <cstddef>
#include "DebugExpressionEvaluator.hpp"
#include "VariableInspector.hpp"
#include "VMVariableInspector.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../environment/manager/VariableManager.hpp"
#include "../environment/manager/ScopeManager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <sys/select.h>
#include <unistd.h>
#endif

namespace debugger {

    namespace {
        struct BreakpointValidation {
            bool verified = false;
            std::string message;
        };

        BreakpointValidation validateBreakpointLocation(const std::string& file, int line) {
            if (file.empty() || line <= 0) {
                return {false, "Invalid breakpoint location"};
            }

            std::error_code ec;
            if (!std::filesystem::exists(file, ec) || ec) {
                return {false, "Source file not found"};
            }

            std::ifstream in(file);
            if (!in.is_open()) {
                return {false, "Source file cannot be opened"};
            }

            int lineCount = 0;
            std::string unused;
            while (std::getline(in, unused)) {
                ++lineCount;
            }

            if (line > lineCount) {
                return {false, "Line is outside the source file"};
            }

            return {true, ""};
        }

        bool waitForProtocolInput(bool& running) {
#ifdef _WIN32
            HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
            if (stdinHandle == INVALID_HANDLE_VALUE || stdinHandle == nullptr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return false;
            }

            DWORD waitResult = WaitForSingleObject(stdinHandle, 10);
            if (waitResult == WAIT_OBJECT_0) {
                return true;
            }
            if (waitResult == WAIT_TIMEOUT) {
                return false;
            }

            running = false;
            return false;
#else
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(STDIN_FILENO, &readSet);

            timeval timeout{};
            timeout.tv_usec = 10'000;

            int result = select(STDIN_FILENO + 1, &readSet, nullptr, nullptr, &timeout);
            if (result > 0 && FD_ISSET(STDIN_FILENO, &readSet)) {
                return true;
            }
            if (result < 0 && errno != EINTR) {
                running = false;
            }
            return false;
#endif
        }
    }

    std::atomic<std::ostream*> DebugProtocol::protocolOutputStream{nullptr};

    void DebugProtocol::setProtocolStream(std::ostream* stream) {
        protocolOutputStream.store(stream, std::memory_order_release);
    }

    DebugProtocol::Message DebugProtocol::parse(const std::string& line) {
        Message msg;
        std::string trimmedLine = line;

        // Remove leading/trailing whitespace
        size_t start = trimmedLine.find_first_not_of(" \t\r\n");
        size_t end = trimmedLine.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            trimmedLine = trimmedLine.substr(start, end - start + 1);
        }

        if (trimmedLine.empty()) {
            return msg;
        }

        // Extract command (first word)
        size_t cmdEnd = trimmedLine.find(' ');
        if (cmdEnd == std::string::npos) {
            msg.command = trimmedLine;
            return msg;
        }

        msg.command = trimmedLine.substr(0, cmdEnd);

        // Parse key=value pairs
        size_t pos = cmdEnd + 1;
        while (pos < trimmedLine.length()) {
            // Skip whitespace
            while (pos < trimmedLine.length() && (trimmedLine[pos] == ' ' || trimmedLine[pos] == '\t')) {
                pos++;
            }

            if (pos >= trimmedLine.length()) break;

            // Find key
            size_t eqPos = trimmedLine.find('=', pos);
            if (eqPos == std::string::npos) break;

            std::string key = trimmedLine.substr(pos, eqPos - pos);

            // Find value
            pos = eqPos + 1;
            std::string value;

            if (pos < trimmedLine.length() && trimmedLine[pos] == '"') {
                // Quoted value
                pos++; // Skip opening quote
                bool escaped = false;
                while (pos < trimmedLine.length()) {
                    char c = trimmedLine[pos];
                    if (escaped) {
                        if (c == 'n') {
                            value += '\n';
                        } else if (c == 'r') {
                            value += '\r';
                        } else if (c == 't') {
                            value += '\t';
                        } else {
                            value += c;
                        }
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        pos++; // Skip closing quote
                        break;
                    } else {
                        value += c;
                    }
                    pos++;
                }
            } else {
                // Unquoted value - read until space
                size_t valueEnd = trimmedLine.find(' ', pos);
                if (valueEnd == std::string::npos) {
                    value = trimmedLine.substr(pos);
                    pos = trimmedLine.length();
                } else {
                    value = trimmedLine.substr(pos, valueEnd - pos);
                    pos = valueEnd;
                }
            }

            msg.parameters[key] = value;
        }

        return msg;
    }

    void DebugProtocol::send(const Message& message) {
        auto* stream = protocolOutputStream.load(std::memory_order_acquire);
        std::ostream& out = stream ? *stream : std::cout;
        out << message.serialize() << std::endl;
        out.flush();
    }

    void DebugProtocol::sendOK() {
        Message msg("OK");
        send(msg);
    }

    void DebugProtocol::sendOK(const std::map<std::string, std::string>& parameters) {
        Message msg("OK");
        for (const auto& [key, value] : parameters) {
            msg.addParameter(key, value);
        }
        send(msg);
    }

    void DebugProtocol::sendError(const std::string& errorMessage) {
        Message msg("ERROR");
        msg.addParameter("message", errorMessage);
        send(msg);
    }

    void DebugProtocol::sendStoppedEvent(const std::string& reason,
                                         const SourceLocation& location,
                                         const std::string& message) {
        Message msg("STOPPED");
        msg.addParameter("reason", reason);
        msg.addParameter("file", location.getFilename());
        msg.addParameter("line", location.getLine());
        msg.addParameter("column", location.getColumn());
        if (!message.empty()) {
            msg.addParameter("message", message);
        }
        send(msg);
    }

    void DebugProtocol::sendStackTrace(const std::vector<CallFrame>& frames) {
        Message msg("STACKTRACE");
        for (size_t i = 0; i < frames.size(); i++) {
            const CallFrame& frame = frames[i];
            std::string frameStr = frame.functionName + "@" +
                                 frame.location.getFilename() + ":" +
                                 std::to_string(frame.location.getLine());
            msg.addParameter("frame" + std::to_string(i), frameStr);
        }
        send(msg);
    }

    void DebugProtocol::sendOutput(const std::string& text, const std::string& category) {
        Message msg("OUTPUT");
        msg.addParameter("text", text);
        msg.addParameter("category", category);
        send(msg);
    }

    void DebugProtocol::sendVariables(
        const std::vector<std::tuple<std::string, std::string, std::string, int64_t>>& vars) {

        Message msg("VARIABLES");
        msg.addParameter("count", static_cast<int>(vars.size()));
        for (size_t i = 0; i < vars.size(); i++) {
            const auto& [name, value, type, refId] = vars[i];
            std::string varStr = name + "=" + value + ":" + type + ":" + std::to_string(refId);
            msg.addParameter("var" + std::to_string(i), varStr);
            const std::string prefix = "var" + std::to_string(i) + "_";
            msg.addParameter(prefix + "name", name);
            msg.addParameter(prefix + "value", value);
            msg.addParameter(prefix + "type", type);
            msg.addParameter(prefix + "ref", refId);
        }
        send(msg);
    }

    void DebugProtocol::sendExpandedVariable(
        const std::vector<std::tuple<std::string, std::string, std::string, int64_t>>& children) {

        Message msg("EXPANDEDVAR");
        msg.addParameter("count", static_cast<int>(children.size()));
        for (size_t i = 0; i < children.size(); i++) {
            const auto& [name, value, type, refId] = children[i];
            std::string childStr = name + "=" + value + ":" + type + ":" + std::to_string(refId);
            msg.addParameter("child" + std::to_string(i), childStr);
            const std::string prefix = "child" + std::to_string(i) + "_";
            msg.addParameter(prefix + "name", name);
            msg.addParameter(prefix + "value", value);
            msg.addParameter(prefix + "type", type);
            msg.addParameter(prefix + "ref", refId);
        }
        send(msg);
    }

    void DebugProtocol::sendEvaluateResult(const std::string& result, const std::string& type, int64_t refId) {
        Message msg("RESULT");
        msg.addParameter("value", result);
        msg.addParameter("type", type);
        if (refId > 0) {
            msg.addParameter("ref", refId);
        }
        send(msg);
    }

    // DebugServer implementation

    DebugServer::DebugServer()
        : running(false), debugContext(&DebugContext::getInstance()),
          variableInspector(std::make_unique<VariableInspector>()),
          vmVariableInspector(std::make_unique<VMVariableInspector>()) {
    }

    DebugServer::~DebugServer() {
        stop();
    }

    void DebugServer::setEnvironment(std::shared_ptr<environment::Environment> env) {
        currentEnvironment = env;
    }

    void DebugServer::setVM(std::shared_ptr<vm::runtime::VirtualMachine> vm) {
        currentVM = vm;
        if (debugContext) {
            debugContext->setConditionEvaluator([this](const std::string& condition) {
                if (!currentVM || !vmVariableInspector) {
                    return true;
                }

                auto result = DebugExpressionEvaluator::evaluate(currentVM, *vmVariableInspector, condition);
                if (!result.success) {
                    DebugProtocol::sendOutput("Conditional breakpoint evaluation failed: " + result.error, "stderr");
                    return true;
                }

                return DebugExpressionEvaluator::isTruthy(result.value);
            });
        }
    }

    void DebugServer::run() {
        running = true;

        // Set up event callback
        debugContext->setEventCallback([](const DebugEvent& event) {
            switch (event.type) {
                case DebugEvent::Type::BREAKPOINT_HIT:
                    DebugProtocol::sendStoppedEvent("breakpoint", event.location);
                    break;
                case DebugEvent::Type::STEP_COMPLETE:
                    DebugProtocol::sendStoppedEvent("step", event.location);
                    break;
                case DebugEvent::Type::EXCEPTION_THROWN:
                    DebugProtocol::sendStoppedEvent("exception", event.location, event.message);
                    DebugProtocol::sendOutput(event.message, "stderr");
                    break;
                case DebugEvent::Type::SCRIPT_STARTED:
                    DebugProtocol::sendOutput(event.message, "console");
                    break;
                case DebugEvent::Type::SCRIPT_COMPLETE: {
                    DebugProtocol::sendOutput(event.message, "console");
                    DebugProtocol::Message msg("TERMINATED");
                    DebugProtocol::send(msg);
                    break;
                }
            }
        });

        // Read commands from stdin without blocking shutdown. On Windows, VS Code
        // talks to us through anonymous pipes where streambuf::in_avail() can
        // stay at zero even after data is written.
        std::string line;
        while (running) {
            if (!waitForProtocolInput(running)) {
                continue;
            }

            if (!std::getline(std::cin, line)) {
                break;
            }

            if (line.empty()) continue;

            DebugProtocol::Message msg = DebugProtocol::parse(line);
            if (!msg.command.empty()) {
                processCommand(msg);
            }
        }
    }

    void DebugServer::stop() {
        running = false;
        if (debugContext) {
            debugContext->setConditionEvaluator(nullptr);
            debugContext->stop();
        }
    }

    void DebugServer::processCommand(const DebugProtocol::Message& message) {
        const std::string& cmd = message.command;

        try {
            if (cmd == "SETBREAKPOINT") {
                handleSetBreakpoint(message);
            } else if (cmd == "CLEARBREAKPOINT") {
                handleClearBreakpoint(message);
            } else if (cmd == "CLEARALL") {
                debugContext->clearAllBreakpoints();
                DebugProtocol::sendOK();
            } else if (cmd == "CLEARFILE") {
                handleClearFile(message);
            } else if (cmd == "CONTINUE") {
                handleContinue();
            } else if (cmd == "STEPINTO") {
                handleStepInto();
            } else if (cmd == "STEPOVER") {
                handleStepOver();
            } else if (cmd == "STEPOUT") {
                handleStepOut();
            } else if (cmd == "GETSTACKTRACE") {
                handleGetStackTrace();
            } else if (cmd == "GETVARIABLES") {
                handleGetVariables(message);
            } else if (cmd == "EXPANDVARIABLE") {
                handleExpandVariable(message);
            } else if (cmd == "SETEXCEPTIONBREAKPOINTS") {
                handleSetExceptionBreakpoints(message);
            } else if (cmd == "EVALUATE") {
                handleEvaluate(message);
            } else if (cmd == "STOP") {
                handleStop();
            } else {
                DebugProtocol::sendError("Unknown command: " + cmd);
            }
        } catch (const std::exception& e) {
            DebugProtocol::sendError(std::string("Command error: ") + e.what());
        }
    }

    void DebugServer::handleSetBreakpoint(const DebugProtocol::Message& message) {
        std::string file = message.getParameter("file");
        int line = message.getIntParameter("line", -1);
        std::string condition = message.getParameter("condition");      // Optional
        std::string logMessage = message.getParameter("logMessage");    // Optional

        if (file.empty() || line <= 0) {
            DebugProtocol::sendError("Invalid breakpoint parameters");
            return;
        }

        auto validation = validateBreakpointLocation(file, line);
        if (validation.verified) {
            debugContext->addBreakpoint(file, line, condition, logMessage);
        }

        DebugProtocol::sendOK({
            {"verified", validation.verified ? "true" : "false"},
            {"line", std::to_string(line)},
            {"message", validation.message}
        });
    }

    void DebugServer::handleClearBreakpoint(const DebugProtocol::Message& message) {
        std::string file = message.getParameter("file");
        int line = message.getIntParameter("line", -1);

        if (file.empty() || line < 0) {
            DebugProtocol::sendError("Invalid breakpoint parameters");
            return;
        }

        debugContext->removeBreakpoint(file, line);
        DebugProtocol::sendOK();
    }

    void DebugServer::handleClearFile(const DebugProtocol::Message& message) {
        std::string file = message.getParameter("file");

        if (file.empty()) {
            DebugProtocol::sendError("Invalid file parameter");
            return;
        }

        debugContext->clearBreakpoints(file);
        DebugProtocol::sendOK();
    }

    void DebugServer::handleContinue() {
        // Clear variable cache when resuming execution
        if (vmVariableInspector) {
            vmVariableInspector->clearCache();
        }
        debugContext->continueExecution();
        DebugProtocol::sendOK();
    }

    void DebugServer::handleStepInto() {
        // Clear variable cache when stepping
        if (vmVariableInspector) {
            vmVariableInspector->clearCache();
        }
        debugContext->stepInto();
        DebugProtocol::sendOK();
    }

    void DebugServer::handleStepOver() {
        // Clear variable cache when stepping
        if (vmVariableInspector) {
            vmVariableInspector->clearCache();
        }
        debugContext->stepOver();
        DebugProtocol::sendOK();
    }

    void DebugServer::handleStepOut() {
        // Clear variable cache when stepping
        if (vmVariableInspector) {
            vmVariableInspector->clearCache();
        }
        debugContext->stepOut();
        DebugProtocol::sendOK();
    }

    void DebugServer::handleGetStackTrace() {
        std::vector<CallFrame> frames = debugContext->getCallStack();
        DebugProtocol::sendStackTrace(frames);
    }

    void DebugServer::handleGetVariables(const DebugProtocol::Message& message) {
        std::string scope = message.getParameter("scope", "local");

        std::vector<DebugVariable> variables;

        // Check if we're in bytecode mode (VM available)
        if (currentVM) {
            if (!vmVariableInspector) {
                DebugProtocol::sendError("No VM variable inspector available");
                return;
            }

            if (scope == "local") {
                variables = vmVariableInspector->getLocalVariables(currentVM);
            } else if (scope == "global") {
                variables = vmVariableInspector->getGlobalVariables(currentVM);
            } else if (scope == "static") {
                variables = vmVariableInspector->getStaticVariables(currentVM);
            } else {
                DebugProtocol::sendError("Invalid scope: " + scope);
                return;
            }
        }
        // AST mode (Environment-based)
        else if (currentEnvironment) {
            if (!variableInspector) {
                DebugProtocol::sendError("No variable inspector available");
                return;
            }

            if (scope == "local") {
                variables = variableInspector->getLocalVariables(currentEnvironment);
            } else if (scope == "global") {
                variables = variableInspector->getGlobalVariables(currentEnvironment);
            } else if (scope == "static") {
                // For AST mode, static variables not yet supported
                DebugProtocol::sendError("Static scope not supported in AST mode");
                return;
            } else {
                DebugProtocol::sendError("Invalid scope: " + scope);
                return;
            }
        }
        else {
            DebugProtocol::sendError("No environment or VM available for variable inspection");
            return;
        }

        // Convert to protocol format: (name, value, type, refId)
        std::vector<std::tuple<std::string, std::string, std::string, int64_t>> varList;
        for (const auto& var : variables) {
            varList.emplace_back(var.name, var.value, var.type, var.referenceId);
        }

        DebugProtocol::sendVariables(varList);
    }

    void DebugServer::handleExpandVariable(const DebugProtocol::Message& message) {
        int64_t refId = message.getIntParameter("ref", 0);
        if (refId == 0) {
            DebugProtocol::sendError("Invalid reference ID");
            return;
        }

        std::vector<DebugVariable> children;

        // Check if we're in bytecode mode (VM available)
        if (currentVM) {
            if (!vmVariableInspector) {
                DebugProtocol::sendError("No VM variable inspector available");
                return;
            }
            children = vmVariableInspector->getVariableChildren(currentVM, refId);
        }
        // AST mode (Environment-based)
        else if (currentEnvironment) {
            if (!variableInspector) {
                DebugProtocol::sendError("No variable inspector available");
                return;
            }
            children = variableInspector->getVariableChildren(refId);
        }
        else {
            DebugProtocol::sendError("No environment or VM available");
            return;
        }

        // Convert to protocol format
        std::vector<std::tuple<std::string, std::string, std::string, int64_t>> childList;
        for (const auto& child : children) {
            childList.emplace_back(child.name, child.value, child.type, child.referenceId);
        }

        DebugProtocol::sendExpandedVariable(childList);
    }

    void DebugServer::handleSetExceptionBreakpoints(const DebugProtocol::Message& message) {
        std::vector<std::string> filters;

        // Parse filter parameters (filter0=all, filter1=uncaught, etc.)
        int i = 0;
        while (true) {
            std::string filterKey = "filter" + std::to_string(i);
            std::string filter = message.getParameter(filterKey);
            if (filter.empty()) {
                break;
            }
            filters.push_back(filter);
            i++;
        }

        debugContext->setExceptionBreakpoints(filters);
        DebugProtocol::sendOK();
    }

    void DebugServer::handleEvaluate(const DebugProtocol::Message& message) {
        std::string expression = message.getParameter("expr");
        (void)message.getIntParameter("frame", 0);

        if (expression.empty()) {
            DebugProtocol::sendError("Empty expression");
            return;
        }

        try {
            size_t start = expression.find_first_not_of(" \t\r\n");
            size_t end = expression.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) {
                DebugProtocol::sendError("Empty expression");
                return;
            }
            std::string trimmedExpression = expression.substr(start, end - start + 1);

            if (currentVM && vmVariableInspector) {
                auto result = DebugExpressionEvaluator::evaluate(currentVM, *vmVariableInspector, trimmedExpression);
                if (!result.success) {
                    DebugProtocol::sendError("Evaluation error: " + result.error);
                    return;
                }

                auto debugVar = vmVariableInspector->formatValue(trimmedExpression, result.value);
                DebugProtocol::sendEvaluateResult(debugVar.value, debugVar.type, debugVar.referenceId);
                return;
            }

            // Fallback to AST mode (Environment-based)
            if (!currentEnvironment || !variableInspector) {
                DebugProtocol::sendError("Environment not available");
                return;
            }

            auto scopeManager = currentEnvironment->getScopeManager();
            if (!scopeManager) {
                DebugProtocol::sendError("No scope manager available");
                return;
            }

            auto varDef = scopeManager->findVariable(trimmedExpression);
            if (!varDef) {
                DebugProtocol::sendError("Variable not found: " + trimmedExpression);
                return;
            }

            // Format and send the result
            auto debugVar = variableInspector->formatValue(trimmedExpression, varDef->getValue());
            DebugProtocol::sendEvaluateResult(debugVar.value, debugVar.type, debugVar.referenceId);

        } catch (const std::exception& e) {
            DebugProtocol::sendError(std::string("Evaluation error: ") + e.what());
        }
    }

    void DebugServer::handleStop() {
        DebugProtocol::sendOK();
        stop();
    }

} // namespace debugger
