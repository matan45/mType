#include "AnnotationValidator.hpp"
#include "../errors/TypeException.hpp"
#include "../value/ValueType.hpp"
#include "../constants/ExceptionConstants.hpp"
#include <sstream>
#include <unordered_set>

namespace validation
{
    void AnnotationValidator::validateScriptAnnotation(
        std::shared_ptr<ClassDefinition> classDefinition,
        const SourceLocation& location)
    {
        if (!classDefinition)
        {
            return;
        }

        const std::string& className = classDefinition->getName();

        // Check 1: Class must not be abstract
        if (classDefinition->isAbstract())
        {
            std::ostringstream oss;
            oss << "Class '" << className << "' is marked with @Script but is abstract.\n\n"
                << "@Script classes must be concrete classes that can be instantiated.\n"
                << "Please remove the 'abstract' modifier or the @Script annotation.";
            throw TypeException(oss.str(), location);
        }

        // Check 2: Must have default constructor (0 parameters)
        bool hasDefaultConstructor = false;
        const auto& constructors = classDefinition->getConstructors();

        for (const auto& constructor : constructors)
        {
            if (constructor->getParameters().empty())
            {
                hasDefaultConstructor = true;
                break;
            }
        }

        if (!hasDefaultConstructor)
        {
            std::ostringstream oss;
            oss << "Class '" << className << "' is marked with @Script but does not have a default constructor.\n\n"
                << "@Script classes must have a constructor with no parameters.\n"
                << "Please add a default constructor:\n"
                << "  constructor() {\n"
                << "    // initialization code\n"
                << "  }";
            throw TypeException(oss.str(), location);
        }

        // Check 3: Must have onUpdate(float): void method.
        // Instance methods carry an implicit 'this', so params.size() == 2 means one real param.
        bool hasUpdateMethod = false;
        const auto& instanceMethods = classDefinition->getInstanceMethods();

        auto it = instanceMethods.find("onUpdate");
        if (it != instanceMethods.end())
        {
            for (const auto& method : it->second)
            {
                const auto& params = method->getParameters();

                if (params.size() == 2 &&
                    params[1].second.basicType == value::ValueType::FLOAT &&
                    method->getReturnType() == value::ValueType::VOID)
                {
                    hasUpdateMethod = true;
                    break;
                }
            }
        }

        if (!hasUpdateMethod)
        {
            std::ostringstream oss;
            oss << "Class '" << className <<
                "' is marked with @Script but does not have the required onUpdate method.\n\n"
                << "@Script classes must have an onUpdate method with signature:\n"
                << "  function onUpdate(float deltaTime): void\n\n"
                << "Please add this method to your class.";
            throw TypeException(oss.str(), location);
        }

        // Check 4: Must have onStart(): void method
        bool hasStartMethod = false;

        auto startIt = instanceMethods.find("onStart");
        if (startIt != instanceMethods.end())
        {
            for (const auto& method : startIt->second)
            {
                const auto& params = method->getParameters();

                if (params.size() == 1 &&
                    method->getReturnType() == value::ValueType::VOID)
                {
                    hasStartMethod = true;
                    break;
                }
            }
        }

        if (!hasStartMethod)
        {
            std::ostringstream oss;
            oss << "Class '" << className <<
                "' is marked with @Script but does not have the required onStart method.\n\n"
                << "@Script classes must have an onStart method with signature:\n"
                << "  function onStart(): void\n\n"
                << "Please add this method to your class.";
            throw TypeException(oss.str(), location);
        }

        // Check 5: Must have onDestroy(): void method
        bool hasDestroyMethod = false;

        auto destroyIt = instanceMethods.find("onDestroy");
        if (destroyIt != instanceMethods.end())
        {
            for (const auto& method : destroyIt->second)
            {
                const auto& params = method->getParameters();

                if (params.size() == 1 &&
                    method->getReturnType() == value::ValueType::VOID)
                {
                    hasDestroyMethod = true;
                    break;
                }
            }
        }

        if (!hasDestroyMethod)
        {
            std::ostringstream oss;
            oss << "Class '" << className <<
                "' is marked with @Script but does not have the required onDestroy method.\n\n"
                << "@Script classes must have an onDestroy method with signature:\n"
                << "  function onDestroy(): void\n\n"
                << "Please add this method to your class.";
            throw TypeException(oss.str(), location);
        }
    }

