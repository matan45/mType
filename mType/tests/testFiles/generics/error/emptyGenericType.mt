// Test: Empty generic type arguments
// Should fail at parse time with clear error message

import * from "../../lib/collections/List.mt";

class Container<T> {
    List<> items;  // Error: empty generic type
}
