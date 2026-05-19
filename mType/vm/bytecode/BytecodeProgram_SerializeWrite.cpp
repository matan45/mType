#include "BytecodeProgram.hpp"
#include "BytecodeIOHelper.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

namespace vm::bytecode
{
    namespace
    {
        // .mtc format version. MUST match the value in BytecodeProgram_SerializeRead.cpp;
        // the reader rejects any other version with a "recompile" diagnostic.
        //
        // History:
        //   8: MYT-202 compile-time fused superinstructions
        //   9: MYT-B1 dead lambda opcodes removed (LAMBDA_INVOKE / CLOSURE /
        //      LOAD_UPVALUE / STORE_UPVALUE / CLOSE_UPVALUE) — opcode numbering
        //      shifted, so prior .mtc files reference the wrong numeric op for
        //      every entry past the deletions.
        //   10: MYT-290 serialized static-initializer function list.
        //   11: Serialized top-level local names for debugger variable inspection.
        constexpr uint32_t BYTECODE_FORMAT_VERSION = 11;
    }

    std::string BytecodeProgram::disassemble() const {
        std::ostringstream oss;
        oss << "=== Bytecode Disassembly ===\n";
        oss << "Entry Point: " << entryPoint << "\n\n";

        oss << "=== Constant Pool ===\n";
        oss << "Integers (" << constantPool.integers.size() << "):\n";
        for (size_t i = 0; i < constantPool.integers.size(); ++i) {
            oss << "  [" << i << "] " << constantPool.integers[i] << "\n";
        }
        oss << "Floats (" << constantPool.floats.size() << "):\n";
        for (size_t i = 0; i < constantPool.floats.size(); ++i) {
            oss << "  [" << i << "] " << constantPool.floats[i] << "\n";
        }
        oss << "Strings (" << constantPool.strings.size() << "):\n";
        for (size_t i = 0; i < constantPool.strings.size(); ++i) {
            oss << "  [" << i << "] \"" << constantPool.strings[i] << "\"\n";
        }

        oss << "\n=== Functions ===\n";
        for (const auto& [name, func] : functions) {
            oss << func.name << " (offset: " << func.startOffset
                << ", locals: " << func.localCount
                << ", params: " << func.parameterCount << ")\n";
        }

        oss << "\n=== Instructions ===\n";
        for (size_t i = 0; i < instructions.size(); ++i) {
            const auto& instr = instructions[i];
            oss << std::setw(6) << std::setfill('0') << i << ": "
                << std::setw(20) << std::setfill(' ') << std::left
                << getOpCodeName(instr.opcode);

            for (size_t j = 0; j < instr.numOperands(); ++j) {
                oss << " " << instr.operandAt(j);
            }

            if (instr.flags & INSTR_FLAG_NONNULL_RECEIVER) {
                oss << " [nonnull]";
            }

            auto* loc = getSourceLocation(i);
            if (loc) {
                oss << "  ; " << loc->filename << ":" << loc->line;
            }
            oss << "\n";
        }

        return oss.str();
    }

