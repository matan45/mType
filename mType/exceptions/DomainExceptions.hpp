#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

namespace mtype::exceptions {

    /**
     * Base exception for all mType-specific errors
     * Provides common functionality and context information
     */
    class MTypeException : public std::exception {
    protected:
        std::string message_;
        std::string location_;
        std::string context_;
        int errorCode_;

    public:
        MTypeException(const std::string& message,
                      const std::string& location = "",
                      const std::string& context = "",
                      int errorCode = 0)
            : message_(message), location_(location), context_(context), errorCode_(errorCode) {}

        const char* what() const noexcept override {
            return message_.c_str();
        }

        const std::string& getMessage() const { return message_; }
        const std::string& getLocation() const { return location_; }
        const std::string& getContext() const { return context_; }
        int getErrorCode() const { return errorCode_; }

        virtual std::string getDetailedMessage() const {
            std::string detailed = message_;
            if (!location_.empty()) {
                detailed += " at " + location_;
            }
            if (!context_.empty()) {
                detailed += " in " + context_;
            }
            return detailed;
        }
    };

    // ===============================
    // Type System Exceptions
    // ===============================

    /**
     * Base class for type-related errors
     */
    class TypeException : public MTypeException {
    private:
        std::string typeName_;

    public:
        TypeException(const std::string& message,
                     const std::string& typeName = "",
                     const std::string& location = "",
                     const std::string& context = "")
            : MTypeException(message, location, context, 1000), typeName_(typeName) {}

        const std::string& getTypeName() const { return typeName_; }

        std::string getDetailedMessage() const override {
            std::string detailed = "Type error: " + message_;
            if (!typeName_.empty()) {
                detailed += " (type: '" + typeName_ + "')";
            }
            if (!location_.empty()) {
                detailed += " at " + location_;
            }
            return detailed;
        }
    };

    /**
     * Thrown when type conversion fails
     */
    class TypeConversionException : public TypeException {
    private:
        std::string sourceType_;
        std::string targetType_;

    public:
        TypeConversionException(const std::string& message,
                              const std::string& sourceType = "",
                              const std::string& targetType = "",
                              const std::string& location = "")
            : TypeException(message, sourceType, location, "type conversion")
            , sourceType_(sourceType), targetType_(targetType) {}

        const std::string& getSourceType() const { return sourceType_; }
        const std::string& getTargetType() const { return targetType_; }
    };

    /**
     * Thrown when type resolution fails
     */
    class TypeResolutionException : public TypeException {
    public:
        TypeResolutionException(const std::string& message,
                              const std::string& typeName = "",
                              const std::string& location = "")
            : TypeException(message, typeName, location, "type resolution") {}
    };

    /**
     * Thrown when generic type instantiation fails
     */
    class GenericInstantiationException : public TypeException {
    private:
        std::vector<std::string> typeArguments_;

    public:
        GenericInstantiationException(const std::string& message,
                                    const std::string& genericType,
                                    const std::vector<std::string>& typeArgs = {},
                                    const std::string& location = "")
            : TypeException(message, genericType, location, "generic instantiation")
            , typeArguments_(typeArgs) {}

        const std::vector<std::string>& getTypeArguments() const { return typeArguments_; }
    };

    // ===============================
    // Lambda System Exceptions
    // ===============================

    /**
     * Base class for lambda-related errors
     */
    class LambdaException : public MTypeException {
    protected:
        std::string lambdaSignature_;

    public:
        LambdaException(const std::string& message,
                       const std::string& signature = "",
                       const std::string& location = "",
                       const std::string& context = "lambda")
            : MTypeException(message, location, context, 2000), lambdaSignature_(signature) {}

        const std::string& getLambdaSignature() const { return lambdaSignature_; }

        std::string getDetailedMessage() const override {
            std::string detailed = "Lambda error: " + message_;
            if (!lambdaSignature_.empty()) {
                detailed += " (signature: " + lambdaSignature_ + ")";
            }
            if (!location_.empty()) {
                detailed += " at " + location_;
            }
            return detailed;
        }
    };

    /**
     * Thrown when lambda parameter count doesn't match expected
     */
    class LambdaParameterMismatchException : public LambdaException {
    private:
        int expectedCount_;
        int actualCount_;

    public:
        LambdaParameterMismatchException(int expected, int actual,
                                       const std::string& signature = "",
                                       const std::string& location = "")
            : LambdaException("Parameter count mismatch", signature, location, "lambda parameter validation")
            , expectedCount_(expected), actualCount_(actual) {}

