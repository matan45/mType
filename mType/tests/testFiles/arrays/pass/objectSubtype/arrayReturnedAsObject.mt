// MYT-281/282: a function with `Object` return type can return an array.
// The return-type validator's array→Object exemption applies on the
// return path the same way it applies on assignment / parameter passing.
print("Testing array returned as Object");

function makeIntArr(): Object {
    int[] a = new int[3];
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    return a;
}

function makeStringArr(): Object {
    string[] s = new string[2];
    s[0] = "hello";
    s[1] = "world";
    return s;
}

Object o1 = makeIntArr();
int[] back1 = (int[])o1;
print("Int[] back[0]: " + back1[0]);
print("Int[] back length: " + back1.length);

Object o2 = makeStringArr();
string[] back2 = (string[])o2;
print("String[] back[0]: " + back2[0]);
print("String[] back[1]: " + back2[1]);

print("Done");
