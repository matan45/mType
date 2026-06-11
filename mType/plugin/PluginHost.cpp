#include "PluginHost.hpp"
#include "PluginLoader.hpp"
#include "../environment/Environment.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../environment/registry/ClassRegistry.hpp"
#include "../environment/registry/FunctionRegistry.hpp"
#include "../value/ValueShim.hpp"
#include "../value/InternedString.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ObjectInstance.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../errors/RuntimeException.hpp"

#include <memory>
#include <string>
#include <vector>

/*
 * The opaque MTypeValue / MTypeContext types are defined at file scope in
 * PluginHost.hpp. Everything below converts host C function-pointer-table
 * calls into operations on the underlying value::Value / NativeArray /
 * ObjectInstance / NativeRegistry types.
 *
 * Lifetime: every MTypeValue we hand back to the plugin is allocated in
 * MTypeContext::arena (deque<MTypeValue>) and is destroyed when the trampoline
 * drains the arena after the plugin returns. The plugin's returned pointer is
 * copied into a local value::Value before drain.
 */

namespace plugin
{
    namespace
    {
        ::value::ValueType tagToValueType(::MTypeTag t)
        {
            switch (t)
            {
            case MT_TAG_INT:    return ::value::ValueType::INT;
            case MT_TAG_FLOAT:  return ::value::ValueType::FLOAT;
            case MT_TAG_BOOL:   return ::value::ValueType::BOOL;
            case MT_TAG_STRING: return ::value::ValueType::STRING;
            case MT_TAG_OBJECT: return ::value::ValueType::OBJECT;
            case MT_TAG_NULL:   return ::value::ValueType::NULL_TYPE;
            case MT_TAG_VOID:
            default:            return ::value::ValueType::VOID;
            }
        }

        ::MTypeTag valueTypeToTag(const ::value::Value& v)
        {
            using VT = ::value::ValueType;
            switch (v.tag())
            {
            case VT::INT:        return MT_TAG_INT;
            case VT::FLOAT:      return MT_TAG_FLOAT;
            case VT::BOOL:       return MT_TAG_BOOL;
            case VT::STRING:     return MT_TAG_STRING;
            case VT::ARRAY:      return MT_TAG_ARRAY;
            case VT::OBJECT:     return MT_TAG_OBJECT;
            case VT::NULL_TYPE:  return MT_TAG_NULL;
            case VT::VOID:       return MT_TAG_VOID;
            default:             return MT_TAG_OTHER;
            }
        }

        ::MTypeValue* arenaPush(::MTypeContext* ctx, ::value::Value&& v)
        {
            ctx->arena.values.emplace_back();
            ctx->arena.values.back().v = std::move(v);
            return &ctx->arena.values.back();
        }

        /* Forward-declared: hostMakeString needs it for a null-arg guard. */
        void hostRaiseError(::MTypeContext* ctx,
                            const char* exceptionType,
                            const char* message);

        /* ---- vtable function impls ---- */

        ::MTypeValue* hostMakeNull(::MTypeContext* ctx)
        {
            return arenaPush(ctx, ::value::Value(nullptr));
        }
        ::MTypeValue* hostMakeVoid(::MTypeContext* ctx)
        {
            return arenaPush(ctx, ::value::Value());
        }
        ::MTypeValue* hostMakeBool(::MTypeContext* ctx, int v)
        {
            return arenaPush(ctx, ::value::Value(v != 0));
        }
        ::MTypeValue* hostMakeInt(::MTypeContext* ctx, int64_t v)
        {
            return arenaPush(ctx, ::value::Value(v));
        }
        ::MTypeValue* hostMakeFloat(::MTypeContext* ctx, double v)
        {
            return arenaPush(ctx, ::value::Value(v));
        }
        ::MTypeValue* hostMakeString(::MTypeContext* ctx, const char* utf8, size_t len)
        {
            /* (utf8 == null, len > 0) used to fall into std::string("", len) which
             * reads `len` bytes past the empty string literal — undefined behaviour.
             * Raise instead so the script sees the contract violation. */
            if (utf8 == nullptr && len > 0)
            {
                hostRaiseError(ctx, "PluginError",
                               "makeString: null pointer with non-zero length");
                return arenaPush(ctx, ::value::Value(nullptr));
            }
            std::string s(utf8 ? utf8 : "", len);
            return arenaPush(ctx, ::value::Value(std::move(s)));
        }
        ::MTypeValue* hostMakeArray(::MTypeContext* ctx, ::MTypeTag elemTag, size_t length)
        {
            ::value::ValueType vt = tagToValueType(elemTag);
            auto arr = std::make_shared<::value::NativeArray>(length, vt);
            return arenaPush(ctx, ::value::Value(std::move(arr)));
        }
        ::MTypeValue* hostMakeObject(::MTypeContext* ctx, const char* className)
        {
            if (!ctx)
            {
                return nullptr;
            }
            if (!className || !ctx->env)
            {
                return arenaPush(ctx, ::value::Value(nullptr));
            }
            if (ctx->vm && ctx->vm->getProgram())
            {
                try
                {
                    ::value::Value created = ctx->vm->createObject(
                        className, std::span<const ::value::Value>{});
                    return arenaPush(ctx, std::move(created));
                }
                catch (const std::exception& e)
                {
                    hostRaiseError(ctx, "PluginError",
                                   (std::string("makeObject '") + className + "' failed: " + e.what()).c_str());
                    return arenaPush(ctx, ::value::Value(nullptr));
                }
            }
            auto classDef = ctx->env->findClass(className);
            if (!classDef)
            {
                return arenaPush(ctx, ::value::Value(nullptr));
            }
            auto inst = std::make_shared<::runtimeTypes::klass::ObjectInstance>(classDef);
            inst->registerWithGC();
            return arenaPush(ctx, ::value::Value(std::move(inst)));
        }

