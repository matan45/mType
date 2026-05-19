#include "BytecodeProgram.hpp"
#include "BytecodeIOHelper.hpp"
#include "BytecodeProgramReadValidators.hpp"
#include "../../constants/SecurityConstants.hpp"
#include <stdexcept>

namespace vm::bytecode
{
    void BytecodeProgram::readFunctions(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        readv::validateCount(count, constants::security::MAX_FUNCTION_COUNT, "functions");
        for (size_t i = 0; i < count; ++i) {
            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            readv::validateStringLen(len, "function name");
            std::string name(len, '\0');
            in.read(&name[0], len);

            FunctionMetadata func;
            func.name = name;
            func.mangledName = name;
            in.read(reinterpret_cast<char*>(&func.startOffset), sizeof(func.startOffset));
            in.read(reinterpret_cast<char*>(&func.instructionCount), sizeof(func.instructionCount));
            in.read(reinterpret_cast<char*>(&func.localCount), sizeof(func.localCount));
            in.read(reinterpret_cast<char*>(&func.parameterCount), sizeof(func.parameterCount));
            in.read(reinterpret_cast<char*>(&func.isStatic), sizeof(func.isStatic));
            in.read(reinterpret_cast<char*>(&func.isNative), sizeof(func.isNative));

            // Native functions have no compiled body, so their offsets are
            // not constrained by the instruction array.
            if (!func.isNative) {
                if (func.startOffset > instructions.size() ||
                    func.instructionCount > instructions.size() ||
                    func.startOffset + func.instructionCount > instructions.size()) {
                    throw std::runtime_error(
                        "Bytecode deserialization rejected: function '" + name +
                        "' has out-of-range body [" + std::to_string(func.startOffset) +
                        " .. +" + std::to_string(func.instructionCount) +
                        "] (instruction count " + std::to_string(instructions.size()) + ")");
                }
            }

            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            readv::validateStringLen(len, "function return type");
            std::string returnType(len, '\0');
            in.read(&returnType[0], len);
            func.returnType = returnType;

            size_t paramCount;
            in.read(reinterpret_cast<char*>(&paramCount), sizeof(paramCount));
            readv::validateCount(paramCount, constants::security::MAX_PARAMETERS_PER_FUNCTION, "function parameters");
            func.parameterNames.resize(paramCount);
            for (size_t j = 0; j < paramCount; ++j) {
                in.read(reinterpret_cast<char*>(&len), sizeof(len));
                readv::validateStringLen(len, "function parameter name");
                std::string paramName(len, '\0');
                in.read(&paramName[0], len);
                func.parameterNames[j] = paramName;
            }

            size_t entryCount;
            in.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));
            readv::validateCount(entryCount, constants::security::MAX_EXCEPTION_TABLE_ENTRIES, "function exception table entries");
            for (size_t j = 0; j < entryCount; ++j) {
                size_t startIP, endIP, catchIP, finallyIP, catchVarSlot;
                uint32_t nestingLevel;

                in.read(reinterpret_cast<char*>(&startIP), sizeof(startIP));
                in.read(reinterpret_cast<char*>(&endIP), sizeof(endIP));
                in.read(reinterpret_cast<char*>(&catchIP), sizeof(catchIP));
                in.read(reinterpret_cast<char*>(&finallyIP), sizeof(finallyIP));
                in.read(reinterpret_cast<char*>(&nestingLevel), sizeof(nestingLevel));
                in.read(reinterpret_cast<char*>(&catchVarSlot), sizeof(catchVarSlot));

                in.read(reinterpret_cast<char*>(&len), sizeof(len));
                readv::validateStringLen(len, "exception type");
                std::string exceptionType(len, '\0');
                in.read(&exceptionType[0], len);

                in.read(reinterpret_cast<char*>(&len), sizeof(len));
                readv::validateStringLen(len, "catch var name");
                std::string catchVarName(len, '\0');
                in.read(&catchVarName[0], len);

                readv::validateExceptionEntry(startIP, endIP, catchIP, finallyIP,
                                              instructions.size(),
                                              ("function '" + name + "'").c_str());

                ExceptionTableEntry entry(startIP, endIP, catchIP, finallyIP, exceptionType, nestingLevel, catchVarName, catchVarSlot);
                func.exceptionTable.addEntry(entry);
            }

