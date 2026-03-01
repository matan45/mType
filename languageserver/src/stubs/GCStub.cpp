// GC stub for language server - LSP doesn't need garbage collection
#include "../../../mType/gc/GC.hpp"
#include "../../../mType/gc/GCCoordinator.hpp"
#include "../../../mType/gc/WriteBarrier.hpp"

namespace gc
{
    // GCCoordinator stubs
    GCCoordinator::GCCoordinator() {}
    GCCoordinator::~GCCoordinator() {}

    void GCCoordinator::setRootCollector(RootCollector collector) {}
    void GCCoordinator::setReferenceVisitor(ReferenceVisitor visitor) {}
    void GCCoordinator::onDeallocation(void* object) {}
    void GCCoordinator::onRefCountDecrement(void* object) {}
    void GCCoordinator::maybeCollect() {}
    void GCCoordinator::forceCollect() {}
    void GCCoordinator::reset() {}

    // Static member definitions
    std::unique_ptr<GCCoordinator> GC::coordinator = nullptr;
    std::mutex GC::initMutex;
    bool GC::initialized = false;

    void GC::initialize() {
        // No-op for LSP
    }

    void GC::shutdown() {
        // No-op for LSP
    }

    GCCoordinator* GC::get() {
        return nullptr;
    }

    bool GC::isInitialized() {
        return false;
    }

    void GC::onRefCountDecrement(void* object) {
        // No-op for LSP
    }

    void GC::maybeCollect() {
        // No-op for LSP
    }

    void GC::forceCollect() {
        // No-op for LSP
    }

    void GC::printStats() {
        // No-op for LSP
    }

    const GCStats* GC::getStats() {
        return nullptr;
    }

    void GC::reset() {
        // No-op for LSP
    }

    // GCTracker stub
    static GCTracker* trackerInstance = nullptr;

    GCTracker& GCTracker::getInstance() {
        static GCTracker instance;
        return instance;
    }

    void GCTracker::destroyInstance() {
        // No-op for LSP
    }

    void GCTracker::unregisterObject(void* rawPtr) {
        // No-op for LSP
    }

    GCObjectHeader* GCTracker::getHeader(void* rawPtr) {
        return nullptr;
    }

    const GCObjectHeader* GCTracker::getHeader(void* rawPtr) const {
        return nullptr;
    }

    bool GCTracker::isObjectAlive(void* rawPtr) const {
        return true; // LSP doesn't track objects
    }

    std::vector<void*> GCTracker::getAllTrackedPointers() const {
        return {};
    }

    size_t GCTracker::cleanupDeadObjects() {
        return 0;
    }

    void GCTracker::reset() {
        // No-op for LSP
    }

    // WriteBarrier stubs
    void visitObjectInstanceReferences(void* object, std::function<void(void*)> callback) {
        // No-op for LSP
    }

    void visitLambdaReferences(void* object, std::function<void(void*)> callback) {
        // No-op for LSP
    }

    void visitNativeArrayReferences(void* object, std::function<void(void*)> callback) {
        // No-op for LSP
    }

    void visitFlatMultiArrayReferences(void* object, std::function<void(void*)> callback) {
        // No-op for LSP
    }

    void visitSparseMultiArrayReferences(void* object, std::function<void(void*)> callback) {
        // No-op for LSP
    }

    void visitPromiseReferences(void* object, std::function<void(void*)> callback) {
        // No-op for LSP
    }

    void visitReferences(void* object, config::GCObjectType type, std::function<void(void*)> callback) {
        // No-op for LSP
    }

    void breakReferences(void* object, config::GCObjectType type) {
        // No-op for LSP
    }

} // namespace gc
