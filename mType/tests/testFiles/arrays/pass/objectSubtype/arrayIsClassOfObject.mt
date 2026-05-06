// MYT-281: `arr isClassOf Object` is true for any array-shaped Value.
print("Testing array isClassOf Object");

int[] a = new int[1];
a[0] = 42;

if (a isClassOf Object) {
    print("int[] is Object: yes");
} else {
    print("int[] is Object: no");
}

string[] s = new string[1];
s[0] = "x";
if (s isClassOf Object) {
    print("string[] is Object: yes");
} else {
    print("string[] is Object: no");
}

print("Done");
