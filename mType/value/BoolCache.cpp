#include "BoolCache.hpp"
#include "../environment/registry/ClassDefinition.hpp"

namespace value {

BoolCache& BoolCache::getInstance() {
    static BoolCache instance;
    return instance;
}

void BoolCache::initialize() {
    auto& instance = getInstance();
    instance.initialized_ = true;
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> BoolCache::createCachedBool(
    bool value,
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> boolClassDef)
{
    if (!boolClassDef) {
        return nullptr;
    }

    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(
        boolClassDef,
        std::unordered_map<std::string, std::string>{}
    );

    instance->setField("value", value);
    return instance;
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> BoolCache::getBool(
    bool value,
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> boolClassDef)
{
    auto& instance = getInstance();
    if (!instance.initialized_) {
        initialize();
    }

    auto& slot = value ? instance.trueInstance_ : instance.falseInstance_;
    if (!slot && boolClassDef) {
        slot = createCachedBool(value, boolClassDef);
    }
    return slot;
}

void BoolCache::clear() {
    auto& instance = getInstance();
    instance.falseInstance_.reset();
    instance.trueInstance_.reset();
    instance.initialized_ = false;
}

} // namespace value
