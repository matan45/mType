#include "ScriptInterpreter.hpp"
#include "NativeFunctionRegistry.hpp"
#include "BytecodeService.hpp"
#include "ScriptAPI.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../vm/runtime/VirtualMachine.hpp"

namespace services
{
    using Value = value::Value;

    void ScriptInterpreter::registerNativeFunction(const std::string& name, NativeFunction function)
    {
        nativeRegistry->registerNativeFunction(name, function);
    }

    void ScriptInterpreter::registerNativeVariable(const std::string& name, const Value& value)
    {
        nativeRegistry->registerNativeVariable(name, value);
    }

    void ScriptInterpreter::registerNativeClass(const std::string& name)
    {
        nativeRegistry->registerNativeClass(name);
    }

    void ScriptInterpreter::registerNativeMethod(const std::string& className,
                                                 const std::string& methodName,
                                                 NativeFunction function,
                                                 bool isStatic)
    {
        nativeRegistry->registerNativeMethod(className, methodName, function, isStatic);
    }

    void ScriptInterpreter::registerNativeField(const std::string& className,
                                                const std::string& fieldName,
                                                const value::Value& value,
                                                bool isStatic)
    {
        nativeRegistry->registerNativeField(className, fieldName, value, isStatic);
    }

    Value ScriptInterpreter::callFunction(const std::string& functionName, const std::vector<Value>& args)
    {
        return scriptAPI->callFunction(functionName, args);
    }

    Value ScriptInterpreter::callMethod(const Value& object, const std::string& methodName,
                                        const std::vector<Value>& args)
    {
        return scriptAPI->callMethod(object, methodName, args);
    }

    Value ScriptInterpreter::callStaticMethod(const std::string& className, const std::string& methodName,
                                              const std::vector<Value>& args)
    {
        return scriptAPI->callStaticMethod(className, methodName, args);
    }

    Value ScriptInterpreter::callLambda(const Value& lambda, const std::vector<Value>& args)
    {
        return scriptAPI->callLambda(lambda, args);
    }

    Value ScriptInterpreter::getStaticField(const std::string& className, const std::string& fieldName)
    {
        return scriptAPI->getStaticField(className, fieldName);
    }

    void ScriptInterpreter::setStaticField(const std::string& className, const std::string& fieldName,
                                           const Value& value)
    {
        scriptAPI->setStaticField(className, fieldName, value);
    }

    Value ScriptInterpreter::getVariable(const std::string& variableName)
    {
        return scriptAPI->getVariable(variableName);
    }

    void ScriptInterpreter::setVariable(const std::string& variableName, const Value& value)
    {
        scriptAPI->setVariable(variableName, value);
    }

    Value ScriptInterpreter::createObject(const std::string& className, const std::vector<Value>& constructorArgs)
    {
        return scriptAPI->createObject(className, constructorArgs);
    }

    bool ScriptInterpreter::isObjectOfClass(const Value& object, const std::string& className)
    {
        return scriptAPI->isObjectOfClass(object, className);
    }

    std::string ScriptInterpreter::getObjectClassName(const Value& object)
    {
        return scriptAPI->getObjectClassName(object);
    }

    bool ScriptInterpreter::classImplementsInterface(const std::string& className, const std::string& interfaceName)
    {
        if (!environment)
        {
            return false;
        }

        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            return false;
        }

        auto interfaceRegistry = environment->getInterfaceRegistry();
        return classDef->implementsInterface(interfaceName, interfaceRegistry);
    }

    void ScriptInterpreter::compileToFile(const std::string& sourceFile, const std::string& outputFile)
    {
        bytecodeService->compileToFile(sourceFile, outputFile);
    }

    void ScriptInterpreter::compileToFile(const std::string& sourceFile, const std::string& outputFile,
                                          const std::vector<std::string>& searchPaths,
                                          const std::unordered_map<std::string, std::string>& aliases)
    {
        ImportConfig config;
        config.searchPaths = searchPaths;
        config.aliases = aliases;
        bytecodeService->compileToFile(sourceFile, outputFile, config);
    }

    void ScriptInterpreter::runCompiledBytecode(const std::string& bytecodeFile)
    {
        bytecodeService->runCompiledBytecode(bytecodeFile);
    }

    void ScriptInterpreter::loadCompiledBytecode(const std::string& bytecodeFile)
    {
        cachedBytecodeProgram = bytecodeService->loadCompiledBytecodeWithoutExecuting(bytecodeFile);
        runCachedStaticInitializers();
    }

    void ScriptInterpreter::loadFromProgram(vm::bytecode::BytecodeProgram program, bool runStaticInitializers)
    {
        cachedBytecodeProgram = bytecodeService->loadFromProgram(std::move(program));
        if (runStaticInitializers)
        {
            runCachedStaticInitializers();
        }
    }

    void ScriptInterpreter::runFromProgram(vm::bytecode::BytecodeProgram program)
    {
        // Load + execute an already-deserialized bytecode program; the
        // cached unique_ptr keeps it alive past the call.
        cachedBytecodeProgram = bytecodeService->runFromProgram(std::move(program));
    }

    void ScriptInterpreter::loadLibrary(const std::string& mtcLibPath)
    {
        if (!libraryLoader) {
            libraryLoader = std::make_unique<vm::runtime::LibraryLoader>();
        }
        libraryLoader->loadLibrary(mtcLibPath, *vm, environment);
    }

    void ScriptInterpreter::unloadLibrary(const std::string& libraryName)
    {
        if (!transitiveDependencyLoader) {
            throw std::runtime_error("Cannot unload library: no libraries have been loaded");
        }
        transitiveDependencyLoader->unloadLibrary(libraryName, *vm, environment);
    }

    void ScriptInterpreter::loadLibraryWithDependencies(const std::string& mtcLibPath)
    {
        transitiveDependencyLoader->loadLibraryWithDependencies(mtcLibPath, *vm, environment);
    }

    void ScriptInterpreter::loadLibrariesWithDependencies(const std::vector<std::string>& paths)
    {
        transitiveDependencyLoader->loadLibrariesWithDependencies(paths, *vm, environment);
    }

    void ScriptInterpreter::addLibrarySearchPath(const std::string& path)
    {
        transitiveDependencyLoader->addSearchPath(path);
    }

    void ScriptInterpreter::runStaticInitializersForLoadedPrograms()
    {
        if (vm)
        {
            vm->runStaticInitializersForLoadedPrograms();
        }
    }

    void ScriptInterpreter::runCachedStaticInitializers()
    {
        if (vm && cachedBytecodeProgram)
        {
            vm->runStaticInitializers(*cachedBytecodeProgram);
        }
    }
}
