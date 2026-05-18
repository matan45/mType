#include "MethodResolver.hpp"
#include <cstddef>
#include "../context/ExecutionContext.hpp"
#include "../VirtualMachine.hpp"

namespace vm::runtime::utils
{
    namespace
    {
        const bytecode::BytecodeProgram::FunctionMetadata* findByPrefix(
            const bytecode::BytecodeProgram* program,
            const std::string& prefix,
            std::string& outQualifiedName)
        {
            for (const auto& [fname, fmeta] : program->getFunctions())
            {
                if (fname.rfind(prefix, 0) == 0 &&
                    (fname.size() == prefix.size() || fname[prefix.size()] == '/'))
                {
                    outQualifiedName = fname;
                    return &fmeta;
                }
            }
            return nullptr;
        }
    }

    MethodResolution MethodResolver::resolve(
        const std::string& qualifiedName,
        const std::string& definingClassName,
        const std::string& simpleMethodName,
        const bytecode::BytecodeProgram* currentProgram,
        const std::vector<const bytecode::BytecodeProgram*>* loadedPrograms,
        size_t currentProgramIndex)
    {
        MethodResolution result;
        result.qualifiedName = qualifiedName;
        result.programIndex = currentProgramIndex;
        result.program = currentProgram;

        if (!currentProgram)
        {
            return result;
        }

        // 1. exact match in caller's program
        result.funcMetadata = currentProgram->getFunction(qualifiedName);

        // 2. prefix fallback in caller's program
        if (!result.funcMetadata)
        {
            const std::string prefix = definingClassName + "::" + simpleMethodName;
            std::string matched;
            auto* fmeta = findByPrefix(currentProgram, prefix, matched);
            if (fmeta)
            {
                result.funcMetadata = fmeta;
                result.qualifiedName = matched;
            }
        }

        if (result.funcMetadata || !loadedPrograms)
        {
            return result;
        }

        // 3 + 4. search loaded library programs
        const std::string prefix = definingClassName + "::" + simpleMethodName;
        for (size_t i = 0; i < loadedPrograms->size(); ++i)
        {
            const auto* libProgram = (*loadedPrograms)[i];
            if (!libProgram)
            {
                continue;
            }

            // exact match
            if (auto* libFunc = libProgram->getFunction(qualifiedName))
            {
                result.funcMetadata = libFunc;
                result.program = libProgram;
                result.programIndex = i;
                return result;
            }

            // prefix fallback
            std::string matched;
            if (auto* libFunc = findByPrefix(libProgram, prefix, matched))
            {
                result.funcMetadata = libFunc;
                result.program = libProgram;
                result.programIndex = i;
                result.qualifiedName = matched;
                return result;
            }
        }

        return result;
    }

    MethodResolution MethodResolver::resolve(
        const std::string& qualifiedName,
        const std::string& definingClassName,
        const std::string& simpleMethodName,
        const ExecutionContext& context,
        const VirtualMachine& vm)
    {
        const size_t startIndex = context.callStack.empty()
            ? 0 : context.callStack.back().programIndex;
        return resolve(qualifiedName, definingClassName, simpleMethodName,
                       context.program, &vm.getLoadedPrograms(), startIndex);
    }
}
