#pragma once

#include <memory>
#include <string>

#include "ImportSummary.hpp"
#include "TypeReferenceCollector.hpp"

namespace environment { class Environment; }

namespace mtype::lsp {

/**
 * Decides whether a type name resolves in one document (MYT-440). Errs on
 * the side of "known": a missed squiggle is acceptable, a false one isn't.
 *
 * Known names:
 *   - anything not starting with an uppercase letter (primitives such as
 *     `int` / `string`, keywords) and single letters (generic parameters);
 *   - the compiler builtins Object, Promise, Array, and the wrapper types
 *     Int, Float, Bool, String (excluded by the MYT-440 spec);
 *   - classes and interfaces in the document's environment, which holds its
 *     own declarations plus everything its imports reach;
 *   - names the import summary lists (selected names and aliases);
 *   - types and generic parameters declared anywhere in the document;
 *   - `<X>Builder` when `X` is known: Lombok's @Builder companion, which the
 *     LS only synthesizes for the open document, not for imported files.
 */
class KnownTypeNames {
public:
    KnownTypeNames(std::shared_ptr<environment::Environment> env,
                   const ImportSummary& imports,
                   const TypeReferences& document);

    bool isKnown(const std::string& name) const;

private:
    bool isDeclaredOrImported(const std::string& name) const;

    std::shared_ptr<environment::Environment> env_;
    const ImportSummary& imports_;
    const TypeReferences& document_;
};

} // namespace mtype::lsp
