/*
 * Minimal mType native plugin (MYT-289).
 *
 * Builds into hello.dll / hello.so / hello.dylib. Loaded from .mt scripts via
 * __plugin_load("./hello.dll"). Exposes:
 *
 *   __hello_greet(name: string): string
 *       Returns "hello, <name>".
 *
 *   __hello_apply_twice(funcName: string, value: int): int
 *       Demo of plugin -> mType reentrancy (ABI v2). Calls the named mType
 *       function twice on `value` via host->callFunction and returns the
 *       result. e.g. with `function double(int n): int { return n * 2; }`,
 *       __hello_apply_twice("double", 3) returns 12.
 *
 *   __hello_count_natives(): int
 *       Demo of host->listFunctions enumeration. Counts how many functions
 *       (mType + native) are currently registered with the engine.
 */

#include "PluginHostApi.h"

#include <cstring>
#include <string>

namespace {
const MTypePluginHost* g_host = nullptr;

MTypeValue* greet(void*, MTypeContext* ctx,
                  const MTypeValue* const* args, int argc)
{
    if (argc != 1 || g_host->getTag(args[0]) != MT_TAG_STRING) {
        g_host->raiseError(ctx, "PluginError", "__native__hello_greet: expected one string arg");
        return g_host->makeNull(ctx);
    }
    size_t nameLen = 0;
    const char* name = g_host->getString(args[0], &nameLen);

    std::string out = "hello, ";
    out.append(name, nameLen);
    return g_host->makeString(ctx, out.c_str(), out.size());
}

MTypeValue* applyTwice(void*, MTypeContext* ctx,
                       const MTypeValue* const* args, int argc)
{
    if (argc != 2
        || g_host->getTag(args[0]) != MT_TAG_STRING
        || g_host->getTag(args[1]) != MT_TAG_INT) {
        g_host->raiseError(ctx, "PluginError",
                           "__native__hello_apply_twice: expected (string funcName, int value)");
        return g_host->makeNull(ctx);
    }

    size_t nameLen = 0;
    const char* funcName = g_host->getString(args[0], &nameLen);
    int64_t v = g_host->getInt(args[1]);

    if (!g_host->hasFunction(ctx, funcName)) {
        g_host->raiseError(ctx, "PluginError",
                           (std::string("__native__hello_apply_twice: no such function '")
                            + funcName + "'").c_str());
        return g_host->makeNull(ctx);
    }

    /* First call: f(v) */
    MTypeValue* a0 = g_host->makeInt(ctx, v);
    const MTypeValue* call1args[] = { a0 };
    MTypeValue* r1 = g_host->callFunction(ctx, funcName, call1args, 1);
    if (g_host->getTag(r1) != MT_TAG_INT) {
        g_host->raiseError(ctx, "PluginError",
                           "__native__hello_apply_twice: callee did not return int");
        return g_host->makeNull(ctx);
    }

    /* Second call: f(f(v)) */
    const MTypeValue* call2args[] = { r1 };
    MTypeValue* r2 = g_host->callFunction(ctx, funcName, call2args, 1);
    if (g_host->getTag(r2) != MT_TAG_INT) {
        g_host->raiseError(ctx, "PluginError",
                           "__native__hello_apply_twice: callee did not return int (2nd call)");
        return g_host->makeNull(ctx);
    }
    return r2;
}

void countCb(void* userData, const char* /*name*/) {
    ++(*static_cast<int64_t*>(userData));
}

MTypeValue* countNatives(void*, MTypeContext* ctx,
                         const MTypeValue* const*, int argc)
{
    if (argc != 0) {
        g_host->raiseError(ctx, "PluginError", "__native__hello_count_natives: expected no args");
        return g_host->makeNull(ctx);
    }
    int64_t total = 0;
    g_host->listFunctions(ctx, &countCb, &total);
    return g_host->makeInt(ctx, total);
}
}  // namespace

extern "C" MTYPE_PLUGIN_EXPORT
int mtype_plugin_register(uint32_t hostAbiVersion,
                          const MTypePluginHost* host,
                          MTypeContext* registrationCtx)
{
    if (hostAbiVersion != MTYPE_PLUGIN_ABI_VERSION) {
        return 1;  /* ABI mismatch — host will close us. */
    }
    g_host = host;
    host->registerFunction(registrationCtx, "__native__hello_greet",         &greet,        nullptr);
    host->registerFunction(registrationCtx, "__native__hello_apply_twice",   &applyTwice,   nullptr);
    host->registerFunction(registrationCtx, "__native__hello_count_natives", &countNatives, nullptr);
    return 0;
}
