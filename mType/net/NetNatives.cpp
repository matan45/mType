#include "NetNatives.hpp"
#include <cstddef>
#include <cstdint>

// Platform-specific network implementations
#ifdef _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include "WinHttpClient.hpp"
#   include "WinSocket.hpp"
#   include "WinSockInit.hpp"
#   include <winsock2.h>
#   include <ws2tcpip.h>
    // <windows.h> (pulled in by winsock2.h) defines VOID as a macro for `void`,
    // which collides with our ValueType::VOID enumerator referenced in headers
    // included below. Drop it before any project headers see it.
#   ifdef VOID
#       undef VOID
#   endif
    using HttpClientImpl    = net::WinHttpClient;
    using SocketImpl        = net::WinSocket;
    using SocketServerImpl  = net::WinSocketServer;
#else
#   include "CurlHttpClient.hpp"
#   include "PosixSocket.hpp"
    using HttpClientImpl    = net::CurlHttpClient;
    using SocketImpl        = net::PosixSocket;
    using SocketServerImpl  = net::PosixSocketServer;
#endif

#include "SocketRegistry.hpp"
#include "AsyncHelper.hpp"
#include "NetErrors.hpp"
#include "HashMapMarshal.hpp"

#include "../runtime/EventLoop.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../value/AsyncPromiseValue.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ObjectInstancePool.hpp"
#include "../value/ValueShim.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../errors/RuntimeException.hpp"

