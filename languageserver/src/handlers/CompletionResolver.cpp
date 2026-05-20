#include "CompletionResolver.hpp"

#include "internal/CompletionHelpers.hpp"

#include "../../../mType/environment/Environment.hpp"
#include "../../../mType/environment/registry/AnnotationRegistry.hpp"
#include "../../../mType/environment/registry/ClassDefinition.hpp"
#include "../../../mType/environment/registry/ClassRegistry.hpp"
#include "../../../mType/environment/registry/FieldDefinition.hpp"
#include "../../../mType/environment/registry/FunctionRegistry.hpp"
#include "../../../mType/environment/registry/MethodDefinition.hpp"

#include <string>
#include <utility>

namespace mtype::lsp
{
    namespace
    {
        std::string resolveFunctionDocs(
            const environment::Environment& env,
            const std::string& name)
        {
            auto registry = env.getFunctionRegistry();
            if (!registry) return {};
            auto fn = registry->findFunction(name);
            if (!fn) return {};
            std::string returnType = internal::valueTypeToString(fn->getReturnType());
            if (fn->getReturnType() == value::ValueType::OBJECT
                && !fn->getReturnClassName().empty()) {
                returnType = fn->getReturnClassName();
            }
            return "**function** `" + name + "("
                + internal::formatParameters(fn->getParametersWithTypes())
                + "): " + returnType + "`";
        }

        std::string resolveMethodDocs(
            const environment::Environment& env,
            const std::string& owner,
            const std::string& name,
            bool isStatic)
        {
            auto classRegistry = env.getClassRegistry();
            if (!classRegistry) return {};
            auto classDef = classRegistry->findClass(owner);
            if (!classDef) return {};
            const auto& methods =
                isStatic ? classDef->getStaticMethods() : classDef->getInstanceMethods();
            auto it = methods.find(name);
            if (it == methods.end() || it->second.empty()) return {};
            std::string md = std::string("**") + (isStatic ? "static method" : "method")
                + "** `" + internal::methodDetail(it->second.front(), owner,
                                                  it->second.size(), isStatic) + "`";
            if (it->second.size() > 1) md += "\n\nMultiple overloads available.";
            return md;
        }

        std::string resolveFieldDocs(
            const environment::Environment& env,
            const std::string& owner,
            const std::string& name,
            bool isStatic)
        {
            auto classRegistry = env.getClassRegistry();
            if (!classRegistry) return {};
            auto classDef = classRegistry->findClass(owner);
            if (!classDef) return {};
            const auto& fields =
                isStatic ? classDef->getStaticFields() : classDef->getInstanceFields();
            auto it = fields.find(name);
            if (it == fields.end() || !it->second) return {};
            return std::string("**") + (isStatic ? "static field" : "field")
                + "** `" + internal::fieldDetail(it->second, owner, isStatic) + "`";
        }

        std::string resolveAnnotationDocs(
            const environment::Environment& env,
            const std::string& name)
        {
            auto registry = env.getAnnotationRegistry();
            if (!registry || !registry->hasAnnotation(name)) return {};
            return "**annotation** `@" + name + "`";
        }
    }

    CompletionResolver::CompletionResolver(DocumentManager* docMgr)
        : documentManager_(docMgr) {}

    CompletionItem CompletionResolver::resolveCompletion(const CompletionItem& item) const
    {
        CompletionItem resolved = item;

        // Best-effort — if VS Code dropped the `data` blob, or the document
        // closed since the initial response, hand the item back unchanged.
        if (!resolved.data || !resolved.data->is_object()) return resolved;

        const auto& data = *resolved.data;
        if (!data.contains("uri") || !data.contains("kind") || !data.contains("name")) {
            return resolved;
        }

        const std::string uri = data.value("uri", std::string{});
        const std::string kind = data.value("kind", std::string{});
        const std::string owner = data.value("owner", std::string{});
        const std::string name = data.value("name", std::string{});

        auto doc = documentManager_->getDocument(uri);
        if (!doc || !doc->environment) return resolved;
        const auto& env = *doc->environment;

        std::string md;
        if (kind == "function") {
            md = resolveFunctionDocs(env, name);
        } else if (kind == "method" || kind == "static_method") {
            md = resolveMethodDocs(env, owner, name, kind == "static_method");
        } else if (kind == "field" || kind == "static_field") {
            md = resolveFieldDocs(env, owner, name, kind == "static_field");
        } else if (kind == "annotation") {
            md = resolveAnnotationDocs(env, name);
        }

        if (!md.empty()) resolved.documentation = std::move(md);
        return resolved;
    }
}