            // MYT-110: function-level + per-parameter annotations (v7+).
            readAnnotationList(in, func.annotations);
            size_t fnParamAnnotCount = 0;
            in.read(reinterpret_cast<char*>(&fnParamAnnotCount), sizeof(fnParamAnnotCount));
            if (!in) throw std::runtime_error("Malformed bytecode: failed to read function parameter-annotation count");
            readv::validateCount(fnParamAnnotCount, constants::security::MAX_PARAMETERS_PER_FUNCTION, "function parameter-annotation count");
            func.parameterAnnotations.assign(fnParamAnnotCount, {});
            for (size_t j = 0; j < fnParamAnnotCount; ++j) {
                readAnnotationList(in, func.parameterAnnotations[j]);
            }

            registerFunction(name, func);
        }
    }

    void BytecodeProgram::readAnnotationList(std::istream& in, std::vector<AnnotationData>& list) {
        // Defensive caps: corrupted/truncated .mtc files could otherwise resize
        // vectors from garbage bytes. Real annotations rarely exceed a few dozen.
        constexpr size_t MAX_ANNOTATIONS_PER_HOST = 4096;
        constexpr size_t MAX_ARGS_PER_ANNOTATION  = 256;

        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        if (!in) throw std::runtime_error("Malformed bytecode: failed to read annotation count");
        if (count > MAX_ANNOTATIONS_PER_HOST)
            throw std::runtime_error("Malformed bytecode: annotation count " +
                std::to_string(count) + " exceeds cap " +
                std::to_string(MAX_ANNOTATIONS_PER_HOST));
        list.resize(count);
        for (auto& annot : list) {
            annot.name = BytecodeIOHelper::readString(in);
            int line, column;
            in.read(reinterpret_cast<char*>(&line),   sizeof(line));
            in.read(reinterpret_cast<char*>(&column), sizeof(column));
            std::string filename = BytecodeIOHelper::readString(in);
            annot.location.setLine(line);
            annot.location.setColumn(column);
            annot.location.setFilename(filename);
            size_t argCount;
            in.read(reinterpret_cast<char*>(&argCount), sizeof(argCount));
            if (!in) throw std::runtime_error("Malformed bytecode: failed to read annotation arg count");
            if (argCount > MAX_ARGS_PER_ANNOTATION)
                throw std::runtime_error("Malformed bytecode: annotation arg count " +
                    std::to_string(argCount) + " exceeds cap " +
                    std::to_string(MAX_ARGS_PER_ANNOTATION));
            annot.typedArguments.resize(argCount);
            for (auto& arg : annot.typedArguments) {
                arg.key       = BytecodeIOHelper::readString(in);
                arg.valueType = BytecodeIOHelper::readPrimitive<uint8_t>(in);
                arg.intVal    = BytecodeIOHelper::readPrimitive<int64_t>(in);
                arg.floatVal  = BytecodeIOHelper::readPrimitive<double>(in);
                arg.boolVal   = BytecodeIOHelper::readPrimitive<bool>(in);
                arg.stringVal = BytecodeIOHelper::readString(in);
                arg.arrayVal  = BytecodeIOHelper::readStringVector(in);
            }
        }
    }

    void BytecodeProgram::readFieldMetadata(std::istream& in, FieldMetadata& field) {
        field.name = BytecodeIOHelper::readString(in);
        field.type = BytecodeIOHelper::readString(in);
        field.isStatic = BytecodeIOHelper::readPrimitive<bool>(in);
        field.isFinal = BytecodeIOHelper::readPrimitive<bool>(in);
        field.isPrivate = BytecodeIOHelper::readPrimitive<bool>(in);
        field.isProtected = BytecodeIOHelper::readPrimitive<bool>(in);
        readAnnotationList(in, field.annotations);
    }

    void BytecodeProgram::readMethodMetadata(std::istream& in, MethodMetadata& method) {
        method.name = BytecodeIOHelper::readString(in);
        method.returnType = BytecodeIOHelper::readString(in);
        method.parameterTypes = BytecodeIOHelper::readStringVector(in);
        method.parameterNames = BytecodeIOHelper::readStringVector(in);
        method.isStatic = BytecodeIOHelper::readPrimitive<bool>(in);
        method.isFinal = BytecodeIOHelper::readPrimitive<bool>(in);
        method.isPrivate = BytecodeIOHelper::readPrimitive<bool>(in);
        method.isProtected = BytecodeIOHelper::readPrimitive<bool>(in);
        method.isAbstract = BytecodeIOHelper::readPrimitive<bool>(in);
        method.startOffset = BytecodeIOHelper::readPrimitive<size_t>(in);
        readAnnotationList(in, method.annotations);
        // MYT-110: per-parameter annotations.
        size_t paramCount = 0;
        in.read(reinterpret_cast<char*>(&paramCount), sizeof(paramCount));
        if (!in) throw std::runtime_error("Malformed bytecode: failed to read method parameter-annotation count");
        readv::validateCount(paramCount, constants::security::MAX_PARAMETERS_PER_FUNCTION, "method parameter-annotation count");
        method.parameterAnnotations.assign(paramCount, {});
        for (size_t i = 0; i < paramCount; ++i) {
            readAnnotationList(in, method.parameterAnnotations[i]);
        }
    }

    void BytecodeProgram::readConstructorMetadata(std::istream& in, ConstructorMetadata& ctor) {
        ctor.parameterTypes = BytecodeIOHelper::readStringVector(in);
        ctor.parameterNames = BytecodeIOHelper::readStringVector(in);
        ctor.startOffset = BytecodeIOHelper::readPrimitive<size_t>(in);
        readAnnotationList(in, ctor.annotations);
        // MYT-110: per-parameter annotations.
        size_t paramCount = 0;
        in.read(reinterpret_cast<char*>(&paramCount), sizeof(paramCount));
        if (!in) throw std::runtime_error("Malformed bytecode: failed to read constructor parameter-annotation count");
        readv::validateCount(paramCount, constants::security::MAX_PARAMETERS_PER_FUNCTION, "constructor parameter-annotation count");
        ctor.parameterAnnotations.assign(paramCount, {});
        for (size_t i = 0; i < paramCount; ++i) {
            readAnnotationList(in, ctor.parameterAnnotations[i]);
        }
    }

    void BytecodeProgram::readClassMetadata(std::istream& in, ClassMetadata& classMeta) {
        classMeta.name = BytecodeIOHelper::readString(in);
        classMeta.parentClassName = BytecodeIOHelper::readString(in);
        classMeta.implementedInterfaces = BytecodeIOHelper::readStringVector(in);
        classMeta.genericParameters = BytecodeIOHelper::readStringVector(in);

        classMeta.isAbstract = BytecodeIOHelper::readPrimitive<bool>(in);
        classMeta.isFinal = BytecodeIOHelper::readPrimitive<bool>(in);
        classMeta.isValueClass = BytecodeIOHelper::readPrimitive<bool>(in);

        // MYT-108 typed-args annotations.
        readAnnotationList(in, classMeta.annotations);

        size_t fieldCount;
        in.read(reinterpret_cast<char*>(&fieldCount), sizeof(fieldCount));
        classMeta.instanceFields.resize(fieldCount);
        for (auto& field : classMeta.instanceFields) {
            readFieldMetadata(in, field);
        }

        in.read(reinterpret_cast<char*>(&fieldCount), sizeof(fieldCount));
        classMeta.staticFields.resize(fieldCount);
        for (auto& field : classMeta.staticFields) {
            readFieldMetadata(in, field);
        }

        size_t methodCount;
        in.read(reinterpret_cast<char*>(&methodCount), sizeof(methodCount));
        classMeta.instanceMethods.resize(methodCount);
        for (auto& method : classMeta.instanceMethods) {
            readMethodMetadata(in, method);
        }

        in.read(reinterpret_cast<char*>(&methodCount), sizeof(methodCount));
        classMeta.staticMethods.resize(methodCount);
        for (auto& method : classMeta.staticMethods) {
            readMethodMetadata(in, method);
        }

        size_t ctorCount;
        in.read(reinterpret_cast<char*>(&ctorCount), sizeof(ctorCount));
        classMeta.constructors.resize(ctorCount);
        for (auto& ctor : classMeta.constructors) {
            readConstructorMetadata(in, ctor);
        }
    }

    void BytecodeProgram::readClasses(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));

        classes.resize(count);
        for (size_t i = 0; i < count; ++i) {
            readClassMetadata(in, classes[i]);
        }
    }

    void BytecodeProgram::readAnnotationDeclarations(std::istream& in) {
        // Sanity caps — a single .mtc with more than a few thousand annotation
        // types is almost certainly corrupted.
        constexpr size_t MAX_ANNOTATION_DECLS = 4096;
        constexpr size_t MAX_PARAMS_PER_DECL  = 256;

        size_t declCount = BytecodeIOHelper::readPrimitive<size_t>(in);
        if (!in) throw std::runtime_error("Malformed bytecode: failed to read annotation decl count");
        if (declCount > MAX_ANNOTATION_DECLS)
            throw std::runtime_error("Malformed bytecode: annotation decl count " +
                std::to_string(declCount) + " exceeds cap " +
                std::to_string(MAX_ANNOTATION_DECLS));
        annotationDeclarations.resize(declCount);
        for (auto& decl : annotationDeclarations) {
            decl.name = BytecodeIOHelper::readString(in);
            size_t paramCount = BytecodeIOHelper::readPrimitive<size_t>(in);
            if (!in) throw std::runtime_error("Malformed bytecode: failed to read annotation param count");
            if (paramCount > MAX_PARAMS_PER_DECL)
                throw std::runtime_error("Malformed bytecode: annotation param count " +
                    std::to_string(paramCount) + " exceeds cap " +
                    std::to_string(MAX_PARAMS_PER_DECL));
            decl.params.resize(paramCount);
            for (auto& p : decl.params) {
                p.name         = BytecodeIOHelper::readString(in);
                p.declaredType = BytecodeIOHelper::readPrimitive<uint8_t>(in);
                p.nullable     = BytecodeIOHelper::readPrimitive<bool>(in);
                p.isArray      = BytecodeIOHelper::readPrimitive<bool>(in);
                p.hasDefault   = BytecodeIOHelper::readPrimitive<bool>(in);
                if (p.hasDefault) {
                    p.defaultInt         = BytecodeIOHelper::readPrimitive<int64_t>(in);
                    p.defaultFloat       = BytecodeIOHelper::readPrimitive<double>(in);
                    p.defaultBool        = BytecodeIOHelper::readPrimitive<bool>(in);
                    p.defaultString      = BytecodeIOHelper::readString(in);
                    p.defaultStringArray = BytecodeIOHelper::readStringVector(in);
                }
            }
            // MYT-109 (.mtc v6+): meta-annotations on the annotation declaration.
            readAnnotationList(in, decl.metaAnnotations);
        }
    }

    void BytecodeProgram::readInterfaceMethodSignature(std::istream& in, InterfaceMethodSignature& sig) {
        sig.name = BytecodeIOHelper::readString(in);
        sig.returnType = BytecodeIOHelper::readString(in);
        sig.parameterTypes = BytecodeIOHelper::readStringVector(in);
        sig.parameterNames = BytecodeIOHelper::readStringVector(in);
        sig.genericTypeParameters = BytecodeIOHelper::readStringVector(in);
    }

    void BytecodeProgram::readInterfaceMetadata(std::istream& in, InterfaceMetadata& meta) {
        meta.name = BytecodeIOHelper::readString(in);
        meta.genericParameters = BytecodeIOHelper::readStringVector(in);
        meta.extendsInterfaces = BytecodeIOHelper::readStringVector(in);
        meta.isFinal = BytecodeIOHelper::readPrimitive<bool>(in);

        size_t methodCount;
        in.read(reinterpret_cast<char*>(&methodCount), sizeof(methodCount));
        meta.methods.resize(methodCount);
        for (auto& method : meta.methods) {
            readInterfaceMethodSignature(in, method);
        }
    }

    void BytecodeProgram::readInterfaces(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));

        if (count > constants::security::MAX_FUNCTION_COUNT) {
            throw std::runtime_error("Interface count exceeds security limit");
        }

        interfaces.resize(count);
        for (size_t i = 0; i < count; ++i) {
            readInterfaceMetadata(in, interfaces[i]);
        }
    }
}
