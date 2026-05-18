#include "ExecutorIsolationTestSuite.hpp"

#include "../../vm/runtime/context/ExecutionContext.hpp"
#include "../../vm/runtime/stack/StackManager.hpp"
#include "../../vm/runtime/executors/StackOperationsExecutor.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../vm/bytecode/OpCode.hpp"
#include "../../value/ValueShim.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace tests::testSuite
{
    using namespace testFramework;
    using services::ScriptAPI;
    using namespace vm::runtime;

    namespace
    {
        void require(bool cond, const std::string& msg)
        {
            if (!cond)
            {
                throw std::runtime_error(msg);
            }
        }

        // Build a minimal harness: a BytecodeProgram with a single integer
        // constant in the pool plus a hand-crafted PUSH_INT instruction, a
        // StackManager, an empty call stack, an IP, and the diagnostic refs
        // ExecutionContext demands. No Environment. No VirtualMachine.
        struct MinimalHarness
        {
            vm::bytecode::BytecodeProgram program;
            std::shared_ptr<StackManager> stackManager;
            std::vector<CallFrame> callStack;
            size_t ip = 0;
            ExecutionStats stats{};
            std::chrono::steady_clock::time_point execStart{};
            bool debugEnabled = false;
            std::string sourceFile;
            int sourceLine = 0;

            ExecutionContext makeContext()
            {
                return ExecutionContext(
                    &program, ip, callStack, /*maxStackDepth=*/16,
                    stackManager, stats, execStart,
                    debugEnabled, sourceFile, sourceLine);
            }
        };
    }

    void ExecutorIsolationTestSuite::setupTests()
    {
        // Construct StackOperationsExecutor against a minimal ExecutionContext.
        // Before the ExecutionContext deepening, this would not have compiled —
        // the constructor required a shared Environment and a VirtualMachine
        // back-pointer. The fact that this builds at all is the load-bearing
        // proof.
        addCallbackTest("StackOperationsExecutor isolated PUSH_INT round-trip",
            "",
            [](ScriptAPI&) {
                MinimalHarness h;
                h.stackManager = std::make_shared<StackManager>();
                const size_t constIdx = h.program.getConstantPool().addInteger(42);

                auto ctx = h.makeContext();
                StackOperationsExecutor exec(ctx);

                vm::bytecode::BytecodeProgram::Instruction instr(
                    vm::bytecode::OpCode::PUSH_INT, constIdx);
                exec.handlePushInt(instr);

                require(h.stackManager->size() == 1,
                    "stack size 1 after PUSH_INT");
                require(value::asInt(h.stackManager->peek()) == 42,
                    "PUSH_INT pushed 42");
            });

        addCallbackTest("StackOperationsExecutor isolated DUP",
            "",
            [](ScriptAPI&) {
                MinimalHarness h;
                h.stackManager = std::make_shared<StackManager>();
                auto ctx = h.makeContext();
                StackOperationsExecutor exec(ctx);

                h.stackManager->push(static_cast<int64_t>(7));
                exec.handleDup();

                require(h.stackManager->size() == 2, "stack size 2 after DUP");
                require(value::asInt(h.stackManager->peek(0)) == 7, "top is 7");
                require(value::asInt(h.stackManager->peek(1)) == 7, "second is 7");
            });

        addCallbackTest("StackOperationsExecutor isolated SWAP",
            "",
            [](ScriptAPI&) {
                MinimalHarness h;
                h.stackManager = std::make_shared<StackManager>();
                auto ctx = h.makeContext();
                StackOperationsExecutor exec(ctx);

                h.stackManager->push(static_cast<int64_t>(1));
                h.stackManager->push(static_cast<int64_t>(2));
                exec.handleSwap();

                require(value::asInt(h.stackManager->peek(0)) == 1, "top is 1 after SWAP");
                require(value::asInt(h.stackManager->peek(1)) == 2, "second is 2 after SWAP");
            });
    }
}
