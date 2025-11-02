// Test array return type inference
print("Testing array return type inference");

int[] doubleValues(int[] input) {
    int[] result = new int[input.length];
    for (int i = 0; i < input.length; i++) {
        result[i] = input[i] * 2;
    }
    return result;
}

int[] source = new int[4];
source[0] = 1;
source[1] = 2;
source[2] = 3;
source[3] = 4;

int[] doubled = doubleValues(source);
print("Doubled values:");
for (int i = 0; i < doubled.length; i++) {
    print("  " + doubled[i]);
}

string[] reverseArray(string[] input) {
    string[] result = new string[input.length];
    for (int i = 0; i < input.length; i++) {
        result[i] = input[input.length - 1 - i];
    }
    return result;
}

string[] words = new string[3];
words[0] = "One";
words[1] = "Two";
words[2] = "Three";

string[] reversed = reverseArray(words);
print("Reversed array:");
for (int i = 0; i < reversed.length; i++) {
    print("  " + reversed[i]);
}

print("Array return type inference test completed");
