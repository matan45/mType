#include "AbstractClassValidator.hpp"
#include "../../errors/AbstractClassException.hpp"
#include <sstream>

namespace evaluator {
namespace validation {

    using namespace errors;

    void AbstractClassValidator::validateAbstractClassNotInstantiated(
        std::shared_ptr<ClassDefinition> classDef,
        const SourceLocation& location)
    {
        if (!classDef) {
            return; // Class definition is null, other validators will handle this
        }

        if (classDef->isAbstract()) {
            throw AbstractClassException(
                "Cannot instantiate abstract class '" + classDef->getName() + "'. "
                "Abstract classes must be extended by concrete classes that implement all abstract methods.",
                classDef->getName(),
                location);
        }
    }

    void AbstractClassValidator::validateAllAbstractMethodsImplemented(
        std::shared_ptr<ClassDefinition> classDef,
        const SourceLocation& location)
    {
        if (!classDef) {
            return; // Class definition is null, other validators will handle this
        }

        // Abstract classes don't need to implement all abstract methods
        if (classDef->isAbstract()) {
            return;
        }

        // Get list of unimplemented abstract methods
        auto unimplemented = classDef->getUnimplementedAbstractMethods();

        if (!unimplemented.empty()) {
            std::string methodList = formatUnimplementedMethodsList(unimplemented);

            throw AbstractClassException(
                "Concrete class '" + classDef->getName() + "' must implement all abstract methods. " +
                "Unimplemented abstract methods:\n" + methodList,
                classDef->getName(),
                unimplemented,
                location);
        }
    }

    void AbstractClassValidator::validateAbstractMethodHasNoBody(
        std::shared_ptr<MethodDefinition> method,
        const std::string& className,
        const SourceLocation& location)
    {
        if (!method) {
            return;
        }

        if (method->isAbstract() && method->getBody() != nullptr) {
            throw AbstractClassException(
                "Abstract method '" + method->getName() + "' in class '" + className + "' "
                "must not have a body. Abstract methods should only declare the signature.",
                className,
                method->getName(),
                location);
        }
    }

    void AbstractClassValidator::validateAbstractMethodsOnlyInAbstractClass(
        std::shared_ptr<ClassDefinition> classDef,
        const SourceLocation& location)
    {
        if (!classDef) {
            return;
        }

        // If class is abstract, it's fine to have abstract methods
        if (classDef->isAbstract()) {
            return;
        }

        // Check if any methods are marked as abstract
        const auto& abstractMethods = classDef->getAbstractMethods();
        if (!abstractMethods.empty()) {
            std::vector<std::string> methodList(abstractMethods.begin(), abstractMethods.end());
            std::string methodStr = formatUnimplementedMethodsList(methodList);

            throw AbstractClassException(
                "Concrete class '" + classDef->getName() + "' cannot have abstract methods. " +
                "Either mark the class as abstract or provide implementations for:\n" + methodStr,
                classDef->getName(),
                methodList,
                location);
        }
    }

    void AbstractClassValidator::validateConcreteClassIsComplete(
        std::shared_ptr<ClassDefinition> classDef,
        const SourceLocation& location)
    {
        // Same as validateAllAbstractMethodsImplemented
        validateAllAbstractMethodsImplemented(classDef, location);
    }

    std::vector<std::string> AbstractClassValidator::getUnimplementedMethods(
        std::shared_ptr<ClassDefinition> classDef)
    {
        if (!classDef) {
            return {};
        }

        return classDef->getUnimplementedAbstractMethods();
    }

    std::string AbstractClassValidator::formatUnimplementedMethodsList(
        const std::vector<std::string>& methods)
    {
        std::ostringstream oss;
        for (size_t i = 0; i < methods.size(); ++i) {
            oss << "  - " << methods[i] << "()";
            if (i < methods.size() - 1) {
                oss << "\n";
            }
        }
        return oss.str();
    }

} // namespace validation
} // namespace evaluator
