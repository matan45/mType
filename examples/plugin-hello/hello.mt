// Demonstrates loading a native plugin built by examples/plugin-hello/hello.cpp.
// Run from this directory after building:  mType.exe hello.mt

function double(int n): int {
    return n * 2
}

__plugin_load("./hello.dll")

print(__hello_greet("world"))
print(__hello_greet("native plugin"))

// Plugin -> mType reentrancy: __hello_apply_twice calls double() twice.
// double(double(3)) == 12.
print(__hello_apply_twice("double", 3))

// Plugin enumerates all registered functions (mType + native) via listFunctions.
print(__hello_count_natives())

__plugin_unload("./hello.dll")
