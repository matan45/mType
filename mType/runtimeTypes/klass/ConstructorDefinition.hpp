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
        std::vector<std::pair<std::string, ParameterType>> parametersWithTypes;
        std::shared_ptr<ASTNode> body;
        std::shared_ptr<ASTNode> initializerList;  // For member initialization
        std::shared_ptr<::ast::nodes::classes::SuperConstructorCallNode> superInitializer;

        // Cached computed property - lazily computed from parametersWithTypes
        mutable std::vector<std::pair<std::string, ValueType>> cachedParameters;
        mutable bool parametersCacheValid = false;

      public:
       // Constructor with ParameterType (preserves class/interface information)
       explicit ConstructorDefinition(const std::vector<std::pair<std::string, ParameterType>>& params,
                             std::shared_ptr<ASTNode> b)
            : Definition("constructor"), parametersWithTypes(params), body(b),
              initializerList(nullptr), superInitializer(nullptr) {}

        bool matchesArgCount(size_t argCount) const;

        // Computed property - derives from parametersWithTypes
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const;

        // Get parameters with full type information
        const std::vector<std::pair<std::string, ParameterType>>& getParametersWithTypes() const { return parametersWithTypes; }
        bool hasParametersWithTypes() const { return !parametersWithTypes.empty(); }

        ASTNode* getBody() const { return body.get(); }
        std::shared_ptr<ASTNode> getBodyPtr() const { return body; }
        ASTNode* getInitializerList() const { return initializerList.get(); }
        size_t getParameterCount() const { return parametersWithTypes.size(); }

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
    };
}
