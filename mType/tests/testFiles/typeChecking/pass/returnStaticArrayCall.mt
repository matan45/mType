// MYT-278: returning a static method call typed `T[]` from a function typed `T[]`
// must type-check (previously failed with "expected string[] but got object").
print("Testing return of static array call");

class A {
    public static function names(): string[] {
        string[] arr = new string[2];
        arr[0] = "alpha";
        arr[1] = "beta";
        return arr;
    }

    public static function counts(): int[] {
        int[] arr = new int[3];
        arr[0] = 1;
        arr[1] = 2;
        arr[2] = 3;
        return arr;
    }
}

class B {
    public function getNames(): string[] {
        return A::names();
    }

    public function getCounts(): int[] {
        return A::counts();
    }
}

B b = new B();
string[] ns = b.getNames();
int[] cs = b.getCounts();
print(ns[0]);
print(ns[1]);
print(cs[0]);
print(cs[1]);
print(cs[2]);
print("Done");