        ::MTypeTag hostGetTag(const ::MTypeValue* v)
        {
            return v ? valueTypeToTag(v->v) : MT_TAG_VOID;
        }
        int hostGetBool(const ::MTypeValue* v)
        {
            return (v && ::value::isBool(v->v)) ? (::value::asBool(v->v) ? 1 : 0) : 0;
        }
        int64_t hostGetInt(const ::MTypeValue* v)
        {
            return (v && ::value::isInt(v->v)) ? ::value::asInt(v->v) : 0;
        }
        double hostGetFloat(const ::MTypeValue* v)
        {
            return (v && ::value::isFloat(v->v)) ? ::value::asFloat(v->v) : 0.0;
        }
        const char* hostGetString(const ::MTypeValue* v, size_t* outLen)
        {
            if (!v) { if (outLen) *outLen = 0; return ""; }
            if (::value::isString(v->v))
            {
                const std::string& s = ::value::asString(v->v);
                if (outLen) *outLen = s.size();
                return s.c_str();
            }
            if (::value::isInternedString(v->v))
            {
                const std::string& s = ::value::asInternedString(v->v).getString();
                if (outLen) *outLen = s.size();
                return s.c_str();
            }
            // MYT-317: STRING_INLINE. The inline buffer is not null-terminated;
            // materialize into a thread_local std::string so the returned
            // pointer is null-terminated and stable for the plugin call.
            // Same lifetime contract as the heap forms above (valid until the
            // next host call on this thread).
            if (::value::isInlineString(v->v))
            {
                thread_local std::string scratch;
                scratch.assign(v->v.rawInlineChars(), v->v.rawInlineLen());
                if (outLen) *outLen = scratch.size();
                return scratch.c_str();
            }
            if (outLen) *outLen = 0;
            return "";
        }

        size_t hostArrayLen(const ::MTypeValue* arr)
        {
            if (!arr || !::value::isNativeArray(arr->v)) return 0;
            return ::value::asNativeArray(arr->v)->size();
        }
        ::MTypeValue* hostArrayGet(::MTypeContext* ctx, const ::MTypeValue* arr, size_t index)
        {
            if (!arr || !::value::isNativeArray(arr->v))
            {
                return arenaPush(ctx, ::value::Value(nullptr));
            }
            const auto& nat = ::value::asNativeArray(arr->v);
            if (index >= nat->size())
            {
                return arenaPush(ctx, ::value::Value(nullptr));
            }
            return arenaPush(ctx, nat->get(index));
        }
        ::MTypeStatus hostArraySet(const ::MTypeValue* arr, size_t index, const ::MTypeValue* v)
        {
            if (!arr || !::value::isNativeArray(arr->v) || !v) return MT_ERR_TYPE;
            const auto& nat = ::value::asNativeArray(arr->v);
            if (index >= nat->size()) return MT_ERR_RANGE;
            nat->set(index, v->v);
            return MT_OK;
        }

        ::MTypeValue* hostObjGet(::MTypeContext* ctx, const ::MTypeValue* obj, const char* fieldName)
        {
            if (!obj || !fieldName || !::value::isObject(obj->v))
            {
                return arenaPush(ctx, ::value::Value(nullptr));
            }
            const auto& inst = ::value::asObject(obj->v);
            return arenaPush(ctx, inst->getFieldValue(fieldName));
        }
        ::MTypeStatus hostObjSet(const ::MTypeValue* obj, const char* fieldName, const ::MTypeValue* v)
        {
            if (!obj || !fieldName || !v || !::value::isObject(obj->v)) return MT_ERR_TYPE;
            const auto& inst = ::value::asObject(obj->v);
            inst->setField(fieldName, v->v);
            return MT_OK;
        }

