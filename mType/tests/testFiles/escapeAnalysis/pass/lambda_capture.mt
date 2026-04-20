class Holder {
    public int n;
    public constructor(int a) {
        this.n = a;
    }
}

interface IntSupplier {
    public function get(): int;
}

function makeSupplier(): IntSupplier {
    Holder h = new Holder(21);
    IntSupplier s = (): int -> h.n + 1;
    return s;
}

IntSupplier sup = makeSupplier();
print(sup.get());
