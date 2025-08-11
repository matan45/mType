#include "Function.hpp"
#include "Klass.hpp"
#include <sstream>
#include <algorithm>

namespace mtype::runtime {
    using namespace core;
    
    // ===== MTypeNativeFunction Implementation =====
    
    MTypeNativeFunction::MTypeNativeFunction(const std::string& n,
                                           NativeFunctionPtr f,
                                           size_t a,
                                           const std::vector<ValueType>& pt,
                                           ValueType rt,
                                           const std::string& doc)
        : name(n), function(f), minArity(a), maxArity(a), 
          paramTypes(pt), returnType(rt), documentation(doc), variadic(false) {
    }
    
    MTypeNativeFunction::MTypeNativeFunction(const std::string& n,
                                           NativeFunctionPtr f,
                                           size_t min,
                                           size_t max,
                                           const std::vector<ValueType>& pt,
                                           ValueType rt,
                                           const std::string& doc)
        : name(n), function(f), minArity(min), maxArity(max),
          paramTypes(pt), returnType(rt), documentation(doc), variadic(false) {
    }
    
    MTypeNativeFunction::MTypeNativeFunction(const std::string& n,
                                           NativeFunctionPtr f,
                                           size_t min,
                                           bool var,
                                           const std::vector<ValueType>& pt,
                                           ValueType rt,
                                           const std::string& doc)
        : name(n), function(f), minArity(min), maxArity(SIZE_MAX),
          paramTypes(pt), returnType(rt), documentation(doc), variadic(var) {
    }
    
    Result<Value> MTypeNativeFunction::call(Interpreter* interpreter,
                                           const std::vector<Value>& args) {
        // Validate arguments
        auto validationResult = validateArgs(args);
        if (validationResult.isError()) {
            return Result<Value>::err(validationResult.error());
        }
        
        // Call the native function
        try {
            return function(args);
        } catch (const std::exception& e) {
            return Result<Value>::err(Error::runtime(
                "Native function '" + name + "' threw exception: " + e.what()));
        } catch (...) {
            return Result<Value>::err(Error::runtime(
                "Native function '" + name + "' threw unknown exception"));
        }
    }
    
    Result<void> MTypeNativeFunction::validateArgs(const std::vector<Value>& args) const {
        // Check arity
        if (variadic) {
            if (args.size() < minArity) {
                return Result<void>::err(Error::argument(
                    "Function '" + name + "' expects at least " + 
                    std::to_string(minArity) + " arguments, got " + 
                    std::to_string(args.size())));
            }
        } else {
            if (args.size() < minArity || args.size() > maxArity) {
                if (minArity == maxArity) {
                    return Result<void>::err(Error::argument(
                        "Function '" + name + "' expects exactly " + 
                        std::to_string(minArity) + " arguments, got " + 
                        std::to_string(args.size())));
                } else {
                    return Result<void>::err(Error::argument(
                        "Function '" + name + "' expects between " + 
                        std::to_string(minArity) + " and " + 
                        std::to_string(maxArity) + " arguments, got " + 
                        std::to_string(args.size())));
                }
            }
        }
        
        // Check parameter types if specified
        for (size_t i = 0; i < std::min(args.size(), paramTypes.size()); ++i) {
            if (paramTypes[i] != ValueType::V_NULL && args[i].type != paramTypes[i]) {
                return Result<void>::err(Error::errorType(
                    "Argument " + std::to_string(i + 1) + " to function '" + name + 
                    "' must be " + Value::typeToString(paramTypes[i]) + 
                    ", got " + args[i].typeName()));
            }
        }
        
        return Result<void>::ok();
    }
    
    std::string MTypeNativeFunction::toString() const {
        std::ostringstream oss;
        oss << "<native function " << name << "(";
        
        if (variadic) {
            oss << "...";
        } else if (minArity == maxArity) {
            oss << minArity << " args";
        } else {
            oss << minArity << "-" << maxArity << " args";
        }
        
        oss << ")>";
        return oss.str();
    }
    
    // ===== MTypeFunction Implementation =====
    
    MTypeFunction::MTypeFunction(const std::string& n,
                                const std::vector<FunctionParameter>& params,
                                ASTNodePtr b,
                                std::shared_ptr<Environment> c,
                                ValueType rt)
        : name(n), parameters(params), body(std::move(b)), closure(c),
          returnType(rt), isGenerator(false), isAsync(false),
          modifiers(METHOD_NONE), isMethod(false) {
    }
    
