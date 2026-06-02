// MYT-373: `x != null && x.method()` narrows x for the right operand.

class Foo {
    public function ok(): bool {
        return true;
    }
}

function check(Foo? f): bool {
    return f != null && f.ok();
}

print(check(new Foo()));
print(check(null));

// Expected output:
// true
// false
