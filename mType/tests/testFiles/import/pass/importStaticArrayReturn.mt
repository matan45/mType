// MYT-278: cross-file static method call returning a typed array must be
// directly returnable from a function declared with the same array type.
import { A } from "./staticArrayReturn/A.mt";

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
