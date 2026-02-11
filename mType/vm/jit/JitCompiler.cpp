#include "JitCompiler.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/asmjit.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    JitCompiler::JitCompiler()
    {
    }

    const std::unordered_set<uint8_t>& JitCompiler::getSupportedOpcodes()
    {
        static const std::unordered_set<uint8_t> supported = {
            // Stack operations
            static_cast<uint8_t>(OpCode::PUSH_INT),
            static_cast<uint8_t>(OpCode::PUSH_BOOL),
            static_cast<uint8_t>(OpCode::POP),
            static_cast<uint8_t>(OpCode::DUP),

            // Integer arithmetic
            static_cast<uint8_t>(OpCode::ADD_INT),
            static_cast<uint8_t>(OpCode::SUB_INT),
            static_cast<uint8_t>(OpCode::MUL_INT),
            static_cast<uint8_t>(OpCode::DIV_INT),
            static_cast<uint8_t>(OpCode::NEG),
            static_cast<uint8_t>(OpCode::INC),
            static_cast<uint8_t>(OpCode::DEC),

            // Comparisons (treated as int comparisons in Phase 2)
            static_cast<uint8_t>(OpCode::EQ),
            static_cast<uint8_t>(OpCode::NE),
            static_cast<uint8_t>(OpCode::LT),
            static_cast<uint8_t>(OpCode::GT),
            static_cast<uint8_t>(OpCode::LE),
            static_cast<uint8_t>(OpCode::GE),
            static_cast<uint8_t>(OpCode::EQ_INT),
            static_cast<uint8_t>(OpCode::NE_INT),
            static_cast<uint8_t>(OpCode::LT_INT),
            static_cast<uint8_t>(OpCode::GT_INT),

            // Logical
            static_cast<uint8_t>(OpCode::NOT),

            // Variable operations (local only)
            static_cast<uint8_t>(OpCode::LOAD_LOCAL),
            static_cast<uint8_t>(OpCode::STORE_LOCAL),

            // Control flow
            static_cast<uint8_t>(OpCode::JUMP),
            static_cast<uint8_t>(OpCode::JUMP_IF_FALSE),
            static_cast<uint8_t>(OpCode::JUMP_IF_TRUE),
            static_cast<uint8_t>(OpCode::JUMP_BACK),
            static_cast<uint8_t>(OpCode::RETURN),
            static_cast<uint8_t>(OpCode::RETURN_VALUE),

            // No-ops in JIT (debug/profiling markers)
            static_cast<uint8_t>(OpCode::LINE),
            static_cast<uint8_t>(OpCode::SOURCE_FILE),
            static_cast<uint8_t>(OpCode::NOP),
            static_cast<uint8_t>(OpCode::LOOP_START),
            static_cast<uint8_t>(OpCode::LOOP_END),
            static_cast<uint8_t>(OpCode::PROFILE_ENTER),
            static_cast<uint8_t>(OpCode::PROFILE_EXIT),
        };
        return supported;
    }

    bool JitCompiler::canCompile(const bytecode::BytecodeProgram::FunctionMetadata& meta,
                                  const bytecode::BytecodeProgram& program) const
    {
        if (meta.isNative || meta.isAsync)
        {
            return false;
        }

        const auto& supported = getSupportedOpcodes();
        size_t endOffset = meta.startOffset + meta.instructionCount;

        for (size_t ip = meta.startOffset; ip < endOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (supported.find(static_cast<uint8_t>(instr.opcode)) == supported.end())
            {
                return false;
            }
        }

        return true;
    }

    bool JitCompiler::compile(const std::string& functionName,
                               const bytecode::BytecodeProgram& program,
                               JitCodeCache& codeCache)
    {
        // Already compiled?
        if (codeCache.contains(functionName))
        {
            return true;
        }

        const auto* funcMeta = program.getFunction(functionName);
        if (!funcMeta)
        {
            bailoutCount++;
            return false;
        }

        if (!canCompile(*funcMeta, program))
        {
            bailoutCount++;
            return false;
        }

        // ---- Begin code generation ----
        CodeHolder code;
        code.init(codeCache.getRuntime().environment());

        Compiler cc(&code);

        // Function signature: void jitFunc(JitContext*)
        FuncNode* func = cc.add_func(FuncSignature::build<void, JitContext*>());

        // Get JitContext pointer parameter
        Gp ctxPtr = cc.new_gp64("ctx");
        func->set_arg(0, ctxPtr);

        // Allocate locals array on native stack (int64_t[localCount])
        size_t localCount = funcMeta->localCount;
        if (localCount == 0) localCount = 1; // Minimum 1 slot to avoid zero-size alloc
        Mem localsArea = cc.new_stack(static_cast<uint32_t>(localCount * 8), 8);
        Gp localsBase = cc.new_gp64("localsBase");
        cc.lea(localsBase, localsArea);

        // Allocate operand stack on native stack (int64_t[maxStackDepth])
        // Use a reasonable maximum; actual usage is tracked at compile time
        constexpr size_t MAX_OP_STACK = 64;
        Mem stackArea = cc.new_stack(MAX_OP_STACK * 8, 8);
        Gp stackBase = cc.new_gp64("stackBase");
        cc.lea(stackBase, stackArea);

        // ---- Copy arguments from JitContext into locals ----
        // ctx->args is a Value* array. We call jit_unbox_int for each arg.
        {
            Gp argsPtr = cc.new_gp64("argsPtr");
            cc.mov(argsPtr, Mem(ctxPtr, offsetof(JitContext, args)));

            // sizeof(value::Value) - need to know this for indexing
            // We'll compute it at compile time
            constexpr size_t valueSize = sizeof(value::Value);

            for (size_t i = 0; i < funcMeta->parameterCount; ++i)
            {
                // Call jit_unbox_int(&args[i])
                Gp argAddr = cc.new_gp64();
                cc.lea(argAddr, Mem(argsPtr, static_cast<int32_t>(i * valueSize)));

                InvokeNode* invokeUnbox;
                cc.invoke(Out(invokeUnbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                          FuncSignature::build<int64_t, const value::Value*>());
                invokeUnbox->set_arg(0, argAddr);

                Gp argVal = cc.new_gp64();
                invokeUnbox->set_ret(0, argVal);

                // Store into locals[i]
                cc.mov(Mem(localsBase, static_cast<int32_t>(i * 8)), argVal);
            }
        }

        // Initialize remaining locals to 0
        for (size_t i = funcMeta->parameterCount; i < funcMeta->localCount; ++i)
        {
            cc.mov(Mem(localsBase, static_cast<int32_t>(i * 8)), 0);
        }

        // ---- Create labels for all instruction offsets (for jump targets) ----
        size_t startOffset = funcMeta->startOffset;
        size_t instrCount = funcMeta->instructionCount;
        std::unordered_map<size_t, Label> labels; // bytecode IP -> label

        // First pass: create labels for all jump targets
        for (size_t ip = startOffset; ip < startOffset + instrCount; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                case OpCode::JUMP:
                case OpCode::JUMP_IF_FALSE:
                case OpCode::JUMP_IF_TRUE:
                case OpCode::JUMP_BACK:
                    if (!instr.operands.empty())
                    {
                        size_t target = instr.operands[0];
                        if (labels.find(target) == labels.end())
                        {
                            labels[target] = cc.new_label();
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        // ---- Second pass: emit native code ----
        int stackDepth = 0; // Compile-time operand stack depth tracker

        for (size_t ip = startOffset; ip < startOffset + instrCount; ++ip)
        {
            // Bind label if this instruction is a jump target
            auto labelIt = labels.find(ip);
            if (labelIt != labels.end())
            {
                cc.bind(labelIt->second);
            }

            const auto& instr = program.getInstruction(ip);

            switch (instr.opcode)
            {
                // ===== No-ops =====
                case OpCode::LINE:
                case OpCode::SOURCE_FILE:
                case OpCode::NOP:
                case OpCode::LOOP_START:
                case OpCode::LOOP_END:
                case OpCode::PROFILE_ENTER:
                case OpCode::PROFILE_EXIT:
                    break;

                // ===== Stack Operations =====
                case OpCode::PUSH_INT:
                {
                    int64_t val = program.getConstantPool().getInteger(instr.operands[0]);
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, val);
                    cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    stackDepth++;
                    break;
                }

                case OpCode::PUSH_BOOL:
                {
                    int64_t val = (instr.operands[0] != 0) ? 1 : 0;
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, val);
                    cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    stackDepth++;
                    break;
                }

                case OpCode::POP:
                {
                    stackDepth--;
                    break;
                }

                case OpCode::DUP:
                {
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    stackDepth++;
                    break;
                }

                // ===== Integer Arithmetic =====
                case OpCode::ADD_INT:
                {
                    stackDepth--;
                    Gp right = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.add(Mem(stackBase, (stackDepth - 1) * 8), right);
                    break;
                }

                case OpCode::SUB_INT:
                {
                    stackDepth--;
                    Gp right = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.sub(Mem(stackBase, (stackDepth - 1) * 8), right);
                    break;
                }

                case OpCode::MUL_INT:
                {
                    stackDepth--;
                    Gp left = cc.new_gp64();
                    Gp right = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.imul(left, right);
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), left);
                    break;
                }

                case OpCode::DIV_INT:
                {
                    stackDepth--;
                    Gp right = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));

                    // Check for division by zero
                    cc.test(right, right);
                    Label notZero = cc.new_label();
                    cc.jnz(notZero);

                    // Call jit_throw_div_by_zero() - this throws and never returns
                    InvokeNode* invokeDivZero;
                    cc.invoke(Out(invokeDivZero), reinterpret_cast<uint64_t>(jit_throw_div_by_zero),
                              FuncSignature::build<void>());

                    cc.bind(notZero);
                    Gp left = cc.new_gp64();
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    // x86 idiv: rax = rdx:rax / operand
                    Gp raxReg = cc.new_gp64();
                    Gp rdxReg = cc.new_gp64();
                    cc.mov(raxReg, left);
                    // Sign-extend rax to rdx:rax
                    cc.cqo(rdxReg, raxReg);
                    cc.idiv(rdxReg, raxReg, right);
                    // Result (quotient) is in raxReg
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), raxReg);
                    break;
                }

                case OpCode::NEG:
                {
                    cc.neg(Mem(stackBase, (stackDepth - 1) * 8));
                    break;
                }

                case OpCode::INC:
                {
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.inc(tmp);
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), tmp);
                    break;
                }

                case OpCode::DEC:
                {
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.dec(tmp);
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), tmp);
                    break;
                }

                // ===== Comparisons =====
                // All comparisons: pop two int64_t, push bool (0 or 1)
                case OpCode::EQ:
                case OpCode::EQ_INT:
                {
                    stackDepth--;
                    Gp right = cc.new_gp64();
                    Gp left = cc.new_gp64();
                    Gp result = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.xor_(result, result);
                    cc.cmp(left, right);
                    cc.sete(result.r8());
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
                    break;
                }

                case OpCode::NE:
                case OpCode::NE_INT:
                {
                    stackDepth--;
                    Gp right = cc.new_gp64();
                    Gp left = cc.new_gp64();
                    Gp result = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.xor_(result, result);
                    cc.cmp(left, right);
                    cc.setne(result.r8());
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
                    break;
                }

                case OpCode::LT:
                case OpCode::LT_INT:
                {
                    stackDepth--;
                    Gp right = cc.new_gp64();
                    Gp left = cc.new_gp64();
                    Gp result = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.xor_(result, result);
                    cc.cmp(left, right);
                    cc.setl(result.r8());
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
                    break;
                }

                case OpCode::GT:
                case OpCode::GT_INT:
                {
                    stackDepth--;
                    Gp right = cc.new_gp64();
                    Gp left = cc.new_gp64();
                    Gp result = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.xor_(result, result);
                    cc.cmp(left, right);
                    cc.setg(result.r8());
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
                    break;
                }

                case OpCode::LE:
                {
                    stackDepth--;
                    Gp right = cc.new_gp64();
                    Gp left = cc.new_gp64();
                    Gp result = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.xor_(result, result);
                    cc.cmp(left, right);
                    cc.setle(result.r8());
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
                    break;
                }

                case OpCode::GE:
                {
                    stackDepth--;
                    Gp right = cc.new_gp64();
                    Gp left = cc.new_gp64();
                    Gp result = cc.new_gp64();
                    cc.mov(right, Mem(stackBase, stackDepth * 8));
                    cc.mov(left, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.xor_(result, result);
                    cc.cmp(left, right);
                    cc.setge(result.r8());
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
                    break;
                }

                // ===== Logical =====
                case OpCode::NOT:
                {
                    Gp val = cc.new_gp64();
                    Gp result = cc.new_gp64();
                    cc.mov(val, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.xor_(result, result);
                    cc.test(val, val);
                    cc.sete(result.r8());
                    cc.mov(Mem(stackBase, (stackDepth - 1) * 8), result);
                    break;
                }

                // ===== Variable Operations =====
                case OpCode::LOAD_LOCAL:
                {
                    size_t slot = instr.operands[0];
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(localsBase, static_cast<int32_t>(slot * 8)));
                    cc.mov(Mem(stackBase, stackDepth * 8), tmp);
                    stackDepth++;
                    break;
                }

                case OpCode::STORE_LOCAL:
                {
                    size_t slot = instr.operands[0];
                    // Interpreter behavior: pop value, store to slot, push value back
                    // Net effect: top of stack stays, AND local is updated
                    // In our representation: read top, copy to locals. stackDepth unchanged.
                    Gp tmp = cc.new_gp64();
                    cc.mov(tmp, Mem(stackBase, (stackDepth - 1) * 8));
                    cc.mov(Mem(localsBase, static_cast<int32_t>(slot * 8)), tmp);
                    // stackDepth stays the same (pop + push = no change)
                    break;
                }

                // ===== Control Flow =====
                case OpCode::JUMP:
                {
                    size_t target = instr.operands[0];
                    cc.jmp(labels[target]);
                    break;
                }

                case OpCode::JUMP_IF_FALSE:
                {
                    size_t target = instr.operands[0];
                    stackDepth--;
                    Gp cond = cc.new_gp64();
                    cc.mov(cond, Mem(stackBase, stackDepth * 8));
                    cc.test(cond, cond);
                    cc.jz(labels[target]);
                    break;
                }

                case OpCode::JUMP_IF_TRUE:
                {
                    size_t target = instr.operands[0];
                    stackDepth--;
                    Gp cond = cc.new_gp64();
                    cc.mov(cond, Mem(stackBase, stackDepth * 8));
                    cc.test(cond, cond);
                    cc.jnz(labels[target]);
                    break;
                }

                case OpCode::JUMP_BACK:
                {
                    size_t target = instr.operands[0];

                    // GC safepoint at loop back-edges
                    InvokeNode* invokeGC;
                    cc.invoke(Out(invokeGC), reinterpret_cast<uint64_t>(jit_gc_safepoint),
                              FuncSignature::build<void>());

                    cc.jmp(labels[target]);
                    break;
                }

                case OpCode::RETURN:
                {
                    // No return value - set hasReturnValue = false
                    cc.mov(Mem(ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
                    cc.ret();
                    break;
                }

                case OpCode::RETURN_VALUE:
                {
                    stackDepth--;
                    Gp retVal = cc.new_gp64();
                    cc.mov(retVal, Mem(stackBase, stackDepth * 8));

                    // Call jit_set_return_int(ctx, retVal)
                    InvokeNode* invokeRet;
                    cc.invoke(Out(invokeRet), reinterpret_cast<uint64_t>(jit_set_return_int),
                              FuncSignature::build<void, JitContext*, int64_t>());
                    invokeRet->set_arg(0, ctxPtr);
                    invokeRet->set_arg(1, retVal);

                    cc.ret();
                    break;
                }

                default:
                    // Should not reach here - canCompile() already validated
                    break;
            }
        }

        // End function (in case no explicit RETURN was hit)
        cc.mov(Mem(ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
        cc.end_func();

        // Finalize and add to runtime
        Error err = cc.finalize();
        if (err != Error::kOk)
        {
            bailoutCount++;
            return false;
        }

        JitFunction fn = nullptr;
        err = codeCache.getRuntime().add(&fn, &code);
        if (err != Error::kOk)
        {
            bailoutCount++;
            return false;
        }

        codeCache.store(functionName, fn);
        compileCount++;
        return true;
    }
}
