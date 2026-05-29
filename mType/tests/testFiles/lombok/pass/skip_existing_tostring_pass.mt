// Synthesis is skip-if-exists for @ToString too: a hand-written toString()
// must NOT be overwritten. The custom body returns a sentinel that ignores the
// field, proving the synthesized structural toString was suppressed.
@ToString
@AllArgsConstructor
class Box {
    private int v;

    public function toString(): string {
        return "custom";
    }
}

Box b = new Box(5);
print(b.toString());

// Expected output:
// custom
