#include "ObjectExecutor.hpp"
#include "FunctionExecutor.hpp"
#include "../VirtualMachine.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/BoxingUtils.hpp"
#include "../utils/MethodResolver.hpp"
#include "../../../value/HashUtils.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../services/ScriptAPI.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"

using vm::runtime::utils::autoBoxPrimitive;

namespace vm::runtime
{
    void ObjectExecutor::invokeInstanceMethod(const value::Value& receiverValue,
                                             const std::string& methodName,
                                             std::span<const value::Value> args,
                                             size_t argCount) {
        // MYT-208: tag-aware unwrap. raw is the only handle used for data
        // access (getClassDefinition, getField, contentEquals, etc.); it is
        // valid for both OBJECT and STACK_OBJECT receivers. The original
        // Value is preserved in `receiverValue` so we can push it back as
        // `this` (preserving the tag) and so we can branch on tag when
        // populating the new CallFrame.
        auto* instance = value::asObjectInstanceRaw(receiverValue);
        auto classDef = instance->getClassDefinition();

        std::string simpleMethodName =
            runtimeTypes::klass::SignatureUtils::extractSimpleName(methodName);

        // If methodName already contains a signature (indicated by '/'),
        // use it directly instead of looking up by count only.
        // The compiler already resolved the correct overload.
        std::string qualifiedName = methodName;
        std::string definingClassName = classDef->getName();

        if (methodName.find('/') == std::string::npos && methodName.find("::") == std::string::npos) {
            // Legacy path: methodName is just "add" without class or signature.
            // Cached lookup returns method, defining class, AND qualified name.
            auto lookupResult = classDef->findInstanceMethodForCallNameCached(methodName, argCount);
            if (!lookupResult.method) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "Instance method not found: " + methodName +
                    " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
            }

            definingClassName = lookupResult.definingClassName;
            qualifiedName = lookupResult.qualifiedName;

            auto accessContext = createAccessContext(definingClassName, false);
            validation::AccessValidator::validateMethodAccess(simpleMethodName, lookupResult.method->getAccessModifier(), accessContext);
        } else {
            // New path: methodName already contains full signature like "Container::describe/int".
            // For virtual dispatch, replace the declared class with the ACTUAL object class
            // BUT preserve the signature part to maintain overload resolution.
            // Signed call names must bind the exact overload selected by the compiler.
            auto lookupResult = classDef->findInstanceMethodForCallNameCached(methodName, argCount);
            if (lookupResult.method) {
                definingClassName = lookupResult.definingClassName;

                auto accessContext = createAccessContext(definingClassName, false);
                validation::AccessValidator::validateMethodAccess(simpleMethodName, lookupResult.method->getAccessModifier(), accessContext);

                qualifiedName = lookupResult.qualifiedName;
            }
        }

        auto resolution = utils::MethodResolver::resolve(
            qualifiedName, definingClassName, simpleMethodName, context, *vm);
        auto funcMetadata = resolution.funcMetadata;
        size_t targetProgramIndex = resolution.programIndex;
        const bytecode::BytecodeProgram* targetProgram = resolution.program;
        qualifiedName = resolution.qualifiedName;

