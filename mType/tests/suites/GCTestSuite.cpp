#include "GCTestSuite.hpp"
#include "GCRuntimeCorrectnessTests.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "../../gc/GC.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../value/FlatMultiArray.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../../value/SparseMultiArray.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/arrays/object/FlatMultiObjectArray.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"
#include "../../vm/runtime/context/ExecutionContext.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
    void requireGC(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    void requireTracked(
        void* object,
        gc::config::GCObjectType expectedType,
        const char* message)
    {
        const auto* header = gc::GCTracker::getInstance().getHeader(object);
        requireGC(header && header->type == expectedType, message);
    }

    void requireTrackedStatsMatch(const char* message)
    {
        const auto* stats = gc::GC::getStats();
        requireGC(stats != nullptr, "GC statistics are unavailable");
        requireGC(
            stats->currentTrackedObjects.load() ==
                gc::GCTracker::getInstance().getTotalTrackedObjects(),
            message);
    }

    void verifyPooledObjectReregistration(
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef)
    {
        void* recycledAddress = nullptr;
        {
            auto pooled = value::ObjectInstancePool::getInstance().acquire(classDef);
            recycledAddress = pooled.get();
            const value::Value pooledValue{pooled};
            (void)pooledValue;
        }
        auto recycled = value::ObjectInstancePool::getInstance().acquire(classDef);
        requireGC(recycled.get() == recycledAddress,
                  "ObjectInstancePool did not reuse its released test slot");
        const value::Value recycledValue{recycled};
        (void)recycledValue;
        requireGC(gc::GCTracker::getInstance().isObjectAlive(recycled.get()),
                  "Recycled ObjectInstance retained an expired GC weak reference");
    }

    void verifyAllHeapKindsRegister(services::ScriptInterpreter&)
    {
        const size_t baseline = gc::GCTracker::getInstance().getTotalTrackedObjects();
        {
            auto object = std::make_shared<runtimeTypes::klass::ObjectInstance>(nullptr);
            auto lambda = std::make_shared<vm::runtime::BytecodeLambda>();
            auto native = std::make_shared<value::NativeArray>(1);
            auto flat = std::make_shared<value::FlatMultiArray>(std::vector<size_t>{1});
            auto sparse = std::make_shared<value::SparseMultiArray>(std::vector<size_t>{1});
            auto promise = std::make_shared<value::PromiseValue>();
            static const auto classDef =
                std::make_shared<runtimeTypes::klass::ClassDefinition>("GCBox");
            auto flatObject =
                std::make_shared<mType::value::arrays::FlatMultiObjectArray>(
                    classDef, std::vector<size_t>{1});

            const value::Value values[] = {
                value::Value{object}, value::Value{lambda}, value::Value{native},
                value::Value{flat}, value::Value{sparse}, value::Value{promise},
                value::Value{flatObject}};
            (void)values;

            requireTracked(object.get(), gc::config::GCObjectType::OBJECT_INSTANCE,
                           "ObjectInstance was not GC-registered");
            requireTracked(lambda.get(), gc::config::GCObjectType::BYTECODE_LAMBDA,
                           "BytecodeLambda was not GC-registered");
            requireTracked(native.get(), gc::config::GCObjectType::NATIVE_ARRAY,
                           "NativeArray was not GC-registered");
            requireTracked(flat.get(), gc::config::GCObjectType::FLAT_MULTI_ARRAY,
                           "FlatMultiArray was not GC-registered");
            requireTracked(sparse.get(), gc::config::GCObjectType::SPARSE_MULTI_ARRAY,
                           "SparseMultiArray was not GC-registered");
            requireTracked(promise.get(), gc::config::GCObjectType::PROMISE_VALUE,
                           "PromiseValue was not GC-registered");
            requireTracked(flatObject.get(),
                           gc::config::GCObjectType::FLAT_MULTI_OBJECT_ARRAY,
                           "FlatMultiObjectArray was not GC-registered");
            verifyPooledObjectReregistration(classDef);
        }
        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline,
                  "Dead registered heap kinds did not return to GC baseline");
        requireTrackedStatsMatch(
            "GC tracked-object statistics drifted after dead-object cleanup");
    }

    void createObjectNativeCycle()
    {
        auto object = std::make_shared<runtimeTypes::klass::ObjectInstance>(nullptr);
        object->registerWithGC();
        auto array = std::make_shared<value::NativeArray>(1);
        value::Value objectValue(object);
        value::Value arrayValue(array);
        object->setField("array", arrayValue);
        array->set(0, objectValue);
    }

    void createLambdaNativeCycle()
    {
        auto lambda = std::make_shared<vm::runtime::BytecodeLambda>();
        lambda->capturedFrame = std::make_shared<vm::runtime::SharedStackFrame>();
        auto array = std::make_shared<value::NativeArray>(1);
        value::Value lambdaValue(lambda);
        value::Value arrayValue(array);
        lambda->capturedFrame->setLocal(0, arrayValue);
        array->set(0, lambdaValue);
    }

    void createPromiseFlatCycle()
    {
        auto promise = std::make_shared<value::PromiseValue>();
        auto flat = std::make_shared<value::FlatMultiArray>(std::vector<size_t>{1});
        value::Value promiseValue(promise);
        value::Value flatValue(flat);
        flat->set(0, promiseValue);
        promise->resolve(flatValue);
    }

    void createSparseNativeCycle()
    {
        auto sparse = std::make_shared<value::SparseMultiArray>(std::vector<size_t>{1});
        auto native = std::make_shared<value::NativeArray>(1);
        value::Value sparseValue(sparse);
        value::Value nativeValue(native);
        sparse->set(std::vector<size_t>{0}, nativeValue);
        native->set(0, sparseValue);
    }

    void createFlatObjectNativeCycle()
    {
        auto classDef =
            std::make_shared<runtimeTypes::klass::ClassDefinition>("GCBox");
        auto flatObject = std::make_shared<mType::value::arrays::FlatMultiObjectArray>(
            classDef, std::vector<size_t>{1});
        auto native = std::make_shared<value::NativeArray>(1);
        value::Value flatObjectValue(flatObject);
        value::Value nativeValue(native);
        flatObject->setLinear(0, nativeValue);
        native->set(0, flatObjectValue);
    }

    void verifyCrossTypeCyclesReturnToBaseline(services::ScriptInterpreter&)
    {
        const size_t baseline = gc::GCTracker::getInstance().getTotalTrackedObjects();
        createObjectNativeCycle();
        createLambdaNativeCycle();
        createPromiseFlatCycle();
        createSparseNativeCycle();
        createFlatObjectNativeCycle();
        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline,
                  "Supported cross-type cycles did not return to GC baseline");
        requireTrackedStatsMatch(
            "GC tracked-object statistics drifted after cycle collection");
    }

    template<typename Owner, typename Setter>
    void verifyOldEdgeBarrier(
        std::shared_ptr<Owner> owner,
        Setter&& setOwnerValue,
        const char* failureMessage)
    {
        const size_t baseline = gc::GCTracker::getInstance().getTotalTrackedObjects();
        value::Value ownerValue(owner);
        {
            auto left = std::make_shared<value::NativeArray>(1);
            auto right = std::make_shared<value::NativeArray>(1);
            value::Value leftValue(left);
            value::Value rightValue(right);
            left->set(0, rightValue);
            right->set(0, leftValue);
            setOwnerValue(*owner, leftValue);
        }

        gc::GC::forceCollect();
        setOwnerValue(*owner, value::Value{std::monostate{}});
        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline + 1,
                  failureMessage);

        ownerValue = std::monostate{};
        owner.reset();
        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline,
                  "Old-edge barrier owner did not return to GC baseline");
    }

    void verifyArrayOldEdgeBarriers(services::ScriptInterpreter&)
    {
        verifyOldEdgeBarrier(
            std::make_shared<value::NativeArray>(1),
            [](auto& owner, const auto& value) { owner.set(0, value); },
            "NativeArray overwrite did not collect the detached cycle");
        verifyOldEdgeBarrier(
            std::make_shared<value::FlatMultiArray>(std::vector<size_t>{1}),
            [](auto& owner, const auto& value) { owner.set(0, value); },
            "FlatMultiArray overwrite did not collect the detached cycle");
        verifyOldEdgeBarrier(
            std::make_shared<value::SparseMultiArray>(std::vector<size_t>{1}),
            [](auto& owner, const auto& value) {
                owner.set(std::vector<size_t>{0}, value);
            },
            "SparseMultiArray overwrite did not collect the detached cycle");

        static const auto classDef =
            std::make_shared<runtimeTypes::klass::ClassDefinition>("GCBarrierBox");
        verifyOldEdgeBarrier(
            std::make_shared<mType::value::arrays::FlatMultiObjectArray>(
                classDef, std::vector<size_t>{1}),
            [](auto& owner, const auto& value) { owner.setLinear(0, value); },
            "FlatMultiObjectArray overwrite did not collect the detached cycle");
    }

    template<typename Owner, typename Setter>
    void verifyOwnerTeardownBarrier(
        std::shared_ptr<Owner> owner,
        Setter&& setOwnerValue,
        const char* failureMessage)
    {
        const size_t baseline = gc::GCTracker::getInstance().getTotalTrackedObjects();
        value::Value ownerValue(owner);
        {
            auto left = std::make_shared<value::NativeArray>(1);
            auto right = std::make_shared<value::NativeArray>(1);
            value::Value leftValue(left);
            value::Value rightValue(right);
            left->set(0, rightValue);
            right->set(0, leftValue);
            setOwnerValue(*owner, leftValue);
        }

        gc::GC::forceCollect();
        ownerValue = std::monostate{};
        owner.reset();
        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline,
                  failureMessage);
    }

    void verifyOwnerTeardownBarriers(services::ScriptInterpreter&)
    {
        verifyOwnerTeardownBarrier(
            std::make_shared<value::NativeArray>(1),
            [](auto& owner, const auto& value) { owner.set(0, value); },
            "NativeArray teardown orphaned a child-only cycle");
        verifyOwnerTeardownBarrier(
            std::make_shared<value::FlatMultiArray>(std::vector<size_t>{1}),
            [](auto& owner, const auto& value) { owner.set(0, value); },
            "FlatMultiArray teardown orphaned a child-only cycle");
        verifyOwnerTeardownBarrier(
            std::make_shared<value::SparseMultiArray>(std::vector<size_t>{1}),
            [](auto& owner, const auto& value) {
                owner.set(std::vector<size_t>{0}, value);
            },
            "SparseMultiArray teardown orphaned a child-only cycle");
        verifyOwnerTeardownBarrier(
            std::make_shared<value::PromiseValue>(),
            [](auto& owner, const auto& value) { owner.resolve(value); },
            "PromiseValue teardown orphaned a child-only cycle");

        auto lambda = std::make_shared<vm::runtime::BytecodeLambda>();
        lambda->capturedFrame = std::make_shared<vm::runtime::SharedStackFrame>();
        verifyOwnerTeardownBarrier(
            std::move(lambda),
            [](auto& owner, const auto& value) {
                owner.capturedFrame->setLocal(0, value);
            },
            "BytecodeLambda teardown orphaned a captured child-only cycle");

        static const auto classDef =
            std::make_shared<runtimeTypes::klass::ClassDefinition>("GCTeardownBox");
        verifyOwnerTeardownBarrier(
            std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef),
            [](auto& owner, const auto& value) { owner.setField("edge", value); },
            "ObjectInstance teardown orphaned a child-only cycle");
        verifyOwnerTeardownBarrier(
            std::make_shared<mType::value::arrays::FlatMultiObjectArray>(
                classDef, std::vector<size_t>{1}),
            [](auto& owner, const auto& value) { owner.setLinear(0, value); },
            "FlatMultiObjectArray teardown orphaned a child-only cycle");
    }

    void verifyMultipleVMRootCollectors(services::ScriptInterpreter& interpreter)
    {
        auto* coordinator = gc::GC::get();
        requireGC(coordinator != nullptr, "GC coordinator is not initialized");
        const size_t baseline = coordinator->getRootCollectorCount();

        auto first =
            std::make_shared<vm::runtime::VirtualMachine>(interpreter.getEnvironment());
        auto second =
            std::make_shared<vm::runtime::VirtualMachine>(interpreter.getEnvironment());
        requireGC(coordinator->getRootCollectorCount() == baseline + 2,
                  "Both VM root collectors were not registered");

        first.reset();
        requireGC(coordinator->getRootCollectorCount() == baseline + 1,
                  "Destroyed VM root collector remained registered");
        gc::GC::forceCollect();

        second.reset();
        requireGC(coordinator->getRootCollectorCount() == baseline,
                  "VM root collector count did not return to baseline");
    }

    void verifyAsyncPromiseCallbacksAreReentrant(services::ScriptInterpreter&)
    {
        auto promise = std::make_shared<value::AsyncPromiseValue>();
        int callbackCount = 0;
        promise->then([&](value::Value) {
            ++callbackCount;
            promise->then([&](value::Value) { ++callbackCount; });
        });
        promise->resolve(value::Value{int64_t{7}});
        requireGC(callbackCount == 2,
                  "Reentrant promise callback was not delivered exactly once");
    }

}

