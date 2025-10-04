#pragma once

#include "../base/EvaluationContext.hpp"
#include "../base/AccessContext.hpp"
#include "../managers/InstanceManager.hpp"
#include "../validation/AccessValidator.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>
#include <string>
#include <vector>

// Forward declarations
namespace evaluator {
    class StatementEvaluator;
}

namespace evaluator {
namespace objects {

    using namespace base;
    using namespace managers;
    using namespace value;

    /**
     * @brief Handles static member access and method invocation
     *
     * Responsibilities:
     * - Static field access and assignment
     * - Static method invocation (with and without generics)
     * - Generic type resolution for static methods
     * - Method instantiation and caching for generic static methods
     *
     * Design Principles:
     * - Single Responsibility: Only static member operations
     * - Delegates field operations to InstanceManager
     * - Handles complex generic method instantiation
     */
    class StaticMemberHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        InstanceManager* instanceManager;
        evaluator::StatementEvaluator* stmtEvaluator;

    public:
        explicit StaticMemberHandler(std::shared_ptr<EvaluationContext> ctx,
                                    InstanceManager* instMgr)
            : context(ctx), instanceManager(instMgr), stmtEvaluator(nullptr) {}

        void setStatementEvaluator(evaluator::StatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        /**
         * Access a static field value
         */
        Value accessStaticMember(const std::string& className, const std::string& memberName);

        /**
         * Assign a value to a static field
         */
        void assignStaticMember(const std::string& className, const std::string& memberName,
                               const Value& value);

        /**
         * Call a static method (non-generic)
         */
        Value callStaticMethod(const std::string& className, const std::string& methodName,
                              const std::vector<Value>& args,
                              const errors::SourceLocation& location = errors::SourceLocation{});

        /**
         * Call a static generic method with type arguments
         */
        Value callStaticMethodWithGenerics(const std::string& className, const std::string& methodName,
                                          const std::vector<Value>& args,
                                          const std::vector<std::string>& genericTypeArguments,
                                          const errors::SourceLocation& location = errors::SourceLocation{});
    };

} // namespace objects
} // namespace evaluator
