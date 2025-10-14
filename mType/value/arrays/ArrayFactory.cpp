#include "ArrayFactory.hpp"
#include "../FlatMultiArray.hpp"
#include "../SparseMultiArray.hpp"
#include "../simd/SIMDConfig.hpp"
#include "../simd/detection/CPUFeatures.hpp"
#include <numeric>
namespace mType
{
    namespace value
    {
        namespace arrays
        {
            std::shared_ptr<::value::NativeArray> ArrayFactory::create1DArray(
                size_t size,
                ::value::ValueType elemType,
                const std::string& elemTypeName,
                std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                environment::registry::ClassRegistry* classRegistry
            )
            {
                // Try to resolve ClassDefinition if not provided
                if (!classDef && classRegistry && elemType == ::value::ValueType::OBJECT && !elemTypeName.empty())
                {
                    classDef = resolveClassDefinition(elemTypeName, classRegistry);
                }

                // For object arrays with ClassDefinition, use SoA-optimized constructor
                if (elemType == ::value::ValueType::OBJECT && classDef && meetsThreshold(size))
                {
                    return std::make_shared<::value::NativeArray>(size, classDef);
                }

                // For all other cases (primitives, strings, objects without ClassDef),
                // use standard constructor which will auto-select SIMD/StringPool optimization
                return std::make_shared<::value::NativeArray>(size, elemType, elemTypeName);
            }

            ::value::Value ArrayFactory::createMultiDimensionalArray(
                const std::vector<size_t>& dimensions,
                ::value::ValueType elemType,
                const std::string& elemTypeName,
                std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                environment::registry::ClassRegistry* classRegistry
            )
            {
                if (dimensions.empty())
                {
                    throw std::invalid_argument("Dimensions cannot be empty");
                }

                // For single dimension, use 1D array creation
                if (dimensions.size() == 1)
                {
                    return create1DArray(dimensions[0], elemType, elemTypeName, classDef, classRegistry);
                }

                // Try to resolve ClassDefinition if not provided
                if (!classDef && classRegistry && elemType == ::value::ValueType::OBJECT && !elemTypeName.empty())
                {
                    classDef = resolveClassDefinition(elemTypeName, classRegistry);
                }

                // For object arrays with ClassDefinition, use FlatMultiObjectArray with SoA
                size_t totalSize = calculateTotalSize(dimensions);
                if (elemType == ::value::ValueType::OBJECT && classDef && meetsThreshold(totalSize))
                {
                    auto flatMultiObjArray = std::make_shared<FlatMultiObjectArray>(classDef, dimensions);
                    return flatMultiObjArray;
                }

                // For other types (primitives, strings, objects without ClassDef),
                // use existing FlatMultiArray
                ::value::Value defaultValue = std::monostate{};
                switch (elemType)
                {
                case ::value::ValueType::INT:
                    defaultValue = 0;
                    break;
                case ::value::ValueType::FLOAT:
                    defaultValue = 0.0f;
                    break;
                case ::value::ValueType::BOOL:
                    defaultValue = false;
                    break;
                case ::value::ValueType::STRING:
                    defaultValue = std::string("");
                    break;
                default:
                    defaultValue = std::monostate{};
                    break;
                }

                return std::make_shared<::value::FlatMultiArray>(dimensions, defaultValue);
            }

            std::shared_ptr<::value::NativeArray> ArrayFactory::createFromLiteral(
                const std::vector<::value::Value>& elements,
                ::value::ValueType expectedType,
                const std::string& elemTypeName,
                environment::registry::ClassRegistry* classRegistry
            )
            {
                if (elements.empty())
                {
                    // Empty array
                    return std::make_shared<::value::NativeArray>(0);
                }

                // Try to resolve ClassDefinition for object arrays
                std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef = nullptr;
                if (expectedType == ::value::ValueType::OBJECT && classRegistry && !elemTypeName.empty())
                {
                    classDef = resolveClassDefinition(elemTypeName, classRegistry);
                }

                // Create optimized array
                auto array = create1DArray(elements.size(), expectedType, elemTypeName, classDef, classRegistry);

                // Populate with literal values
                for (size_t i = 0; i < elements.size(); ++i)
                {
                    array->set(i, elements[i]);
                }

                return array;
            }

