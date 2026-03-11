#pragma once

namespace vm::profiler
{
    enum class ProfilerMode
    {
        DISABLED,
        LIGHT,
        FULL
    };

    enum class ProfilerOutputFormat
    {
        CONSOLE,
        JSON
    };
}
