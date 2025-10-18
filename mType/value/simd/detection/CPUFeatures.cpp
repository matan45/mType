#include "CPUFeatures.hpp"
#include "../SIMDConfig.hpp"

#ifdef MTYPE_ARCH_X64
#ifdef MTYPE_PLATFORM_WINDOWS
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

namespace mType
{
    namespace value
    {
        namespace simd
        {
            CPUFeatures::CPUFeatures()
            {
                detectFeatures();
            }

            const CPUFeatures& CPUFeatures::instance()
            {
                static CPUFeatures instance;
                return instance;
            }

            void CPUFeatures::detectFeatures()
            {
#ifdef MTYPE_ARCH_X64
                uint32_t eax, ebx, ecx, edx;

                // Check CPUID support and get maximum CPUID level
                cpuid(0, eax, ebx, ecx, edx);
                uint32_t maxCPUIDLevel = eax;  // Save max level for later use
                if (maxCPUIDLevel == 0) return;

                // Feature flags in CPUID.01H
                cpuid(1, eax, ebx, ecx, edx);

                sse2_ = (edx & (1 << 26)) != 0;
                sse4_1_ = (ecx & (1 << 19)) != 0;
                fma_ = (ecx & (1 << 12)) != 0;

                // Check for AVX OS support (XSAVE and AVX bits in XCR0)
                bool osAVXSupport = (ecx & (1 << 27)) != 0 && (ecx & (1 << 28)) != 0;
                if (osAVXSupport && isOSAVXSupported())
                {
                    avx_ = true;

                    // Extended features in CPUID.07H (requires max CPUID level >= 7)
                    if (maxCPUIDLevel >= 7)
                    {
                        cpuid(7, eax, ebx, ecx, edx);
                        avx2_ = (ebx & (1 << 5)) != 0;
                    }
                }

#elif defined(MTYPE_ARCH_ARM64)
                // NEON is mandatory on ARM64
                neon_ = true;
#endif
            }

#ifdef MTYPE_ARCH_X64
            void CPUFeatures::cpuid(uint32_t level, uint32_t& eax, uint32_t& ebx,
                                    uint32_t& ecx, uint32_t& edx)
            {
#ifdef MTYPE_PLATFORM_WINDOWS
                int info[4];
                __cpuid(info, static_cast<int>(level));
                eax = info[0];
                ebx = info[1];
                ecx = info[2];
                edx = info[3];
#else
                __get_cpuid(level, &eax, &ebx, &ecx, &edx);
#endif
            }

            bool CPUFeatures::isOSAVXSupported()
            {
#ifdef MTYPE_PLATFORM_WINDOWS
                // Use _xgetbv intrinsic on Windows
                unsigned long long xcr0 = _xgetbv(0);
                return (xcr0 & 0x6) == 0x6; // Check XMM and YMM state
#else
                // Use inline assembly on Linux/macOS
                uint32_t xcr0_low, xcr0_high;
                __asm__("xgetbv": "=a"(xcr0_low), "=d"(xcr0_high): "c"(0));
                return (xcr0_low & 0x6) == 0x6;
#endif
            }
#endif

            CPUFeatures::InstructionSet CPUFeatures::getBestInstructionSet() const
            {
#ifdef MTYPE_ARCH_X64
                if (avx2_) return InstructionSet::AVX2;
                if (sse2_) return InstructionSet::SSE2;
#elif defined(MTYPE_ARCH_ARM64)
                if (neon_) return InstructionSet::NEON;
#endif
                return InstructionSet::SCALAR;
            }
        } // namespace simd
    } // namespace value
} // namespace mType
