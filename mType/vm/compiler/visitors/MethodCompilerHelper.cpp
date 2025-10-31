#include "MethodCompilerHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"

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
                                     static_cast<uint32_t>(classNameIndex),
                                     static_cast<uint32_t>(0), node); // 0 arguments
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

        // Register default constructor
        std::string className = node->getClassName();
        std::string constructorName = className + "::<init>/0"; // 0 args

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

        ctx.program.registerFunction(metadata.name, metadata);

        // If this is a generic class, also register with full generic name
        if (node->isGeneric())
        {
            std::string fullClassName = node->getFullClassName();
            std::string genericConstructorName = fullClassName + "::<init>/0";
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
                    ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex), fieldNode);
                }
            }
        }
    }

    MethodCompilerHelper::MethodParameters MethodCompilerHelper::collectMethodParameters(ast::MethodNode* node, bool isStatic)
    {
        MethodParameters result;

        // Get parameters with type information
        auto genericParams = node->getGenericParameters();

        // For instance methods, 'this' is implicitly the first parameter
        if (!isStatic)
        {
            result.paramNames.push_back("this");
            result.paramTypes.push_back(ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "object");
        }

        // Add method parameters with full type names (including class names for objects)
        for (const auto& param : genericParams)
        {
            result.paramNames.push_back(param.first);
            // Use toString() to get the full type name (e.g., "int", "string", "MyClass", "List<int>")
            std::string paramTypeStr = param.second->toString();
            result.paramTypes.push_back(paramTypeStr);
        }

        // Convert return type to string, preserving class names for object types
        auto genericReturnType = node->getGenericReturnType();
        if (genericReturnType) {
            result.returnTypeStr = genericReturnType->toString();
        } else {
            result.returnTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(node->getReturnType());
        }

        return result;
    }

    MethodCompilerHelper::MethodBodyInfo MethodCompilerHelper::compileMethodBodyWithFrame(ast::MethodNode* node, const MethodParameters& params, bool isStatic)
    {
        // Enter function frame for local variable tracking
        ctx.functionFrameManager.enterFunctionFrame(params.returnTypeStr,
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false);
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
                ctx.variableTracker.declareLocal(param.first, value::ValueType::OBJECT, param.second->toString());
            }
            else
            {
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

        // Build qualified method name for registry
        // Instance methods: ClassName::methodName
        // Static methods: ClassName::methodName$static
        std::string qualifiedMethodName = node->getName();
        if (ctx.currentClassNode)
        {
            qualifiedMethodName = ctx.currentClassNode->getClassName() + "::" + node->getName();
            // Add suffix to distinguish static from instance methods
            if (isStatic)
            {
                qualifiedMethodName += "$static";
            }
        }

        // Register method metadata
        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = qualifiedMethodName;
        metadata.startOffset = methodStart;
        metadata.instructionCount = ctx.program.getCurrentOffset() - methodStart;
        metadata.localCount = bodyInfo.localCount;
        metadata.parameterCount = params.paramNames.size();
        metadata.parameterNames = params.paramNames;
        metadata.parameterTypes = params.paramTypes;
        metadata.returnType = params.returnTypeStr;
        metadata.isStatic = isStatic;
        metadata.isNative = false;
        metadata.isAsync = node->getIsAsync();
        metadata.localVariableNames = bodyInfo.localVarNames;

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

        // Handle generic methods - store generic type parameter names
        if (node->isGeneric())
        {
            const auto& genericParams = node->getGenericTypeParameters();
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

        // Compile method body with frame management
        MethodBodyInfo bodyInfo = compileMethodBodyWithFrame(node, params, isStatic);

        // Restore instance/static method context
        ctx.inInstanceMethod = wasInInstanceMethod;
        ctx.inStaticMethod = wasInStaticMethod;

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

        // Enter function frame for local variable tracking
        ctx.functionFrameManager.enterFunctionFrame(returnTypeStr,
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
                                     static_cast<uint32_t>(classNameIndex),
                                     static_cast<uint32_t>(0), node); // 0 arguments
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
        metadata.localVariableNames = localVarNames;
        metadata.isNative = false;

        ctx.program.registerFunction(constructorName, metadata);

        // If this is a generic class, also register with full generic name
        if (ctx.currentClassNode && ctx.currentClassNode->isGeneric())
        {
            std::string fullClassName = ctx.currentClassNode->getFullClassName();
            std::string genericConstructorName = fullClassName + "::<init>/" + std::to_string(params.size());
            ctx.program.registerFunction(genericConstructorName, metadata);
        }

        return std::monostate{};
    }
}
