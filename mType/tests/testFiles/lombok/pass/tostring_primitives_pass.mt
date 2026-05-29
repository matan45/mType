// @ToString coerces every primitive field type (int, float, bool, string)
// directly in the concatenation — no .toString() call. Existing tostring_pass
// only exercises int; this pins the format for all four primitives.
@ToString
@AllArgsConstructor
class Mixed {
    private int count;
    private float ratio;
    private bool active;
    private string label;
}

Mixed m = new Mixed(2, 1.5, true, "hi");
print(m.toString());

// Expected output:
// Mixed(count=2, ratio=1.5, active=true, label=hi)
