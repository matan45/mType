#include "ScriptAnalyzer.hpp"
#include "../gc/GC.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../json/JsonNatives.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace parser;
using namespace lexer;
using namespace services;
using namespace environment;

// Note: stringToValueType and registerClassesFromMetadata have been refactored into ScriptInterpreter
// Note: resolveImports functionality has been refactored into ImportManager::resolveAllImports()

/**
 * Demonstrate creating objects and calling methods on @Script classes
 */
void demonstrateScriptObjectUsage(const std::string& scriptFile,
                                  constants::ExecutionMode execMode)
{
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Script Class Object Usage Demo\n";
    std::cout << "Execution Mode: Bytecode VM\n";
    std::cout << std::string(80, '=') << "\n\n";

    try
    {
        // Step 1: Parse and register classes (or load from .mtcLib)
        std::cout << "Step 1: Loading script classes...\n";
        ScriptInterpreter interpreter(execMode);
        std::string ext = scriptFile.substr(scriptFile.find_last_of('.'));
        if (ext == ".mtcLib" || ext == ".mtc")
        {
            std::cout << "  Loading compiled bytecode: " << scriptFile << "\n";
            interpreter.loadCompiledBytecode(scriptFile);
        }
        else
        {
            std::cout << "  Parsing source: " << scriptFile << "\n";
            interpreter.parseAndRegisterClasses(scriptFile);
        }
        std::cout << "  Done!\n\n";

        // Step 2: Find @Script classes
        std::cout << "Step 2: Finding @Script annotated classes...\n";
        auto environment = interpreter.getEnvironment();
        auto classRegistry = environment->getClassRegistry();
        auto allClassNames = classRegistry->getAllItemNames();

        std::vector<std::string> scriptClasses;
        for (const auto& className : allClassNames)
        {
            auto classDef = classRegistry->findClass(className);
            if (classDef && classDef->hasAnnotation("Script"))
            {
                scriptClasses.push_back(className);
                std::cout << "  Found: " << className << "\n";
            }
        }
        std::cout << "\n";

        // Step 3: Create instances and call onStart on all @Script classes
        for (const auto& className : scriptClasses)
        {
            std::cout << "Step 3: Creating " << className << " instance...\n";
            std::vector<value::Value> noArgs;
            value::Value obj = interpreter.createObject(className);
            std::cout << "  " << className << " created!\n";

            std::cout << "Step 4: Calling onStart() on " << className << "...\n";
            interpreter.callMethod(obj, "onStart", noArgs);
            std::cout << "  onStart() returned OK\n\n";
        }

        std::cout << std::string(80, '=') << "\n";
        std::cout << "Demo Complete!\n";
        std::cout << std::string(80, '=') << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

/**
 * Find and print all classes with @Script annotation
 * This demonstrates the use case for @Script annotation
 */
void printScriptAnnotatedClasses(std::shared_ptr<environment::Environment> environment)
{
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Classes with @Script annotation:\n";
    std::cout << std::string(80, '=') << "\n\n";

    auto classRegistry = environment->getClassRegistry();
    if (!classRegistry)
    {
        std::cout << "No class registry available\n";
        return;
    }

    auto allClassNames = classRegistry->getAllItemNames();
    int scriptClassCount = 0;

    for (const auto& className : allClassNames)
    {
        auto classDef = classRegistry->findClass(className);
        if (classDef && classDef->hasAnnotation("Script"))
        {
            scriptClassCount++;

            std::cout << "Class: " << className << "\n";

            // Print instance methods
            const auto& methods = classDef->getInstanceMethods();
            if (!methods.empty())
            {
                std::cout << "  Instance Methods:\n";
                for (const auto& [methodName, methodDef] : methods)
                {
                    std::cout << "    - " << methodName << "()\n";
                }
            }

            // Print static methods
            const auto& staticMethods = classDef->getStaticMethods();
            if (!staticMethods.empty())
            {
                std::cout << "  Static Methods:\n";
                for (const auto& [methodName, methodDef] : staticMethods)
                {
                    std::cout << "    - static " << methodName << "()\n";
                }
            }

            // Print instance fields
            const auto& fields = classDef->getInstanceFields();
            if (!fields.empty())
            {
                std::cout << "  Instance Fields:\n";
                for (const auto& [fieldName, fieldDef] : fields)
                {
                    std::cout << "    - " << fieldName << "\n";
                }
            }

            // Print static fields
            const auto& staticFields = classDef->getStaticFields();
            if (!staticFields.empty())
            {
                std::cout << "  Static Fields:\n";
                for (const auto& [fieldName, fieldDef] : staticFields)
                {
                    std::cout << "    - static " << fieldName << "\n";
                }
            }

            std::cout << "\n";
        }
    }

    std::cout << std::string(80, '-') << "\n";
    std::cout << "Total @Script annotated classes: " << scriptClassCount << "\n";
    std::cout << std::string(80, '=') << "\n";
}
