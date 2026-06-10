// MYT-380 regression fixture: class-method body locals must be exposed to the
// debugger by source name, not as local_N fallback slots.

class MethodFrameLocalNamesTarget {
    public int total;

    public constructor() {
        total = 0;
    }

    public function step(int delta): int {
        int dx = delta + 1;
        int dy = delta + 2;
        int mag = dx + dy;
        total = mag;
        return mag;
    }
}
