#include "KnownTypeNames.hpp"

#include "../../../mType/environment/Environment.hpp"
#include "../../../mType/environment/registry/ClassRegistry.hpp"
#include "../../../mType/environment/registry/InterfaceRegistry.hpp"
#include "../../../mType/optimizer/passes/lombok/LombokBuilderCodegen.hpp"

#include <cctype>
#include <unordered_set>
#include <utility>

namespace mtype::lsp {

namespace {

// Resolve without an import: compiler builtins, plus the wrapper classes
// the MYT-440 spec excludes even though lib/primitives declares them.
bool isBuiltinTypeName(const std::string& name) {
    static const std::unordered_set<std::string> kBuiltins = {
        "Object", "Promise", "Array", "Int", "Float", "Bool", "String"
    };
    return kBuiltins.count(name) > 0;
}

} // namespace

KnownTypeNames::KnownTypeNames(std::shared_ptr<environment::Environment> env,
                               const ImportSummary& imports,
                               const TypeReferences& document)
    : env_(std::move(env)), imports_(imports), document_(document) {}

bool KnownTypeNames::isKnown(const std::string& name) const {
    if (name.size() <= 1) return true;
    if (!std::isupper(static_cast<unsigned char>(name.front()))) return true;
    if (isDeclaredOrImported(name)) return true;

    static const std::string kSuffix =
        optimizer::passes::lombok::LombokBuilderCodegen::builderClassName("");
    if (kSuffix.empty() || name.size() <= kSuffix.size()
        || name.compare(name.size() - kSuffix.size(), kSuffix.size(), kSuffix) != 0) {
        return false;
    }
    const std::string owner = name.substr(0, name.size() - kSuffix.size());
    return isDeclaredOrImported(owner);
}

bool KnownTypeNames::isDeclaredOrImported(const std::string& name) const {
    if (isBuiltinTypeName(name)) return true;
    if (document_.declaredTypes.count(name) > 0) return true;
    if (document_.genericParams.count(name) > 0) return true;
    if (imports_.typeNames.count(name) > 0) return true;
    if (!env_) return false;
    if (auto classes = env_->getClassRegistry(); classes && classes->hasClass(name)) return true;
    if (auto interfaces = env_->getInterfaceRegistry();
        interfaces && interfaces->findInterface(name)) {
        return true;
    }
    return false;
}

} // namespace mtype::lsp
