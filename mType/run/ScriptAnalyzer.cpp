#include "ScriptAnalyzer.hpp"
#include "../gc/GC.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
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
        // Step 1: Parse and register classes
        std::cout << "Step 1: Parsing script and registering classes...\n";
        ScriptInterpreter interpreter(execMode);
        interpreter.parseAndRegisterClasses(scriptFile);
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

        // Step 3: Create and use PlayerController
        if (std::find(scriptClasses.begin(), scriptClasses.end(), "PlayerController") != scriptClasses.end())
        {
            std::cout << "Step 3: Creating PlayerController instance...\n";

            std::vector<value::Value> ctorArgs;
            ctorArgs.push_back(value::Value(100)); // health = 100

            value::Value player = interpreter.createObject("PlayerController", ctorArgs);
            std::cout << "  PlayerController created with health=100\n\n";

            std::cout << "Step 4: Calling methods on PlayerController...\n";

            // Call getHealth()
            std::vector<value::Value> noArgs;
            value::Value health = interpreter.callMethod(player, "getHealth", noArgs);
            std::cout << "  player.getHealth() = " << std::get<int64_t>(health) << "\n";

            // Call takeDamage(30)
            std::vector<value::Value> damageArgs;
            damageArgs.push_back(value::Value(30));
            interpreter.callMethod(player, "takeDamage", damageArgs);
            std::cout << "  player.takeDamage(30) called\n";

            // Call getHealth() again
            value::Value newHealth = interpreter.callMethod(player, "getHealth", noArgs);
            std::cout << "  player.getHealth() = " << std::get<int64_t>(newHealth) << " (after damage)\n\n";
        }

        // Step 5: Create and use GameWorld
        if (std::find(scriptClasses.begin(), scriptClasses.end(), "GameWorld") != scriptClasses.end())
        {
            std::cout << "Step 5: Creating GameWorld instance...\n";

            std::vector<value::Value> worldArgs;
            worldArgs.push_back(value::Value(5)); // level = 5

            value::Value world = interpreter.createObject("GameWorld", worldArgs);
            std::cout << "  GameWorld created with level=5\n";

            // Call getLevel()
            std::vector<value::Value> noArgs;
            value::Value level = interpreter.callMethod(world, "getLevel", noArgs);
            std::cout << "  world.getLevel() = " << std::get<int64_t>(level) << "\n\n";
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
