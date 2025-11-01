#pragma once

#include "../../../mType/environment/Environment.hpp"
#include "../../../mType/ast/ASTNode.hpp"
#include <memory>
#include <unordered_map>
#include <string>

namespace mtype::lsp {

// Forward declaration of shared symbol location type
struct SymbolLocationInfo;

/// @brief Utility that traverses the AST and registers symbols in the environment
/// Registers classes, functions, and interfaces with their locations
class SymbolRegistrationVisitor {
public:
    explicit SymbolRegistrationVisitor(std::shared_ptr<environment::Environment> env);

    // Process a program node and register all symbols
    void processProgram(ast::ASTNode* programNode, const std::string& uri);

    const std::unordered_map<std::string, SymbolLocationInfo>& getSymbolLocations() const {
        return symbolLocations_;
    }

private:
    std::shared_ptr<environment::Environment> environment_;
    std::unordered_map<std::string, SymbolLocationInfo> symbolLocations_;
    std::string currentUri_;

    void processStatement(ast::ASTNode* node);
    void processClassNode(ast::ASTNode* node);
    void processInterfaceNode(ast::ASTNode* node);
    void processFunctionNode(ast::ASTNode* node);
};

} // namespace mtype::lsp
