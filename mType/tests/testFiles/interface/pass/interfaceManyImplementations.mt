// Test class implementing 20+ interfaces
// @Script

interface I01 { func m01(): Int; }
interface I02 { func m02(): Int; }
interface I03 { func m03(): Int; }
interface I04 { func m04(): Int; }
interface I05 { func m05(): Int; }
interface I06 { func m06(): Int; }
interface I07 { func m07(): Int; }
interface I08 { func m08(): Int; }
interface I09 { func m09(): Int; }
interface I10 { func m10(): Int; }
interface I11 { func m11(): Int; }
interface I12 { func m12(): Int; }
interface I13 { func m13(): Int; }
interface I14 { func m14(): Int; }
interface I15 { func m15(): Int; }
interface I16 { func m16(): Int; }
interface I17 { func m17(): Int; }
interface I18 { func m18(): Int; }
interface I19 { func m19(): Int; }
interface I20 { func m20(): Int; }
interface I21 { func m21(): Int; }
interface I22 { func m22(): Int; }
interface I23 { func m23(): Int; }
interface I24 { func m24(): Int; }
interface I25 { func m25(): Int; }

class ManyInterfaces implements I01, I02, I03, I04, I05, I06, I07, I08, I09, I10,
                                  I11, I12, I13, I14, I15, I16, I17, I18, I19, I20,
                                  I21, I22, I23, I24, I25 {
    func m01(): Int { return 1; }
    func m02(): Int { return 2; }
    func m03(): Int { return 3; }
    func m04(): Int { return 4; }
    func m05(): Int { return 5; }
    func m06(): Int { return 6; }
    func m07(): Int { return 7; }
    func m08(): Int { return 8; }
    func m09(): Int { return 9; }
    func m10(): Int { return 10; }
    func m11(): Int { return 11; }
    func m12(): Int { return 12; }
    func m13(): Int { return 13; }
    func m14(): Int { return 14; }
    func m15(): Int { return 15; }
    func m16(): Int { return 16; }
    func m17(): Int { return 17; }
    func m18(): Int { return 18; }
    func m19(): Int { return 19; }
    func m20(): Int { return 20; }
    func m21(): Int { return 21; }
    func m22(): Int { return 22; }
    func m23(): Int { return 23; }
    func m24(): Int { return 24; }
    func m25(): Int { return 25; }
}

var impl = new ManyInterfaces();

// Test as different interface types
var i1: I01 = impl;
var i5: I05 = impl;
var i10: I10 = impl;
var i15: I15 = impl;
var i20: I20 = impl;
var i25: I25 = impl;

print(i1.m01());
print(i5.m05());
print(i10.m10());
print(i15.m15());
print(i20.m20());
print(i25.m25());

print("Many interfaces test passed");
