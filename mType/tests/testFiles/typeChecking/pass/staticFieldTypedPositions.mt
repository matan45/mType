// MYT-377: `Class::FIELD` static field access must infer the field's declared
// type in typed parameter and typed assignment positions. Previously inferred
// as void, so any typed position rejected it with "expects <T> but got void".
// Covers int, float, bool, string, object, and array field types.

class Pet {
    public string name;
    public constructor(string n) { name = n; }
    public function speak(): string { return name + " barks"; }
}

class Cfg {
    public static final int    PORT  = 5432;
    public static final float  RATIO = 1.5;
    public static final bool   DEBUG = false;
    public static final string HOST  = "localhost";
    public static final int[]  PORTS = [80, 443, 8080];
    public static final Pet    PET   = new Pet("Rex");
}

function takeInt(int x): void { print("int=" + x); }
function takeFloat(float x): void { print("float=" + x); }
function takeBool(bool x): void { print("bool=" + x); }
function takeStr(string x): void { print("string=" + x); }
function takeArr(int[] x): void { print("arr0=" + x[0]); }
function takeObj(Pet p): void { print("obj=" + p.speak()); }

// (a) typed parameter positions
takeInt(Cfg::PORT);
takeFloat(Cfg::RATIO);
takeBool(Cfg::DEBUG);
takeStr(Cfg::HOST);
takeArr(Cfg::PORTS);
takeObj(Cfg::PET);

// (b) typed assignment positions
int    p   = Cfg::PORT;
float  r   = Cfg::RATIO;
bool   d   = Cfg::DEBUG;
string h   = Cfg::HOST;
int[]  a   = Cfg::PORTS;
Pet    pet = Cfg::PET;

print("assignInt=" + p);
print("assignFloat=" + r);
print("assignBool=" + d);
print("assignStr=" + h);
print("assignArr=" + a[2]);
print("assignObj=" + pet.speak());