        void hostRegisterFunction(::MTypeContext* ctx,
                                  const char* name,
                                  ::MTypeNativeFn fn,
                                  void* userData)
        {
            if (!ctx || !name || !fn || !ctx->registeringPlugin || !ctx->env) return;

            auto* handle = ctx->registeringPlugin;
            auto binding = std::make_unique<PluginNativeBinding>();
            binding->fn = fn;
            binding->userData = userData;
            binding->owner = handle;
            binding->name = name;

            auto* bindingPtr = binding.get();
            handle->bindings.push_back(std::move(binding));
            handle->registeredNames.emplace_back(name);

            ::environment::registry::NativeDelegate delegate{};
            delegate.userData = bindingPtr;
            delegate.invoke = [](void* u,
                                  ::environment::NativeContext& nc,
                                  std::span<const ::value::Value> args) -> ::value::Value
            {
                return ::plugin::pluginNativeTrampoline(u, nc, args);
            };

            ctx->env->getNativeRegistry()->registerNativeFunction(name, delegate);
        }

        void hostRaiseError(::MTypeContext* ctx,
                             const char* exceptionType,
                             const char* message)
        {
            if (!ctx) return;
            ctx->errorPending = true;
            ctx->errorType = (exceptionType && *exceptionType) ? exceptionType : "PluginError";
            ctx->errorMessage = message ? message : "";
        }

        /* ---- ABI v2 ---- */

        int hostHasClass(::MTypeContext* ctx, const char* className)
        {
            if (!ctx || !ctx->env || !className) return 0;
            return ctx->env->getClassRegistry()->hasClass(className) ? 1 : 0;
        }
        int hostHasFunction(::MTypeContext* ctx, const char* funcName)
        {
            if (!ctx || !ctx->env || !funcName) return 0;
            if (ctx->env->getFunctionRegistry()->hasFunction(funcName)) return 1;
            if (ctx->env->getNativeRegistry()->hasNativeFunction(funcName)) return 1;
            return 0;
        }

        void hostListClasses(::MTypeContext* ctx, ::MTypeNameCallback cb, void* userData)
        {
            if (!ctx || !ctx->env || !cb) return;
            const auto& names = ctx->env->getClassRegistry()->getAllItemNames();
            for (const auto& name : names)
            {
                cb(userData, name.c_str());
            }
        }
        void hostListFunctions(::MTypeContext* ctx, ::MTypeNameCallback cb, void* userData)
        {
            if (!ctx || !ctx->env || !cb) return;
            /* Bind to const& so the returned vector lives until end-of-loop and
             * isn't copied. The registries still allocate+sort once per call
             * (ABI promises alphabetical order); making the iteration itself
             * allocation-free would require a forEach-style registry API. */
            const auto& fnNames = ctx->env->getFunctionRegistry()->getAllFunctionNames();
            for (const auto& name : fnNames)
            {
                cb(userData, name.c_str());
            }
            const auto& natNames = ctx->env->getNativeRegistry()->getAllNativeFunctionNames();
            for (const auto& name : natNames)
            {
                cb(userData, name.c_str());
            }
        }

        std::vector<::value::Value> argsToVector(const ::MTypeValue* const* args, int argc)
        {
            std::vector<::value::Value> out;
            if (argc <= 0 || !args) return out;
            out.reserve(static_cast<size_t>(argc));
            for (int i = 0; i < argc; ++i)
            {
                out.emplace_back(args[i] ? args[i]->v : ::value::Value());
            }
            return out;
        }

        ::MTypeValue* hostCallFunction(::MTypeContext* ctx,
                                        const char* funcName,
                                        const ::MTypeValue* const* args,
                                        int argc)
        {
            if (!ctx || !ctx->vm || !funcName)
            {
                return arenaPush(ctx, ::value::Value(nullptr));
            }
            try
            {
                auto mtArgs = argsToVector(args, argc);
                ::value::Value result = ctx->vm->executeFunction(funcName, mtArgs);
                return arenaPush(ctx, std::move(result));
            }
            catch (const std::exception& e)
            {
                hostRaiseError(ctx, "PluginError",
                               (std::string("callFunction '") + funcName + "' failed: " + e.what()).c_str());
                return arenaPush(ctx, ::value::Value(nullptr));
            }
        }

