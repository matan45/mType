#include "SpecializedCollections.hpp"
#include "BoolCache.hpp"
#include "FloatCache.hpp"
#include "IntegerCache.hpp"
#include "NativeArray.hpp"
#include "StringCache.hpp"
#include "StringPool.hpp"
#include "ValueObject.hpp"
#include "ValueShim.hpp"
#include "../environment/Environment.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"

namespace value
{
    bool SpecializedCollectionStorage::extractKey(const Value& value, Key& out) const
    {
        out = Key{};
        if (mode_ == Mode::SHAPE) return extractShapeKey(value, out);
        return extractPrimitiveKey(value, out);
    }

    bool SpecializedCollectionStorage::extractPrimitiveKey(const Value& value, Key& out) const
    {
        out.tag = keyTag_;

        Value source = value;
        if (isValueObject(value))
        {
            auto obj = asValueObject(value);
            if (!obj || obj->getPrimitiveTag() != keyTag_) return false;
            source = obj->getFieldByIndex(0);
        }
        else if (isAnyObject(value))
        {
            auto* obj = asObjectInstanceRaw(value);
            if (!obj || obj->getPrimitiveTag() != keyTag_) return false;
            source = obj->getFieldByIndex(0);
        }

        switch (keyTag_)
        {
        case PrimitiveTypeTag::INT:
            if (!isInt(source)) return false;
            out.intValue = asInt(source);
            return true;
        case PrimitiveTypeTag::FLOAT:
            if (!isFloat(source)) return false;
            out.floatValue = asFloat(source);
            return true;
        case PrimitiveTypeTag::BOOL:
            if (!isBool(source)) return false;
            out.boolValue = asBool(source);
            return true;
        case PrimitiveTypeTag::STRING:
            if (isString(source))
            {
                out.stringValue = asString(source);
                return true;
            }
            if (isInternedString(source))
            {
                out.stringValue = asInternedString(source).getString();
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    bool SpecializedCollectionStorage::extractShapeKey(const Value& value, Key& out) const
    {
        if (shape_.empty()) return false;
        if (!isAnyObject(value)) return false;
        auto* obj = asObjectInstanceRaw(value);
        if (!obj) return false;
        if (obj->getClassDefinitionRaw() != shape_.classDef.get()) return false;
        if (!obj->hasFieldVector()) return false;

        const size_t n = shape_.fields.size();
        out.shapeFieldCount = static_cast<uint8_t>(n);
        for (size_t i = 0; i < n; ++i)
        {
            const Value& fv = obj->getFieldByIndex(shape_.fields[i].fieldIndex);
            if (!isInt(fv)) return false;
            out.shapeInts[i] = asInt(fv);
        }
        return true;
    }

    Value SpecializedCollectionStorage::boxKey(const Key& key, environment::Environment* env) const
    {
        if (mode_ == Mode::SHAPE) return boxShapeKey(key);
        return boxPrimitiveKey(key, env);
    }

    Value SpecializedCollectionStorage::boxPrimitiveKey(const Key& key, environment::Environment* env) const
    {
        if (!env)
        {
            switch (key.tag)
            {
            case PrimitiveTypeTag::INT: return key.intValue;
            case PrimitiveTypeTag::FLOAT: return key.floatValue;
            case PrimitiveTypeTag::BOOL: return key.boolValue;
            case PrimitiveTypeTag::STRING: return Value(StringPool::getInstance().intern(key.stringValue));
            default: return nullptr;
            }
        }

        const char* className = nullptr;
        switch (key.tag)
        {
        case PrimitiveTypeTag::INT: className = "Int"; break;
        case PrimitiveTypeTag::FLOAT: className = "Float"; break;
        case PrimitiveTypeTag::BOOL: className = "Bool"; break;
        case PrimitiveTypeTag::STRING: className = "String"; break;
        default: return nullptr;
        }

        auto classDef = env->findClass(className);
        if (!classDef) return nullptr;

        if (key.tag == PrimitiveTypeTag::INT)
        {
            if (auto cached = IntegerCache::getInt(key.intValue, classDef))
            {
                return cached;
            }
        }
        if (key.tag == PrimitiveTypeTag::BOOL)
        {
            if (auto cached = BoolCache::getBool(key.boolValue, classDef))
            {
                return cached;
            }
        }
        if (key.tag == PrimitiveTypeTag::FLOAT)
        {
            if (auto cached = FloatCache::getFloat(key.floatValue, classDef))
            {
                return cached;
            }
        }
        if (key.tag == PrimitiveTypeTag::STRING)
        {
            if (auto cached = StringCache::getString(key.stringValue, classDef))
            {
                return cached;
            }
        }

        auto boxed = std::make_shared<ValueObject>(classDef);
        switch (key.tag)
        {
        case PrimitiveTypeTag::INT:
            boxed->setFieldByIndex(0, key.intValue);
            break;
        case PrimitiveTypeTag::FLOAT:
            boxed->setFieldByIndex(0, key.floatValue);
            break;
        case PrimitiveTypeTag::BOOL:
            boxed->setFieldByIndex(0, key.boolValue);
            break;
        case PrimitiveTypeTag::STRING:
            boxed->setFieldByIndex(0, Value(StringPool::getInstance().intern(key.stringValue)));
            break;
        default:
            return nullptr;
        }
        return boxed;
    }

    Value SpecializedCollectionStorage::boxShapeKey(const Key& key) const
    {
        if (!shape_.classDef) return nullptr;
        auto inst = std::make_shared<runtimeTypes::klass::ObjectInstance>(shape_.classDef);
        inst->ensureFieldVector();
        for (uint8_t i = 0; i < key.shapeFieldCount; ++i)
        {
            inst->setFieldByIndex(shape_.fields[i].fieldIndex,
                                  static_cast<int64_t>(key.shapeInts[i]));
        }
        inst->registerWithGC();
        return Value(inst);
    }

    std::shared_ptr<NativeArray> SpecializedCollectionStorage::materializeKeys(environment::Environment* env) const
    {
        auto array = std::make_shared<NativeArray>(count_);
        size_t index = 0;
        for (const auto& entry : entries_)
        {
            if (!entry.occupied) continue;
            array->set(index++, boxKey(entry.key, env));
        }
        return array;
    }

    std::shared_ptr<NativeArray> SpecializedCollectionStorage::materializeValues() const
    {
        auto array = std::make_shared<NativeArray>(count_);
        size_t index = 0;
        for (const auto& entry : entries_)
        {
            if (!entry.occupied) continue;
            array->set(index++, entry.value);
        }
        return array;
    }
}