namespace tests::testSuite
{
    using namespace testFramework;

    void GCTestSuite::setupTests()
    {
        // === BASIC CYCLE TESTS ===
        // Tests for fundamental circular reference patterns

        addOutputVerificationTest("Simple Cycle (A -> B -> A)",
                                  passPath + "simpleCycle.mt");

        addOutputVerificationTest("Self-Reference (A -> A)",
                                  passPath + "selfReference.mt");

        addOutputVerificationTest("Deep Cycle (A -> B -> C -> D -> A)",
                                  passPath + "deepCycle.mt");

        // === LAMBDA CAPTURE CYCLES ===
        // Tests for circular references involving lambda closures

        addOutputVerificationTest("Lambda Capture Cycle",
                                  passPath + "lambdaCycle.mt");

        // === COLLECTION CYCLES ===
        // Tests for circular references in collection structures

        addOutputVerificationTest("LinkedList Cycle",
                                  passPath + "linkedListCycle.mt");

        // === MIXED REACHABILITY ===
        // Tests with some objects reachable and some in cycles

        addOutputVerificationTest("Mixed Reachability",
                                  passPath + "mixedReachability.mt");

        // === STRESS TESTS ===
        // Tests with many objects and cycles

        addOutputVerificationTest("Stress Test (5000 nodes)",
                                  passPath + "stressTestAbort.mt");

        addOutputVerificationTest("Stress Test Large (20000 nodes)",
                                  passPath + "stressTestAbortLarge.mt");

        addOutputVerificationTest("Force Abort (50000 nodes)",
                                  passPath + "forceAbort.mt");

        // === SURVIVAL UNDER ALLOCATION PRESSURE ===
        // The existing tests prove garbage IS collected; these prove live
        // data is NOT — strings and collection-internal objects must survive
        // the GC cycles triggered by churning 40k cyclic garbage objects.
        addOutputVerificationTest("String Pool Survives GC Pressure",
                                  passPath + "gcStringPoolSurvives.mt");

        addOutputVerificationTest("Objects In Collections Survive GC Pressure",
                                  passPath + "gcObjectsInCollectionsSurvive.mt");

        addInterpreterCallbackTest("All GC Heap Kinds Register", "",
                                   verifyAllHeapKindsRegister);
        addInterpreterCallbackTest("Cross-Type Cycles Return To Baseline", "",
                                   verifyCrossTypeCyclesReturnToBaseline);
        addInterpreterCallbackTest("Array Overwrites Rebuffer Detached Cycles", "",
                                   verifyArrayOldEdgeBarriers);
        addInterpreterCallbackTest("Owner Teardown Rebuffers Child Cycles", "",
                                   verifyOwnerTeardownBarriers);
        addInterpreterCallbackTest("Multiple VM Root Collectors Have Owned Lifetimes", "",
                                   verifyMultipleVMRootCollectors);
        addInterpreterCallbackTest("Async Promise Callbacks Are Reentrant", "",
                                   verifyAsyncPromiseCallbacksAreReentrant);
        registerGCRuntimeCorrectnessTests(*this);
    }
}
