#include "ClassCompiler.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../bytecode/OpCode.hpp"
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

        // Check if there are generic type arguments
        size_t genericStart = fullClassName.find('<');
        if (genericStart == std::string::npos) {
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

        // Simple parsing - split by comma (doesn't handle nested generics)
        size_t start = 0;
        size_t comma = argsStr.find(',');

        while (comma != std::string::npos) {
            std::string typeArg = argsStr.substr(start, comma - start);
            // Trim whitespace
            typeArg.erase(0, typeArg.find_first_not_of(" \t"));
            typeArg.erase(typeArg.find_last_not_of(" \t") + 1);
            if (!typeArg.empty()) {
                typeArguments.push_back(typeArg);
            }
            start = comma + 1;
            comma = argsStr.find(',', start);
        }

        // Add the last type argument
        std::string lastTypeArg = argsStr.substr(start);
        lastTypeArg.erase(0, lastTypeArg.find_first_not_of(" \t"));
        lastTypeArg.erase(lastTypeArg.find_last_not_of(" \t") + 1);
        if (!lastTypeArg.empty()) {
            typeArguments.push_back(lastTypeArg);
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
        parseAndValidateGenericTypeArguments(fullClassName, node->getLocation());

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

            // Find matching constructor and validate parameter types
            const auto& constructors = classDef->getConstructors();
            for (const auto& constructor : constructors)
            {
                if (constructor->getParameterCount() == arguments.size())
                {
                    paramValidator->validateConstructorParameters(arguments, constructor.get(), node->getLocation());
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

            // Validate: Generic type arguments must be object types, not primitives
            if (node->hasGenericTypeArguments())
            {
                const auto& typeArgs = node->getGenericTypeArguments();
                for (const auto& typeArg : typeArgs)
                {
                    if (typeArg == "int" || typeArg == "float" || typeArg == "string" ||
                        typeArg == "bool" || typeArg == "void")
                    {
                        throw errors::TypeException(
                            "Generic type argument '" + typeArg + "' is a primitive type. " +
                            "Generics only support object types. Use wrapper classes like Int, Float, String, Bool instead.",
                            node->getLocation()
                        );
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

                    // Look up the base class to get its generic parameter names
                    auto classDef = ctx.environment->findClass(baseClassName);

                    if (classDef && classDef->isGeneric())
                    {
                        const auto& genericParams = classDef->getGenericParameters();

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
            std::string qualifiedMethodName = baseClassName + "::" + methodName;
            const auto* methodMetadata = ctx.program.getFunction(qualifiedMethodName);

            if (node->hasGenericTypeArguments())
            {
                if (methodMetadata && !methodMetadata->genericTypeParameters.empty())
                {
                    const auto& methodGenericParams = methodMetadata->genericTypeParameters;
                    const auto& methodTypeArgs = node->getGenericTypeArguments();

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
                                "Generics only support object types. Use wrapper classes like Int, Float, String, Bool instead.",
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

            // Pop generic type bindings after validation (method-level first, then class-level)
            if (hasMethodGenericBindings)
            {
                ctx.popGenericTypeBindings();
            }
            if (!genericBindings.empty())
            {
                ctx.popGenericTypeBindings();
            }

            // First, compile the object expression
            node->getObject()->accept(ctx.visitor); // Will need delegation

            // Push all arguments onto stack
            for (const auto& arg : arguments)
            {
                arg->accept(ctx.visitor); // Will need delegation
            }

            // Emit CALL_METHOD instruction with source location
            size_t methodNameIndex = ctx.program.getConstantPool().addString(methodName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                             static_cast<uint32_t>(methodNameIndex),
                             static_cast<uint32_t>(arguments.size()), node);
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
