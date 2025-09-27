#pragma once
#include <vector>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../Definition.hpp"

namespace runtimeTypes::global
{
    using namespace value;
    using namespace ast;
    
    class FunctionDefinition : public Definition
    {
    private:
        ValueType returnType;
        std::string returnClassName;  // For object return types, stores the class/interface name
        std::vector<std::pair<std::string, ParameterType>> parameters;
        std::shared_ptr<ASTNode> body;
        
    public:
        explicit FunctionDefinition(const std::string& name) : Definition(name), returnType(ValueType::VOID), returnClassName(""), body(nullptr) {}

        explicit FunctionDefinition(const std::string& name, ValueType retType, const std::vector<std::pair<std::string, ParameterType>>& params)
            : Definition(name), returnType(retType), returnClassName(""), parameters(params), body(nullptr) {}

        // Constructor with return class name support
        explicit FunctionDefinition(const std::string& name, ValueType retType, const std::string& retClassName, const std::vector<std::pair<std::string, ParameterType>>& params)
            : Definition(name), returnType(retType), returnClassName(retClassName), parameters(params), body(nullptr) {}

        // Backward compatibility constructor for old ValueType parameters
        explicit FunctionDefinition(const std::string& name, ValueType retType, const std::vector<std::pair<std::string, ValueType>>& params)
            : Definition(name), returnType(retType), returnClassName(""), parameters(ParameterType::fromValueTypeVector(params)), body(nullptr) {}

        ValueType getReturnType() const { return returnType; }
        void setReturnType(ValueType type) { returnType = type; }

        const std::string& getReturnClassName() const { return returnClassName; }
        void setReturnClassName(const std::string& className) { returnClassName = className; }
        
        const std::vector<std::pair<std::string, ParameterType>>& getParameters() const { return parameters; }
        void setParameters(const std::vector<std::pair<std::string, ParameterType>>& params) { parameters = params; }

        // Backward compatibility methods for old ValueType format
        std::vector<std::pair<std::string, ValueType>> getParametersAsValueType() const {
            return ParameterType::toValueTypeVector(parameters);
        }
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params) {
            parameters = ParameterType::fromValueTypeVector(params);
        }
        
        size_t getParameterCount() const { return parameters.size(); }
        
        std::shared_ptr<ASTNode> getBody() const { return body; }
        void setBody(std::shared_ptr<ASTNode> bodyNode) { body = bodyNode; }
    };
}
