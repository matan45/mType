#pragma once
#include "../core/Value.hpp"
#include "../core/Environment.hpp"
#include "../ast/AstNode.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace mtype {
    
    // Forward declarations
    class Interpreter;
    
    namespace runtime {
        using namespace core;
        using namespace ast;

		class MTypeFunction;
		class MTypeNativeFunction;
		class MTypeClass;
		class MTypeInstance;
		class MTypeEnum;
		class MTypeConstructor;
		class MTypeBoundMethod;
        
        // Method modifiers
        enum MethodModifier {
            METHOD_NONE = 0,
            METHOD_ABSTRACT = 1 << 0,
            METHOD_VIRTUAL = 1 << 1,
            METHOD_OVERRIDE = 1 << 2,
            METHOD_FINAL = 1 << 3,
            METHOD_STATIC = 1 << 4
        };
        
        // Function parameter definition
        struct FunctionParameter {
            std::string name;
            ValueType type;
            bool hasDefault;
            Value defaultValue;
            
            FunctionParameter(const std::string& n, ValueType t)
                : name(n), type(t), hasDefault(false) {}
                
            FunctionParameter(const std::string& n, ValueType t, const Value& def)
                : name(n), type(t), hasDefault(true), defaultValue(def) {}
        };
        
        // Base class for all callable entities
        class MTypeCallable {
        public:
            virtual ~MTypeCallable() = default;
            
            // Pure virtual methods
            virtual Result<Value> call(Interpreter* interpreter, 
                                       const std::vector<Value>& args) = 0;
            virtual size_t arity() const = 0;
            virtual std::string toString() const = 0;
            virtual bool isNative() const = 0;
        };
        
        // Native function wrapper
        class MTypeNativeFunction : public MTypeCallable {
        private:
            std::string name;
            NativeFunctionPtr function;
            size_t minArity;
            size_t maxArity;
            std::vector<ValueType> paramTypes;
            ValueType returnType;
            std::string documentation;
            bool variadic;
            
        public:
            // Constructor for fixed arity
            MTypeNativeFunction(const std::string& n, 
                               NativeFunctionPtr f, 
                               size_t arity,
                               const std::vector<ValueType>& pt = {},
                               ValueType rt = ValueType::V_NULL,
                               const std::string& doc = "");
            
            // Constructor for variable arity (min/max)
            MTypeNativeFunction(const std::string& n,
                               NativeFunctionPtr f,
                               size_t minArity,
                               size_t maxArity,
                               const std::vector<ValueType>& pt = {},
                               ValueType rt = ValueType::V_NULL,
                               const std::string& doc = "");
            
            // Constructor for variadic functions
            MTypeNativeFunction(const std::string& n,
                               NativeFunctionPtr f,
                               size_t minArity,
                               bool variadic,
                               const std::vector<ValueType>& pt = {},
                               ValueType rt = ValueType::V_NULL,
                               const std::string& doc = "");
            
            // Override methods from MTypeCallable
            Result<Value> call(Interpreter* interpreter, 
                              const std::vector<Value>& args) override;
            size_t arity() const override { return minArity; }
            std::string toString() const override;
            bool isNative() const override { return true; }
            
            // Native function specific methods
            Result<void> validateArgs(const std::vector<Value>& args) const;
            std::string getName() const { return name; }
            std::string getDocumentation() const { return documentation; }
            ValueType getReturnType() const { return returnType; }
            bool isVariadic() const { return variadic; }
            size_t getMinArity() const { return minArity; }
            size_t getMaxArity() const { return maxArity; }
        };
        
        // User-defined function
        class MTypeFunction : public MTypeCallable {
        private:
            std::string name;
            std::vector<FunctionParameter> parameters;
            ASTNodePtr body;
            std::shared_ptr<Environment> closure;
            ValueType returnType;
            bool isGenerator;
            bool isAsync;
            int modifiers;
            std::string documentation;
            
            // For methods
            bool isMethod;
            std::shared_ptr<class MTypeClass> owningClass;
            
        public:
            // Constructor for regular functions
            MTypeFunction(const std::string& n,
                         const std::vector<FunctionParameter>& params,
                         ASTNodePtr body,
                         std::shared_ptr<Environment> closure,
                         ValueType returnType = ValueType::V_NULL);
            
            // Constructor for methods
            MTypeFunction(const std::string& n,
                         const std::vector<FunctionParameter>& params,
                         ASTNodePtr body,
                         std::shared_ptr<Environment> closure,
                         std::shared_ptr<class MTypeClass> owningClass,
                         int modifiers = METHOD_NONE,
                         ValueType returnType = ValueType::V_NULL);
            
            // Override methods from MTypeCallable
            Result<Value> call(Interpreter* interpreter,
                              const std::vector<Value>& args) override;
            size_t arity() const override;
            std::string toString() const override;
            bool isNative() const override { return false; }
            
            // Function specific methods
            std::string getName() const { return name; }
            const std::vector<FunctionParameter>& getParameters() const { return parameters; }
            ASTNodePtr const& getBody() const { return body; }
            std::shared_ptr<Environment> getClosure() const { return closure; }
            ValueType getReturnType() const { return returnType; }
            
            // Method specific
            bool getIsMethod() const { return isMethod; }
            std::shared_ptr<class MTypeClass> getOwningClass() const { return owningClass; }
            int getModifiers() const { return modifiers; }
            
            // Modifier checks
            bool isStatic() const { return modifiers & METHOD_STATIC; }
            bool isAbstract() const { return modifiers & METHOD_ABSTRACT; }
            bool isVirtual() const { return modifiers & METHOD_VIRTUAL; }
            bool isOverride() const { return modifiers & METHOD_OVERRIDE; }
            bool isFinal() const { return modifiers & METHOD_FINAL; }
            
            // Special function types
            bool getIsGenerator() const { return isGenerator; }
            bool getIsAsync() const { return isAsync; }
            void setIsGenerator(bool gen) { isGenerator = gen; }
            void setIsAsync(bool async) { isAsync = async; }
            
            // Documentation
            std::string getDocumentation() const { return documentation; }
            void setDocumentation(const std::string& doc) { documentation = doc; }
            
            // Bind method to instance (for method calls)
            std::shared_ptr<MTypeFunction> bind(std::shared_ptr<class MTypeInstance> instance);
            
            // Helper for creating function environment
            Result<std::shared_ptr<Environment>> createCallEnvironment(
                const std::vector<Value>& args) const;

			MTypeFunction(const MTypeFunction&) = delete;
			MTypeFunction& operator=(const MTypeFunction&) = delete;

			MTypeFunction(MTypeFunction&&) noexcept = default;
			MTypeFunction& operator=(MTypeFunction&&) noexcept = default;
        };
        
        // Lambda function (anonymous function with capture)
        class MTypeLambda : public MTypeFunction {
        private:
            std::vector<std::pair<std::string, Value>> captures;
            
        public:
            MTypeLambda(const std::vector<FunctionParameter>& params,
                       ASTNodePtr body,
                       std::shared_ptr<Environment> closure,
                       const std::vector<std::pair<std::string, Value>>& captures = {},
                       ValueType returnType = ValueType::V_NULL);
            
            std::string toString() const override;
            const std::vector<std::pair<std::string, Value>>& getCaptures() const { 
                return captures; 
            }
        };
        
        // Constructor function (special type of function for class construction)
        class MTypeConstructor : public MTypeFunction {
        private:
            std::shared_ptr<class MTypeClass> targetClass;
            
        public:
            MTypeConstructor(std::shared_ptr<class MTypeClass> cls,
                           const std::vector<FunctionParameter>& params,
                           ASTNodePtr body,
                           std::shared_ptr<Environment> closure);
            
            Result<Value> call(Interpreter* interpreter,
                              const std::vector<Value>& args) override;
            std::string toString() const override;
        };
        
        // Bound method (method bound to a specific instance)
        class MTypeBoundMethod : public MTypeCallable {
        private:
            std::shared_ptr<MTypeFunction> method;
            std::shared_ptr<class MTypeInstance> instance;
            
        public:
            MTypeBoundMethod(std::shared_ptr<MTypeFunction> m,
                           std::shared_ptr<class MTypeInstance> i);
            
            Result<Value> call(Interpreter* interpreter,
                              const std::vector<Value>& args) override;
            size_t arity() const override { return method->arity(); }
            std::string toString() const override;
            bool isNative() const override { return false; }
            
            std::shared_ptr<MTypeFunction> getMethod() const { return method; }
            std::shared_ptr<class MTypeInstance> getInstance() const { return instance; }
        };
        
    } // namespace runtime
    
}
