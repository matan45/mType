#include "ClassCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../validation/CompileTimeValidator.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/AbstractClassException.hpp"
#include "../../../types/TypeSubstitutionService.hpp"

namespace vm::compiler::visitors
{
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
            return typeArguments;
        }

        size_t genericEnd = fullClassName.rfind('>');
        if (genericEnd == std::string::npos || genericEnd <= genericStart) {
            throw errors::TypeException(
                "Malformed generic type arguments in class name: " + fullClassName,
                location
            );
        }

        std::string argsStr = fullClassName.substr(genericStart + 1, genericEnd - genericStart - 1);

        // PHASE 4: diamond operator <> for type inference; types inferred from context.
        if (argsStr.empty() || (argsStr.find_first_not_of(" \t") == std::string::npos)) {
            return typeArguments;
        }

        // Depth-aware split — handles Container<List<String>, HashMap<Int, String>, HashSet<Int>>.
        size_t start = 0;
        size_t depth = 0;

        for (size_t i = 0; i < argsStr.length(); ++i) {
            if (argsStr[i] == '<') {
                depth++;
            } else if (argsStr[i] == '>') {
                depth--;
            } else if (argsStr[i] == ',' && depth == 0) {
                std::string typeArg = argsStr.substr(start, i - start);
                typeArg.erase(0, typeArg.find_first_not_of(" \t"));
                typeArg.erase(typeArg.find_last_not_of(" \t") + 1);
                if (!typeArg.empty()) {
                    typeArguments.push_back(typeArg);
                }
                start = i + 1;
            }
        }

        std::string lastTypeArg = argsStr.substr(start);
        lastTypeArg.erase(0, lastTypeArg.find_first_not_of(" \t"));
        lastTypeArg.erase(lastTypeArg.find_last_not_of(" \t") + 1);
        if (!lastTypeArg.empty()) {
            typeArguments.push_back(lastTypeArg);
        }

        // PHASE 4: Generics only support object types (Int/Float/Bool/String, not primitives).
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

        auto classDef = ctx.env->findClass(baseClassName);
        if (classDef) {
            const auto& genericParams = classDef->getGenericParameters();

            if (genericParams.empty()) {
                throw errors::TypeException(
                    "Class '" + baseClassName + "' is not generic but used with type arguments",
                    location
                );
            }

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

        ::types::TypeSubstitutionService service;

        // Push constructor arguments onto stack (left to right) with auto-boxing
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            bool needsAutoBoxing = false;
            std::string boxClassName;

            if (constructor && i < constructor->getParameterCount())
            {
                const auto& params = constructor->getParametersWithTypes();
                const auto& paramType = params[i].second;

                if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value())
                {
                    std::string expectedClass = paramType.className.value();

                    if (!genericTypeBindings.empty())
                    {
                        expectedClass = service.resolveGenericType(expectedClass, genericTypeBindings);
                    }

                    value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

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

            arguments[i]->accept(ctx.visitor);

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

        std::string baseClassName = fullClassName;
        size_t genStart = fullClassName.find('<');
        if (genStart != std::string::npos) {
            baseClassName = fullClassName.substr(0, genStart);
        }
        auto classDef = ctx.env->findClass(baseClassName);
        bool isValueClass = classDef && classDef->isValueClass();

        if (isValueClass) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                             static_cast<uint64_t>(classNameIndex),
                             static_cast<uint64_t>(arguments.size()), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, node);
        } else {
            // MYT-134: escape analysis may have flagged this allocation as
            // non-escaping; emit NEW_STACK so the runtime pool-borrows and
            // skips GC registration. NEW_STACK shares NEW_OBJECT's operand shape.
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

        std::string baseClassName = fullClassName;
        size_t genericStart = fullClassName.find('<');
        if (genericStart != std::string::npos)
        {
            baseClassName = fullClassName.substr(0, genericStart);
        }

        const auto& arguments = node->getArguments();
        if (ctx.compileTimeValidator)
        {
            ctx.compileTimeValidator->validateConstructorExists(baseClassName, arguments.size(), node->getLocation());
        }

        auto classDef = ctx.env->findClass(baseClassName);
        runtimeTypes::klass::ConstructorDefinition* matchingConstructor = nullptr;
        std::unordered_map<std::string, std::string> genericTypeBindings;

        if (classDef)
        {
            if (classDef->isAbstract()) {
                throw errors::AbstractClassException(
                    "Cannot instantiate abstract class '" + baseClassName + "'",
                    node->getLocation()
                );
            }

            // Build generic type bindings map (e.g., {T -> Int}) for parameter validation.
            const auto& genericParams = classDef->getGenericParameters();
            for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i)
            {
                genericTypeBindings[genericParams[i].name] = typeArguments[i];
            }

            // Try each ctor with matching parameter count and pick the first that validates.
            const auto& constructors = classDef->getConstructors();
            std::string lastError;

            for (const auto& constructor : constructors)
            {
                if (constructor->getParameterCount() == arguments.size())
                {
                    try {
                        paramValidator->validateConstructorParameters(arguments, constructor.get(), node->getLocation(), genericTypeBindings);
                        matchingConstructor = constructor.get();
                        break;
                    }
                    catch (const std::exception& e) {
                        lastError = e.what();
                        continue;
                    }
                }
            }

            if (!matchingConstructor && !lastError.empty()) {
                throw std::runtime_error(lastError);
            }
        }

        emitNewObjectBytecode(node, fullClassName, matchingConstructor, genericTypeBindings);

        return std::monostate{};
    }
}
