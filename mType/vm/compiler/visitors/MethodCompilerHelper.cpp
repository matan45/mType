#include "MethodCompilerHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../validation/ReturnPathValidator.hpp"
#include <iostream>
namespace vm::compiler::visitors
{
    MethodCompilerHelper::MethodCompilerHelper(CompilerContext& context)
        : ctx(context)
    {
    }

    void MethodCompilerHelper::compileDefaultConstructor(ast::ClassNode* node)
    {
        // Emit JUMP to skip over default constructor during main execution
        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP, node);
        size_t constructorStart = ctx.program.getCurrentOffset();

        // Default constructor has only 'this' as parameter
        std::vector<std::string> paramNames = {"this"};

        // Generate constructor name for exception table tracking (no slash for default constructor)
        std::string className = node->getClassName();
        std::string constructorName = className + "::<init>"; // No slash for default constructor with no params

        // Pre-register constructor metadata so exception tables can be added during initialization
        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = constructorName;
        tempMetadata.startOffset = constructorStart;
        tempMetadata.instructionCount = 0;  // Will be updated after compilation
        tempMetadata.localCount = 0;        // Will be updated after compilation
        tempMetadata.parameterCount = 1;    // Just 'this'
        tempMetadata.returnType = "object";
        tempMetadata.isStatic = false;
        tempMetadata.isNative = false;
        ctx.program.registerFunction(constructorName, tempMetadata);

        // Enter function frame
        ctx.functionFrameManager.enterFunctionFrame(constructorName,
                                                    "object",
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
        if (node->hasParentClass())
        {
            std::string parentClassName = node->getParentClassName();
            auto parentClassDef = ctx.environment->getClassRegistry()->findClass(parentClassName);
            if (parentClassDef)
            {
                // Check if parent has a default constructor (0 args)
                auto parentDefaultCtor = parentClassDef->findConstructor(0);
                if (parentDefaultCtor)
                {
                    // Call parent's default constructor: super()
                    std::string currentClassName = node->getClassName();
                    size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_CONSTRUCTOR,
                                     static_cast<uint64_t>(classNameIndex),
                                     static_cast<uint64_t>(0), node); // 0 arguments
                }
            }
        }

        // Initialize instance fields with their default values
        initializeInstanceFields(node);

