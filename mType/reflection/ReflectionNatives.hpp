#pragma once

#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
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
     */
    class ReflectionNatives
    {
    public:
        /**
         * @brief Register all reflection native functions in the environment
         */
        static void registerAll(std::shared_ptr<environment::Environment> env);

    private:
        // ========== Class Reflection Natives ==========

        /**
         * @brief Get Class object from class name
         * args[0]: className (String)
         * Returns: Class object (ObjectInstance with handle)
         */
        static value::Value __reflect_forName(const std::vector<value::Value>& args);

        /**
         * @brief Get simple class name (without package)
         * args[0]: classHandle (int)
         * Returns: String
         */
        static value::Value __reflect_getSimpleName(const std::vector<value::Value>& args);

        /**
         * @brief Get superclass Class object
         * args[0]: classHandle (int)
         * Returns: Class object or null
         */
        static value::Value __reflect_getSuperclass(const std::vector<value::Value>& args);

        /**
         * @brief Get implemented interfaces as Class array
         * args[0]: classHandle (int)
         * Returns: Class[] array
         */
        static value::Value __reflect_getInterfaces(const std::vector<value::Value>& args);

        /**
         * @brief Check if class is an interface
         * args[0]: classHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isInterface(const std::vector<value::Value>& args);

        /**
         * @brief Check if class is abstract
         * args[0]: classHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isAbstract(const std::vector<value::Value>& args);

        /**
         * @brief Check if class is final
         * args[0]: classHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isFinal(const std::vector<value::Value>& args);

        /**
         * @brief Check if object is instance of class
         * args[0]: classHandle (int)
         * args[1]: object (ObjectInstance)
         * Returns: bool
         */
        static value::Value __reflect_isInstance(const std::vector<value::Value>& args);

        /**
         * @brief Check if class is assignable from another
         * args[0]: thisClassHandle (int)
         * args[1]: otherClassHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isAssignableFrom(const std::vector<value::Value>& args);

        /**
         * @brief Create new instance using default constructor
         * args[0]: classHandle (int)
         * Returns: new ObjectInstance
         */
        static value::Value __reflect_newInstance(const std::vector<value::Value>& args);

        /**
         * @brief Check if class is generic
         * args[0]: classHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isGenericClass(const std::vector<value::Value>& args);

        /**
         * @brief Get generic type parameters
         * args[0]: classHandle (int)
         * Returns: String[] array of type parameter names
         */
        static value::Value __reflect_getTypeParameters(const std::vector<value::Value>& args);

        /**
         * @brief Get runtime type arguments bound to a closed parameterized class
         * args[0]: classHandle (int)
         * Returns: int[] array of class handles (empty for open/non-generic classes)
         *
         * For Box<Int>, returns [handle-for-Int]. For the open template Box,
         * returns an empty array. Each returned handle is itself a fully usable
         * class handle — open or closed as appropriate.
         */
        static value::Value __reflect_getTypeArguments(const std::vector<value::Value>& args);

        /**
         * @brief Get class modifier flags
         * args[0]: classHandle (int)
         * Returns: int (modifier flags)
         */
        static value::Value __reflect_getClassModifiers(const std::vector<value::Value>& args);

        /**
         * @brief Get canonical class name (parameterized form for closed handles)
         * args[0]: classHandle (int)
         * Returns: String
         *
         * For a closed handle representing Box<Int> returns "Box<Int>"; for an
         * open handle returns the raw class name ("Box"). Used to back
         * Class.getName() so nested type arguments render correctly.
         */
        static value::Value __reflect_getName(const std::vector<value::Value>& args);

        /**
         * @brief Get raw (template-level) class name, stripping any bindings
         * args[0]: classHandle (int)
         * Returns: String
         *
         * Always returns the underlying ClassDefinition's name. Mirrors
         * ValueObject::getClassName() semantics (Option A — the parameterized
         * name lives only on Class.getName, never on the raw class-name path).
         */
        static value::Value __reflect_getRawName(const std::vector<value::Value>& args);

        // ========== Field Reflection Natives ==========

        /**
         * @brief Get field by name
         * args[0]: classHandle (int)
         * args[1]: fieldName (String)
         * args[2]: declaredOnly (bool)
         * Returns: Field object
         */
        static value::Value __reflect_getField(const std::vector<value::Value>& args);

        /**
         * @brief Get all fields
         * args[0]: classHandle (int)
         * args[1]: declaredOnly (bool)
         * Returns: Field[] array
         */
        static value::Value __reflect_getFields(const std::vector<value::Value>& args);

        /**
         * @brief Get field type as Class
         * args[0]: fieldHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getFieldType(const std::vector<value::Value>& args);

        /**
         * @brief Get field declaring class
         * args[0]: classHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getFieldDeclaringClass(const std::vector<value::Value>& args);

        /**
         * @brief Get field modifier flags
         * args[0]: fieldHandle (int)
         * Returns: int (modifier flags)
         */
        static value::Value __reflect_getFieldModifiers(const std::vector<value::Value>& args);

        /**
         * @brief Get field value from instance
         * args[0]: instance (ObjectInstance)
         * args[1]: fieldHandle (int)
         * args[2]: accessible (bool) - bypass access control
         * Returns: field value
         */
        static value::Value __reflect_getFieldValue(const std::vector<value::Value>& args);

        /**
         * @brief Set field value on instance
         * args[0]: instance (ObjectInstance)
         * args[1]: fieldHandle (int)
         * args[2]: value (any)
         * args[3]: accessible (bool) - bypass access control
         * Returns: void
         */
        static value::Value __reflect_setFieldValue(const std::vector<value::Value>& args);

        /**
         * @brief Get static field value
         * args[0]: classHandle (int)
         * args[1]: fieldHandle (int)
         * Returns: field value
         */
        static value::Value __reflect_getStaticFieldValue(const std::vector<value::Value>& args);

        /**
         * @brief Set static field value
         * args[0]: classHandle (int)
         * args[1]: fieldHandle (int)
         * args[2]: value (any)
         * Returns: void
         */
        static value::Value __reflect_setStaticFieldValue(const std::vector<value::Value>& args);

        /**
         * @brief Get field name
         * args[0]: fieldHandle (int)
         * Returns: String
         */
        static value::Value __reflect_getFieldName(const std::vector<value::Value>& args);

        // ========== Method Reflection Natives ==========

        /**
         * @brief Get method by name and parameter types
         * args[0]: classHandle (int)
         * args[1]: methodName (String)
         * args[2]: paramTypes (array of class handles)
         * args[3]: declaredOnly (bool)
         * Returns: Method object
         */
        static value::Value __reflect_getMethod(const std::vector<value::Value>& args);

        /**
         * @brief Get all methods
         * args[0]: classHandle (int)
         * args[1]: declaredOnly (bool)
         * Returns: Method[] array
         */
        static value::Value __reflect_getMethods(const std::vector<value::Value>& args);

        /**
         * @brief Get method return type as Class
         * args[0]: methodHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getMethodReturnType(const std::vector<value::Value>& args);

        /**
         * @brief Get method parameter types as Class array
         * args[0]: methodHandle (int)
         * Returns: Class[] array
         */
        static value::Value __reflect_getMethodParamTypes(const std::vector<value::Value>& args);

        /**
         * @brief Get method parameter count
         * args[0]: methodHandle (int)
         * Returns: int
         */
        static value::Value __reflect_getMethodParamCount(const std::vector<value::Value>& args);

        /**
         * @brief Get method declaring class
         * args[0]: classHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getMethodDeclaringClass(const std::vector<value::Value>& args);

        /**
         * @brief Get method modifier flags
         * args[0]: methodHandle (int)
         * Returns: int (modifier flags)
         */
        static value::Value __reflect_getMethodModifiers(const std::vector<value::Value>& args);

        /**
         * @brief Check if method is async
         * args[0]: methodHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isMethodAsync(const std::vector<value::Value>& args);

        /**
         * @brief Check if method is generic
         * args[0]: methodHandle (int)
         * Returns: bool
         */
        static value::Value __reflect_isMethodGeneric(const std::vector<value::Value>& args);

        /**
         * @brief Invoke instance method
         * args[0]: instance (ObjectInstance)
         * args[1]: methodHandle (int)
         * args[2]: arguments (array)
         * args[3]: accessible (bool) - bypass access control
         * Returns: method return value
         */
        static value::Value __reflect_invokeMethod(const std::vector<value::Value>& args);

        /**
         * @brief Invoke static method
         * args[0]: classHandle (int)
         * args[1]: methodHandle (int)
         * args[2]: arguments (array)
         * args[3]: accessible (bool) - bypass access control
         * Returns: method return value
         */
        static value::Value __reflect_invokeStaticMethod(const std::vector<value::Value>& args);

        /**
         * @brief Get method name
         * args[0]: methodHandle (int)
         * Returns: String
         */
        static value::Value __reflect_getMethodName(const std::vector<value::Value>& args);

        // ========== Constructor Reflection Natives ==========

        /**
         * @brief Get constructor by parameter types
         * args[0]: classHandle (int)
         * args[1]: paramTypes (array of class handles)
         * args[2]: declaredOnly (bool)
         * Returns: Constructor object
         */
        static value::Value __reflect_getConstructor(const std::vector<value::Value>& args);

        /**
         * @brief Get all constructors
         * args[0]: classHandle (int)
         * args[1]: declaredOnly (bool)
         * Returns: Constructor[] array
         */
        static value::Value __reflect_getConstructors(const std::vector<value::Value>& args);

        /**
         * @brief Get constructor parameter types as Class array
         * args[0]: ctorHandle (int)
         * Returns: Class[] array
         */
        static value::Value __reflect_getConstructorParamTypes(const std::vector<value::Value>& args);

        /**
         * @brief Get constructor parameter count
         * args[0]: ctorHandle (int)
         * Returns: int
         */
        static value::Value __reflect_getConstructorParamCount(const std::vector<value::Value>& args);

        /**
         * @brief Get constructor modifier flags
         * args[0]: ctorHandle (int)
         * Returns: int (modifier flags)
         */
        static value::Value __reflect_getConstructorModifiers(const std::vector<value::Value>& args);

        /**
         * @brief Create new instance with constructor arguments
         * args[0]: classHandle (int)
         * args[1]: ctorHandle (int)
         * args[2]: arguments (array)
         * args[3]: accessible (bool) - bypass access control
         * Returns: new ObjectInstance
         */
        static value::Value __reflect_newInstanceWithArgs(const std::vector<value::Value>& args);

        /**
         * @brief Get constructor declaring class
         * args[0]: classHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getConstructorDeclaringClass(const std::vector<value::Value>& args);

        // ========== Annotation Reflection Natives ==========

        /**
         * @brief Get class annotations
         * args[0]: classHandle (int)
         * Returns: Annotation[] array
         */
        static value::Value __reflect_getClassAnnotations(const std::vector<value::Value>& args);

        /**
         * @brief Get specific class annotation by name
         * args[0]: classHandle (int)
         * args[1]: annotationName (String)
         * Returns: Annotation object or null
         */
        static value::Value __reflect_getClassAnnotation(const std::vector<value::Value>& args);

        /**
         * @brief Check if class has specific annotation
         * args[0]: classHandle (int)
         * args[1]: annotationName (String)
         * Returns: bool
         */
        static value::Value __reflect_hasClassAnnotation(const std::vector<value::Value>& args);

        /**
         * @brief Get method annotations
         * args[0]: methodHandle (int)
         * Returns: Annotation[] array
         */
        static value::Value __reflect_getMethodAnnotations(const std::vector<value::Value>& args);

        /**
         * @brief Get field annotations
         * args[0]: fieldHandle (int)
         * Returns: Annotation[] array
         */
        static value::Value __reflect_getFieldAnnotations(const std::vector<value::Value>& args);

        /**
         * @brief Get annotation parameter value
         * args[0]: annotationHandle (int)
         * args[1]: parameterKey (String)
         * Returns: String (parameter value)
         */
        static value::Value __reflect_getAnnotationParam(const std::vector<value::Value>& args);

        /**
         * @brief Check if annotation has parameter
         * args[0]: annotationHandle (int)
         * args[1]: parameterKey (String)
         * Returns: bool
         */
        static value::Value __reflect_hasAnnotationParam(const std::vector<value::Value>& args);

        /**
         * @brief Get all annotation parameter keys
         * args[0]: annotationHandle (int)
         * Returns: String[] array
         */
        static value::Value __reflect_getAnnotationParams(const std::vector<value::Value>& args);

        // ========== Typed Annotation Parameter Accessors (MYT-108) ==========

        /// args: (annotationHandle:int, paramKey:string) -> int (throws on type mismatch)
        static value::Value __reflect_getAnnotationInt(const std::vector<value::Value>& args);
        /// args: (annotationHandle:int, paramKey:string) -> float
        static value::Value __reflect_getAnnotationFloat(const std::vector<value::Value>& args);
        /// args: (annotationHandle:int, paramKey:string) -> bool
        static value::Value __reflect_getAnnotationBool(const std::vector<value::Value>& args);
        /// args: (annotationHandle:int, paramKey:string) -> string
        static value::Value __reflect_getAnnotationString(const std::vector<value::Value>& args);
        /// args: (annotationHandle:int, paramKey:string) -> Class (handle)
        static value::Value __reflect_getAnnotationClass(const std::vector<value::Value>& args);
        /// args: (annotationHandle:int, paramKey:string) -> int[] (Class handles)
        static value::Value __reflect_getAnnotationClassArray(const std::vector<value::Value>& args);
        /// args: (annotationHandle:int, paramKey:string) -> bool
        static value::Value __reflect_isAnnotationParamNull(const std::vector<value::Value>& args);

        // Method-level annotation reflection (singular accessors, MYT-108)
        /// args: (methodHandle:int, annotationName:string) -> Annotation handle or null
        static value::Value __reflect_getMethodAnnotation(const std::vector<value::Value>& args);
        /// args: (methodHandle:int, annotationName:string) -> bool
        static value::Value __reflect_hasMethodAnnotation(const std::vector<value::Value>& args);

        // Field-level annotation reflection (MYT-108 M5)
        /// args: (fieldHandle:int, annotationName:string) -> Annotation handle or null
        static value::Value __reflect_getFieldAnnotation(const std::vector<value::Value>& args);
        /// args: (fieldHandle:int, annotationName:string) -> bool
        static value::Value __reflect_hasFieldAnnotation(const std::vector<value::Value>& args);

        // Constructor-level annotation reflection (MYT-109)
        /// args: (ctorHandle:int) -> int[] (Annotation handles)
        static value::Value __reflect_getConstructorAnnotations(const std::vector<value::Value>& args);
        /// args: (ctorHandle:int, annotationName:string) -> Annotation handle or null
        static value::Value __reflect_getConstructorAnnotation(const std::vector<value::Value>& args);
        /// args: (ctorHandle:int, annotationName:string) -> bool
        static value::Value __reflect_hasConstructorAnnotation(const std::vector<value::Value>& args);

        // Meta-annotation reflection (MYT-109 3a)
        /// args: (annotationHandle:int, metaAnnotationName:string) -> Annotation handle or null
        static value::Value __reflect_getAnnotationMeta(const std::vector<value::Value>& args);

        // Object-style annotation access (MYT-108 M6)
        /// args: (target:int, annotationClassHandle:int) -> ObjectInstance or null
        static value::Value __reflect_getAnnotationObject(const std::vector<value::Value>& args);

        // ========== Parameter Reflection Natives ==========

        /**
         * @brief Get method parameters as Parameter array
         * args[0]: methodHandle (int)
         * Returns: Parameter[] array
         */
        static value::Value __reflect_getMethodParameters(const std::vector<value::Value>& args);

        /**
         * @brief Get constructor parameters as Parameter array
         * args[0]: ctorHandle (int)
         * Returns: Parameter[] array
         */
        static value::Value __reflect_getConstructorParameters(const std::vector<value::Value>& args);

        /**
         * @brief Get parameter type as Class
         * args[0]: typeHandle (int)
         * Returns: Class object
         */
        static value::Value __reflect_getParameterType(const std::vector<value::Value>& args);

    private:
        // ========== Helper Methods ==========

        static void validateArgCount(const std::vector<value::Value>& args, size_t expected, const std::string& funcName);
        static int64_t extractInt(const value::Value& arg, const std::string& funcName, const std::string& paramName);
        static const std::string& extractString(const value::Value& arg, const std::string& funcName, const std::string& paramName);
        static bool extractBool(const value::Value& arg, const std::string& funcName, const std::string& paramName);
        static std::shared_ptr<runtimeTypes::klass::ObjectInstance> extractObject(
            const value::Value& arg, const std::string& funcName, const std::string& paramName);

        // Convert ValueType to type name string
        static std::string valueTypeToTypeName(value::ValueType type);

        // Get Class handle for a type name (creates primitive class handles if needed)
        static int getClassHandleForTypeName(const std::string& typeName,
                                             std::shared_ptr<environment::Environment> env);

        // Environment reference for class registry lookups
        static std::shared_ptr<environment::Environment> currentEnvironment;

        // VM reference for method/constructor invocation
        static std::shared_ptr<vm::runtime::VirtualMachine> currentVM;

    public:
        // Set the current environment (called during initialization)
        static void setEnvironment(std::shared_ptr<environment::Environment> env);

        // Set the current VM (called during initialization)
        static void setVM(std::shared_ptr<vm::runtime::VirtualMachine> vm);

        // Cleanup static resources (call before program exit to avoid static destruction order issues)
        static void cleanup();
    };

} // namespace reflection
