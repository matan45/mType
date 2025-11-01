#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace mtype::lsp {

class JsonRpc {
public:
    static std::string createResponse(const json& id, const json& result) {
        json response = {
            {"jsonrpc", "2.0"},
            {"id", id},
            {"result", result}
        };
        return formatMessage(response);
    }

    static std::string createError(const json& id, int code, const std::string& message) {
        json response = {
            {"jsonrpc", "2.0"},
            {"id", id},
            {"error", {
                {"code", code},
                {"message", message}
            }}
        };
        return formatMessage(response);
    }

    static std::string createNotification(const std::string& method, const json& params) {
        json notification = {
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params}
        };
        return formatMessage(notification);
    }

    static std::string readMessage(std::istream& in) {
        std::string line;
        int contentLength = 0;

        // Read headers
        while (std::getline(in, line)) {
            if (line == "\r" || line.empty()) {
                break;
            }
            if (line.find("Content-Length: ") == 0) {
                contentLength = std::stoi(line.substr(16));
            }
        }

        if (contentLength == 0) {
            return "";
        }

        // Read content
        std::string content(contentLength, '\0');
        in.read(&content[0], contentLength);

        return content;
    }

    static void writeMessage(std::ostream& out, const std::string& message) {
        out << "Content-Length: " << message.length() << "\r\n\r\n";
        out << message;
        out.flush();
    }

private:
    static std::string formatMessage(const json& j) {
        return j.dump();
    }
};

} // namespace mtype::lsp
