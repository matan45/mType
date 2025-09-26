#pragma once
#include "../Definition.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../../ast/GenericType.hpp"
#include <vector>
#include <memory>

// Forward declarations
namespace runtimeTypes::klass {
    class ClassDefinition;
}

namespace ast::nodes::expressions {
    class LambdaNode;
}

namespace runtimeTypes::klass {
    
    // Method signature for interface methods
    struct MethodSignature {
        std::string name;
        std::shared_ptr<ast::GenericType> returnType;
        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> parameters;
        std::vector<ast::GenericTypeParameter> genericParameters;
    };
    
    class InterfaceDefinition : public Definition {
    private:
        std::vector<MethodSignature> methodSignatures;
        std::vector<ast::GenericTypeParameter> genericParameters;
        std::vector<std::string> extendsInterfaces; // Interface inheritance
        bool isGenericInterface;

    public:
        InterfaceDefinition(const std::string& name,
                           const std::vector<ast::GenericTypeParameter>& generics = {},
                           const std::vector<std::string>& extendedInterfaces = {})
            : Definition(name), genericParameters(generics), extendsInterfaces(extendedInterfaces),
              isGenericInterface(!generics.empty()) {}

        void addMethodSignature(const MethodSignature& signature) {
            methodSignatures.push_back(signature);
        }

        const std::vector<MethodSignature>& getMethodSignatures() const {
            return methodSignatures;
        }

        bool hasMethod(const std::string& methodName) const {
            for (const auto& sig : methodSignatures) {
                if (sig.name == methodName) return true;
            }
            return false;
        }

        /**
     * Check if this is a functional interface (SAM - Single Abstract Method)
     */
        bool isFunctionalInterface() const {
            return methodSignatures.size() == 1;
        }
    
        /**
         * Get the single method signature for functional interface
         * Returns nullptr if not a functional interface
         */
        const MethodSignature* getFunctionalMethod() const {
            if (isFunctionalInterface()) {
                return &methodSignatures[0];
            }
            return nullptr;
        }
    
        /**
         * Create anonymous class from lambda for this interface
         */
        std::shared_ptr<ClassDefinition> createLambdaImplementation(
            ast::nodes::expressions::LambdaNode* lambda) const;

        // Interface inheritance support
        void addExtendedInterface(const std::string& interfaceName) {
            extendsInterfaces.push_back(interfaceName);
        }

        const std::vector<std::string>& getExtendedInterfaces() const {
            return extendsInterfaces;
        }

        bool extendsInterface(const std::string& interfaceName) const {
            for (const auto& extendedInterface : extendsInterfaces) {
                if (extendedInterface == interfaceName) return true;
            }
            return false;
        }

        // Method signature compatibility checking
        MethodSignature* findMethod(const std::string& methodName, size_t paramCount) const {
            for (const auto& sig : methodSignatures) {
                if (sig.name == methodName && sig.parameters.size() == paramCount) {
                    return const_cast<MethodSignature*>(&sig);
                }
            }
            return nullptr;
        }

        bool isGeneric() const { return isGenericInterface; }
        size_t getGenericParameterCount() const { return genericParameters.size(); }
        const std::vector<ast::GenericTypeParameter>& getGenericParameters() const {
            return genericParameters;
        }
    };
}