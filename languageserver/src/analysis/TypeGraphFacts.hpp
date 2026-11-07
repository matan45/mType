#pragma once

#include "../utils/LSPTypes.hpp"
#include "../../../mType/ast/AccessModifier.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace mtype::lsp::analysis
{
    /**
     * MYT-432 — the per-file facts the workspace type graph is built from.
     * Plain data: TypeGraphExtractor produces it from one parse, and
     * WorkspaceTypeGraph stores and queries it. Ranges are 0-based LSP
     * ranges with the document outline's semantics (utils/DeclarationRanges).
     */
    enum class TypeDeclKind
    {
        Class,
        Interface
    };

    enum class SupertypeRelation
    {
        Extends,   // class extends class, interface extends interface
        Implements // class implements interface
    };

    enum class TypeMemberKind
    {
        Field,
        Method // an interface signature is a Method with isAbstract set
    };

    struct SupertypeEdge
    {
        std::string name;     // generic args stripped, import alias mapped back to the original
        std::string spelling; // as written in the source, e.g. "Cmp<Dog>"
        SupertypeRelation relation = SupertypeRelation::Extends;
        std::string uriHint;  // canonical URI of the declaring file when known; "" = match by name
    };

    struct TypeDecl
    {
        std::string name;
        std::string fileUri;                    // canonical URI of the declaring file
        TypeDeclKind kind = TypeDeclKind::Class;
        std::vector<std::string> genericParams; // "T", "K", ...
        bool isAbstract = false;
        bool isFinal = false;
        bool isValueClass = false;
        Range nameRange{};
        Range declRange{};
        std::vector<SupertypeEdge> supertypes;  // extends first, then implements
    };

    struct TypeMember
    {
        std::string ownerName;
        std::string ownerUri;
        std::string name;
        TypeMemberKind kind = TypeMemberKind::Method;
        Range nameRange{};
        Range declRange{};
        std::size_t arity = 0;
        std::vector<std::string> paramTypes; // GenericType::toString() with spaces removed
        std::string type;                    // return type (method) or declared type (field)
        ast::AccessModifier access = ast::AccessModifier::PRIVATE;
        bool isStatic = false;
        bool isAbstract = false;
        bool isFinal = false;
        bool isSynthetic = false; // generated (Lombok): no source position, ranges = owner's name
    };

    struct TypeFacts
    {
        TypeDecl decl;
        std::vector<TypeMember> members; // fields, methods, interface signatures; no constructors
    };
}
