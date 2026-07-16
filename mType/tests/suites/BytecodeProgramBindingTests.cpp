#include "BytecodeProgramBindingTests.hpp"

#include "BytecodeOptimizationTestSuite.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../runtime/EventLoop.hpp"
#include "../../services/BytecodeProgramBinding.hpp"
#include "../../services/BytecodeService.hpp"
#include "../../services/ScriptAPI.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../vm/bytecode/OpCode.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace tests::testSuite
{
    namespace
    {
        void require(bool condition, const std::string& message)
        {
            if (!condition) throw std::runtime_error(message);
        }

        vm::bytecode::BytecodeProgram makeHaltProgram()
        {
            vm::bytecode::BytecodeProgram program;
            program.emit(vm::bytecode::OpCode::HALT);
            return program;
        }

        struct TemporaryBytecodeFile
        {
            std::filesystem::path path;

            ~TemporaryBytecodeFile()
            {
                std::error_code ignored;
                std::filesystem::remove(path, ignored);
            }
        };
    }

    void registerBytecodeProgramBindingTests(
        BytecodeOptimizationTestSuite& suite)
    {
        suite.addCallbackTest(
            "Bytecode service keeps executed files owned across replacement", "",
            [](services::ScriptAPI& bootstrapApi) {
                auto environment = bootstrapApi.getEnvironment();
                auto localVm =
                    std::make_shared<vm::runtime::VirtualMachine>(environment);
                services::ScriptAPI localApi(
                    environment, localVm.get(), nullptr);
                services::BytecodeService service(
                    environment, nullptr, localVm, &localApi);
                services::BytecodeProgramBinding::ProgramOwner owner;

                auto serialized = makeHaltProgram();
                const auto serializedId = serialized.getProgramId();
                TemporaryBytecodeFile file{
                    std::filesystem::temp_directory_path()
                    / ("mtype-binding-"
                       + std::to_string(serializedId.value) + ".mtc")};
                {
                    std::ofstream output(file.path, std::ios::binary);
                    require(output.good(),
                        "could not create temporary bytecode artifact");
                    serialized.serialize(output);
                    require(output.good(),
                        "could not serialize temporary bytecode artifact");
                }

                service.runCompiledBytecode(file.path.string(), owner);
                require(owner
                        && localVm->getProgram() == owner.get()
                        && localApi.getBytecodeProgram() == owner.get()
                        && localVm->getStats().instructionsExecuted != 0,
                    "compiled-bytecode execution did not retain its owner");
                const auto firstAddress =
                    reinterpret_cast<uintptr_t>(owner.get());
                const auto firstId = owner->getProgramId();

                auto replacement = makeHaltProgram();
                const auto replacementId = replacement.getProgramId();
                service.runFromProgram(std::move(replacement), owner);
                require(owner
                        && reinterpret_cast<uintptr_t>(owner.get())
                            != firstAddress
                        && owner->getProgramId() == replacementId
                        && owner->getProgramId() != firstId
                        && localVm->getProgram() == owner.get()
                        && localApi.getBytecodeProgram() == owner.get()
                        && !localVm->getLoadedPrograms().empty()
                        && localVm->getLoadedPrograms().front() == owner.get(),
                    "service replacement left an owner/raw-binding mismatch");

                services::BytecodeProgramBinding::release(
                    localVm, &localApi, owner);
                require(!owner && localVm->getProgram() == nullptr
                        && localApi.getBytecodeProgram() == nullptr,
                    "service owner release left a dangling raw binding");
            });

        suite.addCallbackTest(
            "Inactive bytecode owner release preserves the newer binding", "",
            [](services::ScriptAPI& bootstrapApi) {
                auto environment = bootstrapApi.getEnvironment();
                auto localVm =
                    std::make_shared<vm::runtime::VirtualMachine>(environment);
                services::ScriptAPI localApi(
                    environment, localVm.get(), nullptr);
                services::BytecodeProgramBinding::ProgramOwner oldOwner;
                services::BytecodeProgramBinding::ProgramOwner newOwner;

                services::BytecodeProgramBinding::replace(
                    localVm, &localApi, oldOwner,
                    std::make_unique<vm::bytecode::BytecodeProgram>(
                        makeHaltProgram()));
                services::BytecodeProgramBinding::replace(
                    localVm, &localApi, newOwner,
                    std::make_unique<vm::bytecode::BytecodeProgram>(
                        makeHaltProgram()));
                const auto* expected = newOwner.get();

                services::BytecodeProgramBinding::release(
                    localVm, &localApi, oldOwner);
                require(!oldOwner && newOwner
                        && localVm->getProgram() == expected
                        && localApi.getBytecodeProgram() == expected,
                    "releasing an inactive owner tore down the active program");

                services::BytecodeProgramBinding::release(
                    localVm, &localApi, newOwner);
            });

        suite.addCallbackTest(
            "Program replacement cancels the prior EventLoop generation", "",
            [](services::ScriptAPI& bootstrapApi) {
                auto environment = bootstrapApi.getEnvironment();
                auto localVm =
                    std::make_shared<vm::runtime::VirtualMachine>(environment);
                services::ScriptAPI localApi(
                    environment, localVm.get(), nullptr);
                services::BytecodeProgramBinding::ProgramOwner owner;
                services::BytecodeProgramBinding::replace(
                    localVm, &localApi, owner,
                    std::make_unique<vm::bytecode::BytecodeProgram>(
                        makeHaltProgram()));

                bool staleTaskRan = false;
                auto* eventLoop = localVm->ensureEventLoop();
                const size_t staleTask = eventLoop->scheduleTask(
                    [&staleTaskRan]() -> value::Value {
                        staleTaskRan = true;
                        return std::monostate{};
                    });

                services::BytecodeProgramBinding::replace(
                    localVm, &localApi, owner,
                    std::make_unique<vm::bytecode::BytecodeProgram>(
                        makeHaltProgram()));
                eventLoop->run();
                require(!staleTaskRan && !eventLoop->getTask(staleTask),
                    "retired-program EventLoop work survived replacement");

                services::BytecodeProgramBinding::release(
                    localVm, &localApi, owner);
            });

        suite.addCallbackTest(
            "Stale lambdas reject replacement program identities", "",
            [](services::ScriptAPI& bootstrapApi) {
                auto environment = bootstrapApi.getEnvironment();
                auto localVm =
                    std::make_shared<vm::runtime::VirtualMachine>(environment);
                services::ScriptAPI localApi(
                    environment, localVm.get(), nullptr);
                services::BytecodeProgramBinding::ProgramOwner owner;

                auto first = std::make_unique<vm::bytecode::BytecodeProgram>(
                    makeHaltProgram());
                auto lambda = std::make_shared<vm::runtime::BytecodeLambda>();
                lambda->owningProgramId = first->getProgramId();
                lambda->instructionPointer = 0;
                lambda->parameterCount = 0;
                services::BytecodeProgramBinding::replace(
                    localVm, &localApi, owner, std::move(first));

                services::BytecodeProgramBinding::replace(
                    localVm, &localApi, owner,
                    std::make_unique<vm::bytecode::BytecodeProgram>(
                        makeHaltProgram()));
                bool rejected = false;
                try
                {
                    (void)localVm->invokeLambda(lambda, {});
                }
                catch (const errors::RuntimeException& error)
                {
                    rejected = std::string(error.what()).find(
                        "owning bytecode program") != std::string::npos;
                }
                require(rejected,
                    "lambda from a retired program was interpreted as new bytecode");

                services::BytecodeProgramBinding::release(
                    localVm, &localApi, owner);
            });

        suite.addCallbackTest(
            "Direct VM execution synchronizes the main program domain", "",
            [](services::ScriptAPI& bootstrapApi) {
                auto environment = bootstrapApi.getEnvironment();
                auto localVm =
                    std::make_shared<vm::runtime::VirtualMachine>(environment);
                auto first = makeHaltProgram();
                auto second = makeHaltProgram();

                localVm->setProgram(&first);
                (void)localVm->execute(second);
                require(localVm->getProgram() == &second
                        && !localVm->getLoadedPrograms().empty()
                        && localVm->getLoadedPrograms().front() == &second
                        && localVm->getStats().instructionsExecuted != 0,
                    "VirtualMachine::execute left stale program routing state");

                services::BytecodeProgramBinding::clearCurrent(
                    localVm, nullptr);
            });

        suite.addCallbackTest(
            "Script interpreter safely replaces cached bytecode owners", "",
            [](services::ScriptAPI&) {
                services::ScriptInterpreter interpreter;
                auto first = makeHaltProgram();
                const auto firstId = first.getProgramId();
                interpreter.loadFromProgram(std::move(first), false);
                const auto* firstBound = interpreter.getVM()->getProgram();
                require(firstBound && firstBound->getProgramId() == firstId,
                    "interpreter did not retain its first cached program");
                const auto firstAddress =
                    reinterpret_cast<uintptr_t>(firstBound);

                auto second = makeHaltProgram();
                const auto secondId = second.getProgramId();
                interpreter.runFromProgram(std::move(second));
                const auto* secondBound = interpreter.getVM()->getProgram();
                require(secondBound
                        && reinterpret_cast<uintptr_t>(secondBound)
                            != firstAddress
                        && secondBound->getProgramId() == secondId
                        && interpreter.getVM()->getStats().instructionsExecuted != 0,
                    "interpreter cached-program replacement was not durable");
            });
    }
}
