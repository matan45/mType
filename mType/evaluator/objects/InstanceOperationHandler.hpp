#pragma once

#include "../base/EvaluationContext.hpp"
#include "../managers/InstanceManager.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// Forward declarations
namespace evaluator {
    class StatementEvaluator;
}

namespace evaluator {
namespace objects {

    using namespace base;
    using namespace managers;
    using namespace value;
    using namespace runtimeTypes::klass;

    /**
     * @brief Handles instance creation and member operations
     *
     * Responsibilities:
     * - Object instance creation (with and without generic type bindings)
     * - Instance field access and assignment
     * - Instance method invocation
     * - Lambda-backed interface method calls
     * - Parameter validation and conversion
     *
     * Design Principles:
     * - Single Responsibility: Only instance-level operations
     * - Delegates low-level operations to InstanceManager
     * - Handles complex method invocation with context management
     */
    class InstanceOperationHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        InstanceManager* instanceManager;
        evaluator::StatementEvaluator* stmtEvaluator;

    public:
        explicit InstanceOperationHandler(std::shared_ptr<EvaluationContext> ctx,
                                         InstanceManager* instMgr)
            : context(ctx), instanceManager(instMgr), stmtEvaluator(nullptr) {}

        void setStatementEvaluator(evaluator::StatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        /**
         * Create an object instance
         */
        std::shared_ptr<ObjectInstance> createInstance(const std::string& className,
                                                       const std::vector<Value>& constructorArgs);

        /**
         * Create an object instance with generic type bindings
         */
        std::shared_ptr<ObjectInstance> createInstanceWithTypeBindings(
            const std::string& className,
            const std::vector<Value>& constructorArgs,
            const std::unordered_map<std::string, std::string>& typeBindings);

        /**
         * Access an instance field value
         */
        Value accessMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName,
                          const errors::SourceLocation& location = errors::SourceLocation{});

        /**
         * Assign a value to an instance field
         */
        void assignMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName,
                         const Value& value,
                         const errors::SourceLocation& location = errors::SourceLocation{});

        /**
         * Call an instance method
         */
        Value callMethod(std::shared_ptr<ObjectInstance> object, const std::string& methodName,
                        const std::vector<Value>& args,
                        const errors::SourceLocation& location = errors::SourceLocation{});
    };

} // namespace objects
} // namespace evaluator
