#pragma once
#include <vector>
#include <string>
#include <utility>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../Definition.hpp"

namespace runtimeTypes::klass
{
    using namespace value;
    using namespace ast;

    class ConstructorDefinition : public Definition
    {
    private:
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::vector<std::pair<std::string, ParameterType>> parametersWithTypes;
        std::shared_ptr<ASTNode> body;
        std::shared_ptr<ASTNode> initializerList;  // For member initialization
        std::shared_ptr<::ast::nodes::classes::SuperConstructorCallNode> superInitializer; // Super initializer

        // NEW: Super constructor call tracking
        bool hasSuperConstructorCall;
        std::vector<std::shared_ptr<ASTNode>> superCallArgs;
      public:
       // Old constructor (backward compatibility)
       explicit ConstructorDefinition(const std::vector<std::pair<std::string, ValueType>>& params,
                             std::shared_ptr<ASTNode> b)
            : Definition("constructor"), parameters(params), body(b),
              initializerList(nullptr), superInitializer(nullptr), hasSuperConstructorCall(false) {}

       // NEW: Constructor with ParameterType (preserves class/interface information)
       explicit ConstructorDefinition(const std::vector<std::pair<std::string, ParameterType>>& params,
                             std::shared_ptr<ASTNode> b)
            : Definition("constructor"), parametersWithTypes(params), body(b),
              initializerList(nullptr), superInitializer(nullptr), hasSuperConstructorCall(false) {
            // Also populate the old parameters format for backward compatibility
            for (const auto& param : parametersWithTypes) {
                parameters.emplace_back(param.first, param.second.basicType);
            }
       }

        bool matchesArgCount(size_t argCount) const;

        // Getter methods
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const { return parameters; }

        // NEW: Get parameters with full type information
        const std::vector<std::pair<std::string, ParameterType>>& getParametersWithTypes() const { return parametersWithTypes; }
        bool hasParametersWithTypes() const { return !parametersWithTypes.empty(); }

        ASTNode* getBody() const { return body.get(); }
        std::shared_ptr<ASTNode> getBodyPtr() const { return body; }
        ASTNode* getInitializerList() const { return initializerList.get(); }
        size_t getParameterCount() const { return parameters.size(); }

        // Setter methods
        void setBody(std::shared_ptr<ASTNode> b) { body = b; }
        void setInitializerList(std::shared_ptr<ASTNode> init) { initializerList = init; }

        // Super initializer accessors
        void setSuperInitializer(std::shared_ptr<::ast::nodes::classes::SuperConstructorCallNode> superCall) {
            superInitializer = superCall;
        }
        ::ast::nodes::classes::SuperConstructorCallNode* getSuperInitializer() const {
            return superInitializer.get();
        }
        bool hasSuperInitializer() const {
            return superInitializer != nullptr;
        }

        // NEW: Super constructor call methods (legacy)
        bool hasSuperCall() const { return hasSuperConstructorCall; }
        void setHasSuperCall(bool hasSuper) { hasSuperConstructorCall = hasSuper; }
        const std::vector<std::shared_ptr<ASTNode>>& getSuperArgs() const { return superCallArgs; }
        void setSuperArgs(const std::vector<std::shared_ptr<ASTNode>>& args) { superCallArgs = args; }
    };
}
