#include "ClassCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../evaluator/utils/ValueConverter.hpp"
#include <unordered_set>


namespace vm::compiler::visitors
{
    ClassCompiler::ClassCompiler(CompilerContext& context)
        : ctx(context)
    {
    }

    void ClassCompiler::validateMethodParameters(
        const std::string& methodName,
        const std::string& qualifiedName,
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
        const ast::SourceLocation& location)
    {
        const auto* methodMetadata = ctx.program.getFunction(qualifiedName);

        // Skip validation if method not found or is native
        if (!methodMetadata || methodMetadata->isNative) {
            return;
        }

        // For instance methods, parameterCount includes 'this', so subtract 1
        // For static methods, parameterCount is just the declared parameters
        size_t expectedParamCount = methodMetadata->parameterCount;
        if (!methodMetadata->isStatic && expectedParamCount > 0) {
            expectedParamCount -= 1;  // Exclude 'this' from count
        }

        // Check parameter count
        if (expectedParamCount != arguments.size()) {
            throw errors::EnvironmentException(
                "Method '" + methodName + "' expects " +
                std::to_string(expectedParamCount) +
                " parameter(s) but got " + std::to_string(arguments.size()),
                location
            );
        }

        // For instance methods, parameterTypes[0] is 'this', so we skip it
        size_t parameterTypeOffset = (!methodMetadata->isStatic && !methodMetadata->parameterTypes.empty()) ? 1 : 0;

        // Validate parameter types
        if (methodMetadata->parameterTypes.size() <= parameterTypeOffset) {
            return;  // No actual parameters to validate (only 'this' or empty)
        }

        for (size_t i = 0; i < arguments.size(); ++i) {
            // Make sure we don't go out of bounds
            if (i + parameterTypeOffset >= methodMetadata->parameterTypes.size()) {
                break;
            }
            std::string expectedType = methodMetadata->parameterTypes[i + parameterTypeOffset];

            // Resolve generic type parameters if present
            expectedType = ctx.resolveGenericType(expectedType);

            // Skip validation for unresolved generic type parameters (like K, V, T, E)
            // These are single uppercase letters or short names that couldn't be resolved
            if (expectedType.length() <= 2 && std::isupper(expectedType[0])) {
                continue;  // Skip validation for generic type parameters
            }

            value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());
            std::string argTypeStr = evaluator::utils::ValueConverter::valueTypeToString(argType);

            // Check if expected type is a primitive
            bool isPrimitive = (expectedType == "int" || expectedType == "float" ||
                              expectedType == "string" || expectedType == "bool" ||
                              expectedType == "void");

            if (isPrimitive) {
                // For primitive types, check exact match
                if (argType != value::ValueType::OBJECT && argTypeStr != expectedType) {
                    // Allow null for any type
                    if (!dynamic_cast<ast::NullNode*>(arguments[i].get())) {
                        throw errors::TypeException(
                            "Method '" + methodName + "' parameter " + std::to_string(i + 1) +
                            " expects " + expectedType + " but got " + argTypeStr,
                            location
                        );
                    }
                }
            } else {
                // Expected type is an object/class
                if (argType != value::ValueType::OBJECT) {
                    // null can be passed to object types
                    if (!dynamic_cast<ast::NullNode*>(arguments[i].get())) {
                        throw errors::TypeException(
                            "Method '" + methodName + "' parameter " + std::to_string(i + 1) +
                            " expects " + expectedType + " but got " + argTypeStr,
                            location
                        );
                    }
                } else if (expectedType != "object") {
                    // For objects, check class name compatibility
                    std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());
                    if (!argClassName.empty() && argClassName != expectedType) {
                        // Strip generic parameters for comparison (List<int> -> List)
                        std::string expectedBase = expectedType;
                        std::string argBase = argClassName;
                        size_t expectedAngle = expectedType.find('<');
                        size_t argAngle = argClassName.find('<');
                        if (expectedAngle != std::string::npos) {
                            expectedBase = expectedType.substr(0, expectedAngle);
                        }
                        if (argAngle != std::string::npos) {
                            argBase = argClassName.substr(0, argAngle);
                        }

                        // If base types match, it's compatible (generic type parameters will be validated at runtime)
                        if (expectedBase == argBase) {
                            continue;  // Compatible generic types
                        }

                        // Check if argClassName is assignable to expectedType (inheritance)
                        if (!ctx.typeValidator.isClassCompatible(argClassName, expectedType)) {
                            throw errors::TypeException(
                                "Method '" + methodName + "' parameter " + std::to_string(i + 1) +
                                " expects " + expectedType + " but got " + argClassName,
                                location
                            );
                        }
                    }
                }
            }
        }
    }

    value::Value ClassCompiler::compileClass(ast::ClassNode* node)
    {
        // Use full class name with generic parameters for generic classes
        std::string className = node->isGeneric() ?
            node->getFullClassName() : node->getClassName();

        // Set current class context for field access resolution
        auto* previousClass = ctx.currentClassNode;
        ctx.currentClassNode = node;

        // Register class metadata (for runtime type checking and instanceof)
        size_t classNameIndex = ctx.program.getConstantPool().addString(className);

        // Handle generics - store generic parameter names
        if (node->isGeneric()) {
            const auto& genericParams = node->getGenericParameters();
            for (const auto& param : genericParams) {
                size_t paramNameIndex = ctx.program.getConstantPool().addString(param.name);
                // Generic parameters are stored in constant pool for type resolution
            }
        }

        // Handle inheritance - store parent class name if present
        if (node->hasParentClass()) {
            std::string parentClassName = node->getParentClassName();
            size_t parentNameIndex = ctx.program.getConstantPool().addString(parentClassName);
            // The parent class relationship is handled at runtime by the environment
        }

        // Handle interfaces - store interface names
        const auto& interfaces = node->getImplementedInterfaces();
        for (const auto& interfaceName : interfaces) {
            size_t interfaceNameIndex = ctx.program.getConstantPool().addString(interfaceName);
            // Interfaces are also handled at runtime
        }

        // Compile static fields initialization
        for (const auto& field : node->getFields()) {
            if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get())) {
                if (fieldNode->getIsStatic()) {
                    field->accept(ctx.visitor);  // Will need delegation
                }
            }
        }

        // Compile static methods (they're like standalone functions)
        for (const auto& method : node->getMethods()) {
            if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get())) {
                if (methodNode->getIsStatic()) {
                    method->accept(ctx.visitor);  // Will need delegation
                }
            }
        }

        // Compile constructors
        if (!node->getConstructors().empty()) {
            for (const auto& constructor : node->getConstructors()) {
                constructor->accept(ctx.visitor);  // Will need delegation
            }
        } else {
            // No explicit constructor - generate a default one
            compileDefaultConstructor(node);
        }

        // Compile instance methods
        for (const auto& method : node->getMethods()) {
            if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get())) {
                if (!methodNode->getIsStatic()) {
                    method->accept(ctx.visitor);  // Will need delegation
                }
            }
        }

        // Restore previous class context
        ctx.currentClassNode = previousClass;

        return std::monostate{};
    }

    void ClassCompiler::compileDefaultConstructor(ast::ClassNode* node)
    {
        // Emit JUMP to skip over default constructor during main execution
        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        size_t constructorStart = ctx.program.getCurrentOffset();

        // Default constructor has only 'this' as parameter
        std::vector<std::string> paramNames = {"this"};

        // Enter function frame
        ctx.functionFrameManager.enterFunctionFrame("object",
            ctx.variableTracker.getNextLocalSlot(),
            ctx.variableTracker.getCurrentScopeDepth(),
            false);
        ctx.variableTracker.beginScope();

        // Track 'this' as local
        ctx.variableTracker.declareLocal("this", value::ValueType::OBJECT,
            node->getClassName());

        // Update max local slot
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        // If parent class exists and has a default constructor, call it
        if (node->hasParentClass()) {
            std::string parentClassName = node->getParentClassName();
            auto parentClassDef = ctx.environment->getClassRegistry()->findClass(parentClassName);
            if (parentClassDef) {
                // Check if parent has a default constructor (0 args)
                auto parentDefaultCtor = parentClassDef->findConstructor(0);
                if (parentDefaultCtor) {
                    // Call parent's default constructor: super()
                    std::string currentClassName = node->getClassName();
                    size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
                    ctx.program.emit(bytecode::OpCode::SUPER_CONSTRUCTOR,
                                     static_cast<uint32_t>(classNameIndex),
                                     static_cast<uint32_t>(0));  // 0 arguments
                }
            }
        }

        // Initialize instance fields with their default values
        initializeInstanceFields(node);

        // Return 'this'
        ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, 0);
        ctx.program.emit(bytecode::OpCode::RETURN_VALUE);

        size_t localCount = ctx.functionFrameManager.getLocalCount();
        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        size_t constructorEnd = ctx.program.getCurrentOffset();
        ctx.emitter.patchJump(skipJump);

        // Register default constructor
        std::string className = node->getClassName();
        std::string constructorName = className + "::<init>/0";  // 0 args

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = constructorName;
        metadata.startOffset = constructorStart;
        metadata.instructionCount = constructorEnd - constructorStart;
        metadata.localCount = localCount;
        metadata.parameterCount = 1;  // Just 'this'
        metadata.parameterNames = paramNames;
        metadata.returnType = "object";
        metadata.isStatic = false;
        metadata.isNative = false;

        ctx.program.registerFunction(metadata.name, metadata);

        // If this is a generic class, also register with full generic name
        if (node->isGeneric()) {
            std::string fullClassName = node->getFullClassName();
            std::string genericConstructorName = fullClassName + "::<init>/0";
            metadata.name = genericConstructorName;
            ctx.program.registerFunction(genericConstructorName, metadata);
        }
    }

    void ClassCompiler::initializeInstanceFields(ast::ClassNode* node)
    {
        auto& fields = node->getFields();
        for (const auto& fieldPtr : fields) {
            if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get())) {
                // Only initialize instance fields (not static)
                if (!fieldNode->getIsStatic() && fieldNode->getInitialValue()) {
                    // Load 'this' FIRST (which is in local slot 0)
                    ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, 0);

                    // Compile the initializer expression (pushes value)
                    fieldNode->getInitialValue()->accept(ctx.visitor);  // Will need delegation

                    // Store in field
                    std::string fieldName = fieldNode->getName();
                    size_t fieldNameIndex = ctx.program.getConstantPool().addString(fieldName);
                    ctx.program.emit(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex));
                }
            }
        }
    }

    value::Value ClassCompiler::compileMethod(ast::MethodNode* node)
    {
        std::string methodName = node->getName();
        bool isStatic = node->getIsStatic();

        // Set instance method context
        bool wasInInstanceMethod = ctx.inInstanceMethod;
        ctx.inInstanceMethod = !isStatic;

        // Handle generic methods - store generic type parameter names
        if (node->isGeneric()) {
            const auto& genericParams = node->getGenericTypeParameters();
            for (const auto& param : genericParams) {
                size_t paramNameIndex = ctx.program.getConstantPool().addString(param.name);
                // Generic type parameters are available at runtime for type resolution
            }
        }

        // Emit JUMP to skip over method body during main execution
        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Method starts here
        size_t methodStart = ctx.program.getCurrentOffset();

        // Get parameters with type information
        auto genericParams = node->getGenericParameters();
        std::vector<std::string> paramNames;
        std::vector<std::string> paramTypes;

        // For instance methods, 'this' is implicitly the first parameter
        if (!isStatic) {
            paramNames.push_back("this");
            paramTypes.push_back(ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "object");
        }

        // Add method parameters with full type names (including class names for objects)
        for (const auto& param : genericParams) {
            paramNames.push_back(param.first);
            // Use toString() to get the full type name (e.g., "int", "string", "MyClass", "List<int>")
            std::string paramTypeStr = param.second->toString();
            paramTypes.push_back(paramTypeStr);
        }

        // Convert return type to string
        value::ValueType returnType = node->getReturnType();
        std::string returnTypeStr = evaluator::utils::ValueConverter::valueTypeToString(returnType);

        // Enter function frame for local variable tracking
        ctx.functionFrameManager.enterFunctionFrame(returnTypeStr,
            ctx.variableTracker.getNextLocalSlot(),
            ctx.variableTracker.getCurrentScopeDepth(),
            false);
        ctx.variableTracker.beginScope();  // Method body scope

        // Track parameters as locals (including 'this' for instance methods)
        if (!isStatic) {
            // Add 'this' parameter
            ctx.variableTracker.declareLocal("this", value::ValueType::OBJECT,
                ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "");
        }

        // Add actual method parameters with their types
        for (const auto& param : genericParams) {
            // Extract type from GenericType for variable tracking
            if (param.second->isGenericParameter()) {
                // Generic type parameter (like T, E) - treat as object for now
                ctx.variableTracker.declareLocal(param.first, value::ValueType::OBJECT, param.second->toString());
            } else {
                // Concrete type
                value::ValueType concreteType = param.second->getConcreteType();
                std::string className = (concreteType == value::ValueType::OBJECT) ? param.second->toString() : "";
                ctx.variableTracker.declareLocal(param.first, concreteType, className);
            }
        }

        // Update max local slot after parameters
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        // Compile method body
        auto* body = node->getBodyPtr();
        if (body) {
            body->accept(ctx.visitor);  // Will need delegation
        }

        // Restore instance method context
        ctx.inInstanceMethod = wasInInstanceMethod;

        // Emit implicit return for void methods (if no explicit return)
        if (returnType == value::ValueType::VOID) {
            ctx.program.emit(bytecode::OpCode::PUSH_NULL);
            // Wrap in Promise if async method
            if (node->getIsAsync()) {
                ctx.program.emit(bytecode::OpCode::CREATE_PROMISE);
            }
            ctx.program.emit(bytecode::OpCode::RETURN_VALUE);
        }

        // Calculate local count before exiting frame
        size_t localCount = ctx.functionFrameManager.getLocalCount();

        ctx.variableTracker.endScope();      // End method body scope
        ctx.functionFrameManager.exitFunctionFrame();

        size_t methodEnd = ctx.program.getCurrentOffset();

        // Patch skip jump to here (after method)
        ctx.emitter.patchJump(skipJump);

        // Build qualified method name for registry
        std::string qualifiedMethodName = methodName;
        if (ctx.currentClassNode) {
            qualifiedMethodName = ctx.currentClassNode->getClassName() + "::" + methodName;
        }

        // Register method metadata
        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = qualifiedMethodName;
        metadata.startOffset = methodStart;
        metadata.instructionCount = methodEnd - methodStart;
        metadata.localCount = localCount;
        metadata.parameterCount = paramNames.size();
        metadata.parameterNames = paramNames;
        metadata.parameterTypes = paramTypes;  // Add parameter types for validation
        metadata.returnType = returnTypeStr;
        metadata.isStatic = isStatic;
        metadata.isNative = false;
        metadata.isAsync = node->getIsAsync();  // Copy async flag from AST

        ctx.program.registerFunction(qualifiedMethodName, metadata);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileConstructor(ast::ConstructorNode* node)
    {
        auto params = node->getParameters();

        // Set instance method context (constructors are like instance methods)
        bool wasInInstanceMethod = ctx.inInstanceMethod;
        ctx.inInstanceMethod = true;

        // Emit JUMP to skip over constructor body during main execution
        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Constructor starts here
        size_t constructorStart = ctx.program.getCurrentOffset();

        // Get parameters
        std::vector<std::string> paramNames;
        paramNames.push_back("this");  // 'this' is always the first parameter for constructors
        for (const auto& param : params) {
            paramNames.push_back(param.first);
        }

        // Constructor returns an object instance
        std::string returnTypeStr = evaluator::utils::ValueConverter::valueTypeToString(value::ValueType::OBJECT);

        // Enter function frame for local variable tracking
        ctx.functionFrameManager.enterFunctionFrame(returnTypeStr,
            ctx.variableTracker.getNextLocalSlot(),
            ctx.variableTracker.getCurrentScopeDepth(),
            false);
        ctx.variableTracker.beginScope();  // Constructor body scope

        // Track parameters as locals (including 'this')
        for (const auto& paramName : paramNames) {
            ctx.variableTracker.declareLocal(paramName, value::ValueType::VOID, "");
        }

        // Update max local slot
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        // Handle super constructor call
        if (ctx.currentClassNode && ctx.currentClassNode->hasParentClass()) {
            std::string parentClassName = ctx.currentClassNode->getParentClassName();
            auto parentClassDef = ctx.environment->getClassRegistry()->findClass(parentClassName);

            if (node->hasSuperInitializer()) {
                // Explicit super() call - compile it
                auto* superInit = node->getSuperInitializer();
                if (superInit) {
                    superInit->accept(ctx.visitor);  // Will need delegation
                }
            } else if (parentClassDef) {
                // No explicit super() - automatically call parent's default constructor if it exists
                auto parentDefaultCtor = parentClassDef->findConstructor(0);
                if (parentDefaultCtor) {
                    // Emit SUPER_CONSTRUCTOR call with 0 arguments
                    std::string currentClassName = ctx.currentClassNode->getClassName();
                    size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
                    ctx.program.emit(bytecode::OpCode::SUPER_CONSTRUCTOR,
                                     static_cast<uint32_t>(classNameIndex),
                                     static_cast<uint32_t>(0));  // 0 arguments
                }
            }
        }

        // Initialize instance fields with their default values (before constructor body)
        if (ctx.currentClassNode) {
            initializeInstanceFields(ctx.currentClassNode);
        }

        // Compile constructor body
        auto* body = node->getBodyPtr();
        if (body) {
            body->accept(ctx.visitor);  // Will need delegation
        }

        // Restore instance method context
        ctx.inInstanceMethod = wasInInstanceMethod;

        // Constructors implicitly return 'this'
        ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, 0);
        ctx.program.emit(bytecode::OpCode::RETURN_VALUE);

        // Calculate local count before exiting frame
        size_t localCount = ctx.functionFrameManager.getLocalCount();

        ctx.variableTracker.endScope();      // End constructor body scope
        ctx.functionFrameManager.exitFunctionFrame();

        size_t constructorEnd = ctx.program.getCurrentOffset();

        // Patch skip jump to here (after constructor)
        ctx.emitter.patchJump(skipJump);

        // Register constructor metadata
        std::string className = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        std::string constructorName = className + "::<init>/" + std::to_string(params.size());

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = constructorName;
        metadata.startOffset = constructorStart;
        metadata.instructionCount = constructorEnd - constructorStart;
        metadata.localCount = localCount;
        metadata.parameterCount = paramNames.size();
        metadata.parameterNames = paramNames;
        metadata.returnType = returnTypeStr;
        metadata.isStatic = false;
        metadata.isNative = false;

        ctx.program.registerFunction(constructorName, metadata);

        // If this is a generic class, also register with full generic name
        if (ctx.currentClassNode && ctx.currentClassNode->isGeneric()) {
            std::string fullClassName = ctx.currentClassNode->getFullClassName();
            std::string genericConstructorName = fullClassName + "::<init>/" + std::to_string(params.size());
            ctx.program.registerFunction(genericConstructorName, metadata);
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileField(ast::FieldNode* node)
    {
        std::string fieldName = node->getName();
        bool isStatic = node->getIsStatic();

        // Only compile static fields here (instance fields are initialized in constructor)
        if (isStatic) {
            // Get initial value or null
            auto* initValue = node->getInitialValue();
            if (initValue) {
                initValue->accept(ctx.visitor);  // Will need delegation
            } else {
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            }

            // Store in static field with fully qualified name: ClassName::fieldName
            std::string qualifiedName = fieldName;
            if (ctx.currentClassNode) {
                qualifiedName = ctx.currentClassNode->getClassName() + "::" + fieldName;
            }
            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);

            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint32_t>(nameIndex), node);
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileNew(ast::NewNode* node)
    {
        std::string fullClassName = node->getClassName();

        // Parse generic type arguments from className (e.g., "Box<Int>" -> "Box" and ["Int"])
        std::string baseClassName = fullClassName;
        std::vector<std::string> typeArguments;

        size_t genericStart = fullClassName.find('<');
        if (genericStart != std::string::npos) {
            baseClassName = fullClassName.substr(0, genericStart);

            // Extract type arguments from within <>
            size_t genericEnd = fullClassName.rfind('>');
            if (genericEnd != std::string::npos && genericEnd > genericStart) {
                std::string typeArgsStr = fullClassName.substr(genericStart + 1, genericEnd - genericStart - 1);

                // Parse individual type arguments
                size_t start = 0;
                size_t depth = 0;
                for (size_t i = 0; i < typeArgsStr.length(); ++i) {
                    if (typeArgsStr[i] == '<') depth++;
                    else if (typeArgsStr[i] == '>') depth--;
                    else if (typeArgsStr[i] == ',' && depth == 0) {
                        std::string arg = typeArgsStr.substr(start, i - start);
                        // Trim whitespace
                        arg.erase(0, arg.find_first_not_of(" \t"));
                        arg.erase(arg.find_last_not_of(" \t") + 1);
                        typeArguments.push_back(arg);
                        start = i + 1;
                    }
                }
                // Add last argument
                std::string arg = typeArgsStr.substr(start);
                arg.erase(0, arg.find_first_not_of(" \t"));
                arg.erase(arg.find_last_not_of(" \t") + 1);
                if (!arg.empty()) {
                    typeArguments.push_back(arg);
                }

                // Validate that type arguments are not primitive types
                static const std::unordered_set<std::string> primitiveTypes = {
                    "int", "float", "bool", "string", "void"
                };

                for (const auto& typeArg : typeArguments) {
                    if (primitiveTypes.find(typeArg) != primitiveTypes.end()) {
                        throw errors::TypeException(
                            "Generic type arguments cannot be primitive types. Use wrapper classes instead (Int, Float, Bool, String). Found: " + typeArg,
                            node->getLocation()
                        );
                    }
                }
            }
        }

        // Validate constructor parameters
        const auto& arguments = node->getArguments();

        // Try to find the class definition to get constructor parameter types
        auto classDef = ctx.environment->findClass(baseClassName);
        if (classDef) {
            // Get constructors from the class
            const auto& constructors = classDef->getConstructors();

            // Find a constructor that matches the argument count
            bool foundMatchingConstructor = false;
            for (const auto& constructor : constructors) {
                if (constructor->getParameterCount() == arguments.size()) {
                    foundMatchingConstructor = true;

                    // Validate parameter types
                    const auto& params = constructor->getParametersWithTypes();
                    for (size_t i = 0; i < arguments.size(); ++i) {
                        const auto& paramType = params[i].second;
                        value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

                        // Skip validation if we can't infer the type
                        if (argType == value::ValueType::VOID) {
                            continue;
                        }

                        // For object types, check class names
                        if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value()) {
                            std::string expectedClass = paramType.className.value();

                            // Skip generic type parameters (single uppercase letters)
                            if (expectedClass.length() <= 2 && std::isupper(expectedClass[0])) {
                                continue;
                            }

                            if (argType != value::ValueType::OBJECT) {
                                // Allow null
                                if (!dynamic_cast<ast::NullNode*>(arguments[i].get())) {
                                    std::string argTypeStr = evaluator::utils::ValueConverter::valueTypeToString(argType);
                                    throw errors::TypeException(
                                        "Constructor parameter " + std::to_string(i + 1) +
                                        " expects " + expectedClass + " but got " + argTypeStr,
                                        node->getLocation()
                                    );
                                }
                            } else {
                                // Check class name match
                                std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());
                                if (!argClassName.empty() && argClassName != expectedClass && expectedClass != "object") {
                                    if (!dynamic_cast<ast::NullNode*>(arguments[i].get())) {
                                        throw errors::TypeException(
                                            "Constructor parameter " + std::to_string(i + 1) +
                                            " expects " + expectedClass + " but got " + argClassName,
                                            node->getLocation()
                                        );
                                    }
                                }
                            }
                        }
                        // For primitive types
                        else {
                            std::string expectedTypeStr = evaluator::utils::ValueConverter::valueTypeToString(paramType.basicType);
                            std::string argTypeStr = evaluator::utils::ValueConverter::valueTypeToString(argType);

                            if (paramType.basicType != argType) {
                                // Allow int to float conversion
                                if (!(paramType.basicType == value::ValueType::FLOAT && argType == value::ValueType::INT)) {
                                    throw errors::TypeException(
                                        "Constructor parameter " + std::to_string(i + 1) +
                                        " expects " + expectedTypeStr + " but got " + argTypeStr,
                                        node->getLocation()
                                    );
                                }
                            }
                        }
                    }
                    break;  // Found matching constructor, stop checking
                }
            }
        }

        // Push constructor arguments onto stack (left to right)
        for (const auto& arg : arguments) {
            arg->accept(ctx.visitor);  // Will need delegation
        }

        // Store the FULL class name including generics (e.g., "Box<Int>")
        size_t classNameIndex = ctx.program.getConstantPool().addString(fullClassName);

        // Emit NEW_OBJECT instruction with full class name and argument count
        ctx.program.emit(bytecode::OpCode::NEW_OBJECT,
                     static_cast<uint32_t>(classNameIndex),
                     static_cast<uint32_t>(arguments.size()));

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMemberAccess(ast::MemberAccessNode* node)
    {
        std::string memberName = node->getMemberName();
        bool isStaticAccess = node->getIsStaticAccess();

        if (isStaticAccess) {
            // Static field access: ClassName::fieldName
            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint32_t>(fieldNameIndex), node);
        } else {
            // Instance field access: object.fieldName
            // First, compile the object expression
            node->getObject()->accept(ctx.visitor);  // Will need delegation

            // Check if this is array.length access
            if (memberName == "length") {
                // Special case: array.length should use ARRAY_LENGTH opcode
                ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_LENGTH, node);
            } else {
                // Regular field access - emit GET_FIELD instruction
                size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
            }
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMemberAssignment(ast::MemberAssignmentNode* node)
    {
        std::string memberName = node->getMemberName();

        // Compile the object expression
        node->getObject()->accept(ctx.visitor);  // Will need delegation

        // Compile the value to assign
        node->getValue()->accept(ctx.visitor);  // Will need delegation

        // Emit SET_FIELD instruction (object and value are on stack)
        size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMethodCall(ast::MethodCallNode* node)
    {
        std::string methodName = node->getMethodName();
        bool isStaticCall = node->getIsStaticCall();
        const auto& arguments = node->getArguments();

        if (isStaticCall) {
            // Static method call: ClassName::methodName(args) or this::methodName(args)

            // Build fully qualified method name
            std::string className;
            auto* objectNode = node->getObject();
            if (auto* varNode = dynamic_cast<ast::VariableNode*>(objectNode)) {
                className = varNode->getName();
                // If using "this::", replace with actual class name
                if (className == "this" && ctx.currentClassNode) {
                    className = ctx.currentClassNode->getClassName();
                }
            }
            std::string qualifiedName = className + "::" + methodName;

            // Validate parameter count and types
            validateMethodParameters(qualifiedName, qualifiedName, arguments, node->getLocation());

            // Push all arguments onto stack
            for (const auto& arg : arguments) {
                arg->accept(ctx.visitor);  // Will need delegation
            }

            // Emit CALL_STATIC instruction with fully qualified name
            size_t methodNameIndex = ctx.program.getConstantPool().addString(qualifiedName);
            ctx.program.emit(bytecode::OpCode::CALL_STATIC,
                         static_cast<uint32_t>(methodNameIndex),
                         static_cast<uint32_t>(arguments.size()));
        } else {
            // Instance method call: object.methodName(args)

            // Infer the class of the object for type checking
            std::string objectClassName = ctx.typeInference.inferExpressionClassName(node->getObject());

            // Validate parameter count and types for instance methods
            if (!objectClassName.empty()) {
                std::string qualifiedName = objectClassName + "::" + methodName;
                validateMethodParameters(methodName, qualifiedName, arguments, node->getLocation());
            }

            // First, compile the object expression
            node->getObject()->accept(ctx.visitor);  // Will need delegation

            // Push all arguments onto stack
            for (const auto& arg : arguments) {
                arg->accept(ctx.visitor);  // Will need delegation
            }

            // Emit CALL_METHOD instruction
            size_t methodNameIndex = ctx.program.getConstantPool().addString(methodName);
            ctx.program.emit(bytecode::OpCode::CALL_METHOD,
                         static_cast<uint32_t>(methodNameIndex),
                         static_cast<uint32_t>(arguments.size()));
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperConstructorCall(ast::SuperConstructorCallNode* node)
    {
        // Push arguments onto stack
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments) {
            arg->accept(ctx.visitor);  // Will need delegation
        }

        // Emit SUPER_CONSTRUCTOR instruction with current class name
        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        ctx.program.emit(bytecode::OpCode::SUPER_CONSTRUCTOR,
                     static_cast<uint32_t>(classNameIndex),
                     static_cast<uint32_t>(arguments.size()));

        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperMethodCall(ast::SuperMethodCallNode* node)
    {
        std::string methodName = node->getMethodName();

        // Push arguments onto stack
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments) {
            arg->accept(ctx.visitor);  // Will need delegation
        }

        // Emit SUPER_INVOKE instruction with current class name
        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        size_t methodNameIndex = ctx.program.getConstantPool().addString(methodName);

        ctx.program.emit(bytecode::OpCode::SUPER_INVOKE,
                     std::vector<uint32_t>{
                         static_cast<uint32_t>(classNameIndex),
                         static_cast<uint32_t>(methodNameIndex),
                         static_cast<uint32_t>(arguments.size())
                     });

        return std::monostate{};
    }
}
