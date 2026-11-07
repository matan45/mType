#pragma once

#include "TypeGraphFacts.hpp"
#include "../../../mType/ast/ASTNode.hpp"
#include "../../../mType/token/Token.hpp"

#include <string>
#include <vector>

namespace mtype::lsp::analysis
{
    /**
     * MYT-432 — turns one parsed file into type-graph facts: every top-level
     * class and interface, with its supertype edges and its members.
     *
     * Pure: no I/O and no environment. `content` and `tokens` must be the
     * source `program` was parsed from, because the AST records only start
     * positions and the ranges are recovered from the tokens.
     *
     * - Edge names have their generic arguments stripped
     *   (`Comparable<Dog>` → `Comparable`), and an import alias
     *   (`import {Comparable as Cmp}`) is mapped back to the type it names.
     * - An edge's `uriHint` is the file its supertype resolves to: this file
     *   for a local declaration, or the target of a relative selective import.
     *   Anything else is left empty and matched by name.
     * - Nodes without a source position in this file were generated (Lombok).
     *   Generated members are kept but flagged `isSynthetic` and given the
     *   owner's name range. Generated types (`@Builder` companions) are
     *   skipped.
     */
    class TypeGraphExtractor
    {
    public:
        static std::vector<TypeFacts> extract(const ast::ASTNode* program,
                                              const std::string& content,
                                              const std::vector<token::Token>& tokens,
                                              const std::string& fileUri);
    };
}
