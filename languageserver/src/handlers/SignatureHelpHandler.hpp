#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../utils/LSPTypes.hpp"
#include "../DocumentManager.hpp"

namespace mtype::lsp {

struct ParameterInfo {
    std::string name;
    std::string type;
};

struct SignatureInfo {
    std::string name;
    std::vector<ParameterInfo> parameters;
    std::string returnType;
    bool isStatic = false;
};

struct SignatureHelp {
    std::vector<SignatureInfo> signatures;
    int activeSignature = 0;
    int activeParameter = 0;

    json toJson() const;
};

class SignatureHelpHandler {
public:
    explicit SignatureHelpHandler(DocumentManager* docMgr);

    std::optional<SignatureHelp> handleSignatureHelp(
        const std::string& uri,
        const Position& position
    );

private:
    // Parse text before cursor to find function call context
    struct FunctionCallContext {
        std::string functionName;
        bool isMethodCall = false;
        std::string objectName;   // for method calls: the object/class name
    };

    std::optional<FunctionCallContext> findFunctionCall(const std::string& textBeforeCursor) const;

    // Find method signatures from the document's parsed symbols
    std::vector<SignatureInfo> findMethodSignatures(
        const std::string& uri,
        const std::string& objectName,
        const std::string& methodName,
        int line
    ) const;

    // Find function/constructor signatures
    std::vector<SignatureInfo> findFunctionSignatures(
        const std::string& uri,
        const std::string& functionName,
        int line
    ) const;

    // Get built-in function signatures
    std::optional<SignatureInfo> getBuiltInSignature(const std::string& name) const;

    // Count the active parameter index from text after the opening parenthesis
    int getActiveParameter(const std::string& textBeforeCursor) const;

    DocumentManager* documentManager_;
};

} // namespace mtype::lsp