    void BytecodeProgram::serialize(std::ostream& out) const {
        uint32_t magic = 0x4D545950;  // "MTYP"
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

        uint32_t version = BYTECODE_FORMAT_VERSION;
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));

        out.write(reinterpret_cast<const char*>(&entryPoint), sizeof(entryPoint));

        writeConstantPool(out);
        writeInstructions(out);
        writeFunctions(out);
        writeSourceLocations(out);
        writeClasses(out);
        writeInterfaces(out);
        writeGlobalExceptionTable(out);
        // MYT-108: annotation declarations (.mtc v5+).
        writeAnnotationDeclarations(out);
        // MYT-290: static-initializer function names.
        BytecodeIOHelper::writeStringVector(out, staticInitializerFunctions);
        // Top-level local names enable debugger variable inspection.
        BytecodeIOHelper::writeStringVector(out, topLevelLocalNames);

        size_t len = sourceFilePath.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(sourceFilePath.data(), len);
    }

    void BytecodeProgram::writeConstantPool(std::ostream& out) const {
        size_t count = constantPool.integers.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        out.write(reinterpret_cast<const char*>(constantPool.integers.data()),
                 count * sizeof(int64_t));

        count = constantPool.floats.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        out.write(reinterpret_cast<const char*>(constantPool.floats.data()),
                 count * sizeof(double));

        count = constantPool.strings.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& str : constantPool.strings) {
            size_t len = str.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(str.data(), len);
        }
    }

    void BytecodeProgram::writeInstructions(std::ostream& out) const {
        size_t count = instructions.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& instr : instructions) {
            out.write(reinterpret_cast<const char*>(&instr.opcode), sizeof(instr.opcode));
            out.write(reinterpret_cast<const char*>(&instr.flags), sizeof(instr.flags));
            size_t opCount = instr.numOperands();
            out.write(reinterpret_cast<const char*>(&opCount), sizeof(opCount));
            size_t inline_n = opCount < 3 ? opCount : 3;
            if (inline_n > 0) {
                out.write(reinterpret_cast<const char*>(instr.inlineOperands),
                         inline_n * sizeof(uint64_t));
            }
            if (opCount > 3) {
                out.write(reinterpret_cast<const char*>(instr.overflow.get()),
                         (opCount - 3) * sizeof(uint64_t));
            }
        }
    }

    void BytecodeProgram::writeFunctions(std::ostream& out) const {
        // MYT-139: iterate functionIndexToName (stable insertion order) rather
        // than the functions map. Unordered map iteration is non-deterministic,
        // so re-registering in map order shuffled function indices and broke
        // CALL_FAST opcodes baking the index at compile time (e.g. renderDrawable
        // resolving to `tan` at runtime).
        size_t count = functionIndexToName.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& name : functionIndexToName) {
            auto it = functions.find(name);
            if (it == functions.end()) {
                throw std::runtime_error(
                    "BytecodeProgram::writeFunctions: index registered but no "
                    "metadata for function '" + name + "'");
            }
            const auto& func = it->second;
            size_t len = name.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(name.data(), len);

            // MYT-286 serialize-time backstop for stale instructionCount.
            // Root cause is in updateFunctionOffsets: when peephole removes
            // instructions inside a function body, the recorded length stays at
            // pre-shrink value. The proper fix (teach updateFunctionOffsets to
            // shrink instructionCount of the enclosing function) broke 22 tests
            // in call sites that read the loose count without defensive caps;
            // pinning those down is a follow-up. Until then, clamp here so the
            // on-disk range never overruns instructions.size(). Runtime users
            // (JIT scanCalleeOpcodes, JumpThreading, fuseLocalArrayOps) already
            // cap defensively, so a tighter on-disk count is safe.
            size_t writeCount = func.instructionCount;
            if (!func.isNative) {
                // Guard the subtraction: startOffset can be >= instructions.size()
                // on corrupt/placeholder metadata, in which case size_t arithmetic
                // would wrap. Producing 0 here is fine — the deserializer's
                // startOffset > size check rejects that record anyway.
                size_t available = (func.startOffset < instructions.size())
                                   ? instructions.size() - func.startOffset
                                   : 0;
                if (writeCount > available) {
#ifndef NDEBUG
                    std::cerr << "[MYT-286] writeFunctions clamped '"
                              << name << "': "
                              << writeCount << " -> " << available
                              << " (startOffset=" << func.startOffset
                              << ", instructions.size=" << instructions.size()
                              << ")\n";
#endif
                    writeCount = available;
                }
            }
            out.write(reinterpret_cast<const char*>(&func.startOffset), sizeof(func.startOffset));
            out.write(reinterpret_cast<const char*>(&writeCount), sizeof(writeCount));
            out.write(reinterpret_cast<const char*>(&func.localCount), sizeof(func.localCount));
            out.write(reinterpret_cast<const char*>(&func.parameterCount), sizeof(func.parameterCount));
            out.write(reinterpret_cast<const char*>(&func.isStatic), sizeof(func.isStatic));
            out.write(reinterpret_cast<const char*>(&func.isNative), sizeof(func.isNative));

            len = func.returnType.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(func.returnType.data(), len);

            size_t paramCount = func.parameterNames.size();
            out.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
            for (const auto& paramName : func.parameterNames) {
                len = paramName.size();
                out.write(reinterpret_cast<const char*>(&len), sizeof(len));
                out.write(paramName.data(), len);
            }

            const auto& exceptionTable = func.exceptionTable;
            size_t entryCount = exceptionTable.size();
            out.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));
            for (const auto& entry : exceptionTable.getEntries()) {
                out.write(reinterpret_cast<const char*>(&entry.startIP), sizeof(entry.startIP));
                out.write(reinterpret_cast<const char*>(&entry.endIP), sizeof(entry.endIP));
                out.write(reinterpret_cast<const char*>(&entry.catchIP), sizeof(entry.catchIP));
                out.write(reinterpret_cast<const char*>(&entry.finallyIP), sizeof(entry.finallyIP));
                out.write(reinterpret_cast<const char*>(&entry.nestingLevel), sizeof(entry.nestingLevel));
                out.write(reinterpret_cast<const char*>(&entry.catchVarSlot), sizeof(entry.catchVarSlot));

                len = entry.exceptionType.size();
                out.write(reinterpret_cast<const char*>(&len), sizeof(len));
                out.write(entry.exceptionType.data(), len);

                len = entry.catchVarName.size();
                out.write(reinterpret_cast<const char*>(&len), sizeof(len));
                out.write(entry.catchVarName.data(), len);
            }

            // MYT-110: function-level annotations + per-parameter annotations (v7+).
            writeAnnotationList(out, func.annotations);
            size_t fnParamAnnotCount = func.parameterNames.size();
            out.write(reinterpret_cast<const char*>(&fnParamAnnotCount), sizeof(fnParamAnnotCount));
            for (size_t i = 0; i < fnParamAnnotCount; ++i) {
                const std::vector<AnnotationData>& list =
                    (i < func.parameterAnnotations.size())
                    ? func.parameterAnnotations[i]
                    : std::vector<AnnotationData>{};
                writeAnnotationList(out, list);
            }
        }
    }

    void BytecodeProgram::writeSourceLocations(std::ostream& out) const {
        std::vector<std::string> filenamePool;
        std::unordered_map<std::string, uint32_t> filenameIndexMap;

        for (const auto& [offset, loc] : sourceLocations) {
            if (filenameIndexMap.find(loc.filename) == filenameIndexMap.end()) {
                filenameIndexMap[loc.filename] = static_cast<uint32_t>(filenamePool.size());
                filenamePool.push_back(loc.filename);
            }
        }

        size_t poolSize = filenamePool.size();
        out.write(reinterpret_cast<const char*>(&poolSize), sizeof(poolSize));
        for (const auto& filename : filenamePool) {
            size_t len = filename.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(filename.data(), len);
        }

        size_t count = sourceLocations.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& [offset, loc] : sourceLocations) {
            out.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
            out.write(reinterpret_cast<const char*>(&loc.line), sizeof(loc.line));
            out.write(reinterpret_cast<const char*>(&loc.column), sizeof(loc.column));

            uint32_t filenameIndex = filenameIndexMap[loc.filename];
            out.write(reinterpret_cast<const char*>(&filenameIndex), sizeof(filenameIndex));
        }
    }

    void BytecodeProgram::writeAnnotationList(std::ostream& out, const std::vector<AnnotationData>& list) {
        size_t count = list.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& annot : list) {
            BytecodeIOHelper::writeString(out, annot.name);
            int line   = annot.location.getLine();
            int column = annot.location.getColumn();
            out.write(reinterpret_cast<const char*>(&line),   sizeof(line));
            out.write(reinterpret_cast<const char*>(&column), sizeof(column));
            BytecodeIOHelper::writeString(out, annot.location.getFilename());
            size_t argCount = annot.typedArguments.size();
            out.write(reinterpret_cast<const char*>(&argCount), sizeof(argCount));
            for (const auto& arg : annot.typedArguments) {
                BytecodeIOHelper::writeString(out, arg.key);
                BytecodeIOHelper::writePrimitive(out, arg.valueType);
                BytecodeIOHelper::writePrimitive(out, arg.intVal);
                BytecodeIOHelper::writePrimitive(out, arg.floatVal);
                BytecodeIOHelper::writePrimitive(out, arg.boolVal);
                BytecodeIOHelper::writeString(out, arg.stringVal);
                BytecodeIOHelper::writeStringVector(out, arg.arrayVal);
            }
        }
    }

    void BytecodeProgram::writeFieldMetadata(std::ostream& out, const FieldMetadata& field) const {
        BytecodeIOHelper::writeString(out, field.name);
        BytecodeIOHelper::writeString(out, field.type);
        BytecodeIOHelper::writePrimitive(out, field.isStatic);
        BytecodeIOHelper::writePrimitive(out, field.isFinal);
        BytecodeIOHelper::writePrimitive(out, field.isPrivate);
        BytecodeIOHelper::writePrimitive(out, field.isProtected);
        writeAnnotationList(out, field.annotations);
    }

    void BytecodeProgram::writeMethodMetadata(std::ostream& out, const MethodMetadata& method) const {
        BytecodeIOHelper::writeString(out, method.name);
        BytecodeIOHelper::writeString(out, method.returnType);
        BytecodeIOHelper::writeStringVector(out, method.parameterTypes);
        BytecodeIOHelper::writeStringVector(out, method.parameterNames);
        BytecodeIOHelper::writePrimitive(out, method.isStatic);
        BytecodeIOHelper::writePrimitive(out, method.isFinal);
        BytecodeIOHelper::writePrimitive(out, method.isPrivate);
        BytecodeIOHelper::writePrimitive(out, method.isProtected);
        BytecodeIOHelper::writePrimitive(out, method.isAbstract);
        BytecodeIOHelper::writePrimitive(out, method.startOffset);
        writeAnnotationList(out, method.annotations);
        // MYT-110: per-parameter annotations, parallel to parameterNames.
        size_t paramCount = method.parameterNames.size();
        out.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
        for (size_t i = 0; i < paramCount; ++i) {
            const std::vector<AnnotationData>& list =
                (i < method.parameterAnnotations.size())
                ? method.parameterAnnotations[i]
                : std::vector<AnnotationData>{};
            writeAnnotationList(out, list);
        }
    }

    void BytecodeProgram::writeConstructorMetadata(std::ostream& out, const ConstructorMetadata& ctor) const {
        BytecodeIOHelper::writeStringVector(out, ctor.parameterTypes);
        BytecodeIOHelper::writeStringVector(out, ctor.parameterNames);
        BytecodeIOHelper::writePrimitive(out, ctor.startOffset);
        writeAnnotationList(out, ctor.annotations);
        // MYT-110: per-parameter annotations, parallel to parameterNames.
        size_t paramCount = ctor.parameterNames.size();
        out.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
        for (size_t i = 0; i < paramCount; ++i) {
            const std::vector<AnnotationData>& list =
                (i < ctor.parameterAnnotations.size())
                ? ctor.parameterAnnotations[i]
                : std::vector<AnnotationData>{};
            writeAnnotationList(out, list);
        }
    }

    void BytecodeProgram::writeClassMetadata(std::ostream& out, const ClassMetadata& classMeta) const {
        BytecodeIOHelper::writeString(out, classMeta.name);
        BytecodeIOHelper::writeString(out, classMeta.parentClassName);
        BytecodeIOHelper::writeStringVector(out, classMeta.implementedInterfaces);
        BytecodeIOHelper::writeStringVector(out, classMeta.genericParameters);

        BytecodeIOHelper::writePrimitive(out, classMeta.isAbstract);
        BytecodeIOHelper::writePrimitive(out, classMeta.isFinal);
        BytecodeIOHelper::writePrimitive(out, classMeta.isValueClass);

        // MYT-108 typed-args annotations.
        writeAnnotationList(out, classMeta.annotations);

        size_t fieldCount = classMeta.instanceFields.size();
        out.write(reinterpret_cast<const char*>(&fieldCount), sizeof(fieldCount));
        for (const auto& field : classMeta.instanceFields) {
            writeFieldMetadata(out, field);
        }

        fieldCount = classMeta.staticFields.size();
        out.write(reinterpret_cast<const char*>(&fieldCount), sizeof(fieldCount));
        for (const auto& field : classMeta.staticFields) {
            writeFieldMetadata(out, field);
        }

        size_t methodCount = classMeta.instanceMethods.size();
        out.write(reinterpret_cast<const char*>(&methodCount), sizeof(methodCount));
        for (const auto& method : classMeta.instanceMethods) {
            writeMethodMetadata(out, method);
        }

        methodCount = classMeta.staticMethods.size();
        out.write(reinterpret_cast<const char*>(&methodCount), sizeof(methodCount));
        for (const auto& method : classMeta.staticMethods) {
            writeMethodMetadata(out, method);
        }

        size_t ctorCount = classMeta.constructors.size();
        out.write(reinterpret_cast<const char*>(&ctorCount), sizeof(ctorCount));
        for (const auto& ctor : classMeta.constructors) {
            writeConstructorMetadata(out, ctor);
        }
    }

    void BytecodeProgram::writeClasses(std::ostream& out) const {
        size_t count = classes.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& classMeta : classes) {
            writeClassMetadata(out, classMeta);
        }
    }

    void BytecodeProgram::writeAnnotationDeclarations(std::ostream& out) const {
        size_t declCount = annotationDeclarations.size();
        out.write(reinterpret_cast<const char*>(&declCount), sizeof(declCount));
        for (const auto& decl : annotationDeclarations) {
            BytecodeIOHelper::writeString(out, decl.name);
            size_t paramCount = decl.params.size();
            out.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
            for (const auto& p : decl.params) {
                BytecodeIOHelper::writeString(out, p.name);
                BytecodeIOHelper::writePrimitive(out, p.declaredType);
                BytecodeIOHelper::writePrimitive(out, p.nullable);
                BytecodeIOHelper::writePrimitive(out, p.isArray);
                BytecodeIOHelper::writePrimitive(out, p.hasDefault);
                if (p.hasDefault) {
                    BytecodeIOHelper::writePrimitive(out, p.defaultInt);
                    BytecodeIOHelper::writePrimitive(out, p.defaultFloat);
                    BytecodeIOHelper::writePrimitive(out, p.defaultBool);
                    BytecodeIOHelper::writeString(out, p.defaultString);
                    BytecodeIOHelper::writeStringVector(out, p.defaultStringArray);
                }
            }
            // MYT-109 (.mtc v6+): meta-annotations on the annotation declaration.
            writeAnnotationList(out, decl.metaAnnotations);
        }
    }

    void BytecodeProgram::writeInterfaceMethodSignature(std::ostream& out, const InterfaceMethodSignature& sig) const {
        BytecodeIOHelper::writeString(out, sig.name);
        BytecodeIOHelper::writeString(out, sig.returnType);
        BytecodeIOHelper::writeStringVector(out, sig.parameterTypes);
        BytecodeIOHelper::writeStringVector(out, sig.parameterNames);
        BytecodeIOHelper::writeStringVector(out, sig.genericTypeParameters);
    }

    void BytecodeProgram::writeInterfaceMetadata(std::ostream& out, const InterfaceMetadata& meta) const {
        BytecodeIOHelper::writeString(out, meta.name);
        BytecodeIOHelper::writeStringVector(out, meta.genericParameters);
        BytecodeIOHelper::writeStringVector(out, meta.extendsInterfaces);
        BytecodeIOHelper::writePrimitive(out, meta.isFinal);

        size_t methodCount = meta.methods.size();
        out.write(reinterpret_cast<const char*>(&methodCount), sizeof(methodCount));
        for (const auto& method : meta.methods) {
            writeInterfaceMethodSignature(out, method);
        }
    }

    void BytecodeProgram::writeInterfaces(std::ostream& out) const {
        size_t count = interfaces.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& iface : interfaces) {
            writeInterfaceMetadata(out, iface);
        }
    }

    void BytecodeProgram::writeGlobalExceptionTable(std::ostream& out) const {
        size_t entryCount = globalExceptionTable.size();
        out.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));

        for (const auto& entry : globalExceptionTable.getEntries()) {
            out.write(reinterpret_cast<const char*>(&entry.startIP), sizeof(entry.startIP));
            out.write(reinterpret_cast<const char*>(&entry.endIP), sizeof(entry.endIP));
            out.write(reinterpret_cast<const char*>(&entry.catchIP), sizeof(entry.catchIP));
            out.write(reinterpret_cast<const char*>(&entry.finallyIP), sizeof(entry.finallyIP));
            out.write(reinterpret_cast<const char*>(&entry.nestingLevel), sizeof(entry.nestingLevel));
            out.write(reinterpret_cast<const char*>(&entry.catchVarSlot), sizeof(entry.catchVarSlot));

            size_t len = entry.exceptionType.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(entry.exceptionType.data(), len);

            len = entry.catchVarName.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(entry.catchVarName.data(), len);
        }
    }
}
