#include "ASTExecutionStrategy.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../runtime/EventLoop.hpp"
#include <thread>
#include <atomic>
#include <chrono>

namespace services
{
    ASTExecutionStrategy::ASTExecutionStrategy(evaluator::Evaluator* eval)
        : evaluator(eval)
    {
    }

    value::Value ASTExecutionStrategy::execute(ast::ASTNode* ast)
    {
        // For AST interpreter, we run the EventLoop in the background
        // and execute the script directly. The await handler uses busy-wait
        // to poll for promise fulfillment, allowing the EventLoop to process
        // delayed tasks in the background.

        auto* eventLoop = evaluator->getEventLoop();

        if (eventLoop)
        {
            return executeWithEventLoop(ast, eventLoop);
        }
        else
        {
            // No event loop - execute directly
            return evaluator->evaluate(ast);
        }
    }

    value::Value ASTExecutionStrategy::executeWithEventLoop(ast::ASTNode* ast, runtime::EventLoop* eventLoop)
    {
        // Start the event loop in a background thread with proper shutdown protocol
        std::atomic<bool> shutdownRequested(false);
        std::thread eventLoopThread([eventLoop, &shutdownRequested]() {
            while (true)
            {
                // Process all available tasks
                bool hasWork = eventLoop->tick();

                // Check shutdown condition AFTER processing current tasks
                // Use acquire semantics to ensure visibility of all writes before shutdown request
                if (shutdownRequested.load(std::memory_order_acquire))
                {
                    // Final drain: Process any remaining tasks that were added during shutdown
                    // This prevents the race condition where tasks are added after the check
                    while (eventLoop->tick()) { }
                    break;
                }

                if (!hasWork)
                {
                    // No tasks right now, sleep briefly to avoid busy-waiting
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });

        // Execute the AST in the main thread
        // await will use busy-wait polling while EventLoop processes delayed tasks
        value::Value result;
        try
        {
            result = evaluator->evaluate(ast);
        }
        catch (...)
        {
            // Signal the event loop to stop with release semantics
            // This ensures all writes are visible to the event loop thread
            shutdownRequested.store(true, std::memory_order_release);
            if (eventLoopThread.joinable())
            {
                eventLoopThread.join();
            }
            throw;
        }

        // Request shutdown and wait for clean drain
        // Use release semantics to ensure all writes are visible to the event loop thread
        shutdownRequested.store(true, std::memory_order_release);
        if (eventLoopThread.joinable())
        {
            eventLoopThread.join();
        }

        return result;
    }
}
