#include "DebugProtocol.hpp"
#include <atomic>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <vector>

namespace debugger {

    std::atomic<std::ostream*> DebugProtocol::protocolOutputStream{nullptr};
    std::mutex DebugProtocol::protocolWriterMutex;
    DebugProtocol::ProtocolWriter DebugProtocol::protocolWriter{};

    void DebugProtocol::setProtocolStream(std::ostream* stream) {
        protocolOutputStream.store(stream, std::memory_order_release);
    }

    void DebugProtocol::setProtocolWriter(ProtocolWriter writer) {
        std::lock_guard<std::mutex> lock(protocolWriterMutex);
        protocolWriter = std::move(writer);
    }

    DebugProtocol::Message DebugProtocol::parse(const std::string& line) {
        Message msg;
        std::string trimmedLine = line;

        size_t start = trimmedLine.find_first_not_of(" \t\r\n");
        size_t end = trimmedLine.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            trimmedLine = trimmedLine.substr(start, end - start + 1);
        }

        if (trimmedLine.empty()) {
            return msg;
        }

        size_t cmdEnd = trimmedLine.find(' ');
        if (cmdEnd == std::string::npos) {
            msg.command = trimmedLine;
            return msg;
        }

        msg.command = trimmedLine.substr(0, cmdEnd);

        size_t pos = cmdEnd + 1;
        while (pos < trimmedLine.length()) {
            while (pos < trimmedLine.length() && (trimmedLine[pos] == ' ' || trimmedLine[pos] == '\t')) {
                pos++;
            }

            if (pos >= trimmedLine.length()) break;

            size_t eqPos = trimmedLine.find('=', pos);
            if (eqPos == std::string::npos) break;

            std::string key = trimmedLine.substr(pos, eqPos - pos);

            pos = eqPos + 1;
            std::string value;

            if (pos < trimmedLine.length() && trimmedLine[pos] == '"') {
                pos++;
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
                        pos++;
                        break;
                    } else {
                        value += c;
                    }
                    pos++;
                }
            } else {
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
        const std::string line = message.serialize();
        {
            std::lock_guard<std::mutex> lock(protocolWriterMutex);
            if (protocolWriter) {
                protocolWriter(line);
                return;
            }
        }
        auto* stream = protocolOutputStream.load(std::memory_order_acquire);
        std::ostream& out = stream ? *stream : std::cout;
        out << line << std::endl;
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
}
