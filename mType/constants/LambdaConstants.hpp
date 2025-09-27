#pragma once

namespace constants {
    namespace lambda {
        // Field name used to store lambda implementations in interface instances
        constexpr const char* LAMBDA_FIELD_NAME = "__lambda";

        // Prefix for lambda-generated class names
        constexpr const char* LAMBDA_CLASS_PREFIX = "__LambdaImpl_";

        // Method name for single abstract method in functional interfaces
        constexpr const char* DEFAULT_SAM_METHOD = "invoke";
    }
}