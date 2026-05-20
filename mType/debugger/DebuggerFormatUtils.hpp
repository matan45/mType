#pragma once

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "DebuggerConstants.hpp"
#include "VariableInspector.hpp"
#include "../value/ValueShim.hpp"

namespace debugger
{
    inline std::string formatMultiDimIndex(const std::vector<size_t>& indices)
    {
        std::string s = "[";
        for (size_t i = 0; i < indices.size(); ++i)
        {
            if (i > 0) s += ",";
            s += std::to_string(indices[i]);
        }
        s += "]";
        return s;
    }

    template <typename ArrayPtr, typename ToVar>
    void collectFlatMultiChildren(const ArrayPtr& arr,
                                  const char* diagLabel,
                                  ToVar&& toVar,
                                  std::vector<DebugVariable>& children)
    {
        if (!arr) return;
        auto dims = arr->getDimensions();
        if (dims.empty()) return;

        const size_t firstDim = dims[0];
        const size_t limit = std::min(firstDim, constants::MAX_ARRAY_DISPLAY_ELEMENTS);
        const bool nested = arr->getRank() > 1;

        for (size_t i = 0; i < limit; ++i)
        {
            try
            {
                std::string name = "[" + std::to_string(i) + "]";
                value::Value element = nested
                    ? value::Value(arr->getSubArray(i))
                    : arr->get(std::vector<size_t>{i});
                children.push_back(toVar(name, element));
            }
            catch (const std::exception& e)
            {
                std::cerr << "VMVariableInspector::getVariableChildren() - "
                          << diagLabel << "[" << i << "]: " << e.what() << "\n";
            }
        }
        if (firstDim > limit)
        {
            children.push_back(DebugVariable(
                "[...]",
                "(" + std::to_string(firstDim - limit) + " more elements)",
                "info", false, 0));
        }
    }
} // namespace debugger
