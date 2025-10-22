#pragma once
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include "../ast/ASTNode.hpp"
#include "../constants/ExecutionMode.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"

// Forward declarations
namespace evaluator
{
    class Evaluator;
}

namespace vm::compiler
{
    class BytecodeCompiler;
}

namespace vm::runtime
{
    class VirtualMachine;
}

namespace services
{
    class OptimizationService;
    class NativeFunctionRegistry;
    class ImportResolver;
    class BytecodeService;
    class ScriptAPI;
    class ExecutionStrategy;
}

namespace services
{
    using NativeFunction = std::function<value::Value(const std::vector<value::Value>&)>;

    class ScriptInterpreter
    {
    private:
        std::shared_ptr<environment::Environment> environment;
        std::unique_ptr<evaluator::Evaluator> evaluator;
        std::unique_ptr<vm::compiler::BytecodeCompiler> compiler;
        std::shared_ptr<vm::runtime::VirtualMachine> vm;  // Changed to shared_ptr for enable_shared_from_this support
        std::unique_ptr<OptimizationService> optimizationService;
        std::unique_ptr<NativeFunctionRegistry> nativeRegistry;
        std::unique_ptr<ImportResolver> importResolver;
        std::unique_ptr<BytecodeService> bytecodeService;
        std::unique_ptr<ScriptAPI> scriptAPI;
        std::unique_ptr<ExecutionStrategy> executionStrategy;

        // Execution mode
        constants::ExecutionMode executionMode;

        // Script parsing and setup helpers
        std::pair<std::unique_ptr<ast::ASTNode>, std::unique_ptr<ImportManager>> parseScriptFile(const std::string& filename);
        value::Value executeScriptAST(std::unique_ptr<ast::ASTNode> ast);

        // Constructor helper
        void initializeServices(constants::OptimizationLevel optLevel);

    public:
        ScriptInterpreter();
        explicit ScriptInterpreter(constants::ExecutionMode mode, constants::OptimizationLevel optLevel = constants::OptimizationLevel::Debug);
        ~ScriptInterpreter();
        void runScript(const std::string& filename);

        // Bytecode compilation and execution
        void compileToFile(const std::string& sourceFile, const std::string& outputFile);
        void runCompiledBytecode(const std::string& bytecodeFile);

        // Execution mode control
        void setExecutionMode(constants::ExecutionMode mode);
        void setOptimizationLevel(constants::OptimizationLevel level);
        constants::ExecutionMode getExecutionMode() const;
        constants::OptimizationLevel getOptimizationLevel() const;

        // Memory management methods
        void cleanupRegistries();
        size_t cleanupUnusedInterfaces();
        std::vector<std::string> findUnusedInterfaces() const;
        
        // Native function registration
        void registerNativeFunction(const std::string& name, NativeFunction function);
        void registerNativeVariable(const std::string& name, const value::Value& value);

        // Native class support
        void registerNativeClass(const std::string& name);
        void registerNativeMethod(const std::string& className,
                                  const std::string& methodName,
                                  NativeFunction function,
                                  bool isStatic = false);
        void registerNativeField(const std::string& className,
                                 const std::string& fieldName,
                                 const value::Value& value,
                                 bool isStatic = false);

        // C++ to mType calling interface
        // Function calling
        value::Value callFunction(const std::string& functionName, const std::vector<value::Value>& args = {});

        // Method calling on objects
        value::Value callMethod(const value::Value& object, const std::string& methodName,
                                const std::vector<value::Value>& args = {});

        // Static method calling
        value::Value callStaticMethod(const std::string& className, const std::string& methodName,
                                      const std::vector<value::Value>& args = {});

        // Static field access
        value::Value getStaticField(const std::string& className, const std::string& fieldName);
        void setStaticField(const std::string& className, const std::string& fieldName, const value::Value& value);

        // Variable access
        value::Value getVariable(const std::string& variableName);
        void setVariable(const std::string& variableName, const value::Value& value);

        // Object creation and manipulation
        value::Value createObject(const std::string& className, const std::vector<value::Value>& constructorArgs = {});

        // Utility methods for native functions working with objects
        bool isObjectOfClass(const value::Value& object, const std::string& className);
        std::string getObjectClassName(const value::Value& object);


        // Access to environment and evaluator for advanced operations
        std::shared_ptr<environment::Environment> getEnvironment() const;
        evaluator::Evaluator* getEvaluator() const;
    };
}
