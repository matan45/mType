// Test: Class cannot extend interface
// Should fail at parse time with clear error message

interface Comparable {
    function compareTo(other: Comparable): int;
}

class MyClass extends Comparable {
    function compareTo(other: Comparable): int {
        return 0;
    }
}
