// MYT-225: Field access on a class/value-class variable other than `this`
// throws GET_FIELD_TYPED at runtime.
//
// EXPECTED:
//   "sum: 4"  (a.x + b.x + a.x = 1 + 2 + 1)
//
// ACTUAL (broken):
//   error[MT-E5005]: GET_FIELD_TYPED requires an object instance
//   The first occurrence of `other.x` (or any `varName.field` that isn't
//   `this.x`) throws at runtime, even though `other` is a valid instance.
//
// Probably a regression from the MYT-212 static-field-binding fix.

value class Vec {
    public int x;

    public constructor(int x) {
        this.x = x;
    }

    // `this.x` works; `other.x` throws.
    public function plusBoth(Vec other): int {
        return this.x + other.x + this.x;
    }
}

Vec a = new Vec(1);
Vec b = new Vec(2);

int sum = a.plusBoth(b);
print("sum: " + sum);
