// Synthesis is skip-if-exists: a user-declared getSize() must NOT be
// overwritten by @Getter. The hand-written body (returning 99, not the field)
// proves the synthesized getter was suppressed.
@Getter
class Widget {
    private int size = 5;

    public function getSize(): int {
        return 99;
    }
}

Widget w = new Widget();
print(w.getSize());
