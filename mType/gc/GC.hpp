#pragma once

// Main GC header - includes all GC components
#include "GCConfig.hpp"
#include "GCStats.hpp"
#include "GCTracker.hpp"
#include "SuspectBuffer.hpp"
#include "CycleDetector.hpp"
#include "GCCoordinator.hpp"
#include "WriteBarrier.hpp"

namespace gc
{
    /**
     * @brief Global GC coordinator singleton
     *
     * Provides a single GC coordinator instance for the entire application.
     * This is used by write barriers and object allocation hooks.
     */
    class GC
    {
    private:
        static std::unique_ptr<GCCoordinator> coordinator;
        static std::mutex initMutex;
        static bool initialized;

    public:
        /**
         * @brief Initialize the global GC coordinator
         *
         * Call this once at application startup (e.g., in VirtualMachine constructor).
         */
        static void initialize();

        /**
         * @brief Shutdown the global GC coordinator
         *
         * Call this at application shutdown.
         */
        static void shutdown();

        /**
         * @brief Get the global GC coordinator
         *
         * Returns nullptr if not initialized.
         */
        static GCCoordinator* get();

        /**
         * @brief Check if GC is initialized
         */
        static bool isInitialized();

        /**
         * @brief Convenience: Register an object allocation
         */
        template<typename T>
        static void registerAllocation(std::shared_ptr<T> object, config::GCObjectType type)
        {
            if (coordinator)
            {
                coordinator->onAllocation(object, type);
            }
        }

        /**
         * @brief Convenience: Notify of reference count decrement
         */
        static void onRefCountDecrement(void* object);

        /**
         * @brief Convenience: Maybe trigger collection
         */
        static void maybeCollect();

        /**
         * @brief Convenience: Force collection
         */
        static void forceCollect();

        /**
         * @brief Print GC statistics to stdout
         */
        static void printStats();

        /**
         * @brief Get GC statistics
         */
        static const GCStats* getStats();

        /**
         * @brief Reset all GC state (for test isolation)
         *
         * Clears all tracked objects, suspects, and statistics.
         * Use between tests to ensure clean state.
         */
        static void reset();
    };
}