    void AnnotationValidator::validateEntryPointAnnotation(
        std::shared_ptr<ClassDefinition> classDefinition,
        const SourceLocation& location)
    {
        if (!classDefinition)
        {
            return;
        }

        const std::string& className = classDefinition->getName();

        // Check 1: Class must not be abstract
        if (classDefinition->isAbstract())
        {
            std::ostringstream oss;
            oss << "Class '" << className << "' is marked with @EntryPoint but is abstract.\n\n"
                << "@EntryPoint classes must be concrete classes.\n"
                << "Please remove the 'abstract' modifier or the @EntryPoint annotation.";
            throw TypeException(oss.str(), location);
        }

        // Check 2: Must have a static main method.
        // Static methods carry no implicit 'this', so params.size() == 1 means one real param (args).
        bool hasMainMethod = false;
        const auto& staticMethods = classDefinition->getStaticMethods();

        auto it = staticMethods.find("main");
        if (it != staticMethods.end())
        {
            for (const auto& method : it->second)
            {
                const auto& params = method->getParameters();

                if (params.size() == 1 &&
                    method->getReturnType() == value::ValueType::VOID &&
                    method->getAccessModifier() == ast::AccessModifier::PUBLIC)
                {
                    hasMainMethod = true;
                    break;
                }
            }
        }

        if (!hasMainMethod)
        {
            std::ostringstream oss;
            oss << "Class '" << className
                << "' is marked with @EntryPoint but does not have the required main method.\n\n"
                << "@EntryPoint classes must have a static main method with signature:\n"
                << " public static function main(string[] args): void\n\n"
                << "Please add this method to your class.";
            throw TypeException(oss.str(), location);
        }
    }

    void AnnotationValidator::validateThrowAnnotation(
        std::shared_ptr<ast::nodes::annotations::AnnotationNode> throwAnnotation,
        std::shared_ptr<Environment> environment,
        const SourceLocation& location)
    {
        if (!throwAnnotation)
        {
            return;
        }

        std::string exceptionsParam = throwAnnotation->getParameter("exceptions");
        if (exceptionsParam.empty())
        {
            throw TypeException(
                "@Throw annotation must specify at least one exception class",
                location
            );
        }

        std::vector<std::string> exceptionNames = parseExceptionList(exceptionsParam);

        checkForDuplicates(exceptionNames, location);

        auto classRegistry = environment->getClassRegistry();
        for (const auto& exceptionName : exceptionNames)
        {
            auto exceptionClass = classRegistry->findClass(exceptionName);
            if (!exceptionClass)
            {
                std::ostringstream oss;
                oss << "Exception class '" << exceptionName << "' in @Throw annotation does not exist.\n\n"
                    << "Make sure the exception class is defined and imported if necessary.";
                throw TypeException(oss.str(), location);
            }

            if (!isExceptionClass(exceptionName, environment))
            {
                std::ostringstream oss;
                oss << "Class '" << exceptionName << "' in @Throw annotation must extend "
                    << constants::exception::BASE_EXCEPTION_CLASS << ".\n\n"
                    << "Exception classes must inherit from the "
                    << constants::exception::BASE_EXCEPTION_CLASS << " base class.";
                throw TypeException(oss.str(), location);
            }
        }
    }

    static std::string trim(const std::string& str)
    {
        const std::string whitespace = " \t\n\r";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos)
        {
            return "";
        }
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    std::vector<std::string> AnnotationValidator::parseExceptionList(const std::string& exceptionsParam)
    {
        std::vector<std::string> exceptionNames;
        std::stringstream ss(exceptionsParam);
        std::string token;

        while (std::getline(ss, token, ','))
        {
            std::string trimmed = trim(token);
            if (!trimmed.empty())
            {
                exceptionNames.push_back(trimmed);
            }
        }

        return exceptionNames;
    }

    bool AnnotationValidator::isExceptionClass(
        const std::string& className,
        std::shared_ptr<Environment> environment)
    {
        auto classRegistry = environment->getClassRegistry();
        auto classDefinition = classRegistry->findClass(className);

        if (!classDefinition)
        {
            return false;
        }

        if (className == constants::exception::BASE_EXCEPTION_CLASS)
        {
            return true;
        }

        // Walk parent chain looking for Exception.
        std::string currentClassName = className;
        while (true)
        {
            auto currentClass = classRegistry->findClass(currentClassName);
            if (!currentClass)
            {
                break;
            }

            if (!currentClass->hasParentClass())
            {
                break;
            }

            std::string parentClassName = currentClass->getParentClassName();
            if (parentClassName == constants::exception::BASE_EXCEPTION_CLASS)
            {
                return true;
            }

            currentClassName = parentClassName;
        }

        return false;
    }

    void AnnotationValidator::checkForDuplicates(
        const std::vector<std::string>& exceptionNames,
        const SourceLocation& location)
    {
        // Duplicates are currently swallowed silently; future iteration could log a warning.
        std::unordered_set<std::string> seen;
        for (const auto& name : exceptionNames)
        {
            if (seen.find(name) != seen.end())
            {
                continue;
            }
            seen.insert(name);
        }
    }
}
