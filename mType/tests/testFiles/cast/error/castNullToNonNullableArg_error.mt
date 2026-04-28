// MYT-220: cast-of-null cannot satisfy a non-nullable parameter.

class Holder {
    public function name(): string { return "h"; }
}

function take(Holder h): void { print(h.name()); }

take((Holder)null);
