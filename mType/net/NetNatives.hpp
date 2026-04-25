#pragma once
#include <span>
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include <memory>
#include <vector>

namespace vm::runtime { class VirtualMachine; }

namespace net
{
    class NetNatives
    {
    public:
        // Register all __net_* functions in the environment.
        static void registerAll(std::shared_ptr<environment::Environment> env);

        // Process-exit cleanup: close all open sockets/servers and join threads.
        static void cleanup();

    private:
        using NativeContext = environment::NativeContext;

        // ---- HTTP -------------------------------------------------------
        static value::Value __net_http_sendAsync(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ---- DNS --------------------------------------------------------
        static value::Value __net_dns_resolveAsync(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ---- TCP socket -------------------------------------------------
        static value::Value __net_socket_create(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_socket_connectAsync(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_socket_sendAsync(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_socket_receiveAsync(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_socket_close(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_socket_isConnected(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_socket_setTimeout(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ---- TCP server -------------------------------------------------
        static value::Value __net_tcpserver_create(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_tcpserver_listen(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_tcpserver_onConnection(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_tcpserver_onError(void* userData, NativeContext& ctx, std::span<const value::Value> args);
        static value::Value __net_tcpserver_stop(void* userData, NativeContext& ctx, std::span<const value::Value> args);

        // ---- helpers ----------------------------------------------------
        static void validateArgCount(std::span<const value::Value> args, size_t expected,
                                     const std::string& funcName);
        static std::string extractString(const value::Value& arg, const std::string& funcName);
        static int64_t extractInt(const value::Value& arg, const std::string& funcName);
    };
}
