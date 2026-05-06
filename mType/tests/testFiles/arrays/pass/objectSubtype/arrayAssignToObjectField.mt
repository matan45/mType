// MYT-281: an `Object` field accepts arrays of any element type.
print("Testing array assigned to Object field");

class Holder {
    public Object data;

    constructor() {
        data = null;
    }
}

Holder h1 = new Holder();
int[] ints = new int[2];
ints[0] = 7;
ints[1] = 8;
h1.data = ints;

int[] back1 = (int[])h1.data;
print("Int round-trip: " + back1[0] + ", " + back1[1]);

Holder h2 = new Holder();
string[] strs = new string[2];
strs[0] = "hello";
strs[1] = "world";
h2.data = strs;

string[] back2 = (string[])h2.data;
print("String round-trip: " + back2[0] + " " + back2[1]);

print("Done");
