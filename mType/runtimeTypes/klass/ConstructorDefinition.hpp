#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <utility>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/AccessModifier.hpp"
#include "../../types/UnifiedType.hpp"
#include "../../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../Definition.hpp"

namespace runtimeTypes::klass
{
    using namespace value;
    using namespace ast;

    class ConstructorDefinition : public Definition
    {
    private:
        std::vector<std::pair<std::string, ParameterType>> parametersWithTypes;
        std::shared_ptr<ASTNode> body;
        std::shared_ptr<ASTNode> initializerList;  // For member initialization
        std::shared_ptr<::ast::nodes::classes::SuperConstructorCallNode> superInitializer;
        ast::AccessModifier accessModifier;

        // Cached computed property - lazily computed from parametersWithTypes
        mutable std::vector<std::pair<std::string, ValueType>> cachedParameters;
        mutable bool parametersCacheValid = false;

        // Unified type parameter information for precise type handling
        std::vector<std::pair<std::string, ::types::UnifiedTypePtr>> unifiedParameters;

        // MYT-108: per-constructor annotations populated by ClassRegistrar.
        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> annotations;

        // MYT-110: per-parameter annotations, parallel to parametersWithTypes.
        std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> parameterAnnotations;

      public:
        // Phase 2b (allocation perf): opaque per-constructor cache for the
        // jit_new_object → VirtualMachine::createObject hot path. Without it
        // every allocation pays a string concat (`className + "::<init>/" +
        // typeSignature`) + two hashmap lookups (getFunction + internFrameName).
        // The cache is keyed by programTag so reloading a program invalidates.
        // Types are void* / uint32_t to avoid pulling BytecodeProgram.hpp into
        // mtype-core — callers static_cast back to the concrete types.
        struct CallSiteCache
        {
            const void* programTag = nullptr;     // BytecodeProgram* (identity tag)
            const void* funcMetadata = nullptr;   // BytecodeProgram::FunctionMetadata*
            uint32_t frameNameHandle = UINT32_MAX;// FunctionNameHandle::id
        };

      private:
        mutable CallSiteCache callSiteCache;

        // Phase 3 (allocation perf): the body is strictly `this.F_k = param_k`
        // assignments. When true, trivialFieldAssignments lists the (fieldName,
        // paramIndex) pairs in body order; the VM can copy args directly into
        // instance fields and skip the CallFrame + bytecode interpret loop.
        // Populated by ClassCompiler after constructor analysis.
        bool trivialConstructor = false;
        std::vector<std::pair<std::string, size_t>> trivialFieldAssignments;

        // Phase 4 (allocation perf): the same mapping pre-resolved to field
        // indices (via ClassDefinition::getFieldIndex at compile time) so the
        // fast path can call setFieldByIndex and avoid per-write string hashing.
        // Parallel to trivialFieldAssignments; always same length.
        std::vector<std::pair<size_t, size_t>> trivialFieldIndexAssignments;

      public:
        CallSiteCache& getCallSiteCache() const { return callSiteCache; }

        bool isTrivialConstructor() const { return trivialConstructor; }
        const std::vector<std::pair<std::string, size_t>>& getTrivialFieldAssignments() const
        {
            return trivialFieldAssignments;
        }
        const std::vector<std::pair<size_t, size_t>>& getTrivialFieldIndexAssignments() const
        {
            return trivialFieldIndexAssignments;
        }
        void setTrivialFieldAssignments(std::vector<std::pair<std::string, size_t>> assignments,
                                        std::vector<std::pair<size_t, size_t>> indexAssignments)
        {
            trivialFieldAssignments = std::move(assignments);
            trivialFieldIndexAssignments = std::move(indexAssignments);
            trivialConstructor = true;
        }
       // Legacy constructor with ParameterType (preserves class/interface information)
       explicit ConstructorDefinition(const std::vector<std::pair<std::string, ParameterType>>& params,
                             std::shared_ptr<ASTNode> b,
                             ast::AccessModifier modifier = ast::AccessModifier::PUBLIC)
            : Definition("constructor"), parametersWithTypes(params), body(b),
              initializerList(nullptr), superInitializer(nullptr), accessModifier(modifier), unifiedParameters() {}

