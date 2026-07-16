#pragma once

#include "../value/AsyncPromiseValue.hpp"
#include "../runtime/EventLoop.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
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

    }

    // Spawn a worker thread that performs `work()`, then posts the result back
    // to the event loop where it resolves/rejects the returned promise on the
    // VM thread. The worker crosses the boundary with T / std::string only;
    // `convert` and Promise settlement both run after EventLoop::tick() drains
    // the completion on the owner thread.
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
        if (!eventLoop)
        {
            detail::settleReject(promise, "async operation requires an EventLoop");
            return promise;
        }

        std::weak_ptr<value::AsyncPromiseValue> weakPromise = promise;
        auto postHandle = eventLoop->getPostHandle();

        const bool launched = eventLoop->launchWorker(
            [weakPromise, postHandle, work = std::move(work),
             convert = std::move(convert)]() mutable {
            try
            {
                T result = work();
                postHandle.post(
                    [weakPromise, result = std::move(result),
                     convert = std::move(convert)]() mutable {
                        auto lockedPromise = weakPromise.lock();
                        if (!lockedPromise) return;
                        try
                        {
                            value::Value value = convert(std::move(result));
                            detail::settleResolve(lockedPromise, value);
                        }
                        catch (const std::exception& exception)
                        {
                            detail::settleReject(lockedPromise, exception.what());
                        }
                        catch (...)
                        {
                            detail::settleReject(
                                lockedPromise, "unknown async conversion error");
                        }
                    });
            }
            catch (const std::exception& e)
            {
                const std::string error = e.what();
                postHandle.post([weakPromise, error]() {
                    if (auto lockedPromise = weakPromise.lock())
                        detail::settleReject(lockedPromise, error);
                });
            }
            catch (...)
            {
                postHandle.post([weakPromise]() {
                    if (auto lockedPromise = weakPromise.lock())
                        detail::settleReject(
                            lockedPromise, "unknown async worker error");
                });
            }
        });

        if (!launched)
        {
            detail::settleReject(promise, "EventLoop is shutting down");
        }

        return promise;
    }

    // Variant for void-returning work.
    inline std::shared_ptr<value::AsyncPromiseValue> runAsyncVoid(
        ::runtime::EventLoop* eventLoop,
        std::function<void()> work)
    {
        auto promise = std::make_shared<value::AsyncPromiseValue>();
        if (!eventLoop)
        {
            detail::settleReject(promise, "async operation requires an EventLoop");
            return promise;
        }

        std::weak_ptr<value::AsyncPromiseValue> weakPromise = promise;
        auto postHandle = eventLoop->getPostHandle();

        const bool launched = eventLoop->launchWorker(
            [weakPromise, postHandle, work = std::move(work)]() mutable {
            try
            {
                work();
                postHandle.post([weakPromise]() {
                    if (auto lockedPromise = weakPromise.lock())
                    {
                        value::Value value = nullptr;
                        detail::settleResolve(lockedPromise, value);
                    }
                });
            }
            catch (const std::exception& e)
            {
                const std::string error = e.what();
                postHandle.post([weakPromise, error]() {
                    if (auto lockedPromise = weakPromise.lock())
                        detail::settleReject(lockedPromise, error);
                });
            }
            catch (...)
            {
                postHandle.post([weakPromise]() {
                    if (auto lockedPromise = weakPromise.lock())
                        detail::settleReject(
                            lockedPromise, "unknown async worker error");
                });
            }
        });

        if (!launched)
        {
            detail::settleReject(promise, "EventLoop is shutting down");
        }

        return promise;
    }
}