#include <iostream>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace net
{
    namespace
    {
        struct ServerCallbacks
        {
            value::Value onConnection;
            value::Value onError;
            bool hasOnConnection = false;
            bool hasOnError = false;
        };

        std::mutex g_serverCallbackMutex;
        std::unordered_map<int, ServerCallbacks> g_serverCallbacks;

        ServerCallbacks getCallbacks(int handle)
        {
            std::lock_guard<std::mutex> lk(g_serverCallbackMutex);
            auto it = g_serverCallbacks.find(handle);
            if (it == g_serverCallbacks.end()) return {};
            return it->second;
        }

        void setOnConnection(int handle, value::Value cb)
        {
            std::lock_guard<std::mutex> lk(g_serverCallbackMutex);
            g_serverCallbacks[handle].onConnection = std::move(cb);
            g_serverCallbacks[handle].hasOnConnection = true;
        }
        void setOnError(int handle, value::Value cb)
        {
            std::lock_guard<std::mutex> lk(g_serverCallbackMutex);
            g_serverCallbacks[handle].onError = std::move(cb);
            g_serverCallbacks[handle].hasOnError = true;
        }
        void clearCallbacks(int handle)
        {
            std::lock_guard<std::mutex> lk(g_serverCallbackMutex);
            g_serverCallbacks.erase(handle);
        }

        void invokeCallback(vm::runtime::VirtualMachine* vm,
                            const value::Value& cb, const value::Value& arg)
        {
            if (!vm) return;

            if (value::isLambda(cb))
            {
                auto lambda = value::asLambda(cb);
                if (!lambda) return;
                try { vm->invokeLambda(lambda, { arg }); }
                catch (const std::exception& e) {
                    std::cerr << "[net] server callback threw: " << e.what() << std::endl;
                }
            }
            else if (value::isObject(cb))
            {
                auto inst = value::asObject(cb);
                if (!inst) return;
                try {
                    const value::Value oneArg[] = { arg };
                    vm->invokeMethod(inst, "accept", std::span<const value::Value>(oneArg));
                }
                catch (const std::exception& e) {
                    std::cerr << "[net] server callback threw: " << e.what() << std::endl;
                }
            }
        }

        value::Value makeTcpSocketInstance(environment::Environment& env, int handle)
        {
            auto classDef = env.findClass("TcpSocket");
            if (!classDef)
                throw errors::RuntimeException("TcpSocket class not loaded");
            auto inst = value::ObjectInstancePool::getInstance().acquire(classDef);
            inst->setField("handle", static_cast<int64_t>(handle));
            return inst;
        }
    }

    void NetNatives::registerAll(std::shared_ptr<environment::Environment> env)
    {
#ifdef _WIN32
        WinSockInit::ensure();
#endif
        auto reg = env->getNativeRegistry();

        reg->registerNativeFunction("__net_http_sendAsync",        { nullptr, __net_http_sendAsync });
        reg->registerNativeFunction("__net_dns_resolveAsync",      { nullptr, __net_dns_resolveAsync });
        reg->registerNativeFunction("__net_socket_create",         { nullptr, __net_socket_create });
        reg->registerNativeFunction("__net_socket_connectAsync",   { nullptr, __net_socket_connectAsync });
        reg->registerNativeFunction("__net_socket_sendAsync",      { nullptr, __net_socket_sendAsync });
        reg->registerNativeFunction("__net_socket_receiveAsync",   { nullptr, __net_socket_receiveAsync });
        reg->registerNativeFunction("__net_socket_close",          { nullptr, __net_socket_close });
        reg->registerNativeFunction("__net_socket_isConnected",    { nullptr, __net_socket_isConnected });
        reg->registerNativeFunction("__net_socket_setTimeout",     { nullptr, __net_socket_setTimeout });
        reg->registerNativeFunction("__net_tcpserver_create",      { nullptr, __net_tcpserver_create });
        reg->registerNativeFunction("__net_tcpserver_listen",      { nullptr, __net_tcpserver_listen });
        reg->registerNativeFunction("__net_tcpserver_onConnection",{ nullptr, __net_tcpserver_onConnection });
        reg->registerNativeFunction("__net_tcpserver_onError",     { nullptr, __net_tcpserver_onError });
        reg->registerNativeFunction("__net_tcpserver_stop",        { nullptr, __net_tcpserver_stop });
    }

    void NetNatives::cleanup()
    {
        {
            std::lock_guard<std::mutex> lk(g_serverCallbackMutex);
            g_serverCallbacks.clear();
        }
        SocketRegistry::instance().cleanup();
    }

    void NetNatives::validateArgCount(std::span<const value::Value> args, size_t expected,
                                      const std::string& funcName)
    {
        if (args.size() != expected)
        {
            throw errors::RuntimeException(funcName + " expects " + std::to_string(expected) +
                                           " args, got " + std::to_string(args.size()));
        }
    }

    std::string NetNatives::extractString(const value::Value& arg, const std::string& funcName)
    {
        if (value::isString(arg)) return value::asString(arg);
        if (value::isInternedString(arg)) return value::asInternedString(arg).getString();
        throw errors::RuntimeException(funcName + ": expected string argument");
    }

    int64_t NetNatives::extractInt(const value::Value& arg, const std::string& funcName)
    {
        if (value::isInt(arg)) return value::asInt(arg);
        throw errors::RuntimeException(funcName + ": expected int argument");
    }

    value::Value NetNatives::__net_http_sendAsync(void* userData, environment::NativeContext& ctx,
                                                  std::span<const value::Value> args)
    {
        validateArgCount(args, 5, "__net_http_sendAsync");
        std::string method    = extractString(args[0], "__net_http_sendAsync");
        std::string url       = extractString(args[1], "__net_http_sendAsync");
        std::string body      = extractString(args[2], "__net_http_sendAsync");
        auto        headers   = hashMapToStdMap(args[3]);
        int64_t     timeoutMs = extractInt(args[4], "__net_http_sendAsync");

        auto env = ctx.env;
        auto vm  = ctx.vm;
        ::runtime::EventLoop* loop = vm ? vm->getEventLoop() : nullptr;

        HttpRequestData req;
        req.method    = method;
        req.url       = url;
        req.body      = body;
        req.headers   = std::move(headers);
        req.timeoutMs = static_cast<int>(timeoutMs);

        auto promise = runAsync<HttpResponseData>(
            loop,
            [req]() {
                HttpClientImpl client;
                return client.send(req);
            },
            [env](HttpResponseData resp) -> value::Value {
                auto classDef = env->findClass("HttpResponse");
                if (!classDef)
                    throw errors::RuntimeException("HttpResponse class not loaded");
                auto inst = value::ObjectInstancePool::getInstance().acquire(classDef);
                inst->setField("status", static_cast<int64_t>(resp.status));
                inst->setField("body",   resp.body);
                inst->setField("headers", stdMapToHashMap(env, resp.headers));
                return inst;
            });

        return value::Value(std::static_pointer_cast<value::PromiseValue>(promise));
    }

    value::Value NetNatives::__net_dns_resolveAsync(void* userData, environment::NativeContext& ctx,
                                                    std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__net_dns_resolveAsync");
        std::string host = extractString(args[0], "__net_dns_resolveAsync");

        auto vm   = ctx.vm;
        ::runtime::EventLoop* loop = vm ? vm->getEventLoop() : nullptr;

        auto promise = runAsync<std::vector<std::string>>(
            loop,
            [host]() -> std::vector<std::string> {
#ifdef _WIN32
                WinSockInit::ensure();
#endif
                addrinfo hints{};
                hints.ai_family   = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;
                addrinfo* res = nullptr;
                int rc = getaddrinfo(host.c_str(), nullptr, &hints, &res);
                if (rc != 0 || !res) throw std::runtime_error("dns error");
                std::vector<std::string> out;
                for (addrinfo* ai = res; ai; ai = ai->ai_next)
                {
                    char buf[INET6_ADDRSTRLEN] = {0};
                    if (ai->ai_family == AF_INET)
                    {
                        inet_ntop(AF_INET,
                                  &reinterpret_cast<sockaddr_in*>(ai->ai_addr)->sin_addr,
                                  buf, sizeof(buf));
                        out.emplace_back(buf);
                    }
                    else if (ai->ai_family == AF_INET6)
                    {
                        inet_ntop(AF_INET6,
                                  &reinterpret_cast<sockaddr_in6*>(ai->ai_addr)->sin6_addr,
                                  buf, sizeof(buf));
                        out.emplace_back(buf);
                    }
                }
                freeaddrinfo(res);
                return out;
            },
            [](std::vector<std::string> addrs) -> value::Value {
                auto arr = std::make_shared<value::NativeArray>(addrs.size(), value::ValueType::STRING);
                for (size_t i = 0; i < addrs.size(); ++i) arr->set(i, addrs[i]);
                return arr;
            });

        return value::Value(std::static_pointer_cast<value::PromiseValue>(promise));
    }

    value::Value NetNatives::__net_socket_create(void* userData, environment::NativeContext& ctx,
                                                 std::span<const value::Value> args)
    {
        validateArgCount(args, 0, "__net_socket_create");
        auto sock = std::make_shared<SocketImpl>();
        return static_cast<int64_t>(SocketRegistry::instance().registerSocket(sock));
    }

    value::Value NetNatives::__net_socket_connectAsync(void* userData, environment::NativeContext& ctx,
                                                       std::span<const value::Value> args)
    {
        validateArgCount(args, 3, "__net_socket_connectAsync");
        int         handle = static_cast<int>(extractInt(args[0], "__net_socket_connectAsync"));
        std::string host   = extractString(args[1], "__net_socket_connectAsync");
        int64_t     port   = extractInt(args[2], "__net_socket_connectAsync");
        auto vm = ctx.vm;
        ::runtime::EventLoop* loop = vm ? vm->getEventLoop() : nullptr;
        auto promise = runAsyncVoid(loop, [handle, host, port]() {
            SocketRegistry::instance().getSocket(handle)->connect(host, static_cast<int>(port));
        });
        return value::Value(std::static_pointer_cast<value::PromiseValue>(promise));
    }

    value::Value NetNatives::__net_socket_sendAsync(void* userData, environment::NativeContext& ctx,
                                                    std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__net_socket_sendAsync");
        int         handle = static_cast<int>(extractInt(args[0], "__net_socket_sendAsync"));
        std::string data   = extractString(args[1], "__net_socket_sendAsync");
        auto vm = ctx.vm;
        ::runtime::EventLoop* loop = vm ? vm->getEventLoop() : nullptr;
        auto promise = runAsync<int>(loop, [handle, data]() {
            return SocketRegistry::instance().getSocket(handle)->send(data);
        }, [](int sent) -> value::Value { return static_cast<int64_t>(sent); });
        return value::Value(std::static_pointer_cast<value::PromiseValue>(promise));
    }

    value::Value NetNatives::__net_socket_receiveAsync(void* userData, environment::NativeContext& ctx,
                                                       std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__net_socket_receiveAsync");
        int     handle   = static_cast<int>(extractInt(args[0], "__net_socket_receiveAsync"));
        int64_t maxBytes = extractInt(args[1], "__net_socket_receiveAsync");
        auto vm = ctx.vm;
        ::runtime::EventLoop* loop = vm ? vm->getEventLoop() : nullptr;
        auto promise = runAsync<std::string>(loop, [handle, maxBytes]() {
            return SocketRegistry::instance().getSocket(handle)->recv(static_cast<int>(maxBytes));
        }, [](std::string s) -> value::Value { return s; });
        return value::Value(std::static_pointer_cast<value::PromiseValue>(promise));
    }

    value::Value NetNatives::__net_socket_close(void* userData, environment::NativeContext& ctx,
                                                std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__net_socket_close");
        SocketRegistry::instance().closeSocket(
            static_cast<int>(extractInt(args[0], "__net_socket_close")));
        return std::monostate{};
    }

    value::Value NetNatives::__net_socket_isConnected(void* userData, environment::NativeContext& ctx,
                                                      std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__net_socket_isConnected");
        try {
            return SocketRegistry::instance()
                .getSocket(static_cast<int>(extractInt(args[0], "__net_socket_isConnected")))
                ->isConnected();
        }
        catch (...) { return false; }
    }

    value::Value NetNatives::__net_socket_setTimeout(void* userData, environment::NativeContext& ctx,
                                                     std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__net_socket_setTimeout");
        SocketRegistry::instance()
            .getSocket(static_cast<int>(extractInt(args[0], "__net_socket_setTimeout")))
            ->setTimeout(static_cast<int>(extractInt(args[1], "__net_socket_setTimeout")));
        return std::monostate{};
    }

    value::Value NetNatives::__net_tcpserver_create(void* userData, environment::NativeContext& ctx,
                                                    std::span<const value::Value> args)
    {
        validateArgCount(args, 0, "__net_tcpserver_create");
        auto srv = std::make_shared<SocketServerImpl>();
        return static_cast<int64_t>(SocketRegistry::instance().registerServer(srv));
    }

    value::Value NetNatives::__net_tcpserver_listen(void* userData, environment::NativeContext& ctx,
                                                    std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__net_tcpserver_listen");
        int     handle = static_cast<int>(extractInt(args[0], "__net_tcpserver_listen"));
        int64_t port   = extractInt(args[1], "__net_tcpserver_listen");
        auto srv = SocketRegistry::instance().getServer(handle);
        auto env = ctx.env;
        auto vm  = ctx.vm;
        ::runtime::EventLoop* loop = vm ? vm->getEventLoop() : nullptr;

        srv->start(static_cast<int>(port),
            [env, vm, loop, handle](uintptr_t clientFd) {
#ifdef _WIN32
                int clientHandle = SocketRegistry::instance().registerSocket(
                    std::make_shared<SocketImpl>(clientFd));
#else
                int clientHandle = SocketRegistry::instance().registerSocket(
                    std::make_shared<SocketImpl>(static_cast<int>(clientFd)));
#endif
                if (loop) loop->post([env, vm, handle, clientHandle]() {
                    auto cbs = getCallbacks(handle);
                    if (cbs.hasOnConnection && vm)
                        invokeCallback(vm.get(), cbs.onConnection,
                                       makeTcpSocketInstance(*env, clientHandle));
                });
            },
            [vm, loop, handle, env](const std::string& errMsg) {
                if (loop) loop->post([vm, handle, errMsg, env]() {
                    auto cbs = getCallbacks(handle);
                    if (cbs.hasOnError && vm)
                    {
                        auto classDef = env->findClass("NetworkException");
                        value::Value excArg = errMsg;
                        if (classDef)
                        {
                            auto inst = value::ObjectInstancePool::getInstance().acquire(classDef);
                            inst->setField("message", errMsg);
                            excArg = inst;
                        }
                        invokeCallback(vm.get(), cbs.onError, excArg);
                    }
                });
            });
        return std::monostate{};
    }

    value::Value NetNatives::__net_tcpserver_onConnection(void* userData,
                                                          environment::NativeContext& ctx,
                                                          std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__net_tcpserver_onConnection");
        setOnConnection(static_cast<int>(extractInt(args[0], "__net_tcpserver_onConnection")),
                        args[1]);
        return std::monostate{};
    }

    value::Value NetNatives::__net_tcpserver_onError(void* userData, environment::NativeContext& ctx,
                                                     std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__net_tcpserver_onError");
        setOnError(static_cast<int>(extractInt(args[0], "__net_tcpserver_onError")), args[1]);
        return std::monostate{};
    }

    value::Value NetNatives::__net_tcpserver_stop(void* userData, environment::NativeContext& ctx,
                                                  std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__net_tcpserver_stop");
        int handle = static_cast<int>(extractInt(args[0], "__net_tcpserver_stop"));
        clearCallbacks(handle);
        SocketRegistry::instance().closeServer(handle);
        return std::monostate{};
    }
}