        // Return 'this'
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);

        size_t localCount = ctx.functionFrameManager.getLocalCount();

        // Capture local variable names for debugging (before exiting frame)
        const auto& locals = ctx.variableTracker.getLocals();
        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
            if (local.slot < localCount)
            {
                localVarNames[local.slot] = local.name;
            }
        }

        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        size_t constructorEnd = ctx.program.getCurrentOffset();
        ctx.emitter.patchJump(skipJump);

        // Update default constructor metadata (preserving exception table from initialization)
        auto* existingMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
            ctx.program.getFunction(constructorName)
        );

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = constructorName;
        metadata.startOffset = constructorStart;
        metadata.instructionCount = constructorEnd - constructorStart;
        metadata.localCount = localCount;
        metadata.parameterCount = 1; // Just 'this'
        metadata.parameterNames = paramNames;
        metadata.returnType = "object";
        metadata.isStatic = false;
        metadata.isNative = false;
        metadata.localVariableNames = localVarNames;

        // Preserve exception table built during initialization
        if (existingMetadata) {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        ctx.program.registerFunction(metadata.name, metadata);

        // If this is a generic class, also register with full generic name
        if (node->isGeneric())
        {
            std::string fullClassName = node->getFullClassName();
            std::string genericConstructorName = fullClassName + "::<init>"; // No slash for empty signature
            metadata.name = genericConstructorName;
            ctx.program.registerFunction(genericConstructorName, metadata);
        }
    }

    void MethodCompilerHelper::initializeInstanceFields(ast::ClassNode* node)
    {
        auto& fields = node->getFields();
        for (const auto& fieldPtr : fields)
        {
            if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get()))
            {
                // Only initialize instance fields (not static)
                if (!fieldNode->getIsStatic() && fieldNode->getInitialValue())
                {
                    // Load 'this' FIRST (which is in local slot 0)
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, fieldNode);

                    // Compile the initializer expression (pushes value)
                    fieldNode->getInitialValue()->accept(ctx.visitor);

                    // Store in field
                    std::string fieldName = fieldNode->getName();
                    size_t fieldNameIndex = ctx.program.getConstantPool().addString(fieldName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint64_t>(fieldNameIndex), fieldNode);
                }
            }
        }
    }

    bool MethodCompilerHelper::isValidTypeName(const std::string& typeName,
                                                const std::vector<std::string>& validGenericParams)
    {
        std::string baseTypeName = typeName;

        // Strip nullable suffix '?'
        if (!baseTypeName.empty() && baseTypeName.back() == '?')
        {
            baseTypeName.pop_back();
        }

        // Handle array types: int[], Item[][], etc.
        // Strip all array brackets to get the element type
        size_t bracketPos = baseTypeName.find('[');
        if (bracketPos != std::string::npos)
        {
            baseTypeName = baseTypeName.substr(0, bracketPos);
        }

        // Extract base type name (handle generics like "List<T>", "Array<K>")
        size_t anglePos = baseTypeName.find('<');
        if (anglePos != std::string::npos)
        {
            baseTypeName = baseTypeName.substr(0, anglePos);
        }

        // Check if base type is a primitive type (including Array for array types, object for generic constraints, and Promise for async/await)
        if (baseTypeName == "int" || baseTypeName == "float" || baseTypeName == "string" ||
            baseTypeName == "bool" || baseTypeName == "void" || baseTypeName == "Array" ||
            baseTypeName == "object" || baseTypeName == "Promise")
        {
            return true;
        }

        // Check if it's a declared generic type parameter (check element type for arrays)
        for (const auto& genericParam : validGenericParams)
        {
            if (baseTypeName == genericParam)
            {
                return true;
            }
        }

        // Check if base type is an existing class or interface
        if (ctx.environment->findClass(baseTypeName) != nullptr)
        {
            return true;
        }
        if (ctx.environment->findInterface(baseTypeName) != nullptr)
        {
            return true;
        }

        return false;
    }

    MethodCompilerHelper::MethodParameters MethodCompilerHelper::collectMethodParameters(ast::MethodNode* node, bool isStatic)
    {
        MethodParameters result;

        // Build list of valid generic type parameter names
        std::vector<std::string> validGenericParams;

        // Add class-level generic parameters
        if (ctx.currentClassNode && ctx.currentClassNode->isGeneric())
        {
            for (const auto& param : ctx.currentClassNode->getGenericParameters())
            {
                validGenericParams.push_back(param.name);
            }
        }

        // Add method-level generic parameters
        if (node->isGeneric())
        {
            for (const auto& param : node->getGenericTypeParameters())
            {
                validGenericParams.push_back(param.name);
            }
        }

        // Get parameters with type information
        auto genericParams = node->getGenericParameters();

        // For instance methods, 'this' is implicitly the first parameter
        if (!isStatic)
        {
            result.paramNames.push_back("this");
            result.paramTypes.push_back(ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "object");
            result.paramNullable.push_back(false); // 'this' is never nullable
        }

        // Add method parameters with full type names (including class names for objects)
        for (const auto& param : genericParams)
        {
            result.paramNames.push_back(param.first);
            // Use toString() to get the full type name, then strip nullable suffix
            std::string paramTypeStr = param.second->toString();
            bool paramIsNullable = param.second->isNullable();
            // Strip '?' from type string - nullable tracked separately
            if (!paramTypeStr.empty() && paramTypeStr.back() == '?')
            {
                paramTypeStr.pop_back();
            }
            result.paramTypes.push_back(paramTypeStr);
            result.paramNullable.push_back(paramIsNullable);

            // Validate parameter type exists
            if (!isValidTypeName(paramTypeStr, validGenericParams))
            {
                throw errors::TypeException(
                    "Undefined type '" + paramTypeStr + "' in parameter '" + param.first + "'. " +
                    "Type must be a primitive, declared generic parameter, or existing class/interface.",
                    node->getLocation()
                );
            }
        }

        // Convert return type to string, preserving class names for object types
        auto genericReturnType = node->getGenericReturnType();
        if (genericReturnType) {
            result.returnTypeStr = genericReturnType->toString();
        } else {
            result.returnTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(node->getReturnType());
        }

        // Validate return type exists
        if (!isValidTypeName(result.returnTypeStr, validGenericParams))
        {
            throw errors::TypeException(
                "Undefined type '" + result.returnTypeStr + "' in return type. " +
                "Type must be a primitive, declared generic parameter, or existing class/interface.",
                node->getLocation()
            );
        }

        return result;
    }

    MethodCompilerHelper::MethodBodyInfo MethodCompilerHelper::compileMethodBodyWithFrame(ast::MethodNode* node, const MethodParameters& params,
                                                                                          bool isStatic, const std::string& qualifiedMethodName)
    {
        // Use the passed-in qualified method name (with signature for overloaded methods)
        // This ensures exception tables are registered with the same name used at runtime

        // Enter function frame for local variable tracking
        ctx.functionFrameManager.enterFunctionFrame(qualifiedMethodName,
                                                    params.returnTypeStr,
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false,
                                                    node->getIsAsync());
        ctx.variableTracker.beginScope(); // Method body scope

        // Track parameters as locals (including 'this' for instance methods)
        if (!isStatic)
        {
            // Add 'this' parameter
            ctx.variableTracker.declareLocal("this", value::ValueType::OBJECT,
                                             ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "");
        }

        // Add actual method parameters with their types
        auto genericParams = node->getGenericParameters();
        for (const auto& param : genericParams)
        {
            // Extract type from GenericType for variable tracking
            if (param.second->isGenericParameter())
            {
                // Generic type parameter (like T, E) - treat as object for now
                // Strip nullable suffix '?' from className - nullability is tracked separately
                std::string paramClassName = param.second->toString();
                if (!paramClassName.empty() && paramClassName.back() == '?')
                {
                    paramClassName.pop_back();
                }
                ctx.variableTracker.declareLocal(param.first, value::ValueType::OBJECT,
                                                 paramClassName, param.second->isNullable());
            }
            else
            {
                // Concrete type
                value::ValueType concreteType = param.second->getConcreteType();
                std::string className = (concreteType == value::ValueType::OBJECT) ? param.second->toString() : "";
                // Strip nullable suffix '?' from className - nullability is tracked separately
                if (!className.empty() && className.back() == '?')
                {
                    className.pop_back();
                }
                ctx.variableTracker.declareLocal(param.first, concreteType, className,
                                                 param.second->isNullable());
            }
        }

        // Update max local slot after parameters
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        // Validate return paths for non-void methods
        auto* body = node->getBodyPtr();
        if (body)
        {
            std::string className = ctx.currentClassNode ? ctx.currentClassNode->getClassName() + "::" : "";
            std::string methodName = className + node->getName();

            validation::ReturnPathValidator::validateMethodReturns(
                body,
                node->getReturnType(),
                params.returnTypeStr,
                methodName,
                node->getLocation()
            );
        }

        // Compile method body
        if (body)
        {
            body->accept(ctx.visitor);
        }

        // Emit implicit return for void methods
        bool isVoidMethod = (node->getReturnType() == value::ValueType::VOID);
        bool isAsyncVoidMethod = (node->getIsAsync() && params.returnTypeStr == "Promise<void>");

        if (isVoidMethod || isAsyncVoidMethod)
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            if (node->getIsAsync())
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }
            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }

        // Calculate local count before exiting frame
        size_t localCount = ctx.functionFrameManager.getLocalCount();

        // Capture local variable names for debugging (before exiting frame)
        const auto& locals = ctx.variableTracker.getLocals();
        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
            if (local.slot < localCount)
            {
                localVarNames[local.slot] = local.name;
            }
        }

        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        return MethodBodyInfo{ localCount, localVarNames };
    }

    void MethodCompilerHelper::finalizeMethodCompilation(ast::MethodNode* node, const MethodParameters& params,
                                                          size_t methodStart, size_t skipJump, const MethodBodyInfo& bodyInfo, bool isStatic)
    {
        // Patch skip jump to current position (after method)
        ctx.emitter.patchJump(skipJump);

        // Build qualified method name for registry with overload signature
        // Instance methods: ClassName::methodName/paramType1,paramType2
        // Static methods: ClassName::methodName/paramType1,paramType2$static
        std::string qualifiedMethodName = node->getName();
        if (ctx.currentClassNode)
        {
            // Build mangled name with parameter signature
            std::string className = ctx.currentClassNode->getClassName();
            std::string methodName = node->getName();

            // Generate type signature from parameter types
            // IMPORTANT: Skip "this" parameter for instance methods (it's added implicitly)
            std::string typeSignature = "";
            size_t startIndex = (isStatic ? 0 : 1); // Skip "this" for instance methods
            for (size_t i = startIndex; i < params.paramTypes.size(); ++i)
            {
                if (i > startIndex) typeSignature += ",";
                typeSignature += params.paramTypes[i];
            }

            // Only add slash if we have a signature
            if (typeSignature.empty()) {
                qualifiedMethodName = className + "::" + methodName;
            } else {
                qualifiedMethodName = className + "::" + methodName + "/" + typeSignature;
            }

            // Add suffix to distinguish static from instance methods
            if (isStatic)
            {
                qualifiedMethodName += "$static";
            }
        }

        // Update method metadata (preserving exception table from body compilation)
        auto* existingMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
            ctx.program.getFunction(qualifiedMethodName)
        );

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = qualifiedMethodName;
        metadata.startOffset = methodStart;
        metadata.instructionCount = ctx.program.getCurrentOffset() - methodStart;
        metadata.localCount = bodyInfo.localCount;
        metadata.parameterCount = params.paramNames.size();
        metadata.parameterNames = params.paramNames;
        metadata.parameterTypes = params.paramTypes;
        metadata.parameterNullable = params.paramNullable;
        metadata.returnType = params.returnTypeStr;
        metadata.isStatic = isStatic;
        metadata.isNative = false;
        metadata.isAsync = node->getIsAsync();
        metadata.localVariableNames = bodyInfo.localVarNames;

        // Store generic type parameters for generic methods
        if (node->isGeneric())
        {
            const auto& genericParams = node->getGenericTypeParameters();
            for (const auto& param : genericParams)
            {
                metadata.genericTypeParameters.push_back(param.name);
            }
        }

        // Preserve exception table built during body compilation
        if (existingMetadata) {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }
        
        ctx.program.registerFunction(qualifiedMethodName, metadata);
    }

    value::Value MethodCompilerHelper::compileMethod(ast::MethodNode* node)
    {
        bool isStatic = node->getIsStatic();

        // Set instance/static method context
        bool wasInInstanceMethod = ctx.inInstanceMethod;
        bool wasInStaticMethod = ctx.inStaticMethod;
        ctx.inInstanceMethod = !isStatic;
        ctx.inStaticMethod = isStatic;

        // Synchronize TypeInferenceEngine with class context for field type inference
        ctx.typeInference.setCurrentClassContext(ctx.currentClassNode, ctx.inInstanceMethod);

        // Handle generic methods - store generic type parameter names
        if (node->isGeneric())
        {
            const auto& genericParams = node->getGenericTypeParameters();

            // Validate: method-level generic parameters should not shadow class-level ones
            if (ctx.currentClassNode && ctx.currentClassNode->isGeneric())
            {
                const auto& classGenericParams = ctx.currentClassNode->getGenericParameters();
                for (const auto& methodParam : genericParams)
                {
                    for (const auto& classParam : classGenericParams)
                    {
                        if (methodParam.name == classParam.name)
                        {
                            throw errors::TypeException(
                                "Method generic type parameter '" + methodParam.name +
                                "' shadows class-level type parameter with the same name. " +
                                "Use a different name to avoid confusion.",
                                methodParam.location
                            );
                        }
                    }
                }
            }

            for (const auto& param : genericParams)
            {
                size_t paramNameIndex = ctx.program.getConstantPool().addString(param.name);
                // Generic type parameters are available at runtime for type resolution
            }
        }

        // Emit JUMP to skip over method body during main execution
        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP, node);
        size_t methodStart = ctx.program.getCurrentOffset();

        // Collect method parameters and return type
        MethodParameters params = collectMethodParameters(node, isStatic);

        // Pre-register method metadata so exception tables can be added during body compilation
        // Build mangled name with parameter signature for overload resolution
        std::string qualifiedMethodName = node->getName();
        if (ctx.currentClassNode)
        {
            std::string className = ctx.currentClassNode->getClassName();
            std::string methodName = node->getName();

            // Generate type signature from parameter types
            // IMPORTANT: Skip "this" parameter for instance methods (it's added implicitly)
            std::string typeSignature = "";
            size_t startIndex = (isStatic ? 0 : 1); // Skip "this" for instance methods
            for (size_t i = startIndex; i < params.paramTypes.size(); ++i)
            {
                if (i > startIndex) typeSignature += ",";
                typeSignature += params.paramTypes[i];
            }

            // Only add slash if we have a signature
            if (typeSignature.empty()) {
                qualifiedMethodName = className + "::" + methodName;
            } else {
                qualifiedMethodName = className + "::" + methodName + "/" + typeSignature;
            }

            if (isStatic)
            {
                qualifiedMethodName += "$static";
            }
        }

        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = qualifiedMethodName;
        tempMetadata.startOffset = methodStart;
        tempMetadata.instructionCount = 0;  // Will be updated after compilation
        tempMetadata.localCount = 0;        // Will be updated after compilation
        tempMetadata.parameterCount = params.paramNames.size();  // Set correct count now
        tempMetadata.parameterNames = params.paramNames;
        tempMetadata.parameterTypes = params.paramTypes;
        tempMetadata.parameterNullable = params.paramNullable;
        tempMetadata.returnType = params.returnTypeStr;
        tempMetadata.isAsync = node->getIsAsync();
        tempMetadata.isStatic = isStatic;
        tempMetadata.isNative = false;

        ctx.program.registerFunction(qualifiedMethodName, tempMetadata);

        // Compile method body with frame management (pass mangled name for exception tables)
        MethodBodyInfo bodyInfo = compileMethodBodyWithFrame(node, params, isStatic, qualifiedMethodName);

        // Restore instance/static method context
        ctx.inInstanceMethod = wasInInstanceMethod;
        ctx.inStaticMethod = wasInStaticMethod;

        // Restore TypeInferenceEngine class context
        ctx.typeInference.setCurrentClassContext(ctx.currentClassNode, ctx.inInstanceMethod);

        // Finalize method compilation (patch jump, register metadata)
        finalizeMethodCompilation(node, params, methodStart, skipJump, bodyInfo, isStatic);

        return std::monostate{};
    }

    value::Value MethodCompilerHelper::compileConstructor(ast::ConstructorNode* node)
    {
        const auto& params = node->getParametersWithTypes();

        // Set instance method context (constructors are like instance methods)
        bool wasInInstanceMethod = ctx.inInstanceMethod;
        ctx.inInstanceMethod = true;

        // Synchronize TypeInferenceEngine with class context for field type inference
        ctx.typeInference.setCurrentClassContext(ctx.currentClassNode, ctx.inInstanceMethod);

        // Emit JUMP to skip over constructor body during main execution
        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP, node);

        // Constructor starts here
        size_t constructorStart = ctx.program.getCurrentOffset();

        // Get parameters
        std::vector<std::string> paramNames;
        paramNames.push_back("this"); // 'this' is always the first parameter for constructors
        for (const auto& param : params)
        {
            paramNames.push_back(param.first);
        }

        // Constructor returns an object instance
        std::string returnTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(value::ValueType::OBJECT);

        // Generate constructor name with type signature for overload resolution
        std::string className = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";

        // Build type signature from parameters
        std::string typeSignature;
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) typeSignature += ",";
            const auto& paramType = params[i].second;
            if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value()) {
                typeSignature += paramType.className.value();
            } else {
                typeSignature += ::types::TypeConversionUtils::getTypeDisplayName(paramType.basicType);
            }
        }

        // Only add slash if we have a signature
        std::string constructorName;
        if (typeSignature.empty()) {
            constructorName = className + "::<init>";
        } else {
            constructorName = className + "::<init>/" + typeSignature;
        }

        // Pre-register constructor metadata so exception tables can be added during body compilation
        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = constructorName;
        tempMetadata.startOffset = constructorStart;
        tempMetadata.instructionCount = 0;  // Will be updated after compilation
        tempMetadata.localCount = 0;        // Will be updated after compilation
        tempMetadata.parameterCount = paramNames.size();  // Set correct count now
        tempMetadata.parameterNames = paramNames;
        tempMetadata.returnType = returnTypeStr;
        tempMetadata.isStatic = false;
        tempMetadata.isNative = false;
        ctx.program.registerFunction(constructorName, tempMetadata);

        // Enter function frame for local variable tracking
        ctx.functionFrameManager.enterFunctionFrame(constructorName,
                                                    returnTypeStr,
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false);
        ctx.variableTracker.beginScope(); // Constructor body scope

        // Track parameters as locals (including 'this')
        for (const auto& paramName : paramNames)
        {
            ctx.variableTracker.declareLocal(paramName, value::ValueType::VOID, "");
        }

        // Update max local slot
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        // BEFORE super() call: Assign constructor parameters to matching instance fields
        // This ensures polymorphic methods called in parent constructors see correct field values
        if (ctx.currentClassNode)
        {
            const auto& fields = ctx.currentClassNode->getFields();
            for (size_t i = 0; i < params.size(); ++i)
            {
                const std::string& paramName = params[i].first;

                // Check if there's an instance field with the same name
                for (const auto& field : fields)
                {
                    // Cast to FieldNode to access field-specific methods
                    auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get());
                    if (fieldNode && !fieldNode->getIsStatic() && fieldNode->getName() == paramName)
                    {
                        // Load 'this' (slot 0) - object goes on stack first
                        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);

                        // Load the parameter value (parameters start at slot 1, after 'this')
                        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                                                   static_cast<uint64_t>(i + 1), node);

                        // Set the field (pops value, then object)
                        size_t fieldNameIndex = ctx.program.getConstantPool().addString(paramName);
                        ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD,
                                                   static_cast<uint64_t>(fieldNameIndex), node);
                        break;
                    }
                }
            }
        }

        // Handle super constructor call
        if (ctx.currentClassNode && ctx.currentClassNode->hasParentClass())
        {
            std::string parentClassName = ctx.currentClassNode->getParentClassName();
            auto parentClassDef = ctx.environment->getClassRegistry()->findClass(parentClassName);

            if (node->hasSuperInitializer())
            {
                // Explicit super() call - compile it
                auto* superInit = node->getSuperInitializer();
                if (superInit)
                {
                    superInit->accept(ctx.visitor);
                }
            }
            else if (parentClassDef)
            {
                // No explicit super() - automatically call parent's default constructor if it exists
                auto parentDefaultCtor = parentClassDef->findConstructor(0);
                if (parentDefaultCtor)
                {
                    // Emit SUPER_CONSTRUCTOR call with 0 arguments
                    std::string currentClassName = ctx.currentClassNode->getClassName();
                    size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_CONSTRUCTOR,
                                     static_cast<uint64_t>(classNameIndex),
                                     static_cast<uint64_t>(0), node); // 0 arguments
                }
            }
        }

        // Initialize instance fields with their default values (before constructor body)
        if (ctx.currentClassNode)
        {
            initializeInstanceFields(ctx.currentClassNode);
        }

        // Compile constructor body
        auto* body = node->getBodyPtr();
        if (body)
        {
            body->accept(ctx.visitor);
        }

        // Restore instance method context
        ctx.inInstanceMethod = wasInInstanceMethod;

        // Restore TypeInferenceEngine class context
        ctx.typeInference.setCurrentClassContext(ctx.currentClassNode, ctx.inInstanceMethod);

        // Constructors implicitly return 'this'
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);

        // Calculate local count before exiting frame
        size_t localCount = ctx.functionFrameManager.getLocalCount();

        // Capture local variable names for debugging (before exiting frame)
        const auto& locals = ctx.variableTracker.getLocals();
        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
            if (local.slot < localCount)
            {
                localVarNames[local.slot] = local.name;
            }
        }

        ctx.variableTracker.endScope(); // End constructor body scope
        ctx.functionFrameManager.exitFunctionFrame();

        size_t constructorEnd = ctx.program.getCurrentOffset();

        // Patch skip jump to here (after constructor)
        ctx.emitter.patchJump(skipJump);

        // Update constructor metadata (preserving exception table from body compilation)
        auto* existingMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
            ctx.program.getFunction(constructorName)
        );

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = constructorName;
        metadata.startOffset = constructorStart;
        metadata.instructionCount = constructorEnd - constructorStart;
        metadata.localCount = localCount;
        metadata.parameterCount = paramNames.size();
        metadata.parameterNames = paramNames;
        metadata.returnType = returnTypeStr;
        metadata.isStatic = false;
        metadata.localVariableNames = localVarNames;
        metadata.isNative = false;

        // Preserve exception table built during body compilation
        if (existingMetadata) {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        ctx.program.registerFunction(constructorName, metadata);

        // If this is a generic class, also register with full generic name
        if (ctx.currentClassNode && ctx.currentClassNode->isGeneric())
        {
            std::string fullClassName = ctx.currentClassNode->getFullClassName();
            std::string genericConstructorName = fullClassName + "::<init>/" + typeSignature;
            ctx.program.registerFunction(genericConstructorName, metadata);
        }

        return std::monostate{};
    }
}
