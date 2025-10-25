// Test unbounded wildcard error (if not supported)
import * from "../../lib/collections/List.mt";

class Container<T> {
    T value;
}

function main(): void {
    // ERROR: Unbounded wildcards <?> may not be supported
    List<?> unknownList = new List<int>();
    Container<?> unknownContainer = new Container<int>();

    print("This should not execute");
}

main();
