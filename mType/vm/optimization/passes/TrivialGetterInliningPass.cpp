#include "TrivialGetterInliningPass.hpp"
#include "../base/BytecodeOptimizationContext.hpp"
#include "../../bytecode/OpCode.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace vm::optimization::passes
{
    using OpCode = vm::bytecode::OpCode;

    TrivialGetterInliningPass::TrivialGetterInliningPass()
        : BytecodePass("TrivialGetterInlining", base::PassType::TRANSFORMATION)
    {
    }

    void TrivialGetterInliningPass::optimize(bytecode::BytecodeProgram& program,
                                             base::BytecodeOptimizationContext& context)
    {
        const auto& functions = program.getFunctions();
        const auto& classes = program.getClasses();

        // Phase 1: Detect trivial getter methods
        // Pattern: LOAD_LOCAL 0, GET_FIELD X, RETURN_VALUE
        std::unordered_map<std::string, uint64_t> trivialGetters;

        for (const auto& [name, meta] : functions)
        {
            if (name.find("::") == std::string::npos) continue;
            if (meta.parameterCount != 1) continue;
            if (meta.returnType == "void") continue;
            if (meta.instructionCount != 3) continue;
            if (meta.startOffset + 2 >= program.getInstructionCount()) continue;

            const auto& i0 = program.getInstruction(meta.startOffset);
            const auto& i1 = program.getInstruction(meta.startOffset + 1);
            const auto& i2 = program.getInstruction(meta.startOffset + 2);

            if (i0.opcode == OpCode::LOAD_LOCAL && i0.hasOperands() && i0.inlineOperands[0] == 0 &&
                i1.opcode == OpCode::GET_FIELD && i1.hasOperands() &&
                i2.opcode == OpCode::RETURN_VALUE)
            {
                // Only inline if the field is public — INLINE_GET_FIELD skips access validation.
                // Walk the inheritance chain to find the field's actual declaring class.
                std::string getterClassName = name.substr(0, name.find("::"));
                std::string fieldName = program.getConstantPool().getString(i1.inlineOperands[0]);
                bool fieldIsPublic = false;
                std::string currentClass = getterClassName;
                while (!currentClass.empty()) {
                    const vm::bytecode::BytecodeProgram::ClassMetadata* foundCls = nullptr;
                    for (const auto& cls : classes) {
                        if (cls.name == currentClass) { foundCls = &cls; break; }
                    }
                    if (!foundCls) break;
                    bool foundField = false;
                    for (const auto& field : foundCls->instanceFields) {
                        if (field.name == fieldName) {
                            foundField = true;
                            fieldIsPublic = !(field.isPrivate || field.isProtected);
                            break;
                        }
                    }
                    if (foundField) break;
                    currentClass = foundCls->parentClassName;
                }
                if (fieldIsPublic) {
                    trivialGetters[name] = i1.inlineOperands[0];
                }
            }
        }

        if (trivialGetters.empty()) return;

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

        std::unordered_set<std::string> unsafeGetters;
        for (const auto& [qualifiedName, fieldIdx] : trivialGetters)
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
                    unsafeGetters.insert(qualifiedName);
                    break;
                }
            }
        }

        for (const auto& name : unsafeGetters)
        {
            trivialGetters.erase(name);
        }

        if (trivialGetters.empty()) return;

        // Phase 3: Rewrite CALL_METHOD → INLINE_GET_FIELD
        for (size_t ip = 0; ip < program.getInstructionCount(); ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode != OpCode::CALL_METHOD) continue;
            if (instr.numOperands() < 2) continue;

            const std::string& methodName =
                program.getConstantPool().getString(instr.inlineOperands[0]);
            auto it = trivialGetters.find(methodName);
            if (it == trivialGetters.end()) continue;

            auto& mutableInstr = program.getMutableInstruction(ip);
            mutableInstr.opcode = OpCode::INLINE_GET_FIELD;
            mutableInstr.setSingleOperand(it->second);
            recordTransformation();
            context.setModified(true);
        }
    }
}