            bool ArrayFactory::shouldOptimize(
                size_t size,
                ::value::ValueType elemType,
                std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef
            )
            {
                // Check size threshold
                if (!meetsThreshold(size))
                {
                    return false;
                }

                // Primitives and strings always benefit from optimization
                if (elemType == ::value::ValueType::INT ||
                    elemType == ::value::ValueType::FLOAT ||
                    elemType == ::value::ValueType::BOOL ||
                    elemType == ::value::ValueType::STRING)
                {
                    return true;
                }

                // Objects benefit only if ClassDefinition available (for SoA)
                if (elemType == ::value::ValueType::OBJECT && classDef)
                {
                    return true;
                }

                return false;
            }

            std::string ArrayFactory::getExpectedStorageMode(
                size_t size,
                ::value::ValueType elemType,
                std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef
            )
            {
                if (!meetsThreshold(size))
                {
                    return "HETEROGENEOUS (size < threshold)";
                }

                switch (elemType)
                {
                case ::value::ValueType::INT:
#ifdef MTYPE_SIMD_ENABLED
                    {
                        const auto& features = simd::CPUFeatures::instance();
                        if (features.hasSSE2() || features.hasNEON())
                        {
                            return "SIMD_INT (SSE2/NEON optimized)";
                        }
                    }
#endif
                    return "HETEROGENEOUS (no SIMD support)";

                case ::value::ValueType::FLOAT:
#ifdef MTYPE_SIMD_ENABLED
                    {
                        const auto& features = simd::CPUFeatures::instance();
                        if (features.hasSSE2() || features.hasNEON())
                        {
                            return "SIMD_FLOAT (SSE2/NEON optimized)";
                        }
                    }
#endif
                    return "HETEROGENEOUS (no SIMD support)";

                case ::value::ValueType::BOOL:
#ifdef MTYPE_SIMD_ENABLED
                    {
                        const auto& features = simd::CPUFeatures::instance();
                        if (features.hasSSE2() || features.hasNEON())
                        {
                            return "SIMD_BOOL (SSE2/NEON optimized)";
                        }
                    }
#endif
                    return "HETEROGENEOUS (no SIMD support)";

                case ::value::ValueType::STRING:
                    return "SIMD_STRING (StringPool optimized)";

                case ::value::ValueType::OBJECT:
                    if (classDef)
                    {
                        return "SOA_OBJECT (Structure-of-Arrays optimized)";
                    }
                    return "HETEROGENEOUS (no ClassDefinition)";

                default:
                    return "HETEROGENEOUS";
                }
            }

            // Private helpers

            std::shared_ptr<runtimeTypes::klass::ClassDefinition> ArrayFactory::resolveClassDefinition(
                const std::string& typeName,
                environment::registry::ClassRegistry* classRegistry
            )
            {
                if (!classRegistry || typeName.empty())
                {
                    return nullptr;
                }

                // Try to get class definition from registry
                try
                {
                    return classRegistry->findClass(typeName);
                }
                catch (...)
                {
                    // Class not found - return nullptr
                    return nullptr;
                }
            }

            bool ArrayFactory::meetsThreshold(size_t size)
            {
                return size >= simd::SIMDThreshold::MIN_ELEMENTS;
            }

            size_t ArrayFactory::calculateTotalSize(const std::vector<size_t>& dimensions)
            {
                if (dimensions.empty())
                {
                    return 0;
                }

                size_t total = 1;
                for (size_t dim : dimensions)
                {
                    // Check for overflow before multiplication
                    if (dim == 0)
                    {
                        throw std::invalid_argument("Array dimension cannot be zero");
                    }

                    if (total > SIZE_MAX / dim)
                    {
                        throw std::overflow_error("Array dimensions too large - would exceed maximum size");
                    }

                    total *= dim;
                }

                return total;
            }
        } // namespace arrays
    } // namespace value
} // namespace mType
