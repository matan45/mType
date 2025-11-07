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
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/classes/SuperMemberAccessNode.hpp"
#include "../../../ast/nodes/classes/SuperMemberAssignmentNode.hpp"
#include "../../../ast/nodes/classes/ThisConstructorCallNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../circularDependency/TrueCyclicException.hpp"
#include "../../../circularDependency/DepthLimitException.hpp"
#include <unordered_set>


namespace vm::compiler::visitors
{
    ClassCompiler::ClassCompiler(CompilerContext& context)
        : ctx(context)
        , methodHelper(std::make_unique<MethodCompilerHelper>(context))
        , paramValidator(std::make_unique<ParameterValidator>(context))
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

            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint32_t>(nameIndex), node);
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

    void ClassCompiler::emitNewObjectBytecode(ast::NewNode* node, const std::string& fullClassName)
    {
        const auto& arguments = node->getArguments();

        // Push constructor arguments onto stack (left to right)
        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor);
        }

        // Store the FULL class name including generics (e.g., "Box<Int>")
        size_t classNameIndex = ctx.program.getConstantPool().addString(fullClassName);

        // Emit NEW_OBJECT instruction with full class name and argument count
        ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                         static_cast<uint32_t>(classNameIndex),
                         static_cast<uint32_t>(arguments.size()), node);
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
            std::unordered_map<std::string, std::string> genericTypeBindings;
            const auto& genericParams = classDef->getGenericParameters();
            for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i)
            {
                genericTypeBindings[genericParams[i].name] = typeArguments[i];
            }

            // Find matching constructor and validate parameter types
            const auto& constructors = classDef->getConstructors();
            for (const auto& constructor : constructors)
            {
                if (constructor->getParameterCount() == arguments.size())
                {
                    paramValidator->validateConstructorParameters(arguments, constructor.get(), node->getLocation(), genericTypeBindings);
                    break;
                }
            }
        }

        // Emit bytecode for object creation
        emitNewObjectBytecode(node, fullClassName);

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMemberAccess(ast::MemberAccessNode* node)
    {
        std::string memberName = node->getMemberName();
        bool isStaticAccess = node->getIsStaticAccess();

        if (isStaticAccess)
        {
            // Static field access: ClassName::fieldName
            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint32_t>(fieldNameIndex), node);
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
                ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_GET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
            }
            else
            {
                // Regular instance field access: object.fieldName
                // First, compile the object expression
                node->getObject()->accept(ctx.visitor); // Will need delegation

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
            ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_SET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
        }
        else
        {
            // Regular member assignment: object.field = value
            // Compile the object expression
            node->getObject()->accept(ctx.visitor); // Will need delegation

            // Compile the value to assign
            node->getValue()->accept(ctx.visitor); // Will need delegation

            // Emit SET_FIELD instruction (object and value are on stack)
            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
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

            // Push all arguments onto stack
            for (const auto& arg : arguments)
            {
                arg->accept(ctx.visitor); // Will need delegation
            }

            // Emit CALL_STATIC instruction with fully qualified name
            size_t methodNameIndex = ctx.program.getConstantPool().addString(qualifiedName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                             static_cast<uint32_t>(methodNameIndex),
                             static_cast<uint32_t>(arguments.size()), node);
        }
        else
        {
            // Instance method call: object.methodName(args)

            // Infer the class of the object for type checking
            std::string objectClassName = ctx.typeInference.inferExpressionClassName(node->getObject());

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
            std::string qualifiedMethodName = baseClassName.empty() ? methodName : (baseClassName + "::" + methodName);
            const auto* methodMetadata = ctx.program.getFunction(qualifiedMethodName);

            if (node->hasGenericTypeArguments())
            {
                const auto& methodTypeArgs = node->getGenericTypeArguments();

                // PHASE 4: Validate that method is generic (only if we have metadata)
                if (methodMetadata && methodMetadata->genericTypeParameters.empty())
                {
                    throw errors::TypeException(
                        "Method '" + qualifiedMethodName + "' is not generic but used with type arguments",
                        node->getLocation()
                    );
                }
                // If we don't have metadata, we can't validate - allow it through
                // This handles cases where type inference couldn't determine the class name

                if (methodMetadata && !methodMetadata->genericTypeParameters.empty())
                {
                    const auto& methodGenericParams = methodMetadata->genericTypeParameters;

                    // Validate type argument count matches parameter count
                    if (methodTypeArgs.size() != methodGenericParams.size())
                    {
                        throw errors::TypeException(
                            "Method '" + methodName + "' expects " +
                            std::to_string(methodGenericParams.size()) +
                            " type arguments, but got " +
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

                    // Create bindings: T -> String, U -> Int, etc.
                    for (size_t i = 0; i < methodGenericParams.size(); ++i)
                    {
                        methodGenericBindings[methodGenericParams[i]] = methodTypeArgs[i];
                    }

                    // Push method-level bindings onto stack (will shadow class-level if names conflict)
                    ctx.pushGenericTypeBindings(methodGenericBindings);
                    hasMethodGenericBindings = true;
                }
            }
            else
            {
                // Validate: If method is generic, type arguments are required
                if (methodMetadata && !methodMetadata->genericTypeParameters.empty())
                {
                    throw errors::TypeException(
                        "Generic method '" + methodName + "' requires explicit type arguments. " +
                        "Use '" + methodName + "<" +
                        std::string(methodMetadata->genericTypeParameters.size() > 1 ? "T, U, ..." : "T") +
                        ">(...)'",
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
                std::string qualifiedName = objectClassName + "::" + methodName;
                paramValidator->validateMethodParameters(methodName, qualifiedName, arguments, node->getLocation());
            }

            // First, compile the object expression
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
                else if (!interfaceParameterTypes.empty() && i < interfaceParameterTypes.size())
                {
                    // Use interface parameter types
                    expectedType = interfaceParameterTypes[i];
                    hasExpectedType = true;
                }

                if (hasExpectedType)
                {
                    // Resolve generic type parameters using the bindings
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
                            // Auto-box: compile value, then emit NEW_OBJECT
                            arguments[i]->accept(ctx.visitor);
                            size_t classNameIndex = ctx.program.getConstantPool().addString(boxClassName);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                                        static_cast<uint32_t>(classNameIndex),
                                                        1u, // 1 constructor argument
                                                        arguments[i].get());
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
                // Emit standard CALL_METHOD instruction with method name
                size_t methodNameIndex = ctx.program.getConstantPool().addString(methodName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                 static_cast<uint32_t>(methodNameIndex),
                                 static_cast<uint32_t>(arguments.size()), node);
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
                         static_cast<uint32_t>(classNameIndex),
                         static_cast<uint32_t>(arguments.size()), node);

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
                         static_cast<uint32_t>(classNameIndex),
                         static_cast<uint32_t>(arguments.size()), node);

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
                         std::vector<uint32_t>{
                             static_cast<uint32_t>(classNameIndex),
                             static_cast<uint32_t>(methodNameIndex),
                             static_cast<uint32_t>(arguments.size())
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
                         std::vector<uint32_t>{
                             static_cast<uint32_t>(classNameIndex),
                             static_cast<uint32_t>(memberNameIndex)
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
                         std::vector<uint32_t>{
                             static_cast<uint32_t>(classNameIndex),
                             static_cast<uint32_t>(memberNameIndex)
                         }, node);

        return std::monostate{};
    }
}
