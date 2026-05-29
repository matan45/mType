// Skip-if-exists for @Setter: a user-declared setValue(int) with a transforming
// body must NOT be replaced by the trivial synthesized setter. The doubling body
// (storing value * 2) is observable; getValue() is still synthesized normally.
@Setter
@Getter
@NoArgsConstructor
class Clamp {
    private int value;

    public function setValue(int value): void {
        this.value = value * 2;
    }
}

Clamp c = new Clamp();
c.setValue(5);
print(c.getValue());

// Expected output:
// 10
