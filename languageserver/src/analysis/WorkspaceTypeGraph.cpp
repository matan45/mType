#include "WorkspaceTypeGraph.hpp"

#include "../utils/UriUtils.hpp"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace mtype::lsp::analysis
{
    namespace
    {
        // Query arguments may use any client spelling of a file URI.
        std::string scopeUri(const std::string& fileUri)
        {
            return fileUri.empty() ? std::string{} : UriUtils::canonicalFileUri(fileUri);
        }

        // Identity of one declaration: names are unique within a file.
        std::string nodeKey(const std::string& name, const std::string& uri)
        {
            return uri + '\n' + name;
        }

        std::vector<TypeDecl> sortedDecls(const std::vector<const TypeFacts*>& facts)
        {
            std::vector<TypeDecl> out;
            out.reserve(facts.size());
            for (const auto* f : facts) out.push_back(f->decl);
            std::sort(out.begin(), out.end(), [](const TypeDecl& a, const TypeDecl& b) {
                if (a.name != b.name) return a.name < b.name;
                return a.fileUri < b.fileUri;
            });
            return out;
        }

        // Removes the values matching `pred` under `key`, and the key once empty.
        template <typename Map, typename Pred>
        void eraseFromBucket(Map& map, const std::string& key, Pred pred)
        {
            auto it = map.find(key);
            if (it == map.end()) return;
            auto& bucket = it->second;
            bucket.erase(std::remove_if(bucket.begin(), bucket.end(), pred), bucket.end());
            if (bucket.empty()) map.erase(it);
        }
    } // namespace

    void WorkspaceTypeGraph::replaceFile(const std::string& fileUri, std::vector<TypeFacts> types)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        removeFileLocked(fileUri);
        if (types.empty()) return;

        for (const auto& facts : types)
        {
            declaringFiles_[facts.decl.name].push_back(fileUri);
            for (const auto& edge : facts.decl.supertypes)
            {
                linksBySuper_[edge.name].push_back({facts.decl.name, fileUri, edge.uriHint});
            }
        }
        byFile_[fileUri] = std::move(types);
    }

    void WorkspaceTypeGraph::removeFile(const std::string& fileUri)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        removeFileLocked(fileUri);
    }

    void WorkspaceTypeGraph::removeFileLocked(const std::string& fileUri)
    {
        auto it = byFile_.find(fileUri);
        if (it == byFile_.end()) return;

        for (const auto& facts : it->second)
        {
            eraseFromBucket(declaringFiles_, facts.decl.name,
                [&fileUri](const std::string& uri) { return uri == fileUri; });
            for (const auto& edge : facts.decl.supertypes)
            {
                eraseFromBucket(linksBySuper_, edge.name,
                    [&fileUri](const SubtypeLink& link) { return link.childUri == fileUri; });
            }
        }
        byFile_.erase(it);
    }

    std::vector<TypeDecl> WorkspaceTypeGraph::findTypes(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return sortedDecls(declarationsLocked(name, {}));
    }

    std::vector<TypeDecl> WorkspaceTypeGraph::directSupertypes(const std::string& name,
                                                               const std::string& fileUri) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<const TypeFacts*> parents;
        std::unordered_set<std::string> seen;
        for (const auto* facts : declarationsLocked(name, scopeUri(fileUri)))
        {
            for (const auto& edge : facts->decl.supertypes)
            {
                for (const auto* parent : edgeTargetsLocked(edge))
                {
                    if (seen.insert(nodeKey(parent->decl.name, parent->decl.fileUri)).second)
                    {
                        parents.push_back(parent);
                    }
                }
            }
        }
        return sortedDecls(parents);
    }

    std::vector<TypeDecl> WorkspaceTypeGraph::directSubtypes(const std::string& name,
                                                             const std::string& fileUri) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return sortedDecls(childrenLocked(name, scopeUri(fileUri)));
    }

    std::vector<TypeDecl> WorkspaceTypeGraph::transitiveSubtypes(const std::string& name,
                                                                 const std::string& fileUri) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const std::string rootUri = scopeUri(fileUri);

        // The starting declarations are never results, so a cycle that
        // leads back to them stops there.
        std::unordered_set<std::string> visited;
        for (const auto* root : declarationsLocked(name, rootUri))
        {
            visited.insert(nodeKey(root->decl.name, root->decl.fileUri));
        }

        std::vector<const TypeFacts*> found;
        std::vector<const TypeFacts*> frontier = childrenLocked(name, rootUri);
        while (!frontier.empty())
        {
            std::vector<const TypeFacts*> next;
            for (const auto* facts : frontier)
            {
                if (!visited.insert(nodeKey(facts->decl.name, facts->decl.fileUri)).second) continue;
                found.push_back(facts);
                auto children = childrenLocked(facts->decl.name, facts->decl.fileUri);
                next.insert(next.end(), children.begin(), children.end());
            }
            frontier = std::move(next);
        }
        return sortedDecls(found);
    }

    std::vector<TypeMember> WorkspaceTypeGraph::membersOf(const std::string& typeName,
                                                          const std::string& fileUri) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto decls = declarationsLocked(typeName, scopeUri(fileUri));
        std::sort(decls.begin(), decls.end(), [](const TypeFacts* a, const TypeFacts* b) {
            return a->decl.fileUri < b->decl.fileUri;
        });

        std::vector<TypeMember> out;
        for (const auto* facts : decls)
        {
            out.insert(out.end(), facts->members.begin(), facts->members.end());
        }
        return out;
    }

    const TypeFacts* WorkspaceTypeGraph::findLocked(const std::string& name,
                                                    const std::string& fileUri) const
    {
        auto it = byFile_.find(fileUri);
        if (it == byFile_.end()) return nullptr;
        for (const auto& facts : it->second)
        {
            if (facts.decl.name == name) return &facts;
        }
        return nullptr;
    }

    std::vector<const TypeFacts*> WorkspaceTypeGraph::declarationsLocked(
        const std::string& name, const std::string& fileUri) const
    {
        std::vector<const TypeFacts*> out;
        if (!fileUri.empty())
        {
            if (const auto* facts = findLocked(name, fileUri)) out.push_back(facts);
            return out;
        }
        auto it = declaringFiles_.find(name);
        if (it == declaringFiles_.end()) return out;
        for (const auto& uri : it->second)
        {
            if (const auto* facts = findLocked(name, uri)) out.push_back(facts);
        }
        return out;
    }

    // The hinted file's declaration when it has one, otherwise every
    // declaration of the name.
    std::vector<const TypeFacts*> WorkspaceTypeGraph::edgeTargetsLocked(const SupertypeEdge& edge) const
    {
        if (!edge.uriHint.empty())
        {
            if (const auto* hinted = findLocked(edge.name, edge.uriHint)) return {hinted};
        }
        return declarationsLocked(edge.name, {});
    }

    // Does `link`, an edge pointing at `superName`, point at the declaration
    // in `superUri`? Only a hint naming a different file that declares
    // `superName` rules it out. A hint the graph can't confirm never hides a
    // real subtype.
    bool WorkspaceTypeGraph::linkReachesLocked(const SubtypeLink& link,
                                               const std::string& superName,
                                               const std::string& superUri) const
    {
        if (superUri.empty() || link.uriHint.empty() || link.uriHint == superUri) return true;
        return findLocked(superName, link.uriHint) == nullptr;
    }

    std::vector<const TypeFacts*> WorkspaceTypeGraph::childrenLocked(const std::string& name,
                                                                     const std::string& fileUri) const
    {
        std::vector<const TypeFacts*> out;
        auto it = linksBySuper_.find(name);
        if (it == linksBySuper_.end()) return out;

        std::unordered_set<std::string> seen;
        for (const auto& link : it->second)
        {
            if (!linkReachesLocked(link, name, fileUri)) continue;
            if (!seen.insert(nodeKey(link.childName, link.childUri)).second) continue;
            if (const auto* facts = findLocked(link.childName, link.childUri)) out.push_back(facts);
        }
        return out;
    }
}
