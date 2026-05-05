#pragma once

#include <span>
#include <cstddef>
#include <cstdint>
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
     *
     * Provides the C++ implementation of reflection operations that are
     * called from mType code. These functions bridge the gap between
     * the mType reflection classes (Class, Field, Method, Constructor)
     * and the underlying C++ runtime type definitions.
     *
     * Usage in mType:
     *   Class clazz = Class::forName("Person");
     *   Field[] fields = clazz.getDeclaredFields();
     *
     * Each native uses the NativeBinder raw-form signature:
     *   (void* userData, NativeContext& ctx, std::span<const Value> args)
     * — userData is unused, ctx supplies env / vm pointers, args is the
     * mType-side argument span.
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

        /**
         * @brief Get Class object from class name
         * args[0]: className (String)
         * Returns: Class object (ObjectInstance with handle)
         */
        static value::Value __reflect_forName(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get simple class name (without package)
         * args[0]: classHandle (int)
         * Returns: String
         */
        static value::Value __reflect_getSimpleName(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get superclass Class object
         * args[0]: classHandle (int)
         * Returns: Class object or null
         */
        static value::Value __reflect_getSuperclass(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get implemented interfaces as Class array
         * args[0]: classHandle (int)
         * Returns: Class[] array
         */
        static value::Value __reflect_getInterfaces(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if class is an interface
         * args[0]: classHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isInterface(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if class is abstract
         * args[0]: classHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isAbstract(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if class is final
         * args[0]: classHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isFinal(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if object is instance of class
         * args[0]: classHandle (int)
         * args[1]: object (ObjectInstance)
         * Returns: bool
         */
        static value::Value __reflect_isInstance(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if class is assignable from another
         * args[0]: thisClassHandle (int)
         * args[1]: otherClassHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isAssignableFrom(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Create new instance using default constructor
         * args[0]: classHandle (int)
         * Returns: new ObjectInstance
         */
        static value::Value __reflect_newInstance(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if class is generic
         * args[0]: classHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isGenericClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get generic type parameters
         * args[0]: classHandle (int)
         * Returns: String[] array of type parameter names
         */
        static value::Value __reflect_getTypeParameters(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get runtime type arguments bound to a closed parameterized class
         * args[0]: classHandle (int)
         * Returns: int[] array of class handles (empty for open/non-generic classes)
         *
         * For Box<Int>, returns [handle-for-Int]. For the open template Box,
         * returns an empty array. Each returned handle is itself a fully usable
         * class handle — open or closed as appropriate.
         */
        static value::Value __reflect_getTypeArguments(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get class modifier flags
         * args[0]: classHandle (int)
         * Returns: int (modifier flags)
         */
        static value::Value __reflect_getClassModifiers(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get canonical class name (parameterized form for closed handles)
         * args[0]: classHandle (int)
         * Returns: String
         *
         * For a closed handle representing Box<Int> returns "Box<Int>"; for an
         * open handle returns the raw class name ("Box"). Used to back
         * Class.getName() so nested type arguments render correctly.
         */
        static value::Value __reflect_getName(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get raw (template-level) class name, stripping any bindings
         * args[0]: classHandle (int)
         * Returns: String
         *
         * Always returns the underlying ClassDefinition's name. Mirrors
         * ValueObject::getClassName() semantics (Option A — the parameterized
         * name lives only on Class.getName, never on the raw class-name path).
         */
        static value::Value __reflect_getRawName(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Field Reflection Natives ==========

        /**
         * @brief Get field by name
         * args[0]: classHandle (int)
         * args[1]: fieldName (String)
         * args[2]: declaredOnly (bool)
         * Returns: Field object
         */
        static value::Value __reflect_getField(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get all fields
         * args[0]: classHandle (int)
         * args[1]: declaredOnly (bool)
         * Returns: Field[] array
         */
        static value::Value __reflect_getFields(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get field type as Class
         * args[0]: fieldHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getFieldType(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get field declaring class
         * args[0]: classHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getFieldDeclaringClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get field modifier flags
         * args[0]: fieldHandle (int)
         * Returns: int (modifier flags)
         */
        static value::Value __reflect_getFieldModifiers(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get field value from instance
         * args[0]: instance (ObjectInstance)
         * args[1]: fieldHandle (int)
         * args[2]: accessible (bool) - bypass access control
         * Returns: field value
         */
        static value::Value __reflect_getFieldValue(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Set field value on instance
         * args[0]: instance (ObjectInstance)
         * args[1]: fieldHandle (int)
         * args[2]: value (any)
         * args[3]: accessible (bool) - bypass access control
         * Returns: void
         */
        static value::Value __reflect_setFieldValue(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get static field value
         * args[0]: classHandle (int)
         * args[1]: fieldHandle (int)
         * Returns: field value
         */
        static value::Value __reflect_getStaticFieldValue(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Set static field value
         * args[0]: classHandle (int)
         * args[1]: fieldHandle (int)
         * args[2]: value (any)
         * Returns: void
         */
        static value::Value __reflect_setStaticFieldValue(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get field name
         * args[0]: fieldHandle (int)
         * Returns: String
         */
        static value::Value __reflect_getFieldName(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Method Reflection Natives ==========

        /**
         * @brief Get method by name and parameter types
         * args[0]: classHandle (int)
         * args[1]: methodName (String)
         * args[2]: paramTypes (array of class handles)
         * args[3]: declaredOnly (bool)
         * Returns: Method object
         */
        static value::Value __reflect_getMethod(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get all methods
         * args[0]: classHandle (int)
         * args[1]: declaredOnly (bool)
         * Returns: Method[] array
         */
        static value::Value __reflect_getMethods(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get method return type as Class
         * args[0]: methodHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getMethodReturnType(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get method parameter types as Class array
         * args[0]: methodHandle (int)
         * Returns: Class[] array
         */
        static value::Value __reflect_getMethodParamTypes(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get method parameter count
         * args[0]: methodHandle (int)
         * Returns: int
         */
        static value::Value __reflect_getMethodParamCount(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get method declaring class
         * args[0]: classHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getMethodDeclaringClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get method modifier flags
         * args[0]: methodHandle (int)
         * Returns: int (modifier flags)
         */
        static value::Value __reflect_getMethodModifiers(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if method is async
         * args[0]: methodHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isMethodAsync(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if method is generic
         * args[0]: methodHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isMethodGeneric(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Invoke instance method
         * args[0]: instance (ObjectInstance)
         * args[1]: methodHandle (int)
         * args[2]: arguments (array)
         * args[3]: accessible (bool) - bypass access control
         * Returns: method return value
         */
        static value::Value __reflect_invokeMethod(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Invoke static method
         * args[0]: classHandle (int)
         * args[1]: methodHandle (int)
         * args[2]: arguments (array)
         * args[3]: accessible (bool) - bypass access control
         * Returns: method return value
         */
        static value::Value __reflect_invokeStaticMethod(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get method name
         * args[0]: methodHandle (int)
         * Returns: String
         */
        static value::Value __reflect_getMethodName(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Constructor Reflection Natives ==========

        /**
         * @brief Get constructor by parameter types
         * args[0]: classHandle (int)
         * args[1]: paramTypes (array of class handles)
         * args[2]: declaredOnly (bool)
         * Returns: Constructor object
         */
        static value::Value __reflect_getConstructor(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get all constructors
         * args[0]: classHandle (int)
         * args[1]: declaredOnly (bool)
         * Returns: Constructor[] array
         */
        static value::Value __reflect_getConstructors(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get constructor parameter types as Class array
         * args[0]: ctorHandle (int)
         * Returns: Class[] array
         */
        static value::Value __reflect_getConstructorParamTypes(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get constructor parameter count
         * args[0]: ctorHandle (int)
         * Returns: int
         */
        static value::Value __reflect_getConstructorParamCount(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get constructor modifier flags
         * args[0]: ctorHandle (int)
         * Returns: int (modifier flags)
         */
        static value::Value __reflect_getConstructorModifiers(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Create new instance with constructor arguments
         * args[0]: classHandle (int)
         * args[1]: ctorHandle (int)
         * args[2]: arguments (array)
         * args[3]: accessible (bool) - bypass access control
         * Returns: new ObjectInstance
         */
        static value::Value __reflect_newInstanceWithArgs(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get constructor declaring class
         * args[0]: classHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getConstructorDeclaringClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Annotation Reflection Natives ==========

        /**
         * @brief Get class annotations
         * args[0]: classHandle (int)
         * Returns: Annotation[] array
         */
        static value::Value __reflect_getClassAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get specific class annotation by name
         * args[0]: classHandle (int)
         * args[1]: annotationName (String)
         * Returns: Annotation object or null
         */
        static value::Value __reflect_getClassAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if class has specific annotation
         * args[0]: classHandle (int)
         * args[1]: annotationName (String)
         * Returns: bool
         */
        static value::Value __reflect_hasClassAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get method annotations
         * args[0]: methodHandle (int)
         * Returns: Annotation[] array
         */
        static value::Value __reflect_getMethodAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get field annotations
         * args[0]: fieldHandle (int)
         * Returns: Annotation[] array
         */
        static value::Value __reflect_getFieldAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get annotation parameter value
         * args[0]: annotationHandle (int)
         * args[1]: parameterKey (String)
         * Returns: String (parameter value)
         */
        static value::Value __reflect_getAnnotationParam(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Check if annotation has parameter
         * args[0]: annotationHandle (int)
         * args[1]: parameterKey (String)
         * Returns: bool
         */
        static value::Value __reflect_hasAnnotationParam(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get all annotation parameter keys
         * args[0]: annotationHandle (int)
         * Returns: String[] array
         */
        static value::Value __reflect_getAnnotationParams(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Typed Annotation Parameter Accessors (MYT-108) ==========

        /// args: (annotationHandle:int, paramKey:string) -> int (throws on type mismatch)
        static value::Value __reflect_getAnnotationInt(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (annotationHandle:int, paramKey:string) -> float
        static value::Value __reflect_getAnnotationFloat(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (annotationHandle:int, paramKey:string) -> bool
        static value::Value __reflect_getAnnotationBool(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (annotationHandle:int, paramKey:string) -> string
        static value::Value __reflect_getAnnotationString(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (annotationHandle:int, paramKey:string) -> Class (handle)
        static value::Value __reflect_getAnnotationClass(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (annotationHandle:int, paramKey:string) -> int[] (Class handles)
        static value::Value __reflect_getAnnotationClassArray(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (annotationHandle:int, paramKey:string) -> bool
        static value::Value __reflect_isAnnotationParamNull(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // Method-level annotation reflection (singular accessors, MYT-108)
        /// args: (methodHandle:int, annotationName:string) -> Annotation handle or null
        static value::Value __reflect_getMethodAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (methodHandle:int, annotationName:string) -> bool
        static value::Value __reflect_hasMethodAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // Field-level annotation reflection (MYT-108 M5)
        /// args: (fieldHandle:int, annotationName:string) -> Annotation handle or null
        static value::Value __reflect_getFieldAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (fieldHandle:int, annotationName:string) -> bool
        static value::Value __reflect_hasFieldAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // Constructor-level annotation reflection (MYT-109)
        /// args: (ctorHandle:int) -> int[] (Annotation handles)
        static value::Value __reflect_getConstructorAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (ctorHandle:int, annotationName:string) -> Annotation handle or null
        static value::Value __reflect_getConstructorAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (ctorHandle:int, annotationName:string) -> bool
        static value::Value __reflect_hasConstructorAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // Meta-annotation reflection (MYT-109 3a)
        /// args: (annotationHandle:int, metaAnnotationName:string) -> Annotation handle or null
        static value::Value __reflect_getAnnotationMeta(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // Parameter-level annotation reflection (MYT-110)
        /// args: (methodHandle:int, paramIndex:int) -> int[] (Annotation handles)
        static value::Value __reflect_getMethodParameterAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (methodHandle:int, paramIndex:int, annotationName:string) -> Annotation handle or null
        static value::Value __reflect_getMethodParameterAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (methodHandle:int, paramIndex:int, annotationName:string) -> bool
        static value::Value __reflect_hasMethodParameterAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /// args: (ctorHandle:int, paramIndex:int) -> int[] (Annotation handles)
        static value::Value __reflect_getConstructorParameterAnnotations(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (ctorHandle:int, paramIndex:int, annotationName:string) -> Annotation handle or null
        static value::Value __reflect_getConstructorParameterAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        /// args: (ctorHandle:int, paramIndex:int, annotationName:string) -> bool
        static value::Value __reflect_hasConstructorParameterAnnotation(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // Object-style annotation access (MYT-108 M6)
        /// args: (target:int, annotationClassHandle:int) -> ObjectInstance or null
        static value::Value __reflect_getAnnotationObject(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ========== Parameter Reflection Natives ==========

        /**
         * @brief Get method parameters as Parameter array
         * args[0]: methodHandle (int)
         * Returns: Parameter[] array
         */
        static value::Value __reflect_getMethodParameters(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get constructor parameters as Parameter array
         * args[0]: ctorHandle (int)
         * Returns: Parameter[] array
         */
        static value::Value __reflect_getConstructorParameters(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        /**
         * @brief Get parameter type as Class
         * args[0]: typeHandle (int)
         * Returns: Class object
         */
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
