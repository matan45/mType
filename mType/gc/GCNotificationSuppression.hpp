#pragma once

#include <cstddef>

namespace gc
{
    namespace detail
    {
        inline size_t& referenceNotificationSuppressionDepth() noexcept
        {
            static thread_local size_t depth = 0;
            return depth;
        }
    }

    // Breaking a cycle releases Values while the collector owns the tracker
    // lock. Those releases can run container/frame destructors, whose normal
    // teardown barriers would otherwise re-enter the coordinator. Suppression
    // is collector-thread-local: mutator threads continue buffering changes.
    class ScopedReferenceNotificationSuppression final
    {
    public:
        ScopedReferenceNotificationSuppression() noexcept
        {
            ++detail::referenceNotificationSuppressionDepth();
        }

        ~ScopedReferenceNotificationSuppression()
        {
            --detail::referenceNotificationSuppressionDepth();
        }

        ScopedReferenceNotificationSuppression(
            const ScopedReferenceNotificationSuppression&) = delete;
        ScopedReferenceNotificationSuppression& operator=(
            const ScopedReferenceNotificationSuppression&) = delete;
    };

    inline bool areReferenceNotificationsSuppressed() noexcept
    {
        return detail::referenceNotificationSuppressionDepth() != 0;
    }
}
