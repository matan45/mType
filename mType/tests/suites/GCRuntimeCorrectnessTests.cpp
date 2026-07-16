#include "GCRuntimeCorrectnessTests.hpp"
#include "../testFramework/TestSuite.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "../../environment/registry/FieldDefinition.hpp"
#include "../../environment/registry/VariableDefinition.hpp"
#include "../../gc/GC.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../value/ValueShim.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"
#include "../../vm/runtime/context/ExecutionContext.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
    struct WhiteBoxGCNode
    {
        std::vector<std::shared_ptr<WhiteBoxGCNode>> references;
        // Models two Value copies sharing one intrusive bridge: this is a
        // logical graph edge, but it does not add an underlying shared_ptr
        // owner for CycleDetector's weak use_count snapshot.
        std::vector<WhiteBoxGCNode*> bridgeSharedReferences;
    };

    void requireGC(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    void visitWhiteBoxGCNode(
        void* object,
        const std::function<void(void*)>& callback)
    {
        auto* node = static_cast<WhiteBoxGCNode*>(object);
        for (const auto& child : node->references)
        {
            if (child) callback(child.get());
        }
        for (auto* child : node->bridgeSharedReferences)
        {
            if (child) callback(child);
        }
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

    void verifySharedCapturedFrameSurvivesCollection(
        services::ScriptInterpreter& interpreter)
    {
        const size_t baseline = gc::GCTracker::getInstance().getTotalTrackedObjects();
        auto stack = interpreter.getVM()->getStackManager();
        const size_t initialStackSize = stack->size();

        auto sharedFrame = std::make_shared<vm::runtime::SharedStackFrame>();
        sharedFrame->setLocal(0, value::Value{int64_t{42}});

        auto liveLambda = std::make_shared<vm::runtime::BytecodeLambda>();
        liveLambda->capturedFrame = sharedFrame;
        value::Value liveLambdaValue{liveLambda};
        stack->push(liveLambdaValue);
        liveLambdaValue = std::monostate{};
        liveLambda.reset();

        {
            static const auto classDef =
                std::make_shared<runtimeTypes::klass::ClassDefinition>(
                    "GCSharedFrameBox");
            auto deadLambda = std::make_shared<vm::runtime::BytecodeLambda>();
            auto object =
                std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);
            deadLambda->capturedFrame = sharedFrame;
            deadLambda->capturedThis = object;
            value::Value deadLambdaValue{deadLambda};
            value::Value objectValue{object};
            (void)objectValue;
            object->setField("lambda", deadLambdaValue);
        }

        gc::GC::forceCollect();
        const value::Value marker = sharedFrame->getLocal(0);
        requireGC(value::isInt(marker) && value::asInt(marker) == 42,
                  "Collecting one lambda erased a shared captured frame");

        stack->resize(initialStackSize);
        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline,
                  "Shared-frame collection test did not return to GC baseline");
    }

    void verifyCycleBreakSuppressesNestedTeardown(services::ScriptInterpreter&)
    {
        const size_t baseline = gc::GCTracker::getInstance().getTotalTrackedObjects();
        createLambdaNativeCycle();
        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline,
                  "Nested frame teardown did not complete during cycle breaking");
    }

    void verifyOperandStackRootRemovalRebuffersCycle(
        services::ScriptInterpreter& interpreter)
    {
        const size_t baseline = gc::GCTracker::getInstance().getTotalTrackedObjects();
        auto stack = interpreter.getVM()->getStackManager();
        const size_t initialStackSize = stack->size();

        {
            auto left = std::make_shared<value::NativeArray>(1);
            auto right = std::make_shared<value::NativeArray>(1);
            value::Value leftValue{left};
            value::Value rightValue{right};
            left->set(0, rightValue);
            right->set(0, leftValue);
            stack->push(leftValue);
        }

        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline + 2,
                  "Operand stack root did not preserve its reachable cycle");
        {
            value::Value dropped = stack->pop();
            (void)dropped;
        }
        requireGC(stack->size() == initialStackSize,
                  "Operand stack test did not restore its initial depth");

        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline,
                  "Removing an operand stack root orphaned a child cycle");
    }

    void verifyGlobalRootRemovalRebuffersCycle(
        services::ScriptInterpreter& interpreter)
    {
        const size_t baseline = gc::GCTracker::getInstance().getTotalTrackedObjects();
        auto global = std::make_shared<runtimeTypes::global::VariableDefinition>(
            "__gc_root_removal", value::ValueType::ARRAY);
        interpreter.getEnvironment()->declareVariable("__gc_root_removal", global);

        {
            auto left = std::make_shared<value::NativeArray>(1);
            auto right = std::make_shared<value::NativeArray>(1);
            value::Value leftValue{left};
            value::Value rightValue{right};
            left->set(0, rightValue);
            right->set(0, leftValue);
            global->setValue(leftValue);
        }

        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline + 2,
                  "Global root did not preserve its reachable cycle");
        global->setValue(std::monostate{});
        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline,
                  "Removing a global root orphaned a child cycle");
    }

    void verifyStaticFieldRootRemovalRebuffersCycle(
        services::ScriptInterpreter& interpreter)
    {
        const size_t baseline = gc::GCTracker::getInstance().getTotalTrackedObjects();
        auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(
            "GCStaticRootRemoval");
        auto staticRoot = std::make_shared<runtimeTypes::klass::FieldDefinition>(
            "root", value::ValueType::ARRAY, value::Value{}, true);
        classDef->addStaticField("root", staticRoot);
        interpreter.getEnvironment()->registerClass(classDef->getName(), classDef);
        auto holder = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        {
            auto left = std::make_shared<value::NativeArray>(1);
            auto right = std::make_shared<value::NativeArray>(1);
            value::Value leftValue{left};
            value::Value rightValue{right};
            left->set(0, rightValue);
            right->set(0, leftValue);
            holder->setField("root", leftValue);
        }

        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline + 2,
                  "Static field root did not preserve its reachable cycle");
        holder->setField("root", std::monostate{});
        gc::GC::forceCollect();
        requireGC(gc::GCTracker::getInstance().getTotalTrackedObjects() == baseline,
                  "Removing a static field root orphaned a child cycle");
    }

    void verifyWideCycleTraversalIsComplete(services::ScriptInterpreter&)
    {
        auto& tracker = gc::GCTracker::getInstance();
        const size_t baseline = tracker.getTotalTrackedObjects();
        constexpr size_t fanout = 10001;

        auto root = std::make_shared<WhiteBoxGCNode>();
        std::weak_ptr<WhiteBoxGCNode> cleanupRoot = root;
        void* rootAddress = root.get();
        requireGC(tracker.registerObject(root, gc::config::GCObjectType::UNKNOWN),
                  "Wide graph root registration failed");

        for (size_t i = 0; i < fanout; ++i)
        {
            auto child = std::make_shared<WhiteBoxGCNode>();
            requireGC(tracker.registerObject(child, gc::config::GCObjectType::UNKNOWN),
                      "Wide graph child registration failed");
            root->references.push_back(child);
            child->references.push_back(root);
        }

        gc::SuspectBuffer suspects;
        auto* rootHeader = tracker.getHeader(rootAddress);
        requireGC(rootHeader != nullptr, "Wide graph root header is missing");
        rootHeader->color = gc::config::ObjectColor::PURPLE;
        rootHeader->buffered = true;
        suspects.addSuspect(rootAddress);

        gc::CycleDetector detector(tracker, suspects);
        detector.setReferenceVisitor(visitWhiteBoxGCNode);
        root.reset();
        const auto result =
            detector.collectCycles(std::chrono::milliseconds{5000});

        if (auto cleanup = cleanupRoot.lock()) cleanup->references.clear();
        tracker.cleanupDeadObjects();

        requireGC(result.completed && result.objectsCollected == fanout + 1,
                  "Wide cycle traversal abandoned nodes at high fan-out");
        requireGC(tracker.getTotalTrackedObjects() == baseline,
                  "Wide traversal regression did not restore tracker baseline");
    }

    void verifyAbortRestoresSuspects(services::ScriptInterpreter&)
    {
        auto& tracker = gc::GCTracker::getInstance();
        const size_t baseline = tracker.getTotalTrackedObjects();
        auto left = std::make_shared<WhiteBoxGCNode>();
        auto right = std::make_shared<WhiteBoxGCNode>();
        std::weak_ptr<WhiteBoxGCNode> cleanupRoot = left;
        void* leftAddress = left.get();
        tracker.registerObject(left, gc::config::GCObjectType::UNKNOWN);
        tracker.registerObject(right, gc::config::GCObjectType::UNKNOWN);
        left->references.push_back(right);
        right->references.push_back(left);

        gc::SuspectBuffer suspects;
        auto* header = tracker.getHeader(leftAddress);
        requireGC(header != nullptr, "Abort regression root header is missing");
        header->color = gc::config::ObjectColor::PURPLE;
        header->buffered = true;
        suspects.addSuspect(leftAddress);

        gc::CycleDetector detector(tracker, suspects);
        detector.setReferenceVisitor(visitWhiteBoxGCNode);
        left.reset();
        right.reset();

        const auto aborted = detector.collectCycles(std::chrono::milliseconds{0});
        header = tracker.getHeader(leftAddress);
        const bool restored =
            header && header->color == gc::config::ObjectColor::PURPLE &&
            header->buffered && suspects.isSuspect(leftAddress);
        const auto retried =
            detector.collectCycles(std::chrono::milliseconds{1000});

        if (auto cleanup = cleanupRoot.lock()) cleanup->references.clear();
        tracker.cleanupDeadObjects();

        requireGC(!aborted.completed && restored,
                  "Aborted cycle detection lost its extracted suspect");
        requireGC(retried.completed && retried.objectsCollected == 2,
                  "Restored suspects were not collectible on retry");
        requireGC(tracker.getTotalTrackedObjects() == baseline,
                  "Abort regression did not restore tracker baseline");
    }

    void verifyBlackExternalRootTraversesGrayCycle(
        services::ScriptInterpreter&)
    {
        auto& tracker = gc::GCTracker::getInstance();
        const size_t baseline = tracker.getTotalTrackedObjects();
        auto root = std::make_shared<WhiteBoxGCNode>();
        auto left = std::make_shared<WhiteBoxGCNode>();
        auto right = std::make_shared<WhiteBoxGCNode>();
        std::weak_ptr<WhiteBoxGCNode> cleanupLeft = left;
        void* leftAddress = left.get();

        requireGC(tracker.registerObject(root, gc::config::GCObjectType::UNKNOWN) &&
                  tracker.registerObject(left, gc::config::GCObjectType::UNKNOWN) &&
                  tracker.registerObject(right, gc::config::GCObjectType::UNKNOWN),
                  "External-root graph registration failed");
        root->bridgeSharedReferences.push_back(left.get());
        left->references.push_back(right);
        right->references.push_back(left);

        gc::SuspectBuffer suspects;
        auto* header = tracker.getHeader(leftAddress);
        requireGC(header != nullptr, "External-root candidate header is missing");
        header->color = gc::config::ObjectColor::PURPLE;
        header->buffered = true;
        suspects.addSuspect(leftAddress);

        gc::CycleDetector detector(tracker, suspects);
        detector.setReferenceVisitor(visitWhiteBoxGCNode);
        detector.setExternalRoots({root.get()});
        left.reset();
        right.reset();
        const auto result =
            detector.collectCycles(std::chrono::milliseconds{1000});
        header = tracker.getHeader(leftAddress);
        const bool preserved = result.completed &&
            result.objectsCollected == 0 && header &&
            header->color == gc::config::ObjectColor::BLACK;

        root->bridgeSharedReferences.clear();
        if (auto cleanup = cleanupLeft.lock()) cleanup->references.clear();
        root.reset();
        tracker.cleanupDeadObjects();

        requireGC(preserved,
                  "BLACK external root did not preserve its GRAY child cycle");
        requireGC(tracker.getTotalTrackedObjects() == baseline,
                  "External-root traversal test did not restore GC baseline");
    }
}

