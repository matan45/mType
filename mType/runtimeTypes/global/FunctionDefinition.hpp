#pragma once
#include <vector>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../../ast/nodes/annotations/AnnotationNode.hpp"
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
        std::vector<ast::GenericTypeParameter> genericTypeParameters;  // Generic type parameters like <T, U>
        bool isAsync;  // NEW: Flag to indicate async function
        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> annotations;  // NEW: Annotations for this function

    public:
        explicit FunctionDefinition(const std::string& name) : Definition(name), returnType(ValueType::VOID), returnClassName(""), body(nullptr), isAsync(false) {}

        explicit FunctionDefinition(const std::string& name, ValueType retType, const std::vector<std::pair<std::string, ParameterType>>& params)
            : Definition(name), returnType(retType), returnClassName(""), parameters(params), body(nullptr), isAsync(false) {}

        // Constructor with return class name support
        explicit FunctionDefinition(const std::string& name, ValueType retType, const std::string& retClassName, const std::vector<std::pair<std::string, ParameterType>>& params)
            : Definition(name), returnType(retType), returnClassName(retClassName), parameters(params), body(nullptr), isAsync(false) {}

        // Backward compatibility constructor for old ValueType parameters
        explicit FunctionDefinition(const std::string& name, ValueType retType, const std::vector<std::pair<std::string, ValueType>>& params)
            : Definition(name), returnType(retType), returnClassName(""), parameters(ParameterTypeConverter::fromValueTypeVector(params)), body(nullptr), isAsync(false) {}

        ValueType getReturnType() const { return returnType; }
        void setReturnType(ValueType type) { returnType = type; }

        const std::string& getReturnClassName() const { return returnClassName; }
        void setReturnClassName(const std::string& className) { returnClassName = className; }
        
        const std::vector<std::pair<std::string, ParameterType>>& getParameters() const { return parameters; }
        void setParameters(const std::vector<std::pair<std::string, ParameterType>>& params) { parameters = params; }

        // NEW: Alias for clarity in overload resolution code
        const std::vector<std::pair<std::string, ParameterType>>& getParametersWithTypes() const { return parameters; }

        // Backward compatibility methods for old ValueType format
        std::vector<std::pair<std::string, ValueType>> getParametersAsValueType() const {
            return ParameterTypeConverter::toValueTypeVector(parameters);
        }
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params) {
            parameters = ParameterTypeConverter::fromValueTypeVector(params);
        }
        
        size_t getParameterCount() const { return parameters.size(); }

        std::shared_ptr<ASTNode> getBody() const { return body; }
        void setBody(std::shared_ptr<ASTNode> bodyNode) { body = bodyNode; }

        // Generic type support
        const std::vector<ast::GenericTypeParameter>& getGenericTypeParameters() const { return genericTypeParameters; }
        void setGenericTypeParameters(const std::vector<ast::GenericTypeParameter>& genTypeParams) { genericTypeParameters = genTypeParams; }
        bool isGeneric() const { return !genericTypeParameters.empty(); }

        // NEW: Async support
        bool getIsAsync() const { return isAsync; }
        void setIsAsync(bool async) { isAsync = async; }

        // NEW: Annotation methods
        const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>& getAnnotations() const { return annotations; }
        void addAnnotation(std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation) { annotations.push_back(annotation); }
        bool hasAnnotation(const std::string& annotationName) const {
            for (const auto& annotation : annotations) {
                if (annotation->getName() == annotationName) {
                    return true;
                }
            }
            return false;
        }
        std::shared_ptr<ast::nodes::annotations::AnnotationNode> getAnnotation(const std::string& annotationName) const {
            for (const auto& annotation : annotations) {
                if (annotation->getName() == annotationName) {
                    return annotation;
                }
            }
            return nullptr;
        }
    };
}