        ::MTypeValue* hostCallMethod(::MTypeContext* ctx,
                                      const ::MTypeValue* receiver,
                                      const char* methodName,
                                      const ::MTypeValue* const* args,
                                      int argc)
        {
            if (!ctx || !ctx->vm || !receiver || !methodName)
            {
                return arenaPush(ctx, ::value::Value(nullptr));
            }
            if (!::value::isObject(receiver->v))
            {
                hostRaiseError(ctx, "PluginError",
                               (std::string("callMethod '") + methodName + "': receiver is not an object").c_str());
                return arenaPush(ctx, ::value::Value(nullptr));
            }
            try
            {
                auto mtArgs = argsToVector(args, argc);
                auto inst = ::value::asObject(receiver->v);
                ::value::Value result = ctx->vm->invokeMethod(inst, methodName, mtArgs);
                return arenaPush(ctx, std::move(result));
            }
            catch (const std::exception& e)
            {
                hostRaiseError(ctx, "PluginError",
                               (std::string("callMethod '") + methodName + "' failed: " + e.what()).c_str());
                return arenaPush(ctx, ::value::Value(nullptr));
            }
        }
    }  /* anonymous namespace */

    const ::MTypePluginHost* getHostVTable()
    {
        static ::MTypePluginHost table = []() {
            ::MTypePluginHost t{};
            t.abiVersion        = MTYPE_PLUGIN_ABI_VERSION;
            t._reserved         = 0;
            t.makeNull          = &hostMakeNull;
            t.makeVoid          = &hostMakeVoid;
            t.makeBool          = &hostMakeBool;
            t.makeInt           = &hostMakeInt;
            t.makeFloat         = &hostMakeFloat;
            t.makeString        = &hostMakeString;
            t.makeArray         = &hostMakeArray;
            t.makeObject        = &hostMakeObject;
            t.getTag            = &hostGetTag;
            t.getBool           = &hostGetBool;
            t.getInt            = &hostGetInt;
            t.getFloat          = &hostGetFloat;
            t.getString         = &hostGetString;
            t.arrayLen          = &hostArrayLen;
            t.arrayGet          = &hostArrayGet;
            t.arraySet          = &hostArraySet;
            t.objGet            = &hostObjGet;
            t.objSet            = &hostObjSet;
            t.registerFunction  = &hostRegisterFunction;
            t.raiseError        = &hostRaiseError;
            t.hasClass          = &hostHasClass;
            t.hasFunction       = &hostHasFunction;
            t.listClasses       = &hostListClasses;
            t.listFunctions     = &hostListFunctions;
            t.callFunction      = &hostCallFunction;
            t.callMethod        = &hostCallMethod;
            return t;
        }();
        return &table;
    }

    ::value::Value pluginNativeTrampoline(void* userData,
                                          ::environment::NativeContext& ctx,
                                          std::span<const ::value::Value> args)
    {
        auto* binding = static_cast<PluginNativeBinding*>(userData);
        if (!binding || !binding->fn)
        {
            throw ::errors::RuntimeException("PluginError: invalid native binding");
        }

        ::MTypeContext callCtx;
        callCtx.env = ctx.env;
        callCtx.vm = ctx.vm;

        /* Small-buffer optimization: most plugin calls have ≤8 args, so avoid
         * the per-call heap allocation a std::vector would do. Falls back to
         * a heap buffer for unusually wide calls. */
        constexpr size_t kInlineArgCap = 8;
        const ::MTypeValue* inlinePtrs[kInlineArgCap];
        std::unique_ptr<const ::MTypeValue*[]> heapPtrs;
        const ::MTypeValue** argPtrs = inlinePtrs;
        if (args.size() > kInlineArgCap)
        {
            heapPtrs = std::make_unique<const ::MTypeValue*[]>(args.size());
            argPtrs = heapPtrs.get();
        }
        for (size_t i = 0; i < args.size(); ++i)
        {
            callCtx.arena.values.emplace_back();
            callCtx.arena.values.back().v = args[i];  /* copy; refcounts via Value::copy-ctor */
            argPtrs[i] = &callCtx.arena.values.back();
        }

        ::MTypeValue* ret = binding->fn(binding->userData,
                                         &callCtx,
                                         args.empty() ? nullptr : argPtrs,
                                         static_cast<int>(args.size()));

        if (callCtx.errorPending)
        {
            std::string type = std::move(callCtx.errorType);
            std::string msg  = std::move(callCtx.errorMessage);
            ctx.throwException(type, msg);  /* [[noreturn]] */
        }

        ::value::Value out;
        if (ret != nullptr)
        {
            out = ret->v;  /* copy out before arena destruction */
        }
        return out;
    }
}
