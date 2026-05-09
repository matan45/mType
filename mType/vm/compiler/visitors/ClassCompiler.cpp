#include "ClassCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../analysis/DefiniteAssignmentAnalyzer.hpp"
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

        // Compile static field initializers into an explicit synthetic
        // function. Startup paths invoke these before user code runs.
        compileStaticInitializer(node);

        // Phase 2 (allocation perf): record the set of own-class instance
        // fields that every constructor definitely assigns before any read.
        // initializeObjectFields() uses this to skip redundant default-init
        // setField() calls. Intersection across all constructors — a field
        // is safe to skip only if every ctor writes it first.
        computeSkipDefaultInitFields(node);

        // Phase 3 (allocation perf): flag constructors whose body is strictly
        // `this.F_k = param_k`. The VM's createObject / invokeConstructor
        // fast-paths copy args straight into instance fields, skipping the
        // whole CallFrame push + ctor-bytecode interpret loop.
        markTrivialConstructors(node);

        // Restore previous class context
        ctx.currentClassNode = previousClassNode;

        return std::monostate{};
    }

    void ClassCompiler::compileStaticInitializer(ast::ClassNode* node)
    {
        bool hasStaticInitializers = false;
        for (const auto& fieldPtr : node->getFields())
        {
            auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get());
            if (fieldNode && fieldNode->getIsStatic() && fieldNode->hasInitialValue())
            {
                hasStaticInitializers = true;
                break;
            }
        }

        if (!hasStaticInitializers)
        {
            return;
        }

        const std::string className = node->getClassName();
        const std::string initializerName = className + "::<static_init>$static";

        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP, node);
        size_t initializerStart = ctx.program.getCurrentOffset();

        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = initializerName;
        tempMetadata.startOffset = initializerStart;
        tempMetadata.instructionCount = 0;
        tempMetadata.localCount = 0;
        tempMetadata.parameterCount = 0;
        tempMetadata.returnType = "void";
        tempMetadata.isStatic = true;
        tempMetadata.isNative = false;
        ctx.program.registerFunction(initializerName, tempMetadata);

        ctx.functionFrameManager.enterFunctionFrame(initializerName,
                                                    "void",
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false,
                                                    false);
        ctx.variableTracker.beginScope();

        const bool previousStaticInitializer = ctx.inStaticFieldInitializer;
        ctx.inStaticFieldInitializer = true;

        for (const auto& fieldPtr : node->getFields())
        {
            auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get());
            if (!fieldNode || !fieldNode->getIsStatic() || !fieldNode->hasInitialValue())
            {
                continue;
            }

            fieldNode->getInitialValue()->accept(ctx.visitor);

            std::string qualifiedName = className + "::" + fieldNode->getName();
            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC,
                                         static_cast<uint64_t>(nameIndex),
                                         fieldNode);
        }

        ctx.inStaticFieldInitializer = previousStaticInitializer;

        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);

        size_t localCount = ctx.functionFrameManager.getLocalCount();
        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        size_t initializerEnd = ctx.program.getCurrentOffset();
        ctx.emitter.patchJump(skipJump);

        size_t initializerNameIndex = ctx.program.getConstantPool().addString(initializerName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL,
                                     static_cast<uint64_t>(initializerNameIndex),
                                     0u,
                                     node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = initializerName;
        metadata.startOffset = initializerStart;
        metadata.instructionCount = initializerEnd - initializerStart;
        metadata.localCount = localCount;
        metadata.parameterCount = 0;
        metadata.returnType = "void";
        metadata.isStatic = true;
        metadata.isNative = false;

        if (const auto* existingMetadata = ctx.program.getFunction(initializerName))
        {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        ctx.program.registerFunction(initializerName, metadata);
        ctx.program.addStaticInitializerFunction(initializerName);
    }

    void ClassCompiler::markTrivialConstructors(ast::ClassNode* node)
    {
        auto classDef = ctx.env->findClass(node->getClassName());
        if (!classDef) return;

        const auto& ctorNodes = node->getConstructors();
        const auto& ctorDefs = classDef->getConstructors();

        // Order must line up — ClassRegistrar adds each ctor in AST order.
        // If they don't match (e.g., an implicit default ctor snuck in),
        // bail rather than guess.
        if (ctorNodes.size() != ctorDefs.size()) return;

        for (size_t i = 0; i < ctorNodes.size(); ++i)
        {
            auto* ctor = dynamic_cast<ast::nodes::classes::ConstructorNode*>(ctorNodes[i].get());
            if (!ctor) continue;
            if (ctor->hasSuperInitializer()) continue;

            // Build parameter name list in declaration order for the analyser.
            std::vector<std::string> paramNames;
            const auto& params = ctor->getParametersWithTypes();
            paramNames.reserve(params.size());
            for (const auto& p : params) paramNames.push_back(p.first);

            auto trivial = analysis::DefiniteAssignmentAnalyzer::analyzeTrivialCtor(
                ctor->getBodyPtr(), paramNames);
            if (!trivial.has_value()) continue;

            // Keep only assignments to this-class instance fields. Inherited
            // fields could legitimately live on the slow path; we don't want
            // to silently skip parent-side initialiser bytecode.
            const auto& ownInstanceFields = classDef->getInstanceFields();
            std::vector<std::pair<std::string, size_t>> filtered;
            std::vector<std::pair<size_t, size_t>> indexed;  // Phase 4: pre-resolved field indices
            filtered.reserve(trivial->size());
            indexed.reserve(trivial->size());
            bool allOwned = true;
            for (const auto& pr : *trivial)
            {
                if (ownInstanceFields.count(pr.first) == 0) { allOwned = false; break; }
                size_t fieldIdx = classDef->getFieldIndex(pr.first);
                if (fieldIdx == SIZE_MAX) { allOwned = false; break; }
                filtered.push_back(pr);
                indexed.emplace_back(fieldIdx, pr.second);
            }
            if (!allOwned) continue;

            // Final instance fields with inline initializers (e.g.,
            // `final int CHANNELS = 32`) are set by the constructor's
            // bytecode prologue. The trivial fast path skips the entire
            // constructor bytecode, so those initialisations would be
            // lost. Reject trivial classification when any final instance
            // field exists in this class or its hierarchy.
            bool hasFinalFields = false;
            for (const auto& [fname, fdef] : ownInstanceFields)
            {
                if (fdef->isFinal()) { hasFinalFields = true; break; }
            }
            if (!hasFinalFields)
            {
                // Also check parent classes — their final fields are
                // initialised in the parent constructor prologue that
                // the trivial fast path would skip.
                auto parent = classDef->getParentClass();
                while (parent && !hasFinalFields)
                {
                    for (const auto& [fname, fdef] : parent->getInstanceFields())
                    {
                        if (fdef->isFinal()) { hasFinalFields = true; break; }
                    }
                    parent = parent->getParentClass();
                }
            }
            if (hasFinalFields) continue;

            ctorDefs[i]->setTrivialFieldAssignments(std::move(filtered), std::move(indexed));
        }
    }

    void ClassCompiler::computeSkipDefaultInitFields(ast::ClassNode* node)
    {
        auto classDef = ctx.env->findClass(node->getClassName());
        if (!classDef) return;

        const auto& ctorNodes = node->getConstructors();
        if (ctorNodes.empty()) return;  // implicit default ctor touches nothing — leave empty

        std::unordered_set<std::string> intersection;
        bool first = true;

        for (const auto& ctorNode : ctorNodes)
        {
            auto* ctor = dynamic_cast<ast::nodes::classes::ConstructorNode*>(ctorNode.get());
            if (!ctor)
            {
                intersection.clear();
                break;
            }

            // A super() / this() call running before body statements could
            // read this.X via super's own constructor. The analyser rejects
            // any non-trivial shape, but being explicit here keeps future
            // readers honest: if there's any super-initialiser, bail.
            if (ctor->hasSuperInitializer())
            {
                intersection.clear();
                break;
            }

            auto assigned = analysis::DefiniteAssignmentAnalyzer::analyzeConstructor(ctor->getBodyPtr());
            if (assigned.empty())
            {
                intersection.clear();
                break;
            }

            if (first)
            {
                intersection = std::move(assigned);
                first = false;
            }
            else
            {
                std::unordered_set<std::string> next;
                for (const auto& name : intersection)
                {
                    if (assigned.count(name) > 0) next.insert(name);
                }
                intersection = std::move(next);
                if (intersection.empty()) break;
            }
        }

        // Only keep fields that belong to *this* class (not inherited). Parent
        // classes handle their own fields via their own initializeObjectFields
        // pass / their own skip set.
        std::unordered_set<std::string> ownFieldsOnly;
        const auto& ownInstanceFields = classDef->getInstanceFields();
        for (const auto& name : intersection)
        {
            if (ownInstanceFields.count(name) > 0) ownFieldsOnly.insert(name);
        }

        classDef->setSkipDefaultInitFields(std::move(ownFieldsOnly));
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
            auto classDef = ctx.env->findClass(baseClassName);
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
        auto classDef = ctx.env->findClass(baseClassName);
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
                auto boxClassDef = ctx.env->findClass(boxClassName);
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
        auto classDef = ctx.env->findClass(baseClassName);
        bool isValueClass = classDef && classDef->isValueClass();

        if (isValueClass) {
            // Emit NEW_VALUE_OBJECT + OBJECT_TO_VALUE for value classes
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(arguments.size()), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, node);
        } else {
            // MYT-134: escape analysis may have flagged this allocation as
            // non-escaping; emit NEW_STACK so the runtime can pool-borrow and
            // skip GC registration. NEW_STACK has the same operand shape so the
            // emission differs only in the opcode byte.
            bytecode::OpCode op = node->getIsStackAllocated()
                ? bytecode::OpCode::NEW_STACK
                : bytecode::OpCode::NEW_OBJECT;
            ctx.emitter.emitWithLocation(op,
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
        auto classDef = ctx.env->findClass(baseClassName);
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

                if (!nonNullReceiver)
                {
                    throw errors::TypeException(
                        "Cannot access field '" + memberName + "' on nullable receiver. "
                        "Add a null check (if (x != null) { ... }) or change the receiver's declared type to non-nullable.",
                        node->getLocation()
                    );
                }

                receiverNode->accept(ctx.visitor);

                // MYT-212: when the receiver is a variable with a declared
                // class type and that class declares (or inherits) the field,
                // emit the static-binding variant so a child shadowing the
                // field doesn't redirect the read to the most-derived slot.
                std::string staticReceiverClass;
                if (auto* varNode = dynamic_cast<ast::VariableNode*>(receiverNode))
                {
                    const std::string& varName = varNode->getName();
                    std::string localClass = ctx.variableTracker.getLocalClassNameByName(varName);
                    if (!localClass.empty())
                    {
                        staticReceiverClass = ctx.resolveGenericType(localClass);
                    }
                    else
                    {
                        // Fall back to global registry for non-local vars.
                        std::string globalClass = ctx.globalRegistry.getClassName(varName);
                        if (!globalClass.empty())
                        {
                            staticReceiverClass = ctx.resolveGenericType(globalClass);
                        }
                    }
                }

                bool emittedTyped = false;
                if (!staticReceiverClass.empty())
                {
                    auto classRegistry = ctx.env->getClassRegistry();
                    if (classRegistry)
                    {
                        // Strip any generic args before looking up.
                        std::string baseClass = staticReceiverClass;
                        size_t lt = baseClass.find('<');
                        if (lt != std::string::npos) baseClass = baseClass.substr(0, lt);

                        auto staticDef = classRegistry->findClass(baseClass);
                        if (staticDef && staticDef->getFieldOwnerInHierarchy(memberName, staticDef))
                        {
                            size_t classNameIndex = ctx.program.getConstantPool().addString(baseClass);
                            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD_TYPED,
                                std::vector<uint64_t>{
                                    static_cast<uint64_t>(classNameIndex),
                                    static_cast<uint64_t>(fieldNameIndex)
                                }, node);
                            ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
                            emittedTyped = true;
                        }
                    }
                }

                if (!emittedTyped)
                {
                    // Regular field access - emit GET_FIELD instruction
                    size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
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

            if (!nonNullReceiver)
            {
                throw errors::TypeException(
                    "Cannot assign field '" + memberName + "' on nullable receiver. "
                    "Add a null check (if (x != null) { ... }) or change the receiver's declared type to non-nullable.",
                    node->getLocation()
                );
            }

            receiverNode->accept(ctx.visitor);

            // Compile the value to assign
            node->getValue()->accept(ctx.visitor);

            // Emit SET_FIELD instruction (object and value are on stack)
            size_t fieldNameIndex = ctx.program.getConstantPool().addString(memberName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint64_t>(fieldNameIndex), node);
            ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
        }

        return std::monostate{};
    }

    value::Value ClassCompiler::compileMethodCall(ast::MethodCallNode* node)
    {
        if (node->getIsStaticCall())
        {
            compileStaticMethodCall(node);
        }
        else
        {
            compileInstanceMethodCall(node);
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
            auto classRegistry = ctx.env->getClassRegistry();
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
            auto classRegistry = ctx.env->getClassRegistry();
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
            auto classRegistry = ctx.env->getClassRegistry();
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
        auto boxClassDef = ctx.env->findClass(expectedType);
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
        return !ctx.typeInference.inferExpressionNullable(receiverNode);
    }
}

