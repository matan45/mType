#include "OverrideAnnotationChecker.hpp"

#include "../diagnostics/DiagnosticBuilder.hpp"
#include "../diagnostics/ErrorCodeRegistry.hpp"
#include "../diagnostics/SourceFileCache.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../ast/AccessModifier.hpp"

namespace analysis
{
    namespace
    {
        // Best-effort indent extraction. Reads the source line that
        // contains the method declaration from the SourceFileCache and
        // returns its leading whitespace prefix. Returns an empty string
        // if the file or line is not cached — the inserted annotation
        // will then sit at column 0, which is uglier but still correct.
        std::string indentPrefixOf(const errors::SourceLocation& loc)
        {
            auto line = diagnostics::SourceFileCache::instance().getLine(
                loc.getFilename(), loc.getLine());
            if (!line) return "";
            std::string prefix;
            for (char c : *line)
            {
                if (c == ' ' || c == '\t')
                {
                    prefix.push_back(c);
                }
                else
                {
                    break;
                }
            }
            return prefix;
        }

        diagnostics::Diagnostic buildMissingOverrideDiagnostic(
            const runtimeTypes::klass::MethodDefinition& child,
            const runtimeTypes::klass::MethodDefinition& parent)
        {
            const std::string indent = indentPrefixOf(child.getSourceLocation());

            // Insert "@Override\n<indent>" immediately before the child
            // method declaration. Zero-width edit at the method's start.
            diagnostics::Suggestion fix;
            fix.label = "add '@Override'";
            fix.renderedHint = "help: add '@Override' before the method";
            fix.edits.push_back(diagnostics::TextEdit{
                child.getSourceLocation(),
                child.getSourceLocation(),
                "@Override\n" + indent
            });
            fix.applicability = diagnostics::FixApplicability::MachineApplicable;

            return diagnostics::DiagnosticBuilder(diagnostics::codes::MissingOverride)
                .withMessage("method '" + child.getName()
                              + "' overrides parent method without '@Override' annotation")
                .withPrimary(child.getSourceLocation(), "missing '@Override'")
                .withSecondary(parent.getSourceLocation(), "parent method declared here")
                .withSuggestion(std::move(fix))
                .withSourceException("MissingOverrideWarning")
                .build();
        }
    } // namespace

    std::vector<diagnostics::Diagnostic> OverrideAnnotationChecker::check(
        const runtimeTypes::klass::ClassDefinition& cls)
    {
        std::vector<diagnostics::Diagnostic> out;

        // Skip the check entirely if there's no parent — nothing to override.
        auto parentClass = cls.getParentClass();
        if (!parentClass)
        {
            return out;
        }

        for (const auto& [methodName, overloads] : cls.getInstanceMethods())
        {
            for (const auto& method : overloads)
            {
                if (!method) continue;
                if (method->hasAnnotation("Override")) continue;

                // Walk ANCESTORS only — start from parentClass, not cls,
                // so the local method itself can't shadow itself.
                auto inherited = parentClass->findInstanceMethodBySignatureInHierarchy(
                    method->getName(), method->getParameters());
                if (!inherited) continue;

                // Skip private parents — not actually visible to subclasses,
                // so the child isn't really overriding.
                if (inherited->getAccessModifier() == ast::AccessModifier::PRIVATE)
                {
                    continue;
                }

                out.push_back(buildMissingOverrideDiagnostic(*method, *inherited));
            }
        }

        return out;
    }
}
