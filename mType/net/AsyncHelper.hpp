#pragma once

#include "../value/AsyncPromiseValue.hpp"
#include "../runtime/EventLoop.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <exception>

namespace net
{
    namespace detail
    {
        inline void settleResolve(const std::shared_ptr<value::AsyncPromiseValue>& promise,
                                  const value::Value& v)
        {
            try { promise->resolve(v); }
            catch (const std::exception& e) {
                std::cerr << "[net] promise resolve failed: " << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "[net] promise resolve failed: unknown error" << std::endl;
            }
        }

        inline void settleReject(const std::shared_ptr<value::AsyncPromiseValue>& promise,
                                 const std::string& msg)
        {
            try { promise->reject(msg); }
            catch (const std::exception& e) {
                std::cerr << "[net] promise reject failed: " << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "[net] promise reject failed: unknown error" << std::endl;
            }
        }

        inline void settleOnLoopOrInline(::runtime::EventLoop* loop,
                                         const std::shared_ptr<value::AsyncPromiseValue>& promise,
                                         const value::Value& v,
                                         bool resolve,
                                         const std::string& rejectMsg)
        {
            if (loop)
            {
                if (resolve)
                {
                    loop->post([promise, v]() { settleResolve(promise, v); });
                }
                else
                {
                    loop->post([promise, rejectMsg]() { settleReject(promise, rejectMsg); });
                }
            }
            else
            {
                if (resolve) settleResolve(promise, v);
                else settleReject(promise, rejectMsg);
            }
        }
    }

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
                detail::settleOnLoopOrInline(eventLoop, promise, v, /*resolve=*/true, {});
            }
            catch (const std::exception& e)
            {
                detail::settleOnLoopOrInline(eventLoop, promise, {}, /*resolve=*/false, e.what());
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
                detail::settleOnLoopOrInline(eventLoop, promise, v, /*resolve=*/true, {});
            }
            catch (const std::exception& e)
            {
                detail::settleOnLoopOrInline(eventLoop, promise, {}, /*resolve=*/false, e.what());
            }
        }).detach();

        return promise;
    }
}
