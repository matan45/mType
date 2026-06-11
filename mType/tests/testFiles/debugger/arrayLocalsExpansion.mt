// Debugger fixture: array-local inspection and EXPANDVARIABLE-style child
// expansion. Declares the class only; the suite drives it via interop and
// sets a breakpoint on line 18 (`return sum;`), after arr and sum are live.

class ArrayLocalsTarget {
    public int total;

    public constructor() {
        total = 0;
    }

    public function step(int delta): int {
        int[] arr = new int[3];
        arr[0] = 7;
        arr[1] = 8;
        arr[2] = 9;
        int sum = arr[0] + arr[1] + arr[2] + delta;
        return sum;
    }
}