        int getExpectedCount() const { return expectedCount_; }
        int getActualCount() const { return actualCount_; }

        std::string getDetailedMessage() const override {
            return "Lambda parameter count mismatch: expected " +
                   std::to_string(expectedCount_) + ", got " +
                   std::to_string(actualCount_) +
                   (!lambdaSignature_.empty() ? " for " + lambdaSignature_ : "") +
                   (!location_.empty() ? " at " + location_ : "");
        }
    };

    /**
     * Thrown when lambda parameter types don't match interface
     */
    class LambdaParameterTypeException : public LambdaException {
    private:
        std::string expectedType_;
        std::string actualType_;
        int parameterIndex_;

    public:
        LambdaParameterTypeException(const std::string& expected,
                                   const std::string& actual,
                                   int paramIndex,
                                   const std::string& signature = "",
                                   const std::string& location = "")
            : LambdaException("Parameter type mismatch", signature, location, "lambda type validation")
            , expectedType_(expected), actualType_(actual), parameterIndex_(paramIndex) {}

        const std::string& getExpectedType() const { return expectedType_; }
        const std::string& getActualType() const { return actualType_; }
        int getParameterIndex() const { return parameterIndex_; }
    };

    /**
     * Thrown when lambda return type doesn't match interface
     */
    class LambdaReturnTypeException : public LambdaException {
    private:
        std::string expectedType_;
        std::string actualType_;

    public:
        LambdaReturnTypeException(const std::string& expected,
                                const std::string& actual,
                                const std::string& signature = "",
                                const std::string& location = "")
            : LambdaException("Return type mismatch", signature, location, "lambda return type validation")
            , expectedType_(expected), actualType_(actual) {}

        const std::string& getExpectedType() const { return expectedType_; }
        const std::string& getActualType() const { return actualType_; }
    };

    /**
     * Thrown when lambda tries to implement non-functional interface
     */
    class LambdaInterfaceException : public LambdaException {
    private:
        std::string interfaceName_;

    public:
        LambdaInterfaceException(const std::string& message,
                               const std::string& interfaceName = "",
                               const std::string& location = "")
            : LambdaException(message, "", location, "lambda interface validation")
            , interfaceName_(interfaceName) {}

        const std::string& getInterfaceName() const { return interfaceName_; }
    };

    /**
     * Thrown when lambda closure capture fails
     */
    class LambdaCaptureException : public LambdaException {
    private:
        std::string variableName_;

    public:
        LambdaCaptureException(const std::string& message,
                             const std::string& variableName = "",
                             const std::string& location = "")
            : LambdaException(message, "", location, "lambda closure capture")
            , variableName_(variableName) {}

        const std::string& getVariableName() const { return variableName_; }
    };

    // ===============================
    // Array System Exceptions
    // ===============================

    /**
     * Base class for array-related errors
     */
    class ArrayException : public MTypeException {
    protected:
        std::string arrayType_;
        std::vector<int> dimensions_;

    public:
        ArrayException(const std::string& message,
                      const std::string& arrayType = "",
                      const std::vector<int>& dimensions = {},
                      const std::string& location = "")
            : MTypeException(message, location, "array operation", 3000)
            , arrayType_(arrayType), dimensions_(dimensions) {}

        const std::string& getArrayType() const { return arrayType_; }
        const std::vector<int>& getDimensions() const { return dimensions_; }
    };

    /**
     * Thrown when array index is out of bounds
     */
    class ArrayIndexException : public ArrayException {
    private:
        int index_;
        int arraySize_;

    public:
        ArrayIndexException(int index, int size,
                          const std::string& arrayType = "",
                          const std::string& location = "")
            : ArrayException("Array index out of bounds", arrayType, {size}, location)
            , index_(index), arraySize_(size) {}

        int getIndex() const { return index_; }
        int getArraySize() const { return arraySize_; }

        std::string getDetailedMessage() const override {
            return "Array index out of bounds: index " + std::to_string(index_) +
                   " is not valid for array of size " + std::to_string(arraySize_) +
                   (!arrayType_.empty() ? " (type: " + arrayType_ + ")" : "") +
                   (!location_.empty() ? " at " + location_ : "");
        }
    };

    /**
     * Thrown when array creation fails
     */
    class ArrayCreationException : public ArrayException {
    public:
        ArrayCreationException(const std::string& message,
                             const std::string& arrayType = "",
                             const std::vector<int>& dimensions = {},
                             const std::string& location = "")
            : ArrayException(message, arrayType, dimensions, location) {}
    };

