#include "ClassCompiler.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../optimization/PrimitiveMethodOptimizer.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../errors/AbstractClassException.hpp"
#include "../../../errors/AccessViolationException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/CastExpression.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/classes/SuperMemberAccessNode.hpp"
#include "../../../ast/nodes/classes/SuperMemberAssignmentNode.hpp"
#include "../../../ast/nodes/classes/ThisConstructorCallNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../types/TypeSubstitutionService.hpp"
#include "../../../circularDependency/TrueCyclicException.hpp"
#include "../../../circularDependency/DepthLimitException.hpp"
#include <unordered_set>
#include <iostream>

namespace vm::compiler::visitors
{
    ClassCompiler::ClassCompiler(CompilerContext& context)
        : ctx(context)
        , methodHelper(std::make_unique<MethodCompilerHelper>(context))
        , paramValidator(std::make_unique<ParameterValidator>(context))
        , overloadResolver(std::make_unique<overload::OverloadResolutionHelper>(context))
    {
    }

    value::Value ClassCompiler::compileClass(ast::ClassNode* node)
    {
        // Store current class context
        auto previousClassNode = ctx.currentClassNode;
        ctx.currentClassNode = node;

        // Compile all methods (generates bytecode for each method)
        for (const auto& method : node->getMethods())
        {
            method->accept(ctx.visitor);
        }

        // Compile all constructors (generates bytecode for each constructor)
        for (const auto& constructor : node->getConstructors())
        {
            constructor->accept(ctx.visitor);
        }

        // If no explicit constructors, compile default constructor
        if (node->getConstructors().empty())
        {
            methodHelper->compileDefaultConstructor(node);
        }

        // Compile static field initializers if any
        for (const auto& field : node->getFields())
        {
            field->accept(ctx.visitor);
        }

        // Restore previous class context
        ctx.currentClassNode = previousClassNode;

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMethod(ast::MethodNode* node)
    {
        return methodHelper->compileMethod(node);
    }

    value::Value ClassCompiler::compileConstructor(ast::ConstructorNode* node)
    {
        return methodHelper->compileConstructor(node);
    }

    value::Value ClassCompiler::compileField(ast::FieldNode* node)
    {
        // Static field initialization happens here
        // Instance fields are initialized in constructors
        if (node->getIsStatic() && node->hasInitialValue())
        {
            // Compile the initialization value
            node->getInitialValue()->accept(ctx.visitor);

            // Store in static field using qualified name (ClassName::fieldName)
            std::string className = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
            std::string qualifiedName = className + "::" + node->getName();
            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);

            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint64_t>(nameIndex), node);
        }

