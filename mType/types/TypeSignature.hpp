#pragma once
#include "../vm/bytecode/BytecodeProgram.hpp"
#include <string>
#include <vector>

namespace types
{
    /**
     * Canonical type signature encoder/decoder for cross-library type checking.
     *
     * Signature format:
     *   Primitives: "int", "float", "bool", "string", "void"
     *   Classes: "ClassName" or "ClassName<T1,T2>"
     *   Interfaces: "IName<T>"
     *   Arrays: "Type[]", "Type[][]"
     *   Lambdas: "(ParamType1,ParamType2)->ReturnType"
     *   Generic params: "T" or "T:UpperBound" or "T:Bound1&Bound2"
     *   Methods: "<T,K>methodName(ParamType1,ParamType2):ReturnType"
     */
    class TypeSignature
    {
    public:
        // Encode metadata to canonical type signatures
        static std::string encodeClassSignature(const vm::bytecode::BytecodeProgram::ClassMetadata& classMeta);
        static std::string encodeInterfaceSignature(const vm::bytecode::BytecodeProgram::InterfaceMetadata& interfaceMeta);
        static std::string encodeFunctionSignature(const vm::bytecode::BytecodeProgram::FunctionMetadata& funcMeta);
        static std::string encodeMethodSignature(const vm::bytecode::BytecodeProgram::MethodMetadata& methodMeta);

        // Encode individual type components
        static std::string encodeType(const std::string& typeName);
        static std::string encodeGenericParams(const std::vector<std::string>& params);

        // Signature compatibility checking
        static bool signaturesCompatible(const std::string& expected, const std::string& actual);

    private:
        static bool isArrayType(const std::string& type);
        static bool isLambdaType(const std::string& type);
        static bool isGenericType(const std::string& type);
        static std::string extractBaseType(const std::string& type);
    };
}
