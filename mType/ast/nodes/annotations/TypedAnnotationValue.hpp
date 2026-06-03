#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>
#include <stdexcept>

namespace ast::nodes::annotations
{
    enum class AnnotationValueType : uint8_t
    {
        NULL_VALUE = 0,
        INT = 1,
        FLOAT = 2,
        BOOL = 3,
        STRING = 4,
        CLASS_REF = 5,
        CLASS_ARRAY = 6,
        INT_ARRAY = 7,
        FLOAT_ARRAY = 8,
        BOOL_ARRAY = 9,
        STRING_ARRAY = 10
    };

    /// Carries a typed literal supplied to an annotation parameter
    /// (in declarations as a default, in usages as a named-arg value).
    /// The variant intentionally stores Class references as their textual
    /// identifier names — the registry resolves them to ClassDefinitions
    /// at validation/reflection time.
    class TypedAnnotationValue
    {
    public:
        TypedAnnotationValue() : type_(AnnotationValueType::NULL_VALUE), data_(std::monostate{}) {}

        static TypedAnnotationValue makeNull() { return TypedAnnotationValue(); }
        static TypedAnnotationValue makeInt(int64_t v)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::INT;
            t.data_ = v;
            return t;
        }
        static TypedAnnotationValue makeFloat(double v)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::FLOAT;
            t.data_ = v;
            return t;
        }
        static TypedAnnotationValue makeBool(bool v)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::BOOL;
            t.data_ = v;
            return t;
        }
        static TypedAnnotationValue makeString(std::string v)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::STRING;
            t.data_ = std::move(v);
            return t;
        }
        static TypedAnnotationValue makeClassRef(std::string name)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::CLASS_REF;
            t.data_ = std::move(name);
            return t;
        }
        static TypedAnnotationValue makeClassArray(std::vector<std::string> names)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::CLASS_ARRAY;
            t.data_ = std::move(names);
            return t;
        }
        static TypedAnnotationValue makeIntArray(std::vector<int64_t> values)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::INT_ARRAY;
            t.data_ = std::move(values);
            return t;
        }
        static TypedAnnotationValue makeFloatArray(std::vector<double> values)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::FLOAT_ARRAY;
            t.data_ = std::move(values);
            return t;
        }
        static TypedAnnotationValue makeBoolArray(std::vector<bool> values)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::BOOL_ARRAY;
            t.data_ = std::move(values);
            return t;
        }
        static TypedAnnotationValue makeStringArray(std::vector<std::string> values)
        {
            TypedAnnotationValue t;
            t.type_ = AnnotationValueType::STRING_ARRAY;
            t.data_ = std::move(values);
            return t;
        }

        AnnotationValueType getType() const { return type_; }
        bool isNull() const { return type_ == AnnotationValueType::NULL_VALUE; }

        int64_t asInt() const
        {
            if (type_ != AnnotationValueType::INT) throw std::runtime_error("annotation value: not int");
            return std::get<int64_t>(data_);
        }
        double asFloat() const
        {
            if (type_ != AnnotationValueType::FLOAT) throw std::runtime_error("annotation value: not float");
            return std::get<double>(data_);
        }
        bool asBool() const
        {
            if (type_ != AnnotationValueType::BOOL) throw std::runtime_error("annotation value: not bool");
            return std::get<bool>(data_);
        }
        const std::string& asString() const
        {
            if (type_ != AnnotationValueType::STRING) throw std::runtime_error("annotation value: not string");
            return std::get<std::string>(data_);
        }
        const std::string& asClassRef() const
        {
            if (type_ != AnnotationValueType::CLASS_REF) throw std::runtime_error("annotation value: not class ref");
            return std::get<std::string>(data_);
        }
        const std::vector<std::string>& asClassArray() const
        {
            if (type_ != AnnotationValueType::CLASS_ARRAY) throw std::runtime_error("annotation value: not class array");
            return std::get<std::vector<std::string>>(data_);
        }
        const std::vector<int64_t>& asIntArray() const
        {
            if (type_ != AnnotationValueType::INT_ARRAY) throw std::runtime_error("annotation value: not int array");
            return std::get<std::vector<int64_t>>(data_);
        }
        const std::vector<double>& asFloatArray() const
        {
            if (type_ != AnnotationValueType::FLOAT_ARRAY) throw std::runtime_error("annotation value: not float array");
            return std::get<std::vector<double>>(data_);
        }
        const std::vector<bool>& asBoolArray() const
        {
            if (type_ != AnnotationValueType::BOOL_ARRAY) throw std::runtime_error("annotation value: not bool array");
            return std::get<std::vector<bool>>(data_);
        }
        const std::vector<std::string>& asStringArray() const
        {
            if (type_ != AnnotationValueType::STRING_ARRAY) throw std::runtime_error("annotation value: not string array");
            return std::get<std::vector<std::string>>(data_);
        }

        /// Back-compat accessor for AnnotationNode::getParameter() — formats
        /// any typed value as a display string. Empty for NULL.
        std::string toDisplayString() const
        {
            switch (type_)
            {
            case AnnotationValueType::NULL_VALUE: return "";
            case AnnotationValueType::INT:        return std::to_string(std::get<int64_t>(data_));
            case AnnotationValueType::FLOAT:      return std::to_string(std::get<double>(data_));
            case AnnotationValueType::BOOL:       return std::get<bool>(data_) ? "true" : "false";
            case AnnotationValueType::STRING:     return std::get<std::string>(data_);
            case AnnotationValueType::CLASS_REF:  return std::get<std::string>(data_);
            case AnnotationValueType::CLASS_ARRAY:
            case AnnotationValueType::STRING_ARRAY:
            {
                const auto& arr = std::get<std::vector<std::string>>(data_);
                std::string out;
                for (size_t i = 0; i < arr.size(); ++i) { if (i) out += ","; out += arr[i]; }
                return out;
            }
            case AnnotationValueType::INT_ARRAY:
            {
                const auto& arr = std::get<std::vector<int64_t>>(data_);
                std::string out;
                for (size_t i = 0; i < arr.size(); ++i) { if (i) out += ","; out += std::to_string(arr[i]); }
                return out;
            }
            case AnnotationValueType::FLOAT_ARRAY:
            {
                const auto& arr = std::get<std::vector<double>>(data_);
                std::string out;
                for (size_t i = 0; i < arr.size(); ++i) { if (i) out += ","; out += std::to_string(arr[i]); }
                return out;
            }
            case AnnotationValueType::BOOL_ARRAY:
            {
                const auto& arr = std::get<std::vector<bool>>(data_);
                std::string out;
                for (size_t i = 0; i < arr.size(); ++i) { if (i) out += ","; out += (arr[i] ? "true" : "false"); }
                return out;
            }
            }
            return "";
        }

    private:
        AnnotationValueType type_;
        std::variant<
            std::monostate,
            int64_t,
            double,
            bool,
            std::string,
            std::vector<std::string>,
            std::vector<int64_t>,
            std::vector<double>,
            std::vector<bool>> data_;
    };
}