       // Constructor with unified type parameter information
       explicit ConstructorDefinition(const std::vector<std::pair<std::string, ParameterType>>& params,
                             std::shared_ptr<ASTNode> b,
                             const std::vector<std::pair<std::string, ::types::UnifiedTypePtr>>& uParams,
                             ast::AccessModifier modifier = ast::AccessModifier::PUBLIC)
            : Definition("constructor"), parametersWithTypes(params), body(b),
              initializerList(nullptr), superInitializer(nullptr), accessModifier(modifier), unifiedParameters(uParams) {}

        bool matchesArgCount(size_t argCount) const;

        // Computed property - derives from parametersWithTypes
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const;

        // Get parameters with full type information
        const std::vector<std::pair<std::string, ParameterType>>& getParametersWithTypes() const { return parametersWithTypes; }
        bool hasParametersWithTypes() const { return !parametersWithTypes.empty(); }

        // Get type signature for constructor overload resolution
        std::string getTypeSignature() const;

        ASTNode* getBody() const { return body.get(); }
        std::shared_ptr<ASTNode> getBodyPtr() const { return body; }
        ASTNode* getInitializerList() const { return initializerList.get(); }
        size_t getParameterCount() const { return parametersWithTypes.size(); }

        // Setter methods
        void setBody(std::shared_ptr<ASTNode> b) { body = b; }
        void setInitializerList(std::shared_ptr<ASTNode> init) { initializerList = init; }

        // Super initializer accessors
        void setSuperInitializer(std::shared_ptr<::ast::nodes::classes::SuperConstructorCallNode> superCall) {
            superInitializer = superCall;
        }
        ::ast::nodes::classes::SuperConstructorCallNode* getSuperInitializer() const {
            return superInitializer.get();
        }
        bool hasSuperInitializer() const {
            return superInitializer != nullptr;
        }

        ast::AccessModifier getAccessModifier() const { return accessModifier; }
        void setAccessModifier(ast::AccessModifier modifier) { accessModifier = modifier; }

        // Unified type parameter information getters and setters
        const std::vector<std::pair<std::string, ::types::UnifiedTypePtr>>& getUnifiedParameters() const {
            return unifiedParameters;
        }
        void setUnifiedParameters(const std::vector<std::pair<std::string, ::types::UnifiedTypePtr>>& uParams) {
            unifiedParameters = uParams;
        }
        bool hasUnifiedParameters() const { return !unifiedParameters.empty(); }

        // MYT-108: annotation accessors mirror MethodDefinition's API.
        const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>& getAnnotations() const
        {
            return annotations;
        }
        void addAnnotation(std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation)
        {
            annotations.push_back(std::move(annotation));
        }
        bool hasAnnotation(const std::string& name) const
        {
            for (const auto& a : annotations) if (a && a->getName() == name) return true;
            return false;
        }
        std::shared_ptr<ast::nodes::annotations::AnnotationNode> getAnnotation(const std::string& name) const
        {
            for (const auto& a : annotations) if (a && a->getName() == name) return a;
            return nullptr;
        }

        // MYT-110: per-parameter annotations
        const std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>>&
        getParameterAnnotations() const { return parameterAnnotations; }

        void setParameterAnnotations(
            std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> annotationsByIndex)
        {
            parameterAnnotations = std::move(annotationsByIndex);
            parameterAnnotations.resize(parametersWithTypes.size());
        }

        const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>&
        getParameterAnnotations(size_t paramIndex) const
        {
            static const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> empty;
            if (paramIndex >= parameterAnnotations.size()) return empty;
            return parameterAnnotations[paramIndex];
        }

        bool hasParameterAnnotation(size_t paramIndex, const std::string& name) const
        {
            for (const auto& a : getParameterAnnotations(paramIndex))
                if (a && a->getName() == name) return true;
            return false;
        }

        std::shared_ptr<ast::nodes::annotations::AnnotationNode>
        getParameterAnnotation(size_t paramIndex, const std::string& name) const
        {
            for (const auto& a : getParameterAnnotations(paramIndex))
                if (a && a->getName() == name) return a;
            return nullptr;
        }

        // Reflection support: get modifier flags as bitmask
        // PUBLIC=1, PRIVATE=2, PROTECTED=4
        int getModifierFlags() const {
            int flags = 0;
            switch (accessModifier) {
                case ast::AccessModifier::PUBLIC: flags |= 1; break;
                case ast::AccessModifier::PRIVATE: flags |= 2; break;
                case ast::AccessModifier::PROTECTED: flags |= 4; break;
            }
            return flags;
        }
    };
}
