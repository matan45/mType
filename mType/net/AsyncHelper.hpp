#pragma once

#include "../value/AsyncPromiseValue.hpp"
#include "../runtime/EventLoop.hpp"

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <exception>

namespace net
{
    // Spawn a worker thread that performs `work()`, then posts the result back
    // to the event loop where it resolves/rejects the returned promise on the
    // VM thread. `convert` turns the worker's typed result into a value::Value.
    //
    // If `work()` throws, the exception's what() is used as the rejection
    // reason. Callers should prefix the message with "dns:", "timeout:", or
    // "connection:" so NetErrors can map it to the right mType exception class.
    template<typename T>
    std::shared_ptr<value::AsyncPromiseValue> runAsync(
        ::runtime::EventLoop* eventLoop,
        std::function<T()> work,
        std::function<value::Value(T)> convert)
    {
        auto promise = std::make_shared<value::AsyncPromiseValue>();

        std::thread([promise, eventLoop, work = std::move(work), convert = std::move(convert)]() {
            try
            {
                T result = work();
                value::Value v = convert(std::move(result));
                if (eventLoop)
                {
                    eventLoop->post([promise, v]() {
                        try { promise->resolve(v); } catch (...) {}
                    });
                }
                else
                {
                    try { promise->resolve(v); } catch (...) {}
                }
            }
            catch (const std::exception& e)
            {
                std::string msg = e.what();
                if (eventLoop)
                {
                    eventLoop->post([promise, msg]() {
                        try { promise->reject(msg); } catch (...) {}
                    });
                }
                else
                {
                    try { promise->reject(msg); } catch (...) {}
                }
            }
        }).detach();

        return promise;
    }

    // Variant for void-returning work.
    inline std::shared_ptr<value::AsyncPromiseValue> runAsyncVoid(
        ::runtime::EventLoop* eventLoop,
        std::function<void()> work)
    {
        auto promise = std::make_shared<value::AsyncPromiseValue>();

        std::thread([promise, eventLoop, work = std::move(work)]() {
            try
            {
                work();
                value::Value v = nullptr;
                if (eventLoop)
                {
                    eventLoop->post([promise, v]() {
                        try { promise->resolve(v); } catch (...) {}
                    });
                }
                else
                {
                    try { promise->resolve(v); } catch (...) {}
                }
            }
            catch (const std::exception& e)
            {
                std::string msg = e.what();
                if (eventLoop)
                {
                    eventLoop->post([promise, msg]() {
                        try { promise->reject(msg); } catch (...) {}
                    });
                }
                else
                {
                    try { promise->reject(msg); } catch (...) {}
                }
            }
        }).detach();

        return promise;
    }
}
