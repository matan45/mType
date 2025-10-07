// Generic function
function <T> identity(T value): T {
    return value;
}

// Error: Using primitive type instead of object type
int result = identity<int>(42);
