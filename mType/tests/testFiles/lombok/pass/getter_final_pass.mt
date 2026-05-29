// @Getter generates accessors for final fields too (only @Setter skips them).
@Getter
@AllArgsConstructor
class Const {
    private final int value;
}

Const c = new Const(99);
print(c.getValue());