    // ===============================
    // Object System Exceptions
    // ===============================

    /**
     * Base class for object-related errors
     */
    class ObjectException : public MTypeException {
    protected:
        std::string className_;

    public:
        ObjectException(const std::string& message,
                       const std::string& className = "",
                       const std::string& location = "")
            : MTypeException(message, location, "object operation", 4000), className_(className) {}

        const std::string& getClassName() const { return className_; }
    };

    /**
     * Thrown when null object is accessed
     */
    class NullPointerException : public ObjectException {
    private:
        std::string operation_;

    public:
        NullPointerException(const std::string& operation = "",
                           const std::string& location = "")
            : ObjectException("Null pointer access", "", location), operation_(operation) {}

        const std::string& getOperation() const { return operation_; }

        std::string getDetailedMessage() const override {
            std::string detailed = "Null pointer exception";
            if (!operation_.empty()) {
                detailed += " during " + operation_;
            }
            if (!location_.empty()) {
                detailed += " at " + location_;
            }
            return detailed;
        }
    };

    /**
     * Thrown when class is not found
     */
    class ClassNotFoundException : public ObjectException {
    public:
        ClassNotFoundException(const std::string& className,
                             const std::string& location = "")
            : ObjectException("Class not found: " + className, className, location) {}
    };

    /**
     * Thrown when method is not found
     */
    class MethodNotFoundException : public ObjectException {
    private:
        std::string methodName_;

    public:
        MethodNotFoundException(const std::string& methodName,
                              const std::string& className = "",
                              const std::string& location = "")
            : ObjectException("Method not found: " + methodName, className, location)
            , methodName_(methodName) {}

        const std::string& getMethodName() const { return methodName_; }
    };

    /**
     * Thrown when field is not found
     */
    class FieldNotFoundException : public ObjectException {
    private:
        std::string fieldName_;

    public:
        FieldNotFoundException(const std::string& fieldName,
                             const std::string& className = "",
                             const std::string& location = "")
            : ObjectException("Field not found: " + fieldName, className, location)
            , fieldName_(fieldName) {}

        const std::string& getFieldName() const { return fieldName_; }
    };

    /**
     * Thrown when attempting to modify final field/variable
     */
    class FinalModificationException : public ObjectException {
    private:
        std::string memberName_;

    public:
        FinalModificationException(const std::string& memberName,
                                 const std::string& className = "",
                                 const std::string& location = "")
            : ObjectException("Cannot modify final member: " + memberName, className, location)
            , memberName_(memberName) {}

        const std::string& getMemberName() const { return memberName_; }
    };

    // ===============================
    // Runtime Execution Exceptions
    // ===============================

    /**
     * Thrown when function parameter count mismatches
     */
    class ParameterMismatchException : public MTypeException {
    private:
        std::string functionName_;
        int expectedCount_;
        int actualCount_;

    public:
        ParameterMismatchException(const std::string& functionName,
                                 int expected, int actual,
                                 const std::string& location = "")
            : MTypeException("Parameter count mismatch for " + functionName, location, "function call", 5000)
            , functionName_(functionName), expectedCount_(expected), actualCount_(actual) {}

        const std::string& getFunctionName() const { return functionName_; }
        int getExpectedCount() const { return expectedCount_; }
        int getActualCount() const { return actualCount_; }
    };

    /**
     * Thrown when circular dependency is detected
     */
    class CircularDependencyException : public MTypeException {
    protected:
        std::vector<std::string> dependencyChain_;

    public:
        CircularDependencyException(const std::string& message,
                                  const std::vector<std::string>& chain = {},
                                  const std::string& location = "")
            : MTypeException(message, location, "dependency resolution", 6000)
            , dependencyChain_(chain) {}

        const std::vector<std::string>& getDependencyChain() const { return dependencyChain_; }
    };

    // ===============================
    // Native Function Exceptions
    // ===============================

    /**
     * Thrown by native function implementations
     */
    class NativeFunctionException : public MTypeException {
    private:
        std::string functionName_;

    public:
        NativeFunctionException(const std::string& message,
                              const std::string& functionName = "",
                              const std::string& location = "")
            : MTypeException(message, location, "native function", 7000), functionName_(functionName) {}

        const std::string& getFunctionName() const { return functionName_; }
    };

} // namespace mtype::exceptions