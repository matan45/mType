#include "BytecodeCompiler.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../ast/nodes/annotations/AnnotationDeclarationNode.hpp"
#include "../../services/ImportManager.hpp"
#include "../../environment/registry/ExportRegistry.hpp"
#include "../../environment/registry/AnnotationDefinition.hpp"
#include "../../environment/registry/AnnotationParamSchema.hpp"
#include "../../environment/registry/AnnotationRegistry.hpp"
#include "../../errors/TypeException.hpp"
#include <stdexcept>

namespace vm::compiler
{
    value::Value BytecodeCompiler::visitImportNode(ast::ImportNode* node)
    {
        // Library imports are resolved by LibraryLinker at project build time, not here
        if (node->isLibraryImport()) {
            return std::monostate{};
        }

        // Handle imports at compile time
        std::string filePath = node->getFilePath();

        // Get ImportManager from environment
        auto importManager = environment->getImportManager();
        if (!importManager) {
            throw std::runtime_error("Import manager not available for compilation");
        }

        // Save current file path for proper restoration
        std::string savedCurrentFile = importManager->getCurrentFilePath();

        // Resolve the import path (relative to current file)
        std::string resolvedPath = importManager->resolvePath(filePath);

        // Skip re-compilation if already compiled
        if (compiledImports.find(resolvedPath) != compiledImports.end()) {
            return std::monostate{};
        }

        compiledImports.insert(resolvedPath);

        try {
            // Parse and get the AST for the imported file BEFORE setting current file
            // so parseAndCacheAST uses the correct context.
            auto* importedAST = importManager->parseAndCacheAST(filePath);
            if (!importedAST) {
                throw std::runtime_error("Failed to parse import: " + filePath);
            }

            // Set current file to the resolved path AFTER parsing so nested
            // imports in the imported file resolve relative to its directory.
            importManager->setCurrentFilePath(resolvedPath);

            // IMPORTANT: Process nested imports FIRST so all dependencies are
            // loaded before class registration.
            importHelper.processNestedImports(importedAST, [this](ast::ImportNode* node) {
                visitImportNode(node);
            });

            // Validate selective imports - check that imported symbols are public
            if (node->isSelective()) {
                auto exportRegistry = environment->getExportRegistry();
                if (exportRegistry) {
                    // Collect exported symbols from the imported file
                    importHelper.collectExportedSymbols(importedAST, resolvedPath, exportRegistry);

                    // Validate each requested symbol
                    const auto& requestedSymbols = node->getImportedSymbols();
                    for (const auto& symbolName : requestedSymbols) {
                        if (!exportRegistry->symbolExists(resolvedPath, symbolName)) {
                            throw errors::TypeException(
                                "Cannot import '" + symbolName + "' from '" + filePath + "': " +
                                "Symbol not found",
                                node->getLocation()
                            );
                        }

                        if (!exportRegistry->isSymbolExported(resolvedPath, symbolName)) {
                            throw errors::TypeException(
                                "Cannot import '" + symbolName + "' from '" + filePath + "': " +
                                "Symbol is private and not exported. Only public symbols can be imported.",
                                node->getLocation()
                            );
                        }
                    }
                }
            }

            // IMPORTANT: Register classes and interfaces AFTER nested imports
            // so parent classes from nested imports are available.
            registerClassesForBytecode(importedAST);
            linkParentClasses(importedAST);

            // Validate @Throw annotations now that all classes are registered
            functionRegistrar.validateThrowAnnotations(importedAST);

            // Compile the imported file to generate bytecode for functions and methods.
            // While compiling, top-level decls must stay globals (other modules may import
            // their PUBLIC names); the guard flag gates the promotion path.
            {
                visitors::ImportedFileContextGuard guard(context, true);
                importedAST->accept(*this);
            }

            // Register bytecode function aliases for constructors/methods.
            // Must happen AFTER compilation (importedAST->accept) so constructors exist.
            if ((node->isSelective() || node->isLibrarySelective())
                && !node->getSymbolAliases().empty())
            {
                for (const auto& [original, alias] : node->getSymbolAliases()) {
                    std::string origPrefix = original + "::";
                    std::string aliasPrefix = alias + "::";
                    // Collect names first to avoid modifying map during iteration
                    std::vector<std::pair<std::string, bytecode::BytecodeProgram::FunctionMetadata>> toAdd;
                    for (const auto& [funcName, funcMeta] : program.getFunctions()) {
                        if (funcName.size() > origPrefix.size() &&
                            funcName.substr(0, origPrefix.size()) == origPrefix) {
                            std::string aliasedName = aliasPrefix + funcName.substr(origPrefix.size());
                            if (!program.getFunction(aliasedName)) {
                                toAdd.emplace_back(aliasedName, funcMeta);
                            }
                        }
                    }
                    for (auto& [name, meta] : toAdd) {
                        program.registerFunction(name, meta);
                    }
                }
            }

            // Global variables from imported files are registered in globalRegistry
            // during compilation and persist across file boundaries
            // (see GlobalVariableRegistry::removeVariablesOutOfScope).

            importManager->setCurrentFilePath(savedCurrentFile);
        }
        catch (...) {
            importManager->setCurrentFilePath(savedCurrentFile);
            throw;
        }

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitAnnotationDeclarationNode(ast::AnnotationDeclarationNode* node)
    {
        using ast::nodes::annotations::AnnotationValueType;

        auto annotationRegistry = environment->getAnnotationRegistry();
        if (!annotationRegistry) return value::Value();

        // Don't shadow built-ins (Override/Script/EntryPoint/Throw) — they're
        // pre-registered at Environment::initialize() and re-defining them is
        // caught earlier by the duplicate-type-name check in the parser.
        if (annotationRegistry->hasAnnotation(node->getName())) return value::Value();

        // Register at runtime for the validation/reflection passes.
        auto def = std::make_shared<runtimeTypes::klass::AnnotationDefinition>(node->getName(), false);
        for (const auto& declParam : node->getParams())
        {
            runtimeTypes::klass::AnnotationParamSchema schema;
            schema.name          = declParam.name;
            schema.declaredType  = declParam.declaredType;
            schema.defaultValue  = declParam.defaultValue;
            schema.nullable      = declParam.nullable;
            schema.isArray       = declParam.isArray;
            def->addParam(std::move(schema));
        }
        for (const auto& meta : node->getMetaAnnotations())
        {
            if (meta) def->addMetaAnnotation(meta);
        }
        annotationRegistry->registerAnnotation(node->getName(), def);

        // Also serialize into the bytecode program (.mtc v5+ section) so the
        // declaration survives compile-to-file round-trips.
        bytecode::BytecodeProgram::AnnotationDeclData declData;
        declData.name = node->getName();
        for (const auto& declParam : node->getParams())
        {
            bytecode::BytecodeProgram::AnnotationParamSchemaData p;
            p.name         = declParam.name;
            p.declaredType = static_cast<uint8_t>(declParam.declaredType);
            p.nullable     = declParam.nullable;
            p.isArray      = declParam.isArray;
            p.hasDefault   = declParam.defaultValue.has_value();
            if (p.hasDefault)
            {
                const auto& dv = *declParam.defaultValue;
                switch (dv.getType())
                {
                case AnnotationValueType::INT:         p.defaultInt = dv.asInt(); break;
                case AnnotationValueType::FLOAT:       p.defaultFloat = dv.asFloat(); break;
                case AnnotationValueType::BOOL:        p.defaultBool = dv.asBool(); break;
                case AnnotationValueType::STRING:      p.defaultString = dv.asString(); break;
                case AnnotationValueType::CLASS_REF:   p.defaultString = dv.asClassRef(); break;
                case AnnotationValueType::CLASS_ARRAY: p.defaultStringArray = dv.asClassArray(); break;
                case AnnotationValueType::NULL_VALUE:  break;
                }
            }
            declData.params.push_back(std::move(p));
        }
        // MYT-109 (.mtc v6+): serialize meta-annotations applied to this
        // annotation declaration so `@Retention` / `@Target` / user-defined
        // meta-annotations survive a compile-to-file round-trip.
        for (const auto& meta : node->getMetaAnnotations())
        {
            if (!meta) continue;
            bytecode::BytecodeProgram::AnnotationData annot;
            annot.name = meta->getName();
            annot.location = meta->getLocation();
            for (const auto& key : meta->getKeyOrder())
            {
                if (const auto* v = meta->getTypedParameter(key))
                {
                    bytecode::BytecodeProgram::TypedAnnotationArg arg;
                    arg.key = key;
                    arg.valueType = static_cast<uint8_t>(v->getType());
                    switch (v->getType())
                    {
                    case AnnotationValueType::INT:         arg.intVal    = v->asInt(); break;
                    case AnnotationValueType::FLOAT:       arg.floatVal  = v->asFloat(); break;
                    case AnnotationValueType::BOOL:        arg.boolVal   = v->asBool(); break;
                    case AnnotationValueType::STRING:      arg.stringVal = v->asString(); break;
                    case AnnotationValueType::CLASS_REF:   arg.stringVal = v->asClassRef(); break;
                    case AnnotationValueType::CLASS_ARRAY: arg.arrayVal  = v->asClassArray(); break;
                    case AnnotationValueType::NULL_VALUE:  break;
                    }
                    annot.typedArguments.push_back(std::move(arg));
                }
            }
            declData.metaAnnotations.push_back(std::move(annot));
        }
        program.addAnnotationDeclaration(std::move(declData));
        return value::Value();
    }
}
