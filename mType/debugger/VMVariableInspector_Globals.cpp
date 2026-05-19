/**
 * VMVariableInspector — global and static-field enumeration.
 *
 * Holds getGlobalVariables (environment-resident globals via metadata) and
 * getStaticVariables (per-class static fields formatted as
 * ClassName::fieldName). Split from VMVariableInspector_Collect.cpp so
 * the per-frame local collection — which carries most of the bulk and
 * defensive logging — stays under the 500-line per-.cpp cap.
 */

#include "VMVariableInspector.hpp"
#include <iostream>
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../environment/manager/VariableManager.hpp"
#include "../value/ValueShim.hpp"

namespace debugger
{
    std::vector<DebugVariable> VMVariableInspector::getGlobalVariables(
        std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        try
        {
            if (!vm)
            {
                std::cerr << "VMVariableInspector::getGlobalVariables() - VM is null\n";
                return variables;
            }

            const auto* program = vm->getProgram();
            if (!program)
            {
                return variables;
            }

            const auto& globalMetadata = program->getGlobalVariables();
            if (globalMetadata.empty())
            {
                return variables;
            }

            auto env = vm->getEnvironment();
            if (!env)
            {
                std::cerr << "VMVariableInspector::getGlobalVariables() - Environment is null\n";
                return variables;
            }

            for (const auto& meta : globalMetadata)
            {
                try
                {
                    auto varDef = env->findVariable(meta.name);
                    if (!varDef)
                    {
                        continue;
                    }
                    try
                    {
                        value::Value val = varDef->getValue();
                        variables.push_back(valueToDebugVariable(meta.name, val));
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "VMVariableInspector::getGlobalVariables() - Error getting value for '"
                                  << meta.name << "': " << e.what() << "\n";
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getGlobalVariables() - Error accessing global variable '"
                              << meta.name << "': " << e.what() << "\n";
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "VMVariableInspector::getGlobalVariables() - Unexpected exception: "
                      << e.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "VMVariableInspector::getGlobalVariables() - Unknown exception\n";
        }

        return variables;
    }

    std::vector<DebugVariable> VMVariableInspector::getStaticVariables(
        std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::vector<DebugVariable> variables;

        try
        {
            if (!vm)
            {
                std::cerr << "VMVariableInspector::getStaticVariables() - VM is null\n";
                return variables;
            }

            auto env = vm->getEnvironment();
            if (!env)
            {
                std::cerr << "VMVariableInspector::getStaticVariables() - Environment is null\n";
                return variables;
            }

            auto classRegistry = env->getClassRegistry();
            if (!classRegistry)
            {
                return variables;
            }

            std::vector<std::string> classNames;
            try
            {
                classNames = classRegistry->getAllItemNames();
            }
            catch (const std::exception& e)
            {
                std::cerr << "VMVariableInspector::getStaticVariables() - Error getting class names: "
                          << e.what() << "\n";
                return variables;
            }

            for (const auto& className : classNames)
            {
                try
                {
                    auto classDef = classRegistry->findClass(className);
                    if (!classDef)
                    {
                        continue;
                    }
                    try
                    {
                        const auto& staticFields = classDef->getStaticFields();
                        for (const auto& [fieldName, fieldDef] : staticFields)
                        {
                            if (!fieldDef)
                            {
                                continue;
                            }
                            try
                            {
                                std::string qualifiedName = className + "::" + fieldName;
                                value::Value val = fieldDef->getValue();
                                variables.push_back(valueToDebugVariable(qualifiedName, val));
                            }
                            catch (const std::exception& e)
                            {
                                std::cerr << "VMVariableInspector::getStaticVariables() - Error getting static field '"
                                          << className << "::" << fieldName << "': " << e.what() << "\n";
                            }
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "VMVariableInspector::getStaticVariables() - Error accessing static fields for class '"
                                  << className << "': " << e.what() << "\n";
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "VMVariableInspector::getStaticVariables() - Error processing class '"
                              << className << "': " << e.what() << "\n";
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "VMVariableInspector::getStaticVariables() - Unexpected exception: "
                      << e.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "VMVariableInspector::getStaticVariables() - Unknown exception\n";
        }

        return variables;
    }
} // namespace debugger