        if (!funcMetadata && (simpleMethodName == "toString" || simpleMethodName == "equals" ||
                              simpleMethodName == "hashCode" || simpleMethodName == "getClass")) {
            // Native Object method fallback — when no bytecode exists for an Object method,
            // dispatch to native C++ implementations. This handles both direct Object method
            // calls and cases where overload resolution selects the Object signature (e.g.,
            // equals(null) resolving to equals(Object) when the class only has equals(SpecificType))
            auto computeHashCode = [&instance]() -> int64_t {
                return ::value::hashutils::stringHash(instance->getContentHash());
            };

            if (simpleMethodName == "toString") {
                context.stackManager->push(classDef->getName() + "@" + std::to_string(computeHashCode()));
                return;
            }
            if (simpleMethodName == "equals") {
                // Default Object.equals() — content-based equality. Iterates
                // the declared instance fields and compares element-wise via
                // ObjectInstance::contentEqualsImpl. `==` stays reference-only
                // (see objectIdentity.mt); user overrides customize further.
                // Per implicitObjectInheritance.mt and objectEqualsNull.mt,
                // two instances of the same class with identical field values
                // compare equal.
                if (argCount >= 1) {
                    const auto& otherVal = args[0];
                    // MYT-208: also accept STACK_OBJECT arguments.
                    if (value::isAnyObject(otherVal)) {
                        auto* otherPtr = value::asObjectInstanceRaw(otherVal);
                        if (otherPtr) {
                            context.stackManager->push(instance->contentEquals(*otherPtr));
                        } else {
                            context.stackManager->push(false);
                        }
                    } else {
                        context.stackManager->push(false);
                    }
                } else {
                    context.stackManager->push(false);
                }
                return;
            }
            if (simpleMethodName == "hashCode") {
                context.stackManager->push(computeHashCode());
                return;
            }
            if (simpleMethodName == "getClass") {
                // MYT-42: route through ScriptAPI so language-side
                // obj.getClass() and native ScriptAPI::getClass are the
                // same code path. Builds the parameterized canonical name
                // from the instance's reified type and calls Class.forName.
                services::ScriptAPI api(environment, vm, context.program);
                // MYT-208: pass the original receiver Value so getClass works
                // for both OBJECT and STACK_OBJECT receivers without re-boxing.
                context.stackManager->push(
                    api.getClass(receiverValue).asValue());
                return;
            }
        }
        if (!funcMetadata) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Method '" + qualifiedName + "' has no bytecode. All methods must be compiled to bytecode for VM execution.");
        }

        // Convert lambda arguments to interface implementations if needed.
        // Copy span into a local vector so we can mutate slots for auto-boxing.
        std::vector<value::Value> modifiedArgs(args.begin(), args.end());
        if (functionExecutor) {
            functionExecutor->convertLambdaArgumentsToInterfaces(modifiedArgs, funcMetadata->parameterTypes);
        }

        size_t frameBase = context.stackManager->size();
        // MYT-208: push the original receiver Value (preserves OBJECT vs
        // STACK_OBJECT tag) so the callee's LOAD_LOCAL slot 0 reads the right
        // tag and stays refcount-free for STACK_OBJECT.
        context.stackManager->push(receiverValue);

        for (size_t i = 0; i < argCount; ++i) {
            context.stackManager->push(modifiedArgs[i]);
        }

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = frameBase;
        frame.localBase = frameBase;
        // MYT-197: intern on the target program so the handle is resolvable
        // by the same program that owns the FunctionMetadata.
        frame.functionName = targetProgram->internFrameName(qualifiedName);
        // MYT-208: tag-branch `this` ownership. STACK_OBJECT routes the raw
        // pointer through thisInstanceRaw (lifetime owned by caller frame's
        // stackObjects); OBJECT keeps the shared_ptr in thisInstance.
        if (value::isStackObject(receiverValue)) {
            frame.thisInstanceRaw = instance;
        } else {
            frame.thisInstance = value::asObject(receiverValue);
        }
        frame.definingClassName = definingClassName;
        frame.programIndex = targetProgramIndex;

        context.pushCallFrame(std::move(frame));
        context.stats.functionCalls++;

        if (targetProgram != context.program) {
            context.program = targetProgram;
        }

        vm::profiler::ProfilerHookHelper::onFunctionEntry(qualifiedName);

        if (debugger::DebugHookHelper::isDebuggingEnabled()) {
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc) {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errorsLoc);
            } else {
                auto methodStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                if (methodStartLoc) {
                    errors::SourceLocation errorsLoc(methodStartLoc->filename, methodStartLoc->line, methodStartLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errorsLoc);
                } else {
                    debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errors::SourceLocation());
                }
            }
        }

        // Reserve and initialize local variable slots beyond 'this' + parameters.
        // localCount for instance methods = 'this' (slot 0) + parameters + locals.
        size_t pushedSlots = 1 + argCount;
        if (funcMetadata->localCount > pushedSlots) {
            size_t additionalLocals = funcMetadata->localCount - pushedSlots;
            for (size_t i = 0; i < additionalLocals; ++i) {
                context.stackManager->push(std::monostate{});
            }
        }

        context.instructionPointer = funcMetadata->startOffset - 1;
    }

    void ObjectExecutor::handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr) {
        if (tryDispatchSpecializedCollectionCall(instr)) {
            return;
        }

        const std::string& methodName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];

        // MYT-196: pop arguments into a small-buffer-optimized scratch buffer.
        // Buffer outlives the invokeLambdaMethod / invokeInstanceMethod call,
        // so the span handed to them remains valid.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        value::Value objectValue = context.stackManager->pop();

        // Auto-box raw primitives at escape point (lazy re-boxing support).
        // INVOKE_STRING_CONCAT can leave a raw STRING / INTERNED_STRING on the
        // stack; route those through the same boxing path as int/float/bool.
        // MYT-317: also route STRING_INLINE through auto-box on method dispatch.
        if (value::isInt(objectValue) ||
            value::isFloat(objectValue) ||
            value::isBool(objectValue) ||
            value::isAnyString(objectValue)) {
            objectValue = autoBoxPrimitive(objectValue, environment);
        }

        utils::checkNullReceiver(instr, objectValue, context, environment, "call method", methodName);

        if (value::isLambda(objectValue)) {
            auto lambda = value::asLambda(objectValue);
            invokeLambdaMethod(lambda, args.span(), methodName);
            return;
        }

        // Value-class receiver — materialise via the batch-load helper so the
        // per-call cost is a single vector copy plus one synchronised
        // hashmap fill, instead of the N × setField thrash the previous
        // inline temp-materialisation did.
        if (value::isValueObject(objectValue)) {
            auto valueObj = value::asValueObject(objectValue);
            invokeValueObjectMethod(valueObj, methodName, args.span(), argCount);
            return;
        }

        // MYT-282: array receivers dispatch the four Object methods
        // (toString/equals/hashCode/getClass) through a dedicated runtime
        // intercept. The compile-time path resolves these against
        // lib/Object.mt; the runtime never enters invokeInstanceMethod for
        // arrays because they have no ClassDefinition. Anything else on an
        // array receiver throws "method not found on array".
        if (objectValue.tag() == value::ValueType::ARRAY) {
            invokeArrayObjectMethod(objectValue, methodName, args.span());
            return;
        }

        // MYT-208: accept STACK_OBJECT receivers — invokeInstanceMethod
        // tag-branches the frame.thisInstance / thisInstanceRaw assignment
        // and pushes the original Value onto the stack as `this`.
        if (!value::isAnyObject(objectValue)) {
            utils::ErrorLocationHelper::throwUserException(context, environment,
                "RuntimeException",
                "CALL_METHOD requires an object instance or lambda");
        }

        invokeInstanceMethod(objectValue, methodName, args.span(), argCount);
    }
}