        return std::monostate{};
    }

    std::vector<std::string> ClassCompiler::parseAndValidateGenericTypeArguments(
        const std::string& fullClassName,
        const ast::SourceLocation& location)
    {
        std::vector<std::string> typeArguments;

        // Extract base class name
        std::string baseClassName = fullClassName;
        size_t genericStart = fullClassName.find('<');
        if (genericStart != std::string::npos) {
            baseClassName = fullClassName.substr(0, genericStart);
        }

        // Check if there are generic type arguments
        if (genericStart == std::string::npos) {
            // No type arguments provided - validate that class is not generic
            auto classDef = ctx.environment->findClass(baseClassName);
            if (classDef && !classDef->getGenericParameters().empty()) {
                throw errors::TypeException(
                    "Generic class '" + baseClassName + "' requires " +
                    std::to_string(classDef->getGenericParameters().size()) +
                    " type argument(s)",
                    location
                );
            }
            return typeArguments; // No generics
        }

        size_t genericEnd = fullClassName.rfind('>');
        if (genericEnd == std::string::npos || genericEnd <= genericStart) {
            throw errors::TypeException(
                "Malformed generic type arguments in class name: " + fullClassName,
                location
            );
        }

        // Extract the type arguments string (between < and >)
        std::string argsStr = fullClassName.substr(genericStart + 1, genericEnd - genericStart - 1);

        // PHASE 4: Handle diamond operator <> for type inference
        // If argsStr is empty, this is the diamond operator (e.g., "Container<>")
        // Return empty vector - types will be inferred from context
        if (argsStr.empty() || (argsStr.find_first_not_of(" \t") == std::string::npos)) {
            return typeArguments; // Diamond operator - types will be inferred
        }

        // Parse individual type arguments, respecting nested generics (depth-aware)
        // This correctly handles: Container<List<String>, HashMap<Int, String>, HashSet<Int>>
        size_t start = 0;
        size_t depth = 0;

        for (size_t i = 0; i < argsStr.length(); ++i) {
            if (argsStr[i] == '<') {
                depth++;  // Entering nested generic
            } else if (argsStr[i] == '>') {
                depth--;  // Exiting nested generic
            } else if (argsStr[i] == ',' && depth == 0) {
                // Only split on commas at depth 0 (outermost level)
                std::string typeArg = argsStr.substr(start, i - start);
                // Trim whitespace
                typeArg.erase(0, typeArg.find_first_not_of(" \t"));
                typeArg.erase(typeArg.find_last_not_of(" \t") + 1);
                if (!typeArg.empty()) {
                    typeArguments.push_back(typeArg);
                }
                start = i + 1;
            }
        }

        // Add the last type argument
        std::string lastTypeArg = argsStr.substr(start);
        lastTypeArg.erase(0, lastTypeArg.find_first_not_of(" \t"));
        lastTypeArg.erase(lastTypeArg.find_last_not_of(" \t") + 1);
        if (!lastTypeArg.empty()) {
            typeArguments.push_back(lastTypeArg);
        }

        // PHASE 4: Validate that generic type arguments are not primitives
        // Like Java, generics only support object types - use Int, Float, Bool, String instead of primitives
        for (const auto& typeArg : typeArguments) {
            if (typeArg == "int" || typeArg == "float" || typeArg == "bool" || typeArg == "string" || typeArg == "void") {
                throw errors::TypeException(
                    "Generic type argument '" + typeArg + "' is a primitive type. " +
                    "Generics only support object types. Use wrapper classes instead:\n" +
                    "  - Use 'Int' instead of 'int'\n" +
                    "  - Use 'Float' instead of 'float'\n" +
                    "  - Use 'Bool' instead of 'bool'\n" +
                    "  - Use 'String' instead of 'string'",
                    location
                );
            }
        }

        // PHASE 4: Validate type arguments against class definition
        auto classDef = ctx.environment->findClass(baseClassName);
        if (classDef) {
            const auto& genericParams = classDef->getGenericParameters();

            // Validate: Non-generic class cannot be used with type arguments
            if (genericParams.empty()) {
                throw errors::TypeException(
                    "Class '" + baseClassName + "' is not generic but used with type arguments",
                    location
                );
            }

            // Validate: Number of type arguments must match number of generic parameters
            if (typeArguments.size() != genericParams.size()) {
                throw errors::TypeException(
                    "Class '" + baseClassName + "' expects " +
                    std::to_string(genericParams.size()) +
                    " type argument(s) but " + std::to_string(typeArguments.size()) + " provided",
                    location
                );
            }
        }

        return typeArguments;
    }

    void ClassCompiler::emitNewObjectBytecode(ast::NewNode* node, const std::string& fullClassName,
                                              const runtimeTypes::klass::ConstructorDefinition* constructor,
                                              const std::unordered_map<std::string, std::string>& genericTypeBindings)
    {
        const auto& arguments = node->getArguments();

        // Create service for generic type substitution
        ::types::TypeSubstitutionService service;

        // Push constructor arguments onto stack (left to right) with auto-boxing
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            // Check if we need to apply auto-boxing
            bool needsAutoBoxing = false;
            std::string boxClassName;

            if (constructor && i < constructor->getParameterCount())
            {
                const auto& params = constructor->getParametersWithTypes();
                const auto& paramType = params[i].second;

                // Check if parameter expects an object type
                if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value())
                {
                    std::string expectedClass = paramType.className.value();

                    // Resolve generic type parameters (T -> Int)
                    if (!genericTypeBindings.empty())
                    {
                        expectedClass = service.resolveGenericType(expectedClass, genericTypeBindings);
                    }

                    // Infer argument type
                    value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

                    // Check if auto-boxing is needed
                    if ((expectedClass == "Int" && argType == value::ValueType::INT) ||
                        (expectedClass == "Float" && argType == value::ValueType::FLOAT) ||
                        (expectedClass == "Bool" && argType == value::ValueType::BOOL) ||
                        (expectedClass == "String" && argType == value::ValueType::STRING))
                    {
                        needsAutoBoxing = true;
                        boxClassName = expectedClass;
                    }
                }
            }

            // Compile the argument
            arguments[i]->accept(ctx.visitor);

            // Apply auto-boxing if needed
            if (needsAutoBoxing)
            {
                size_t classNameIndex = ctx.program.getConstantPool().addString(boxClassName);
                auto boxClassDef = ctx.environment->findClass(boxClassName);
                bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
                if (boxIsValue) {
                    ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                               static_cast<uint64_t>(classNameIndex),
                                               1u, arguments[i].get());
                    ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, arguments[i].get());
                } else {
                    ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                               static_cast<uint64_t>(classNameIndex),
                                               1u, arguments[i].get());
                }
            }
        }

        // Store the FULL class name including generics (e.g., "Box<Int>")
        size_t classNameIndex = ctx.program.getConstantPool().addString(fullClassName);

        // Check if the target class is a value class
        std::string baseClassName = fullClassName;
        size_t genStart = fullClassName.find('<');
        if (genStart != std::string::npos) {
            baseClassName = fullClassName.substr(0, genStart);
        }
        auto classDef = ctx.environment->findClass(baseClassName);
        bool isValueClass = classDef && classDef->isValueClass();

        if (isValueClass) {
            // Emit NEW_VALUE_OBJECT + OBJECT_TO_VALUE for value classes
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(arguments.size()), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, node);
        } else {
            // Emit NEW_OBJECT instruction with full class name and argument count
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(arguments.size()), node);
        }
    }

    value::Value ClassCompiler::compileNew(ast::NewNode* node)
    {
        std::string fullClassName = node->getClassName();

        // Parse and validate generic type arguments (e.g., "Box<Int>" -> ["Int"])
        std::vector<std::string> typeArguments = parseAndValidateGenericTypeArguments(fullClassName, node->getLocation());

        // Extract base class name for constructor lookup
        std::string baseClassName = fullClassName;
        size_t genericStart = fullClassName.find('<');
        if (genericStart != std::string::npos)
        {
            baseClassName = fullClassName.substr(0, genericStart);
        }

        // Validate constructor exists at compile time
        const auto& arguments = node->getArguments();
        if (ctx.compileTimeValidator)
        {
            ctx.compileTimeValidator->validateConstructorExists(baseClassName, arguments.size(), node->getLocation());
        }

        // Validate constructor parameters if class definition exists
        auto classDef = ctx.environment->findClass(baseClassName);
        runtimeTypes::klass::ConstructorDefinition* matchingConstructor = nullptr;
        std::unordered_map<std::string, std::string> genericTypeBindings;

        if (classDef)
        {
            // Validate: Cannot instantiate abstract classes
            if (classDef->isAbstract()) {
                throw errors::AbstractClassException(
                    "Cannot instantiate abstract class '" + baseClassName + "'",
                    node->getLocation()
                );
            }

            // Build generic type bindings map for parameter validation
            // Maps generic parameter names (e.g., "T") to concrete types (e.g., "Int")
            const auto& genericParams = classDef->getGenericParameters();
            for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i)
            {
                genericTypeBindings[genericParams[i].name] = typeArguments[i];
            }

            // Find matching constructor by trying all constructors with matching parameter count
            // Constructor overload resolution: try each constructor and pick the first one that validates
            const auto& constructors = classDef->getConstructors();
            std::string lastError;

            for (const auto& constructor : constructors)
            {
                if (constructor->getParameterCount() == arguments.size())
                {
                    try {
                        paramValidator->validateConstructorParameters(arguments, constructor.get(), node->getLocation(), genericTypeBindings);
                        matchingConstructor = constructor.get();
                        break;  // Found a matching constructor
                    }
                    catch (const std::exception& e) {
                        // This constructor didn't match - try the next one
                        lastError = e.what();
                        continue;
                    }
                }
            }

            // If no constructor matched, throw the last error
            if (!matchingConstructor && !lastError.empty()) {
                throw std::runtime_error(lastError);
            }
        }

        // Emit bytecode for object creation with auto-boxing support
        emitNewObjectBytecode(node, fullClassName, matchingConstructor, genericTypeBindings);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMemberAccess(ast::MemberAccessNode* node)
    {
        std::string memberName = node->getMemberName();
        bool isStaticAccess = node->getIsStaticAccess();

        if (isStaticAccess)
        {
            // Static field access: ClassName::fieldName
            // Build qualified name from the object (class name) and member name
            std::string qualifiedName;
            if (auto* varNode = dynamic_cast<ast::VariableNode*>(node->getObject())) {
                qualifiedName = varNode->getName() + "::" + memberName;
            } else {
                // Fallback - shouldn't happen for valid static access
                qualifiedName = memberName;
            }
            size_t fieldNameIndex = ctx.program.getConstantPool().addString(qualifiedName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint64_t>(fieldNameIndex), node);
        }
        else
        {
            // Special case: .length is ALWAYS handled with ARRAY_LENGTH opcode
            // This must be checked BEFORE the SoA optimization because:
            // 1. array[index].length should work for nested arrays
            // 2. .length is not a field in SoA structure
            if (memberName == "length")
            {
                // Compile the object/array expression (could be array[index] or just array)
                node->getObject()->accept(ctx.visitor);
                // Emit ARRAY_LENGTH opcode
                ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_LENGTH, node);
            }
            // Check if this is array[index].field pattern (SoA optimization opportunity)
            else if (auto* indexAccessNode = dynamic_cast<ast::IndexAccessNode*>(node->getObject()))
            {
                // This is array[index].field - use ARRAY_GET_FIELD for SoA optimization!
                // Compile the array expression
                indexAccessNode->getCollection()->accept(ctx.visitor);

                // Compile the index expression
                indexAccessNode->getIndex()->accept(ctx.visitor);

                // Emit optimized ARRAY_GET_FIELD opcode
                // This will use fast path for SoA arrays (direct field access)
                size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_GET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
            }
            else
            {
                // Regular instance field access: object.fieldName
                // First, compile the object expression
                ast::ASTNode* receiverNode = node->getObject();
                bool nonNullReceiver = isReceiverNonNullable(receiverNode);
                receiverNode->accept(ctx.visitor); // Will need delegation

                // Regular field access - emit GET_FIELD instruction
                size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
                if (nonNullReceiver)
                {
                    ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
                }
            }
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMemberAssignment(ast::MemberAssignmentNode* node)
    {
        std::string memberName = node->getMemberName();

        // Check if this is array[index].field = value pattern (SoA optimization opportunity)
        auto* objectNode = node->getObject();
        if (auto* indexAccessNode = dynamic_cast<ast::IndexAccessNode*>(objectNode))
        {
            // This is array[index].field = value - use ARRAY_SET_FIELD for SoA optimization!
            // Compile the array expression
            indexAccessNode->getCollection()->accept(ctx.visitor);

            // Compile the index expression
            indexAccessNode->getIndex()->accept(ctx.visitor);

            // Compile the value to assign
            node->getValue()->accept(ctx.visitor);

            // Emit optimized ARRAY_SET_FIELD opcode
            // This will use fast path for SoA arrays (direct field write)
            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_SET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
        }
        else
        {
            // Regular member assignment: object.field = value
            // Compile the object expression
            ast::ASTNode* receiverNode = node->getObject();
            bool nonNullReceiver = isReceiverNonNullable(receiverNode);
            receiverNode->accept(ctx.visitor); // Will need delegation

            // Compile the value to assign
            node->getValue()->accept(ctx.visitor); // Will need delegation

            // Emit SET_FIELD instruction (object and value are on stack)
            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
            if (nonNullReceiver)
            {
                ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
            }
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMethodCall(ast::MethodCallNode* node)
    {
        std::string methodName = node->getMethodName();
        bool isStaticCall = node->getIsStaticCall();
        const auto& arguments = node->getArguments();

        if (isStaticCall)
        {
            // Static method call: ClassName::methodName(args) or this::methodName(args)

            // Build fully qualified method name
            std::string className;
            auto* objectNode = node->getObject();
            if (auto* varNode = dynamic_cast<ast::VariableNode*>(objectNode))
            {
                className = varNode->getName();
                // If using "this::", replace with actual class name
                if (className == "this" && ctx.currentClassNode)
                {
                    className = ctx.currentClassNode->getClassName();
                }
            }
            std::string qualifiedName = className + "::" + methodName;

            // Resolve method overload to get mangled name
            std::string resolvedMethodName = overloadResolver->resolveStaticMethodOverload(
                className, methodName, arguments, node->getLocation());

            // PHASE 4: Validate generic type arguments
            if (node->hasGenericTypeArguments())
            {
                const auto& typeArgs = node->getGenericTypeArguments();

                // Validate: Generic type arguments must be object types, not primitives
                for (const auto& typeArg : typeArgs)
                {
                    if (typeArg == "int" || typeArg == "float" || typeArg == "string" ||
                        typeArg == "bool" || typeArg == "void")
                    {
                        throw errors::TypeException(
                            "Generic type argument '" + typeArg + "' is a primitive type. " +
                            "Generics only support object types. Use wrapper classes instead:\n" +
                            "  - Use 'Int' instead of 'int'\n" +
                            "  - Use 'Float' instead of 'float'\n" +
                            "  - Use 'Bool' instead of 'bool'\n" +
                            "  - Use 'String' instead of 'string'",
                            node->getLocation()
                        );
                    }
                }

                // Validate: Method must be generic
                const auto* funcMetadata = ctx.program.getFunction(qualifiedName);
                if (funcMetadata) {
                    const auto& genericParams = funcMetadata->genericTypeParameters;

                    // Validate: Non-generic method cannot be used with type arguments
                    if (genericParams.empty()) {
                        throw errors::TypeException(
                            "Method '" + qualifiedName + "' is not generic but used with type arguments",
                            node->getLocation()
                        );
                    }

                    // Validate: Number of type arguments must match number of generic parameters
                    if (typeArgs.size() != genericParams.size()) {
                        throw errors::TypeException(
                            "Method '" + qualifiedName + "' expects " +
                            std::to_string(genericParams.size()) +
                            " type argument(s) but " + std::to_string(typeArgs.size()) + " provided",
                            node->getLocation()
                        );
                    }
                }
                else {
                    // Fallback: Check environment if metadata not yet registered
                    auto classDef = ctx.environment->findClass(className);
                    if (classDef) {
                        auto methodDef = classDef->getMethod(methodName);
                        if (methodDef) {
                            const auto& genericParams = methodDef->getGenericTypeParameters();

                            // Validate: Non-generic method cannot be used with type arguments
                            if (genericParams.empty()) {
                                throw errors::TypeException(
                                    "Method '" + qualifiedName + "' is not generic but used with type arguments",
                                    node->getLocation()
                                );
                            }

                            // Validate: Number of type arguments must match number of generic parameters
                            if (typeArgs.size() != genericParams.size()) {
                                throw errors::TypeException(
                                    "Method '" + qualifiedName + "' expects " +
                                    std::to_string(genericParams.size()) +
                                    " type argument(s) but " + std::to_string(typeArgs.size()) + " provided",
                                    node->getLocation()
                                );
                            }
                        }
                    }
                }
            }

            // Validate static method exists at compile time
            if (ctx.compileTimeValidator)
            {
                ctx.compileTimeValidator->validateStaticMethodExists(className, methodName, arguments.size(), node->getLocation());
            }

            // Validate parameter count and types
            paramValidator->validateMethodParameters(qualifiedName, qualifiedName, arguments, node->getLocation());

            // Get method metadata for auto-boxing
            const auto* methodMetadata = ctx.program.getFunction(qualifiedName);

            // Push all arguments onto stack (with auto-boxing if needed)
            for (size_t i = 0; i < arguments.size(); ++i)
            {
                bool autoBoxed = false;

                // Try auto-boxing if we have method metadata
                if (methodMetadata && !methodMetadata->isNative)
                {
                    // For instance methods, parameterTypes[0] is 'this', so offset by 1
                    size_t paramOffset = (!methodMetadata->isStatic && !methodMetadata->parameterTypes.empty()) ? 1 : 0;

                    if (i + paramOffset < methodMetadata->parameterTypes.size())
                    {
                        std::string expectedType = methodMetadata->parameterTypes[i + paramOffset];
                        expectedType = ctx.resolveGenericType(expectedType);
                        autoBoxed = tryAutoBoxArgument(arguments[i].get(), expectedType);
                    }
                }

                // If not auto-boxed, compile argument normally
                if (!autoBoxed)
                {
                    arguments[i]->accept(ctx.visitor);
                }
            }

            // Emit CALL_STATIC instruction with resolved (mangled) method name
            size_t methodNameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                             static_cast<uint64_t>(methodNameIndex),
                             static_cast<uint64_t>(arguments.size()), node);
        }
        else
        {
            // Instance method call: object.methodName(args)

            // Infer the class of the object for type checking
            std::string objectClassName = ctx.typeInference.inferExpressionClassName(node->getObject());

            // Resolve method overload to get mangled name (if class known)
            std::string resolvedMethodName = methodName;
            if (!objectClassName.empty())
            {
                resolvedMethodName = overloadResolver->resolveInstanceMethodOverload(
                    objectClassName, methodName, arguments, node->getLocation(),
                    node->hasGenericTypeArguments(), node->getGenericTypeArguments().size());
            }

            // Extract generic type bindings from objectClassName if it's a generic instantiation
            // E.g., "Box<String>" -> push {T: String} onto the binding stack
            std::unordered_map<std::string, std::string> genericBindings;
            if (objectClassName.find('<') != std::string::npos)
            {
                // Parse "Box<String>" into base class and type arguments
                size_t angleStart = objectClassName.find('<');
                size_t angleEnd = objectClassName.rfind('>');
                if (angleEnd != std::string::npos && angleEnd > angleStart)
                {
                    std::string baseClassName = objectClassName.substr(0, angleStart);
                    std::string typeArgsStr = objectClassName.substr(angleStart + 1, angleEnd - angleStart - 1);

                    // Look up the base class OR interface to get its generic parameter names
                    auto classDef = ctx.environment->findClass(baseClassName);
                    auto interfaceDef = ctx.environment->findInterface(baseClassName);

                    std::vector<ast::GenericTypeParameter> genericParams;
                    bool isGenericType = false;

                    if (classDef && classDef->isGeneric())
                    {
                        genericParams = classDef->getGenericParameters();
                        isGenericType = true;
                    }
                    else if (interfaceDef && interfaceDef->isGeneric())
                    {
                        genericParams = interfaceDef->getGenericParameters();
                        isGenericType = true;
                    }

                    if (isGenericType)
                    {

                        // Parse type arguments (e.g., "String" or "String, Int")
                        std::vector<std::string> typeArgs;
                        size_t start = 0;
                        size_t depth = 0;
                        for (size_t i = 0; i < typeArgsStr.length(); ++i)
                        {
                            if (typeArgsStr[i] == '<') depth++;
                            else if (typeArgsStr[i] == '>') depth--;
                            else if (typeArgsStr[i] == ',' && depth == 0)
                            {
                                std::string arg = typeArgsStr.substr(start, i - start);
                                arg.erase(0, arg.find_first_not_of(" \t"));
                                arg.erase(arg.find_last_not_of(" \t") + 1);
                                typeArgs.push_back(arg);
                                start = i + 1;
                            }
                        }
                        // Add last argument
                        std::string arg = typeArgsStr.substr(start);
                        arg.erase(0, arg.find_first_not_of(" \t"));
                        arg.erase(arg.find_last_not_of(" \t") + 1);
                        if (!arg.empty())
                        {
                            typeArgs.push_back(arg);
                        }

                        // Create bindings: T -> String, etc.
                        for (size_t i = 0; i < genericParams.size() && i < typeArgs.size(); ++i)
                        {
                            genericBindings[genericParams[i].name] = typeArgs[i];
                        }
                    }
                }
            }

            // Push generic type bindings onto the stack for resolution during validation
            if (!genericBindings.empty())
            {
                ctx.pushGenericTypeBindings(genericBindings);
            }

            // Setup method-level generic type bindings if method has type arguments
            bool hasMethodGenericBindings = false;
            std::unordered_map<std::string, std::string> methodGenericBindings;

            // Extract base class name (without generic parameters) for method lookup
            std::string baseClassName = objectClassName;
            size_t anglePos = objectClassName.find('<');
            if (anglePos != std::string::npos)
            {
                baseClassName = objectClassName.substr(0, anglePos);
            }

            // Build qualified method name for metadata lookup
            // Use resolvedMethodName (with signature) for correct overload identification
            std::string qualifiedMethodName = baseClassName.empty() ? methodName : (baseClassName + "::" + methodName);
            const auto* methodMetadata = ctx.program.getFunction(resolvedMethodName);

            // Fallback to class definition if bytecode metadata not available
            std::shared_ptr<runtimeTypes::klass::MethodDefinition> methodDef;
            if (!methodMetadata && !baseClassName.empty())
            {
                auto classDef = ctx.environment->findClass(baseClassName);
                if (classDef)
                {
                    // Try to find the method by name (may have overloads)
                    auto overloads = classDef->getAllInstanceMethodOverloads(methodName);
                    if (overloads.size() == 1)
                    {
                        methodDef = overloads[0];
                    }
                    else if (overloads.size() > 1)
                    {
                        // Multiple overloads - find one that matches the argument count
                        // (this is for validation purposes; overload resolution happens elsewhere)
                        size_t argCount = arguments.size();
                        for (const auto& overload : overloads)
                        {
                            // Instance methods have 'this' as first param, so compare argCount+1
                            if (overload->getParameters().size() == argCount + 1)
                            {
                                methodDef = overload;
                                break;
                            }
                        }
                        // If no exact match, just use the first one for generic validation
                        if (!methodDef && !overloads.empty())
                        {
                            methodDef = overloads[0];
                        }
                    }
                }
            }

            if (node->hasGenericTypeArguments())
            {
                const auto& methodTypeArgs = node->getGenericTypeArguments();

                // PHASE 4: Validate that method is generic
                bool isGenericMethod = false;
                size_t expectedTypeParamCount = 0;

                if (methodMetadata)
                {
                    isGenericMethod = !methodMetadata->genericTypeParameters.empty();
                    expectedTypeParamCount = methodMetadata->genericTypeParameters.size();
                }
                else if (methodDef)
                {
                    isGenericMethod = !methodDef->getGenericTypeParameters().empty();
                    expectedTypeParamCount = methodDef->getGenericTypeParameters().size();
                }

                // Validate: Non-generic method cannot have type arguments
                // Skip validation if we couldn't determine method info (e.g., chained calls in Phase 1)
                if (!isGenericMethod && (methodMetadata || methodDef))
                {
                    throw errors::TypeException(
                        "Method '" + qualifiedMethodName + "' is not generic but used with type arguments",
                        node->getLocation()
                    );
                }

                // Validate: Type argument count must match
                // Only validate if we have method information
                if ((methodMetadata || methodDef) && methodTypeArgs.size() != expectedTypeParamCount)
                {
                    throw errors::TypeException(
                        "Method '" + methodName + "' expects " +
                        std::to_string(expectedTypeParamCount) +
                        " type argument(s), but got " +
                        std::to_string(methodTypeArgs.size()),
                        node->getLocation()
                    );
                }

                // Validate: Generic type arguments must be object types, not primitives
                for (const auto& typeArg : methodTypeArgs)
                {
                    if (typeArg == "int" || typeArg == "float" || typeArg == "string" ||
                        typeArg == "bool" || typeArg == "void")
                    {
                        throw errors::TypeException(
                            "Generic type argument '" + typeArg + "' is a primitive type. " +
                            "Generics only support object types. Use wrapper classes instead:\n" +
                            "  - Use 'Int' instead of 'int'\n" +
                            "  - Use 'Float' instead of 'float'\n" +
                            "  - Use 'Bool' instead of 'bool'\n" +
                            "  - Use 'String' instead of 'string'",
                            node->getLocation()
                        );
                    }
                }

                // Create bindings from type arguments
                std::vector<std::string> methodGenericParams;
                if (methodMetadata)
                {
                    methodGenericParams = methodMetadata->genericTypeParameters;
                }
                else if (methodDef)
                {
                    for (const auto& param : methodDef->getGenericTypeParameters())
                    {
                        methodGenericParams.push_back(param.name);
                    }
                }

                // Create bindings: T -> String, U -> Int, etc.
                for (size_t i = 0; i < methodGenericParams.size() && i < methodTypeArgs.size(); ++i)
                {
                    methodGenericBindings[methodGenericParams[i]] = methodTypeArgs[i];
                }

                // Push method-level bindings onto stack (will shadow class-level if names conflict)
                if (!methodGenericBindings.empty())
                {
                    ctx.pushGenericTypeBindings(methodGenericBindings);
                    hasMethodGenericBindings = true;
                }
            }
            else
            {
                // PHASE 1: Require explicit type arguments for generic methods
                // Check if method is generic - if so, error that type args are required
                bool requiresTypeArgs = false;
                if (methodMetadata && !methodMetadata->genericTypeParameters.empty())
                {
                    requiresTypeArgs = true;
                }
                else if (methodDef && !methodDef->getGenericTypeParameters().empty())
                {
                    requiresTypeArgs = true;
                }

                if (requiresTypeArgs)
                {
                    throw errors::TypeException(
                        "Generic method '" + methodName + "' requires explicit type arguments. " +
                        "Phase 1 does not support type inference for method calls.",
                        node->getLocation()
                    );
                }
            }

            // Validate instance method exists at compile time
            if (!objectClassName.empty() && ctx.compileTimeValidator)
            {
                // Extract base class name (without generic parameters)
                std::string baseClassName = objectClassName;
                size_t anglePos = objectClassName.find('<');
                if (anglePos != std::string::npos)
                {
                    baseClassName = objectClassName.substr(0, anglePos);
                }
                ctx.compileTimeValidator->validateInstanceMethodExists(baseClassName, methodName, arguments.size(), node->getLocation());
            }

            // Validate parameter count and types for instance methods
            if (!objectClassName.empty())
            {
                // Use the resolved method name (with signature) for validation after overload resolution
                paramValidator->validateMethodParameters(methodName, resolvedMethodName, arguments, node->getLocation());
            }

            // First, compile the object expression
            bool nonNullReceiver = isReceiverNonNullable(node->getObject());
            node->getObject()->accept(ctx.visitor); // Will need delegation

            // Push all arguments onto stack with auto-boxing if needed
            // If no methodMetadata, try looking up interface definition
            std::vector<std::string> interfaceParameterTypes;
            if (!methodMetadata && !baseClassName.empty())
            {
                auto interfaceDef = ctx.environment->findInterface(baseClassName);
                if (interfaceDef)
                {
                    const auto& methodSigs = interfaceDef->getMethodSignatures();
                    for (const auto& methodSig : methodSigs)
                    {
                        if (methodSig.name == methodName)
                        {
                            // Extract parameter type names from GenericType objects
                            for (const auto& param : methodSig.parameters)
                            {
                                std::string paramTypeName = param.second->toString();
                                interfaceParameterTypes.push_back(paramTypeName);
                            }
                            break;
                        }
                    }
                }
            }

            for (size_t i = 0; i < arguments.size(); ++i)
            {
                bool autoBoxed = false;

                // Check if we need to auto-box this argument
                std::string expectedType;
                bool hasExpectedType = false;

                if (methodMetadata && !methodMetadata->parameterTypes.empty())
                {
                    // For instance methods, parameterTypes[0] is 'this', so offset by 1
                    size_t paramIndex = i + 1; // +1 to skip 'this' parameter

                    if (paramIndex < methodMetadata->parameterTypes.size())
                    {
                        expectedType = methodMetadata->parameterTypes[paramIndex];
                        hasExpectedType = true;
                    }
                }
                else if (methodDef && !methodDef->getUnifiedParameters().empty())
                {
                    // Fallback to method definition from ClassDefinition
                    const auto& uParams = methodDef->getUnifiedParameters();
                    // unifiedParameters already exclude 'this' for instance methods
                    if (i < uParams.size())
                    {
                        expectedType = uParams[i].second ? uParams[i].second->toString() : "void";
                        hasExpectedType = true;
                    }
                }
                else if (!interfaceParameterTypes.empty() && i < interfaceParameterTypes.size())
                {
                    // Use interface parameter types
                    expectedType = interfaceParameterTypes[i];
                    hasExpectedType = true;
                }

                if (hasExpectedType)
                {
                    // Resolve generic type parameters using the bindings
                    // Check method-level bindings first, then class-level bindings
                    // E.g., if expectedType is "A" and methodGenericBindings has {A: String}, resolve to "String"
                    if (!methodGenericBindings.empty())
                    {
                        auto it = methodGenericBindings.find(expectedType);
                        if (it != methodGenericBindings.end())
                        {
                            expectedType = it->second;
                        }
                    }
                    // If not found in method bindings, check class-level bindings
                    // E.g., if expectedType is "T" and genericBindings has {T: Int}, resolve to "Int"
                    if (!genericBindings.empty())
                    {
                        auto it = genericBindings.find(expectedType);
                        if (it != genericBindings.end())
                        {
                            expectedType = it->second;
                        }
                    }

                    value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

                        // Check if we need to auto-box primitive to wrapper class
                        bool needsBoxing = false;
                        std::string boxClassName;

                        if (expectedType == "Int" && argType == value::ValueType::INT)
                        {
                            needsBoxing = true;
                            boxClassName = "Int";
                        }
                        else if (expectedType == "Float" && argType == value::ValueType::FLOAT)
                        {
                            needsBoxing = true;
                            boxClassName = "Float";
                        }
                        else if (expectedType == "Bool" && argType == value::ValueType::BOOL)
                        {
                            needsBoxing = true;
                            boxClassName = "Bool";
                        }
                        else if (expectedType == "String" && argType == value::ValueType::STRING)
                        {
                            needsBoxing = true;
                            boxClassName = "String";
                        }

                        if (needsBoxing)
                        {
                            // Auto-box: compile value, then emit NEW_OBJECT or NEW_VALUE_OBJECT
                            arguments[i]->accept(ctx.visitor);
                            size_t classNameIndex = ctx.program.getConstantPool().addString(boxClassName);
                            auto boxClassDef = ctx.environment->findClass(boxClassName);
                            bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
                            if (boxIsValue) {
                                ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                                            static_cast<uint64_t>(classNameIndex),
                                                            1u, arguments[i].get());
                                ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, arguments[i].get());
                            } else {
                                ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                                            static_cast<uint64_t>(classNameIndex),
                                                            1u, arguments[i].get());
                            }
                            autoBoxed = true;
                        }
                }

                // If not auto-boxed, compile argument normally
                if (!autoBoxed)
                {
                    arguments[i]->accept(ctx.visitor);
                }
            }

            // Pop generic type bindings after argument compilation (method-level first, then class-level)
            if (hasMethodGenericBindings)
            {
                ctx.popGenericTypeBindings();
            }
            if (!genericBindings.empty())
            {
                ctx.popGenericTypeBindings();
            }

            // PHASE 3 OPTIMIZATION: Check if this is an optimizable primitive method call
            bytecode::OpCode opcodeToEmit = bytecode::OpCode::CALL_METHOD;

            if (vm::compiler::PrimitiveMethodOptimizer::canOptimizeMethod(baseClassName, methodName, arguments.size())) {
                // Get the optimized opcode for this primitive method
                opcodeToEmit = vm::compiler::PrimitiveMethodOptimizer::getOptimizedOpCode(baseClassName, methodName);

                // Emit optimized opcode (no method name needed - opcode encodes the operation)
                ctx.emitter.emitWithLocation(opcodeToEmit, node);
            } else {
                // Emit standard CALL_METHOD instruction with resolved (mangled) method name
                size_t methodNameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                 static_cast<uint64_t>(methodNameIndex),
                                 static_cast<uint64_t>(arguments.size()), node);
                if (nonNullReceiver)
                {
                    ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
                }
            }
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperConstructorCall(ast::SuperConstructorCallNode* node)
    {
        // Push arguments onto stack
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor); // Will need delegation
        }

        // Emit SUPER_CONSTRUCTOR instruction with current class name
        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_CONSTRUCTOR,
                         static_cast<uint64_t>(classNameIndex),
                         static_cast<uint64_t>(arguments.size()), node);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileThisConstructorCall(ast::ThisConstructorCallNode* node)
    {
        // Push arguments onto stack
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor);
        }

        // Emit THIS_CONSTRUCTOR instruction with current class name and argument count
        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::THIS_CONSTRUCTOR,
                         static_cast<uint64_t>(classNameIndex),
                         static_cast<uint64_t>(arguments.size()), node);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperMethodCall(ast::SuperMethodCallNode* node)
    {
        std::string methodName = node->getMethodName();

        // Validate access modifiers for super method calls
        if (ctx.currentClassNode) {
            auto classRegistry = ctx.environment->getClassRegistry();
            if (classRegistry) {
                auto classDef = classRegistry->findClass(ctx.currentClassNode->getClassName());
                if (classDef && classDef->hasParentClass()) {
                    // Walk up parent class hierarchy to find the method
                    auto parentClass = classDef->getParentClass();
                    while (parentClass) {
                        auto method = parentClass->getMethod(methodName);
                        if (method) {
                            // Found the method - check access modifier
                            if (method->getAccessModifier() == ast::AccessModifier::PRIVATE) {
                                throw errors::AccessViolationException(
                                    "Cannot call private method '" + methodName + "' from parent class '" +
                                    parentClass->getName() + "' in child class '" + classDef->getName() + "'",
                                    node->getLocation()
                                );
                            }
                            // Protected and public are allowed
                            break;
                        }
                        // Continue searching up the hierarchy
                        parentClass = parentClass->hasParentClass() ? parentClass->getParentClass() : nullptr;
                    }
                }
            }
        }

        // Push arguments onto stack
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor); // Will need delegation
        }

        // Emit SUPER_INVOKE instruction with current class name
        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        size_t methodNameIndex = ctx.program.getConstantPool().addString(methodName);

        ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_INVOKE,
                         std::vector<uint64_t>{
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(methodNameIndex),
                             static_cast<uint64_t>(arguments.size())
                         }, node);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperMemberAccess(ast::SuperMemberAccessNode* node)
    {
        std::string memberName = node->getMemberName();

        // Validate access modifiers for super field access
        if (ctx.currentClassNode) {
            auto classRegistry = ctx.environment->getClassRegistry();
            if (classRegistry) {
                auto classDef = classRegistry->findClass(ctx.currentClassNode->getClassName());
                if (classDef && classDef->hasParentClass()) {
                    // Walk up parent class hierarchy to find the field
                    auto parentClass = classDef->getParentClass();
                    while (parentClass) {
                        auto field = parentClass->getField(memberName);
                        if (field) {
                            // Found the field - check access modifier
                            if (field->getAccessModifier() == ast::AccessModifier::PRIVATE) {
                                throw errors::AccessViolationException(
                                    "Cannot access private field '" + memberName + "' from parent class '" +
                                    parentClass->getName() + "' in child class '" + classDef->getName() + "'",
                                    node->getLocation()
                                );
                            }
                            // Protected and public are allowed
                            break;
                        }
                        // Continue searching up the hierarchy
                        parentClass = parentClass->hasParentClass() ? parentClass->getParentClass() : nullptr;
                    }
                }
            }
        }

        // Emit SUPER_GET_FIELD instruction to load field from parent class
        // This is similar to SUPER_INVOKE but for field access instead of method calls
        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        size_t memberNameIndex = ctx.program.getConstantPool().addString(memberName);

        ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_GET_FIELD,
                         std::vector<uint64_t>{
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(memberNameIndex)
                         }, node);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileSuperMemberAssignment(ast::SuperMemberAssignmentNode* node)
    {
        std::string memberName = node->getMemberName();

        // Validate access modifiers for super field assignment
        if (ctx.currentClassNode) {
            auto classRegistry = ctx.environment->getClassRegistry();
            if (classRegistry) {
                auto classDef = classRegistry->findClass(ctx.currentClassNode->getClassName());
                if (classDef && classDef->hasParentClass()) {
                    // Walk up parent class hierarchy to find the field
                    auto parentClass = classDef->getParentClass();
                    while (parentClass) {
                        auto field = parentClass->getField(memberName);
                        if (field) {
                            // Found the field - check access modifier
                            if (field->getAccessModifier() == ast::AccessModifier::PRIVATE) {
                                throw errors::AccessViolationException(
                                    "Cannot access private field '" + memberName + "' from parent class '" +
                                    parentClass->getName() + "' in child class '" + classDef->getName() + "'",
                                    node->getLocation()
                                );
                            }
                            // Protected and public are allowed
                            break;
                        }
                        // Continue searching up the hierarchy
                        parentClass = parentClass->hasParentClass() ? parentClass->getParentClass() : nullptr;
                    }
                }
            }
        }

        // Evaluate the value to assign and push it onto stack
        node->getValue()->accept(ctx.visitor);

        // Emit SUPER_SET_FIELD instruction to store field value to parent class field
        std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
        size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
        size_t memberNameIndex = ctx.program.getConstantPool().addString(memberName);

        ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_SET_FIELD,
                         std::vector<uint64_t>{
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(memberNameIndex)
                         }, node);

        return std::monostate{};
    }

    bool ClassCompiler::tryAutoBoxArgument(ast::ASTNode* argument, const std::string& expectedType)
    {
        // Only auto-box for primitive Box types
        bool isBoxType = (expectedType == "Int" ||
                          expectedType == "Float" ||
                          expectedType == "Bool" ||
                          expectedType == "String");

        if (!isBoxType || !argument)
        {
            return false;  // Not a Box type or no argument node
        }

        // Check if argument expression returns a primitive type matching the target Box type
        value::ValueType argType = ctx.typeInference.inferExpressionType(argument);
        bool needsBoxing = false;

        // Check if we're trying to box a primitive to its corresponding Box type
        if (expectedType == "Int" && argType == value::ValueType::INT)
        {
            needsBoxing = true;
        }
        else if (expectedType == "Float" && argType == value::ValueType::FLOAT)
        {
            needsBoxing = true;
        }
        else if (expectedType == "Bool" && argType == value::ValueType::BOOL)
        {
            needsBoxing = true;
        }
        else if (expectedType == "String" && argType == value::ValueType::STRING)
        {
            needsBoxing = true;
        }

        if (!needsBoxing)
        {
            return false;  // Argument type doesn't match target Box type
        }

        // AUTO-BOXING: Emit bytecode for boxing
        // Equivalent to: new TargetClass(argument)

        // 1. Compile the argument expression (pushes it onto stack)
        argument->accept(ctx.visitor);

        // 2. Emit NEW_OBJECT or NEW_VALUE_OBJECT for the Box class
        size_t classNameIndex = ctx.program.getConstantPool().addString(expectedType);
        auto boxClassDef = ctx.environment->findClass(expectedType);
        bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
        if (boxIsValue) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, argument);
            ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, argument);
        } else {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, argument);
        }

        return true;  // Auto-boxing was applied
    }

    bool ClassCompiler::isReceiverNonNullable(ast::ASTNode* receiverNode)
    {
        // Check if receiver is a simple variable reference
        if (auto* varNode = dynamic_cast<ast::VariableNode*>(receiverNode))
        {
            const std::string& varName = varNode->getName();

            // 'this' is always non-null
            if (varName == "this")
            {
                return true;
            }

            // Check if null-narrowed via smart cast
            if (ctx.nullNarrowing.isNarrowedNonNull(varName))
            {
                return true;
            }

            // Check local variable nullability
            if (ctx.variableTracker.existsInFunction(varName))
            {
                return !ctx.variableTracker.getLocalNullableByName(varName);
            }

            // Check global variable nullability
            if (ctx.globalRegistry.exists(varName))
            {
                return !ctx.globalRegistry.isNullable(varName);
            }
        }

        // NewNode always produces non-null
        if (dynamic_cast<ast::NewNode*>(receiverNode))
        {
            return true;
        }

        // Cast to non-nullable type succeeds or throws
        if (auto* castExpr = dynamic_cast<ast::nodes::expressions::CastExpression*>(receiverNode))
        {
            if (castExpr->getTargetType() && !castExpr->getTargetType()->isNullable())
            {
                return true;
            }
        }

        return false;
    }
}
