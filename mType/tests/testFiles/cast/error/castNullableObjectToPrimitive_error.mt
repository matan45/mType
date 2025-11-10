// Error: Cannot cast nullable object to primitive type
class Wrapper {
    int value;
}

Wrapper? nullableWrapper = null;
int result = (int)nullableWrapper;
