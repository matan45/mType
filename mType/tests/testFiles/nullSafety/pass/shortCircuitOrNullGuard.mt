// MYT-373: `x == null || x.method()` narrows x for the right operand.

class Foo {
    public bool value;

    constructor(bool v) {
        this.value = v;
    }

    public function ok(): bool {
        return this.value;
    }
}

function check(Foo? f): bool {
    return f == null || f.ok();
}

print(check(null));
print(check(new Foo(true)));
print(check(new Foo(false)));

// Expected output:
// true
// true
// false
