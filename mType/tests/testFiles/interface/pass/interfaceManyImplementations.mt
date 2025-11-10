// Test class implementing 20+ interfaces
// @Script

interface I01 { function m01(): int; }
interface I02 { function m02(): int; }
interface I03 { function m03(): int; }
interface I04 { function m04(): int; }
interface I05 { function m05(): int; }
interface I06 { function m06(): int; }
interface I07 { function m07(): int; }
interface I08 { function m08(): int; }
interface I09 { function m09(): int; }
interface I10 { function m10(): int; }
interface I11 { function m11(): int; }
interface I12 { function m12(): int; }
interface I13 { function m13(): int; }
interface I14 { function m14(): int; }
interface I15 { function m15(): int; }
interface I16 { function m16(): int; }
interface I17 { function m17(): int; }
interface I18 { function m18(): int; }
interface I19 { function m19(): int; }
interface I20 { function m20(): int; }
interface I21 { function m21(): int; }
interface I22 { function m22(): int; }
interface I23 { function m23(): int; }
interface I24 { function m24(): int; }
interface I25 { function m25(): int; }

class ManyInterfaces implements I01, I02, I03, I04, I05, I06, I07, I08, I09, I10,
                                  I11, I12, I13, I14, I15, I16, I17, I18, I19, I20,
                                  I21, I22, I23, I24, I25 {
    public function m01(): int { return 1; }
    public function m02(): int { return 2; }
    public function m03(): int { return 3; }
    public function m04(): int { return 4; }
    public function m05(): int { return 5; }
    public function m06(): int { return 6; }
    public function m07(): int { return 7; }
    public function m08(): int { return 8; }
    public function m09(): int { return 9; }
    public function m10(): int { return 10; }
    public function m11(): int { return 11; }
    public function m12(): int { return 12; }
    public function m13(): int { return 13; }
    public function m14(): int { return 14; }
    public function m15(): int { return 15; }
    public function m16(): int { return 16; }
    public function m17(): int { return 17; }
    public function m18(): int { return 18; }
    public function m19(): int { return 19; }
    public function m20(): int { return 20; }
    public function m21(): int { return 21; }
    public function m22(): int { return 22; }
    public function m23(): int { return 23; }
    public function m24(): int { return 24; }
    public function m25(): int { return 25; }
}

ManyInterfaces impl = new ManyInterfaces();

// Test as different interface types
I01 i1 = impl;
I05 i5 = impl;
I10 i10 = impl;
I15 i15 = impl;
I20 i20 = impl;
I25 i25 = impl;

print(i1.m01());
print(i5.m05());
print(i10.m10());
print(i15.m15());
print(i20.m20());
print(i25.m25());

print("Many interfaces test passed");
