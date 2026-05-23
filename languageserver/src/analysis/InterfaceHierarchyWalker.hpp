#pragma once

#include "../../../mType/environment/registry/InterfaceDefinition.hpp"
#include "../../../mType/environment/registry/InterfaceRegistry.hpp"
#include "../../../mType/types/TypeSubstitutionService.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mtype::lsp::analysis {

namespace detail {

template <typename Visitor>
void walkInterfaceHierarchyImpl(
    const std::string& interfaceName,
    const std::shared_ptr<runtimeTypes::klass::InterfaceRegistry>& registry,
    const ::types::TypeSubstitutionService& subst,
    std::unordered_set<std::string>& visiting,
    Visitor&& visit)
{
    if (!registry) return;

    auto [baseName, typeArgs] = subst.parseGenericTypeName(interfaceName);
    if (baseName.empty()) baseName = interfaceName;

    if (!visiting.insert(baseName).second) return;

    auto iface = registry->findInterface(baseName);
    if (!iface) { visiting.erase(baseName); return; }

    std::unordered_map<std::string, std::string> substitutions;
    const auto& genericParams = iface->getGenericParameters();
    if (genericParams.size() == typeArgs.size()) {
        for (std::size_t i = 0; i < genericParams.size(); ++i) {
            substitutions[genericParams[i].name] = typeArgs[i];
        }
    }

    // Resolve parent generic args against the current substitutions so
    // `interface Child<T> extends Parent<List<T>>` viewed as `Child<int>`
    // recurses into `Parent<List<int>>`, not `Parent<List<T>>`.
    for (const auto& parent : iface->getExtendedInterfaces()) {
        std::string resolvedParent = substitutions.empty()
            ? parent
            : subst.resolveGenericType(parent, substitutions);
        walkInterfaceHierarchyImpl(resolvedParent, registry, subst, visiting,
                                   std::forward<Visitor>(visit));
    }

    visit(*iface, substitutions);

    visiting.erase(baseName);
}

}  // namespace detail

// Visit every reachable interface in the hierarchy rooted at `rootName`,
// calling `visit(InterfaceDefinition, substitutions)` for each. Parents are
// visited before the root (post-order on the parent edge, pre-order on the
// node), matching the de-duplication order both legacy DFS sites relied on
// — methods declared on a parent emit before the same name reused on a
// child, so a child override displaces the parent's signature naturally
// when the consumer keeps an "already emitted" set.
//
// `substitutions` maps the iface's own generic parameter names to the
// concrete type strings supplied by the enclosing `implements` clause,
// chained through parent resolution. Cycles are bounded by interface base
// name (so `A<int>` and `A<String>` are still considered one node — a
// stricter check would need full structural typing, which the rest of the
// LSP doesn't do either).
template <typename Visitor>
void walkInterfaceHierarchy(
    const std::string& rootName,
    const std::shared_ptr<runtimeTypes::klass::InterfaceRegistry>& registry,
    const ::types::TypeSubstitutionService& subst,
    Visitor&& visit)
{
    std::unordered_set<std::string> visiting;
    detail::walkInterfaceHierarchyImpl(rootName, registry, subst, visiting,
                                       std::forward<Visitor>(visit));
}

}  // namespace mtype::lsp::analysis
