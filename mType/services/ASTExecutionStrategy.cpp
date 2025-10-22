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
        // Start the event loop in a background thread with keep-alive
        std::atomic<bool> scriptRunning(true);
        std::thread eventLoopThread([eventLoop, &scriptRunning]() {
            // Keep the event loop running while the script executes
            while (scriptRunning.load() || eventLoop->getPendingTaskCount() > 0)
            {
                if (!eventLoop->tick())
                {
                    // No tasks right now, sleep briefly
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
            // Signal the event loop to stop
            scriptRunning.store(false);
            if (eventLoopThread.joinable())
            {
                eventLoopThread.join();
            }
            throw;
        }

        // Signal the event loop to stop and wait for it to finish
        scriptRunning.store(false);
        if (eventLoopThread.joinable())
        {
            eventLoopThread.join();
        }

        return result;
    }
}
