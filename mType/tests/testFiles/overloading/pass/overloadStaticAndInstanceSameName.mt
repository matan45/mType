// Test: Both an instance method and a static method with the same name on
// the same class are reachable. Instance-call uses the receiver; static-call
// uses the class scope.

class C {
    public function m(): void {
        print("instance");
    }

    public static function m(int x): void {
        print("static " + x);
    }
}

function main(): void {
    C obj = new C();
    obj.m();
    C::m(42);
}

main();
