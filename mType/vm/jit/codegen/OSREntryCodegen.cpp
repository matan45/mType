#include "OSREntryCodegen.hpp"
#include <cstdint>

namespace vm::jit::codegen
{
    using namespace asmjit;
    using namespace asmjit::x86;
    constexpr size_t valueSize = sizeof(value::Value);

    static void emitBoxedSlotLoad(Compiler& cc,
                                   const OSREntryCodegen::EntryInfo& info,
                                   const Gp& srcAddr,
                                   size_t slot,
                                   SlotType type)
    {
        if (isBoxedSlotType(type))
        {
            // Boxed local: copy full Value
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(info.localsBase, static_cast<int32_t>(slot * info.localStride)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, srcAddr);
        }
        else if (type == SlotType::FLOAT)
        {
            // Unbox float, then box into Value-sized local
            InvokeNode* unbox;
            cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_float),
                      FuncSignature::build<double, const value::Value*>());
            unbox->set_arg(0, srcAddr);
            Vec val = cc.new_xmm();
            unbox->set_ret(0, val);

            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(info.localsBase, static_cast<int32_t>(slot * info.localStride)));
            InvokeNode* box;
            cc.invoke(Out(box), reinterpret_cast<uint64_t>(jit_box_float),
                      FuncSignature::build<void, value::Value*, double>());
            box->set_arg(0, destAddr);
            box->set_arg(1, val);
        }
        else
        {
            // Unbox int/bool, then box into Value-sized local
            InvokeNode* unbox;
            cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                      FuncSignature::build<int64_t, const value::Value*>());
            unbox->set_arg(0, srcAddr);
            Gp val = cc.new_gp64();
            unbox->set_ret(0, val);

            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(info.localsBase, static_cast<int32_t>(slot * info.localStride)));
            uint64_t boxFn = (type == SlotType::BOOL)
                ? reinterpret_cast<uint64_t>(jit_box_bool)
                : reinterpret_cast<uint64_t>(jit_box_int);
            InvokeNode* box;
            cc.invoke(Out(box), boxFn,
                      FuncSignature::build<void, value::Value*, int64_t>());
            box->set_arg(0, destAddr);
            box->set_arg(1, val);
        }
    }

    static void emitNonBoxedSlotLoad(Compiler& cc,
                                      const OSREntryCodegen::EntryInfo& info,
                                      const Gp& srcAddr,
                                      size_t slot,
                                      SlotType type)
    {
        if (type == SlotType::FLOAT)
        {
            InvokeNode* unbox;
            cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_float),
                      FuncSignature::build<double, const value::Value*>());
            unbox->set_arg(0, srcAddr);
            Vec val = cc.new_xmm();
            unbox->set_ret(0, val);
            cc.movsd(Mem(info.localsBase, static_cast<int32_t>(slot * 8)), val);
        }
        else
        {
            InvokeNode* unbox;
            cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                      FuncSignature::build<int64_t, const value::Value*>());
            unbox->set_arg(0, srcAddr);
            Gp val = cc.new_gp64();
            unbox->set_ret(0, val);
            cc.mov(Mem(info.localsBase, static_cast<int32_t>(slot * 8)), val);
        }
    }

    void OSREntryCodegen::emitStateLoad(Compiler& cc,
                                         const EntryInfo& info,
                                         const std::vector<LocalSlotInfo>& localSlotInfos,
                                         std::unordered_map<size_t, SlotType>& localTypes)
    {
        // Load osrLocals pointer from JitContext
        Gp osrLocalsPtr = cc.new_gp64("osrLocalsPtr");
        cc.mov(osrLocalsPtr, Mem(info.ctxPtr, offsetof(JitContext, osrLocals)));

        for (const auto& slotInfo : localSlotInfos)
        {
            size_t slot = slotInfo.slot;
            SlotType type = slotInfo.type;
            localTypes[slot] = type;

            // Source address: osrLocals[slot]
            Gp srcAddr = cc.new_gp64();
            cc.lea(srcAddr, Mem(osrLocalsPtr, static_cast<int32_t>(slot * valueSize)));

            if (info.usesBoxedTypes)
            {
                emitBoxedSlotLoad(cc, info, srcAddr, slot, type);
            }
            else
            {
                emitNonBoxedSlotLoad(cc, info, srcAddr, slot, type);
            }
        }
    }
}