namespace tests::testSuite
{
    void registerGCRuntimeCorrectnessTests(testFramework::TestSuite& suite)
    {
        suite.addInterpreterCallbackTest(
            "Shared Captured Frame Survives Collection", "",
            verifySharedCapturedFrameSurvivesCollection);
        suite.addInterpreterCallbackTest(
            "Cycle Break Suppresses Nested Teardown", "",
            verifyCycleBreakSuppressesNestedTeardown);
        suite.addInterpreterCallbackTest(
            "Operand Stack Root Removal Rebuffers Cycle", "",
            verifyOperandStackRootRemovalRebuffersCycle);
        suite.addInterpreterCallbackTest(
            "Global Root Removal Rebuffers Cycle", "",
            verifyGlobalRootRemovalRebuffersCycle);
        suite.addInterpreterCallbackTest(
            "Static Field Root Removal Rebuffers Cycle", "",
            verifyStaticFieldRootRemovalRebuffersCycle);
        suite.addInterpreterCallbackTest(
            "Wide Cycle Traversal Is Complete", "",
            verifyWideCycleTraversalIsComplete);
        suite.addInterpreterCallbackTest(
            "GC Abort Restores Suspects", "",
            verifyAbortRestoresSuspects);
        suite.addInterpreterCallbackTest(
            "Black External Root Traverses Gray Cycle", "",
            verifyBlackExternalRootTraversesGrayCycle);
    }
}