    MTypeFunction::MTypeFunction(const std::string& n,
                                const std::vector<FunctionParameter>& params,
                                ASTNodePtr b,
                                std::shared_ptr<Environment> c,
                                std::shared_ptr<MTypeClass> oc,
                                int mods,
                                ValueType rt)
        : name(n), parameters(params), body(std::move(b)), closure(c),
          returnType(rt), isGenerator(false), isAsync(false),
          modifiers(mods), isMethod(true), owningClass(oc) {
    }
    
    Result<Value> MTypeFunction::call(Interpreter* interpreter,
                                     const std::vector<Value>& args) {
		// Create execution environment
		auto envResult = createCallEnvironment(args);
		if (envResult.isError()) {
			return Result<Value>::err(envResult.error());
		}

		auto callEnv = envResult.value();

		// Execute function body
		// Note: This requires the Interpreter to have an execute method
		// that takes an AST node and environment
		// For now, return a placeholder - actual implementation will be in Interpreter
		return Result<Value>::err(Error::runtime("Interpreter not yet implemented"));
    }
    
    size_t MTypeFunction::arity() const {
        // Count required parameters (those without defaults)
        size_t required = 0;
        for (const auto& param : parameters) {
            if (!param.hasDefault) {
                required++;
            }
        }
        return required;
    }
    
    std::string MTypeFunction::toString() const {
        std::ostringstream oss;
        
        if (isMethod) {
            oss << "<method ";
            if (isStatic()) oss << "static ";
            if (isAbstract()) oss << "abstract ";
            if (isVirtual()) oss << "virtual ";
            if (isOverride()) oss << "override ";
            if (isFinal()) oss << "final ";
        } else {
            oss << "<function ";
        }
        
        oss << name << "(";
        
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << Value::typeToString(parameters[i].type) << " " << parameters[i].name;
            if (parameters[i].hasDefault) {
                oss << " = " << parameters[i].defaultValue.toString();
            }
        }
        
        oss << ")";
        
        if (returnType != ValueType::V_NULL) {
            oss << " -> " << Value::typeToString(returnType);
        }
        
