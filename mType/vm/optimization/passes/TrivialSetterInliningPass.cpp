#include "TrivialSetterInliningPass.hpp"
#include "../base/BytecodeOptimizationContext.hpp"
#include "../../bytecode/OpCode.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace vm::optimization::passes
{
    using OpCode = vm::bytecode::OpCode;

    TrivialSetterInliningPass::TrivialSetterInliningPass()
        : BytecodePass("TrivialSetterInlining", base::PassType::TRANSFORMATION)
    {
    }

    void TrivialSetterInliningPass::optimize(bytecode::BytecodeProgram& program,
                                             base::BytecodeOptimizationContext& context)
    {
        const auto& functions = program.getFunctions();
        const auto& classes = program.getClasses();

        // Phase 1: Detect trivial setter methods
        // Pattern: LOAD_LOCAL 0, LOAD_LOCAL 1, SET_FIELD X, PUSH_NULL, RETURN_VALUE
        std::unordered_map<std::string, uint64_t> trivialSetters;

        for (const auto& [name, meta] : functions)
        {
            if (name.find("::") == std::string::npos) continue;
            if (meta.parameterCount != 2) continue;
            if (meta.returnType != "void") continue;
            if (meta.instructionCount != 5) continue;
            if (meta.startOffset + 4 >= program.getInstructionCount()) continue;

            const auto& i0 = program.getInstruction(meta.startOffset);
            const auto& i1 = program.getInstruction(meta.startOffset + 1);
            const auto& i2 = program.getInstruction(meta.startOffset + 2);
            const auto& i3 = program.getInstruction(meta.startOffset + 3);
            const auto& i4 = program.getInstruction(meta.startOffset + 4);

            if (i0.opcode == OpCode::LOAD_LOCAL && i0.hasOperands() && i0.inlineOperands[0] == 0 &&
                i1.opcode == OpCode::LOAD_LOCAL && i1.hasOperands() && i1.inlineOperands[0] == 1 &&
                i2.opcode == OpCode::SET_FIELD && i2.hasOperands() &&
                i3.opcode == OpCode::PUSH_NULL &&
                i4.opcode == OpCode::RETURN_VALUE)
            {
                trivialSetters[name] = i2.inlineOperands[0];
            }
        }

        if (trivialSetters.empty()) return;

        // Phase 2: Override safety check
        std::unordered_map<std::string, std::string> classParent;
        std::unordered_map<std::string, std::unordered_set<std::string>> classMethods;

        for (const auto& cls : classes)
        {
            classParent[cls.name] = cls.parentClassName;
            for (const auto& method : cls.instanceMethods)
            {
                classMethods[cls.name].insert(method.name);
            }
        }

        std::unordered_set<std::string> unsafeSetters;
        for (const auto& [qualifiedName, fieldIdx] : trivialSetters)
        {
            size_t colonPos = qualifiedName.find("::");
            std::string className = qualifiedName.substr(0, colonPos);
            std::string methodName = qualifiedName.substr(colonPos + 2);

            for (const auto& cls : classes)
            {
                if (cls.name == className) continue;

                std::string parent = cls.parentClassName;
                bool isSubclass = false;
                while (!parent.empty())
                {
                    if (parent == className)
                    {
                        isSubclass = true;
                        break;
                    }
                    auto it = classParent.find(parent);
                    if (it == classParent.end()) break;
                    parent = it->second;
                }

                if (isSubclass && classMethods[cls.name].count(methodName))
                {
                    unsafeSetters.insert(qualifiedName);
                    break;
                }
            }
        }

        for (const auto& name : unsafeSetters)
        {
            trivialSetters.erase(name);
        }

        if (trivialSetters.empty()) return;

        // Phase 3: Rewrite CALL_METHOD → INLINE_SET_FIELD
        for (size_t ip = 0; ip < program.getInstructionCount(); ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode != OpCode::CALL_METHOD) continue;
            if (instr.numOperands() < 2) continue;

            const std::string& methodName =
                program.getConstantPool().getString(instr.inlineOperands[0]);
            auto it = trivialSetters.find(methodName);
            if (it == trivialSetters.end()) continue;

            auto& mutableInstr = program.getMutableInstruction(ip);
            mutableInstr.opcode = OpCode::INLINE_SET_FIELD;
            mutableInstr.setSingleOperand(it->second);
            recordTransformation();
            context.setModified(true);
        }
    }
}
