#pragma once
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include <memory>
#include <vector>

namespace vm::runtime { class VirtualMachine; }

namespace net
{
    class NetNatives
    {
    public:
        // Register all __net_* functions in the environment.
        // Call once at startup, after JsonNatives::registerAll.
        static void registerAll(std::shared_ptr<environment::Environment> env);

        // Provide the active VM so async natives can reach the EventLoop and
        // invoke mType callbacks (TcpServer.onConnection) on the VM thread.
        // Call from ScriptInterpreter once VM is constructed.
        static void setVM(std::shared_ptr<vm::runtime::VirtualMachine> vm);

        // Process-exit cleanup: close all open sockets/servers and join threads.
        static void cleanup();

        static std::shared_ptr<environment::Environment> getEnvironment() { return currentEnvironment; }
        static std::shared_ptr<vm::runtime::VirtualMachine> getVM() { return currentVM; }

    private:
        // ---- HTTP -------------------------------------------------------
        static value::Value __net_http_sendAsync(const std::vector<value::Value>& args);

        // ---- DNS --------------------------------------------------------
        static value::Value __net_dns_resolveAsync(const std::vector<value::Value>& args);

        // ---- TCP socket -------------------------------------------------
        static value::Value __net_socket_create(const std::vector<value::Value>& args);
        static value::Value __net_socket_connectAsync(const std::vector<value::Value>& args);
        static value::Value __net_socket_sendAsync(const std::vector<value::Value>& args);
        static value::Value __net_socket_receiveAsync(const std::vector<value::Value>& args);
        static value::Value __net_socket_close(const std::vector<value::Value>& args);
        static value::Value __net_socket_isConnected(const std::vector<value::Value>& args);
        static value::Value __net_socket_setTimeout(const std::vector<value::Value>& args);

        // ---- TCP server -------------------------------------------------
        static value::Value __net_tcpserver_create(const std::vector<value::Value>& args);
        static value::Value __net_tcpserver_listen(const std::vector<value::Value>& args);
        static value::Value __net_tcpserver_onConnection(const std::vector<value::Value>& args);
        static value::Value __net_tcpserver_onError(const std::vector<value::Value>& args);
        static value::Value __net_tcpserver_stop(const std::vector<value::Value>& args);

        // ---- helpers ----------------------------------------------------
        static void validateArgCount(const std::vector<value::Value>& args, size_t expected,
                                     const std::string& funcName);
        static std::string extractString(const value::Value& arg, const std::string& funcName);
        static int64_t extractInt(const value::Value& arg, const std::string& funcName);

        static std::shared_ptr<environment::Environment> currentEnvironment;
        static std::shared_ptr<vm::runtime::VirtualMachine> currentVM;
    };
}
