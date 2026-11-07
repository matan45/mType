#pragma once

#include "TypeGraphFacts.hpp"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mtype::lsp::analysis
{
    /**
     * MYT-432 — workspace-wide supertype/subtype graph plus member
     * locations, so implementation and type-hierarchy queries work across
     * open and closed files. WorkspaceSymbolIndex feeds it from its one
     * parse per file (`replaceFile` / `removeFile` with the canonical file
     * key); this class only stores and answers.
     *
     * Types are keyed by simple name, and the same name may be declared in
     * several files. A query's optional `fileUri` selects one declaration.
     * An empty `fileUri` means every declaration of the name. Edges carry
     * a `uriHint`, the declaring file their source resolves to, so a scoped
     * query skips same-named types in unrelated files. An edge whose hint
     * the graph can't confirm is matched by name, so results are never
     * short. Subtype queries work even when the parent itself isn't indexed
     * (e.g. a library interface outside the workspace).
     *
     * Only explicit edges are stored. The implicit `Object` root is left to
     * consumers.
     *
     * Thread-safe: every method locks, and results are copies sorted by
     * (name, fileUri). Writers run on the index's threads.
     */
    class WorkspaceTypeGraph
    {
    public:
        // Replaces everything `fileUri` contributed. Empty `types` removes it.
        void replaceFile(const std::string& fileUri, std::vector<TypeFacts> types);
        void removeFile(const std::string& fileUri);

        // Every declaration of `name`.
        std::vector<TypeDecl> findTypes(const std::string& name) const;

        // The declarations `name`'s extends/implements edges point at. A
        // parent that isn't indexed is still visible as a raw edge in
        // `findTypes(name)[i].supertypes`.
        std::vector<TypeDecl> directSupertypes(const std::string& name,
                                               const std::string& fileUri = {}) const;

        std::vector<TypeDecl> directSubtypes(const std::string& name,
                                             const std::string& fileUri = {}) const;

        // Breadth-first over subtype edges. Each type appears once, the
        // starting declarations are never included, and cycles (which can
        // only span files) terminate.
        std::vector<TypeDecl> transitiveSubtypes(const std::string& name,
                                                 const std::string& fileUri = {}) const;

        // Fields, methods and interface signatures, declarations ordered by
        // file, members in extraction order (fields, then methods).
        std::vector<TypeMember> membersOf(const std::string& typeName,
                                          const std::string& fileUri = {}) const;

    private:
        // A subtype edge, seen from its parent's side.
        struct SubtypeLink
        {
            std::string childName;
            std::string childUri;
            std::string uriHint;
        };

        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::vector<TypeFacts>> byFile_;           // uri → its types
        std::unordered_map<std::string, std::vector<std::string>> declaringFiles_; // name → uris
        std::unordered_map<std::string, std::vector<SubtypeLink>> linksBySuper_;   // parent name → links

        // All *Locked helpers expect mutex_ to be held.
        void removeFileLocked(const std::string& fileUri);
        const TypeFacts* findLocked(const std::string& name, const std::string& fileUri) const;
        std::vector<const TypeFacts*> declarationsLocked(const std::string& name,
                                                         const std::string& fileUri) const;
        std::vector<const TypeFacts*> edgeTargetsLocked(const SupertypeEdge& edge) const;
        bool linkReachesLocked(const SubtypeLink& link, const std::string& superName,
                               const std::string& superUri) const;
        std::vector<const TypeFacts*> childrenLocked(const std::string& name,
                                                     const std::string& fileUri) const;
    };
}
