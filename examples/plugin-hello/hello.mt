// Demonstrates loading a native plugin built by examples/plugin-hello/hello.cpp.
// Run from this directory after building:  mType.exe hello.mt

function double(int n): int {
    return n * 2
}

// Extension is platform-specific: ".so" on Linux, ".dylib" on macOS.
__plugin_load("./hello.dll")

print(__native__hello_greet("world"))
print(__native__hello_greet("native plugin"))

// Plugin -> mType reentrancy: __native__hello_apply_twice calls double() twice.
// double(double(3)) == 12.
print(__native__hello_apply_twice("double", 3))

// Plugin enumerates all registered functions (mType + native) via listFunctions.
print(__native__hello_count_natives())

__plugin_unload("./hello.dll")
