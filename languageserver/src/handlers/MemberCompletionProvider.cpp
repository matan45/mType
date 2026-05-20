#include "MemberCompletionProvider.hpp"

#include "internal/CompletionHelpers.hpp"

#include "../../../mType/ast/AccessModifier.hpp"
#include "../../../mType/environment/registry/ClassDefinition.hpp"
#include "../../../mType/environment/registry/ClassRegistry.hpp"
#include "../../../mType/environment/registry/FieldDefinition.hpp"
#include "../../../mType/environment/registry/MethodDefinition.hpp"

#include <unordered_set>
#include <utility>

namespace mtype::lsp
{
    namespace
    {
        // Resolve the type name to walk. For `this`/`super` this depends on
        // the cursor's enclosing class; for a regular identifier we route
        // through the per-document type inference. Returns empty when
        // nothing is resolvable. Strips any trailing generic argument list.
        std::string resolveTargetType(
            DocumentManager* documentManager,
            const std::string& uri,
            const std::string& objectName,
            int line,
            bool isStaticAccess,
            const std::string& documentContent,
            environment::registry::ClassRegistry& classRegistry)
        {
            std::string varType;
            if (!isStaticAccess && (objectName == "this" || objectName == "super")) {
                varType = internal::inferEnclosingClass(documentContent, line);
                if (objectName == "super" && !varType.empty()) {
                    auto currentClass = classRegistry.findClass(varType);
                    std::string parentClass = currentClass ? currentClass->getParentClassName() : "";
                    if (parentClass.empty()) {
                        parentClass = classRegistry.getParentClass(varType);
                    }
                    varType = parentClass;
                }
            } else if (isStaticAccess) {
                varType = objectName;
            } else {
                varType = documentManager->getVariableType(uri, objectName, line);
            }

            if (auto angle = varType.find('<'); angle != std::string::npos) {
                varType = varType.substr(0, angle);
            }
            return varType;
        }

        CompletionItem buildMethodItem(
            const std::string& uri,
            const std::string& owner,
            const std::string& methodName,
            const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& firstOverload,
            std::size_t overloadCount,
            bool isStaticAccess)
        {
            CompletionItem item;
            item.label = methodName;
            item.kind = static_cast<int>(CompletionItemKind::Method);
            item.detail = internal::methodDetail(firstOverload, owner, overloadCount, isStaticAccess);
            item.insertText = internal::buildCallSnippet(methodName, firstOverload->getParametersWithTypes());
            item.insertTextFormat = internal::kSnippetInsertTextFormat;
            item.filterText = methodName;
            internal::attachCallCommitChars(item);
            internal::stampResolveData(item, uri,
                                       isStaticAccess ? "static_method" : "method",
                                       owner, methodName);
            return item;
        }

        CompletionItem buildFieldItem(
            const std::string& uri,
            const std::string& owner,
            const std::string& fieldName,
            const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& fieldDef,
            bool isStaticAccess)
        {
            CompletionItem item;
            item.label = fieldName;
            item.kind = static_cast<int>(CompletionItemKind::Field);
            item.detail = internal::fieldDetail(fieldDef, owner, isStaticAccess);
            item.insertText = fieldName;
            item.filterText = fieldName;
            internal::stampResolveData(item, uri,
                                       isStaticAccess ? "static_field" : "field",
                                       owner, fieldName);
            return item;
        }

        // Walks the inheritance chain, deduping by (name, isMethod) so an
        // override in the most-derived class wins. Without the walk, members
        // declared on parent classes / Object never showed up after `obj.`.
        std::vector<CompletionItem> walkInheritance(
            environment::registry::ClassRegistry& classRegistry,
            const std::string& varType,
            const std::string& enclosingClass,
            const std::string& uri,
            bool isStaticAccess)
        {
            std::vector<CompletionItem> items;
            std::unordered_set<std::string> seenMembers;

            constexpr int kMaxInheritanceWalk = 32;  // matches ClassDefinition's depth guard
            auto current = classRegistry.findClass(varType);
            int depth = 0;
            while (current && depth++ < kMaxInheritanceWalk) {
                const std::string& currentOwner = current->getName();

                const auto& methods =
                    isStaticAccess ? current->getStaticMethods() : current->getInstanceMethods();
                for (const auto& [methodName, overloads] : methods) {
                    if (overloads.empty()) continue;
                    const auto& first = overloads.front();
                    const bool privateAndOutside =
                        first->getAccessModifier() == ast::AccessModifier::PRIVATE
                        && enclosingClass != currentOwner;
                    if (privateAndOutside) continue;
                    if (!seenMembers.insert("m:" + methodName).second) continue;
                    items.push_back(buildMethodItem(uri, currentOwner, methodName,
                                                    first, overloads.size(), isStaticAccess));
                }

                const auto& fields =
                    isStaticAccess ? current->getStaticFields() : current->getInstanceFields();
                for (const auto& [fieldName, fieldDef] : fields) {
                    if (!fieldDef) continue;
                    const bool privateAndOutside =
                        fieldDef->getAccessModifier() == ast::AccessModifier::PRIVATE
                        && enclosingClass != currentOwner;
                    if (privateAndOutside) continue;
                    if (!seenMembers.insert("f:" + fieldName).second) continue;
                    items.push_back(buildFieldItem(uri, currentOwner, fieldName,
                                                   fieldDef, isStaticAccess));
                }

                current = current->getParentClass();
            }
            return items;
        }
    }

    MemberCompletionProvider::MemberCompletionProvider(DocumentManager* docMgr)
        : documentManager_(docMgr) {}

    std::vector<CompletionItem> MemberCompletionProvider::getMemberCompletions(
        const std::string& uri,
        const std::string& objectName,
        int line,
        bool isStaticAccess)
    {
        auto doc = documentManager_->getDocument(uri);
        if (!doc || !doc->environment) return {};

        auto classRegistry = doc->environment->getClassRegistry();
        if (!classRegistry) return {};

        const std::string varType = resolveTargetType(
            documentManager_, uri, objectName, line, isStaticAccess,
            doc->content, *classRegistry);
        if (varType.empty() || !classRegistry->hasClass(varType)) return {};

        // Member completions for `obj.` / `Class::` are pure with respect
        // to (uri, version, varType, accessKind) — the dropdown fires on
        // every keystroke after the trigger, so caching saves the full
        // inheritance walk each time.
        const std::string cacheKey = uri + "|v" + std::to_string(doc->version)
            + (isStaticAccess ? "|s|" : "|i|") + varType;
        if (auto it = memberCache_.find(cacheKey);
            it != memberCache_.end() && it->second.docVersion == doc->version) {
            return it->second.items;
        }

        const std::string enclosingClass = internal::inferEnclosingClass(doc->content, line);
        auto items = walkInheritance(*classRegistry, varType, enclosingClass, uri, isStaticAccess);

        // Evict stale entries for this uri so the cache doesn't grow
        // unbounded across document edits.
        const std::string uriPrefix = uri + "|v";
        for (auto it = memberCache_.begin(); it != memberCache_.end(); ) {
            if (it->first.rfind(uriPrefix, 0) == 0
                && it->second.docVersion != doc->version) {
                it = memberCache_.erase(it);
            } else {
                ++it;
            }
        }
        memberCache_[cacheKey] = MemberCacheEntry{ doc->version, items };
        return items;
    }
}
