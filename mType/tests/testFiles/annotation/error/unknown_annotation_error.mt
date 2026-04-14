// MYT-108 §7b case 1: usage of an undeclared annotation must fail with a
// clear error naming the annotation.

class Foo {
    @Bogus
    public function bar(): void {
        print("never reached");
    }
}

new Foo().bar();
