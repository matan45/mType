#pragma once

#include <span>
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include <memory>
#include <vector>

namespace vm::runtime { class VirtualMachine; }

namespace reflection
{
    /**
     * @brief Native functions for the mType reflection API
     */
    class ReflectionNatives
    {
    public:
        /**
         * @brief Register all reflection native functions in the environment
         */
        static void registerAll(std::shared_ptr<environment::Environment> env);

        // Cleanup static resources
        static void cleanup();

    private:
        using NativeContext = environment::NativeContext;

        // ========== Class Reflection Natives ==========
        static value::Value __reflect_forName(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getSimpleName(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getSuperclass(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getInterfaces(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_isInterface(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_isAbstract(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_isFinal(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_isInstance(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_isAssignableFrom(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_newInstance(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_isGenericClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getTypeParameters(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getTypeArguments(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getClassModifiers(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getName(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getRawName(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Field Reflection Natives ==========
        static value::Value __reflect_getField(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getFields(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getFieldType(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getFieldDeclaringClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getFieldModifiers(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getFieldValue(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_setFieldValue(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getStaticFieldValue(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_setStaticFieldValue(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getFieldName(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Method Reflection Natives ==========
        static value::Value __reflect_getMethod(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethods(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodReturnType(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodParamTypes(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodParamCount(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodDeclaringClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodModifiers(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_isMethodAsync(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_isMethodGeneric(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_invokeMethod(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_invokeStaticMethod(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodName(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Constructor Reflection Natives ==========
        static value::Value __reflect_getConstructor(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructors(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructorParamTypes(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructorParamCount(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructorModifiers(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_newInstanceWithArgs(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructorDeclaringClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Annotation Reflection Natives ==========
        static value::Value __reflect_getClassAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getClassAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_hasClassAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getFieldAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationParam(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_hasAnnotationParam(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationParams(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationInt(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationFloat(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationBool(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationString(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationClassArray(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_isAnnotationParamNull(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_hasMethodAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getFieldAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_hasFieldAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructorAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructorAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_hasConstructorAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationMeta(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodParameterAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getMethodParameterAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_hasMethodParameterAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructorParameterAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructorParameterAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_hasConstructorParameterAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getAnnotationObject(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Parameter Reflection Natives ==========
        static value::Value __reflect_getMethodParameters(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getConstructorParameters(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __reflect_getParameterType(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Helper Methods ==========
        static void validateArgCount(std::span<const value::Value> args, size_t expected, const std::string& funcName);
        static int64_t extractInt(const value::Value& arg, const std::string& funcName, const std::string& paramName);
        static const std::string& extractString(const value::Value& arg, const std::string& funcName, const std::string& paramName);
        static bool extractBool(const value::Value& arg, const std::string& funcName, const std::string& paramName);
        static std::shared_ptr<runtimeTypes::klass::ObjectInstance> extractObject(
            const value::Value& arg, const std::string& funcName, const std::string& paramName);
        static std::string valueTypeToTypeName(value::ValueType type);
        static int getClassHandleForTypeName(const std::string& typeName, environment::Environment& env);
    };

} // namespace reflection
