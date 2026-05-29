// @Getter + @Setter with NO constructor annotation: synthesis only adds
// methods, so the class still has zero declared constructors and the compiler's
// default no-arg constructor is auto-generated — `new Bean()` works.
@Getter
@Setter
class Bean {
    private int n = 0;
}

Bean b = new Bean();
b.setN(7);
print(b.getN());
