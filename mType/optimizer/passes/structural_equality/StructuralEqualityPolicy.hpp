#pragma once

#include "../../../ast/NodeClassesDeclaration.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace optimizer::passes::structural_equality
{
    // MYT-274: eligibility rules and inheritance walk for compiler-synthesized
    // hashCode() / equals(Object). Decides whether a class should receive
    // synthesized methods, and gathers the field set the synthesized bodies
    // should hash / compare (own + inherited instance fields).
    //
    // Skip rules (any one disqualifies):
    //   - Abstract classes (no instances)
    //   - Value classes (already covered by primitive-protocol JIT fast path)
    //   - Generic classes (deferred to Phase 1.5)
    //   - Class with zero instance fields after walking parents
    //   - Class already declares the method (handled per-method, independently)
    //   - Parent declares hashCode or equals but child does not declare BOTH
    //     -> hard error: InheritanceException
    //   - Cross-module parent (parent name not in this program's class registry)
    //     -> soft skip: synthesis declines, native fallback stays in effect.

    using ClassRegistry = std::unordered_map<std::string, ast::ClassNode*>;

    class StructuralEqualityPolicy
    {
    public:
        // True iff the class declares a USER-WRITTEN hashCode/0 (synthetic
        // methods are ignored). Used for parent-conflict detection: a parent
        // that this same pass synthesized doesn't count as "the parent has
        // its own contract", which would otherwise spuriously block the
        // child from being synthesized.
        static bool declaresHashCode(const ast::ClassNode* node);
        static bool declaresEquals(const ast::ClassNode* node);

        // True iff the class has ANY hashCode/equals method (including a
        // previously-synthesized one). Used for the "should we synthesize?"
        // gate so the pass is idempotent under fixed-point iteration of
        // OptimizationPassManager — a second invocation after methods are
        // already present must NOT add duplicates.
        static bool hasAnyHashCode(const ast::ClassNode* node);
        static bool hasAnyEquals(const ast::ClassNode* node);

        // Skip-by-class-shape: abstract / value / generic.
        static bool isShapeSkippable(const ast::ClassNode* node);

        // Returns the class's own declared instance (non-static) fields, in
        // declaration order. Inherited fields are NOT included — synthesis
        // composes via super.hashCode()/super.equals() instead, which avoids
        // private-field access issues across the inheritance boundary.
        static std::vector<const ast::FieldNode*> collectOwnInstanceFields(
            const ast::ClassNode* node);

        // True iff every own field has a type the synthesizer can safely emit
        // `.hashCode()` / `.equals()` calls or direct primitive arithmetic for.
        //
        // Two tiers, keyed on whether the class explicitly opted in via
        // @Data / @EqualsAndHashCode (`annotationRequested`):
        //   * NOT requested (automatic MYT-274 synthesis applied to every
        //     eligible class): int-primitive fields only. Anything else falls
        //     back to the slow Object native so value-equality semantics never
        //     change under a user who did not ask for them.
        //   * requested: the broadened set the pre-staged codegen handles —
        //     Int/Float/Bool/String primitives (all define equals/hashCode in
        //     lib/primitives) plus class/interface references (dispatch through
        //     Object, null-guarded). Parameterized generics (Array<T>,
        //     collection types) stay unsupported — no guaranteed value-equality
        //     contract — as do nullable non-object primitives.
        static bool allFieldsSafeForSynthesis(
            const std::vector<const ast::FieldNode*>& ownFields,
            bool annotationRequested);

        // True iff the immediate parent (if any) directly defines hashCode
        // (user-declared OR previously-synthesized). mType's `super.X()`
        // resolves only against the direct parent's own methods — it does
        // NOT walk further up to find an inherited Object.X. So if the
        // parent has no own hashCode (e.g. it was skipped from synthesis
        // because of a string field), `super.hashCode()` would throw at
        // runtime. The synthesis driver checks this gate before emitting a
        // super-compose path.
        static bool parentHasHashCodeMethod(
            const ast::ClassNode* node,
            const ClassRegistry& registry);
        static bool parentHasEqualsMethod(
            const ast::ClassNode* node,
            const ClassRegistry& registry);

        // Throws InheritanceException if any ancestor declares hashCode or
        // equals while the immediate child does not declare BOTH. Walks up
        // to (excluding) the Object root.
        static void enforceParentConflictRule(
            const ast::ClassNode* node,
            const ClassRegistry& registry);
    };
}