        oss << ">";
        return oss.str();
    }
    
    std::shared_ptr<MTypeFunction> MTypeFunction::bind(std::shared_ptr<MTypeInstance> instance) {
        // Create a new function with 'this' bound to the instance
        auto boundFunc = std::make_shared<MTypeFunction>(std::move(*this));
        
        // The bound function will have the instance available in its closure
        // This is typically handled by the interpreter when calling the method
        return boundFunc;
    }
    
    Result<std::shared_ptr<Environment>> MTypeFunction::createCallEnvironment(
        const std::vector<Value>& args) const {
        
        // Create new environment with closure as parent
        auto env = std::make_shared<Environment>(closure.get());
        
        // Check minimum arity
        size_t requiredParams = arity();
        if (args.size() < requiredParams) {
            return Result<std::shared_ptr<Environment>>::err(Error::argument(
                "Function '" + name + "' expects at least " + 
                std::to_string(requiredParams) + " arguments, got " + 
                std::to_string(args.size())));
        }
        
        // Check maximum arity
        if (args.size() > parameters.size()) {
            return Result<std::shared_ptr<Environment>>::err(Error::argument(
                "Function '" + name + "' expects at most " + 
                std::to_string(parameters.size()) + " arguments, got " + 
                std::to_string(args.size())));
        }
        
        // Bind parameters
        for (size_t i = 0; i < parameters.size(); ++i) {
            Value value;
            
            if (i < args.size()) {
                // Use provided argument
                value = args[i];
                
                // Type check
                if (parameters[i].type != ValueType::V_NULL && 
                    value.type != parameters[i].type) {
                    return Result<std::shared_ptr<Environment>>::err(Error::errorType(
                        "Argument '" + parameters[i].name + "' to function '" + name + 
                        "' must be " + Value::typeToString(parameters[i].type) + 
                        ", got " + value.typeName()));
                }
            } else if (parameters[i].hasDefault) {
                // Use default value
                value = parameters[i].defaultValue;
            } else {
                // This shouldn't happen if arity check passed
                return Result<std::shared_ptr<Environment>>::err(Error::argument(
                    "Missing required argument '" + parameters[i].name + 
                    "' for function '" + name + "'"));
            }
            
            // Define parameter in environment
            auto defineResult = env->define(parameters[i].name, value, parameters[i].type);
            if (defineResult.isError()) {
                return Result<std::shared_ptr<Environment>>::err(defineResult.error());
            }
        }
        
        return Result<std::shared_ptr<Environment>>::ok(env);
    }
    
    // ===== MTypeLambda Implementation =====
    
    MTypeLambda::MTypeLambda(const std::vector<FunctionParameter>& params,
                           ASTNodePtr body,
                           std::shared_ptr<Environment> closure,
                           const std::vector<std::pair<std::string, Value>>& cap,
                           ValueType returnType)
        : MTypeFunction("<lambda>", params, std::move(body), closure, returnType),
          captures(cap) {
    }
    
    std::string MTypeLambda::toString() const {
        std::ostringstream oss;
        oss << "<lambda(";
        
        const auto& params = getParameters();
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << params[i].name;
        }
        
        oss << ")";
        
        if (!captures.empty()) {
            oss << " [captures: ";
            for (size_t i = 0; i < captures.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << captures[i].first;
            }
            oss << "]";
        }
        
        oss << ">";
        return oss.str();
    }
    
    // ===== MTypeConstructor Implementation =====
    
    MTypeConstructor::MTypeConstructor(std::shared_ptr<MTypeClass> cls,
                                     const std::vector<FunctionParameter>& params,
                                     ASTNodePtr body,
                                     std::shared_ptr<Environment> closure)
        : MTypeFunction("<constructor>", params, std::move(body), closure, ValueType::V_INSTANCE),
          targetClass(cls) {
    }
    
    Result<Value> MTypeConstructor::call(Interpreter* interpreter,
                                        const std::vector<Value>& args) {
        // Create new instance
        auto instance = std::make_shared<MTypeInstance>(targetClass);
        
        // Create execution environment with 'this' bound to the instance
        auto envResult = createCallEnvironment(args);
        if (envResult.isError()) {
            return Result<Value>::err(envResult.error());
        }
        
        auto callEnv = envResult.value();
        
        // Bind 'this' to the instance
        auto thisResult = callEnv->defineThis(Value(instance));
        if (thisResult.isError()) {
            return Result<Value>::err(thisResult.error());
        }
        
        // Execute constructor body
		//TODO: Uncomment when Interpreter::execute is implemented
        /**auto execResult = interpreter->execute(getBody(), callEnv);
        if (execResult.isError()) {
            return Result<Value>::err(execResult.error());
        }*/
        
        // Return the instance
        return Result<Value>::ok(Value(instance));
    }
    
    std::string MTypeConstructor::toString() const {
        std::ostringstream oss;
        oss << "<constructor for " << targetClass->getName() << ">";
        return oss.str();
    }
    
    // ===== MTypeBoundMethod Implementation =====
    
    MTypeBoundMethod::MTypeBoundMethod(std::shared_ptr<MTypeFunction> m,
                                     std::shared_ptr<MTypeInstance> i)
        : method(m), instance(i) {
    }
    
    Result<Value> MTypeBoundMethod::call(Interpreter* interpreter,
                                        const std::vector<Value>& args) {
        // Create execution environment for the method
        auto envResult = method->createCallEnvironment(args);
        if (envResult.isError()) {
            return Result<Value>::err(envResult.error());
        }
        
        auto callEnv = envResult.value();
        
        // Bind 'this' to the instance
        auto thisResult = callEnv->defineThis(Value(instance));
        if (thisResult.isError()) {
            return Result<Value>::err(thisResult.error());
        }
        
        // Execute method body
        //TODO: Uncomment when Interpreter::execute is implemented
        //return interpreter->execute(method->getBody(), callEnv);
		// For now, return a placeholder - actual implementation will be in Interpreter
		return Result<Value>::err(Error::runtime("Interpreter not yet implemented"));
    }
    
    std::string MTypeBoundMethod::toString() const {
        std::ostringstream oss;
        oss << "<bound " << method->toString() << " of " << instance->toString() << ">";
        return oss.str();
    }
    
}
