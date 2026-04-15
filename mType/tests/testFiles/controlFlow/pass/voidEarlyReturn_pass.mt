// MYT-112 regression guard: bare `return;` must remain legal in void
// functions and void methods (already working; this test prevents future
// regression alongside the constructor fix).

function early(int x): void {
    if (x == 0) {
        return;
    }
    print(x);
}

class Demo {
    public function foo(int x): void {
        if (x == 0) {
            return;
        }
        print(x);
    }
}

early(0);
early(5);
Demo d = new Demo();
d.foo(0);
d.foo(7);
