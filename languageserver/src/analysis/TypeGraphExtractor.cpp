#include "TypeGraphExtractor.hpp"

#include "../utils/DeclarationRanges.hpp"
#include "../utils/UriUtils.hpp"

#include "../../../mType/ast/GenericType.hpp"
#include "../../../mType/ast/GenericTypeParameter.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/FieldNode.hpp"
#include "../../../mType/ast/nodes/classes/InterfaceNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionNode.hpp"
#include "../../../mType/ast/nodes/statements/ImportNode.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include "../../../mType/types/TypeConversionUtils.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mtype::lsp::analysis
{
    namespace
    {
        namespace classes = ::ast::nodes::classes;
        using ::ast::nodes::functions::FunctionNode;
        using ::ast::nodes::statements::ImportNode;
        using ::ast::nodes::statements::ImportType;
        using ::ast::nodes::statements::ProgramNode;
        using GenericParams = std::vector<std::pair<std::string, std::shared_ptr<::ast::GenericType>>>;

        // What the file's imports say about the type names it uses.
        struct ImportScope
        {
            std::unordered_map<std::string, std::string> originalOf; // alias → original name
            std::unordered_map<std::string, std::string> hintOf;     // original name → file URI
        };

        // Everything one file's extraction needs, built once per extract().
        struct FileContext
        {
            const std::string& content;
            const std::vector<token::Token>& tokens;
            const std::string& fileUri;
            ImportScope imports;
            std::unordered_set<std::string> localTypes; // types declared in this file
        };

        // Generated nodes (Lombok) carry the default SourceLocation, not one
        // in this file. A token scan from it would land on the first call
        // site, so they never get ranges of their own.
        bool isFromSource(const ::ast::ASTNode& node, const std::string& fileUri)
        {
            return node.getLocation().getFilename() == fileUri;
        }

        bool isRelativeImport(const std::string& path)
        {
            return path.rfind("./", 0) == 0 || path.rfind("../", 0) == 0;
        }

        // `import {A} from "./a.mt"` names a file relative to the importing
        // one. Resolved lexically, with no I/O. Other spellings (`@alias/...`,
        // bare search-path names, libraries) yield "" and fall back to name
        // matching.
        std::string resolveImportUri(const std::string& fileUri, const std::string& importPath)
        {
            if (!isRelativeImport(importPath)) return {};
            const std::filesystem::path importing(UriUtils::uriToFilePath(fileUri));
            const auto target = (importing.parent_path() / importPath).lexically_normal();
            return UriUtils::filePathToUri(target.string());
        }

        ImportScope collectImports(const ProgramNode& program, const std::string& fileUri)
        {
            ImportScope scope;
            for (const auto& stmt : program.getStatements())
            {
                const auto* importNode = dynamic_cast<const ImportNode*>(stmt.get());
                if (!importNode) continue;
                for (const auto& [original, alias] : importNode->getSymbolAliases())
                {
                    scope.originalOf[alias] = original;
                }
                if (importNode->getImportType() != ImportType::SELECTIVE) continue;
                const std::string uri = resolveImportUri(fileUri, importNode->getFilePath());
                if (uri.empty()) continue;
                for (const auto& symbol : importNode->getImportedSymbols())
                {
                    scope.hintOf[symbol] = uri; // imported symbols are original names
                }
            }
            return scope;
        }

        std::unordered_set<std::string> collectLocalTypes(const ProgramNode& program,
                                                          const std::string& fileUri)
        {
            std::unordered_set<std::string> names;
            for (const auto& stmt : program.getStatements())
            {
                if (!stmt || !isFromSource(*stmt, fileUri)) continue;
                if (const auto* cls = dynamic_cast<const classes::ClassNode*>(stmt.get()))
                {
                    names.insert(cls->getClassName());
                }
                else if (const auto* iface = dynamic_cast<const classes::InterfaceNode*>(stmt.get()))
                {
                    names.insert(iface->getName());
                }
            }
            return names;
        }

        SupertypeEdge makeEdge(const std::string& spelling, SupertypeRelation relation,
                               const FileContext& ctx)
        {
            SupertypeEdge edge;
            edge.spelling = spelling;
            edge.relation = relation;
            edge.name = ::types::TypeConversionUtils::extractBaseTypeName(spelling);
            if (const auto alias = ctx.imports.originalOf.find(edge.name);
                alias != ctx.imports.originalOf.end())
            {
                edge.name = alias->second;
            }
            if (ctx.localTypes.count(edge.name))
            {
                edge.uriHint = ctx.fileUri;
            }
            else if (const auto hint = ctx.imports.hintOf.find(edge.name);
                     hint != ctx.imports.hintOf.end())
            {
                edge.uriHint = hint->second;
            }
            return edge;
        }

        // Name and declaration ranges with the outline's semantics.
        void fillRanges(const ::ast::ASTNode& node, const std::string& name,
                        const FileContext& ctx, Range& nameOut, Range& declOut)
        {
            const auto& loc = node.getLocation();
            nameOut = utils::nameRange(ctx.content, ctx.tokens, loc, name);
            const Range fallback = utils::singleLineFallback(ctx.content, nameOut);
            declOut = utils::declRange(ctx.content, ctx.tokens, loc, fallback);
        }

        // Spaces removed so "Map<K, V>" and "Map<K,V>" compare equal, the
        // same normalization overloadMatchesSignature applies.
        std::string typeSpelling(const std::shared_ptr<::ast::GenericType>& type)
        {
            if (!type) return {};
            std::string text = type->toString();
            text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
            return text;
        }

        std::vector<std::string> paramSpellings(const GenericParams& params)
        {
            std::vector<std::string> out;
            out.reserve(params.size());
            for (const auto& param : params) out.push_back(typeSpelling(param.second));
            return out;
        }

        TypeMember memberBase(const TypeDecl& owner, const ::ast::ASTNode& node,
                              const std::string& name, TypeMemberKind kind,
                              const FileContext& ctx)
        {
            TypeMember m;
            m.ownerName = owner.name;
            m.ownerUri = owner.fileUri;
            m.name = name;
            m.kind = kind;
            if (isFromSource(node, ctx.fileUri))
            {
                fillRanges(node, name, ctx, m.nameRange, m.declRange);
            }
            else
            {
                m.isSynthetic = true;
                m.nameRange = owner.nameRange;
                m.declRange = owner.nameRange;
            }
            return m;
        }

        TypeMember fieldMember(const TypeDecl& owner, const classes::FieldNode& field,
                               const FileContext& ctx)
        {
            TypeMember m = memberBase(owner, field, field.getName(), TypeMemberKind::Field, ctx);
            m.type = typeSpelling(field.getGenericType());
            m.access = field.getAccessModifier();
            m.isStatic = field.getIsStatic();
            m.isFinal = field.getIsFinal();
            return m;
        }

        TypeMember methodMember(const TypeDecl& owner, const classes::MethodNode& method,
                                const FileContext& ctx)
        {
            TypeMember m = memberBase(owner, method, method.getName(), TypeMemberKind::Method, ctx);
            m.arity = method.getParameterCount();
            m.paramTypes = paramSpellings(method.getGenericParameters());
            m.type = typeSpelling(method.getGenericReturnType());
            m.access = method.getAccessModifier();
            m.isStatic = method.getIsStatic();
            m.isAbstract = method.isAbstract();
            m.isFinal = method.isFinal();
            m.isSynthetic = m.isSynthetic || method.isSynthetic();
            return m;
        }

        TypeMember signatureMember(const TypeDecl& owner, const FunctionNode& fn,
                                   const FileContext& ctx)
        {
            TypeMember m = memberBase(owner, fn, fn.getName(), TypeMemberKind::Method, ctx);
            m.arity = fn.getParameterCount();
            m.paramTypes = paramSpellings(fn.getGenericParameters());
            m.type = typeSpelling(fn.getGenericReturnType());
            m.access = ::ast::AccessModifier::PUBLIC; // interface members are public
            m.isAbstract = true;
            return m;
        }

        void addClassMembers(TypeFacts& facts, const classes::ClassNode& cls,
                             const FileContext& ctx)
        {
            for (const auto& node : cls.getFields())
            {
                if (const auto* field = dynamic_cast<const classes::FieldNode*>(node.get()))
                {
                    facts.members.push_back(fieldMember(facts.decl, *field, ctx));
                }
            }
            for (const auto& node : cls.getMethods())
            {
                if (const auto* method = dynamic_cast<const classes::MethodNode*>(node.get()))
                {
                    facts.members.push_back(methodMember(facts.decl, *method, ctx));
                }
            }
        }

        void addInterfaceMembers(TypeFacts& facts, const classes::InterfaceNode& iface,
                                 const FileContext& ctx)
        {
            for (const auto& node : iface.getMethods())
            {
                if (const auto* fn = dynamic_cast<const FunctionNode*>(node.get()))
                {
                    facts.members.push_back(signatureMember(facts.decl, *fn, ctx));
                }
                else if (const auto* method = dynamic_cast<const classes::MethodNode*>(node.get()))
                {
                    // Defensive, as in DocumentSymbolHandler: some interface
                    // parsers may emit MethodNodes.
                    TypeMember m = methodMember(facts.decl, *method, ctx);
                    m.isAbstract = true;
                    facts.members.push_back(std::move(m));
                }
            }
        }

        std::vector<std::string> genericNames(const std::vector<::ast::GenericTypeParameter>& params)
        {
            std::vector<std::string> names;
            names.reserve(params.size());
            for (const auto& param : params) names.push_back(param.name);
            return names;
        }

        TypeDecl declBase(const ::ast::ASTNode& node, const std::string& name,
                          TypeDeclKind kind, const FileContext& ctx)
        {
            TypeDecl decl;
            decl.name = name;
            decl.fileUri = ctx.fileUri;
            decl.kind = kind;
            fillRanges(node, name, ctx, decl.nameRange, decl.declRange);
            return decl;
        }

        TypeFacts classFacts(const classes::ClassNode& cls, const FileContext& ctx)
        {
            TypeFacts facts;
            facts.decl = declBase(cls, cls.getClassName(), TypeDeclKind::Class, ctx);
            facts.decl.genericParams = genericNames(cls.getGenericParameters());
            facts.decl.isAbstract = cls.isAbstract();
            facts.decl.isFinal = cls.isFinal();
            facts.decl.isValueClass = cls.isValueClass();
            if (cls.hasParentClass())
            {
                facts.decl.supertypes.push_back(
                    makeEdge(cls.getParentClassName(), SupertypeRelation::Extends, ctx));
            }
            for (const auto& iface : cls.getImplementedInterfaces())
            {
                facts.decl.supertypes.push_back(makeEdge(iface, SupertypeRelation::Implements, ctx));
            }
            addClassMembers(facts, cls, ctx);
            return facts;
        }

        TypeFacts interfaceFacts(const classes::InterfaceNode& iface, const FileContext& ctx)
        {
            TypeFacts facts;
            facts.decl = declBase(iface, iface.getName(), TypeDeclKind::Interface, ctx);
            facts.decl.genericParams = genericNames(iface.getGenericParameters());
            facts.decl.isFinal = iface.isFinal();
            for (const auto& parent : iface.getExtendedInterfaces())
            {
                facts.decl.supertypes.push_back(makeEdge(parent, SupertypeRelation::Extends, ctx));
            }
            addInterfaceMembers(facts, iface, ctx);
            return facts;
        }
    } // namespace

    std::vector<TypeFacts> TypeGraphExtractor::extract(const ast::ASTNode* program,
                                                       const std::string& content,
                                                       const std::vector<token::Token>& tokens,
                                                       const std::string& fileUri)
    {
        std::vector<TypeFacts> out;
        const auto* prog = dynamic_cast<const ProgramNode*>(program);
        if (!prog) return out;

        const FileContext ctx{content, tokens, fileUri,
                              collectImports(*prog, fileUri),
                              collectLocalTypes(*prog, fileUri)};
        for (const auto& stmt : prog->getStatements())
        {
            const ::ast::ASTNode* node = stmt.get();
            if (!node || !isFromSource(*node, fileUri)) continue; // e.g. a @Builder companion
            if (const auto* cls = dynamic_cast<const classes::ClassNode*>(node))
            {
                out.push_back(classFacts(*cls, ctx));
            }
            else if (const auto* iface = dynamic_cast<const classes::InterfaceNode*>(node))
            {
                out.push_back(interfaceFacts(*iface, ctx));
            }
        }
        return out;
    }
}
