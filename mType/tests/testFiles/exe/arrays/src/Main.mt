// Test: Arrays in standalone exe

function sumArray(int[] arr): int {
    int total = 0;
    for (int i = 0; i < arr.length; i = i + 1) {
        total = total + arr[i];
    }
    return total;
}

function reverseStrings(string[] arr): string[] {
    string[] result = new string[arr.length];
    for (int i = 0; i < arr.length; i = i + 1) {
        result[arr.length - 1 - i] = arr[i];
    }
    return result;
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Int array
        int[] numbers = new int[5];
        for (int i = 0; i < 5; i = i + 1) {
            numbers[i] = (i + 1) * 10;
        }
        print("Array length: " + numbers.length);
        print("First: " + numbers[0]);
        print("Last: " + numbers[4]);

        // Array as parameter
        int total = sumArray(numbers);
        print("Sum: " + total);

        // String array
        string[] names = new string[3];
        names[0] = "Alice";
        names[1] = "Bob";
        names[2] = "Charlie";

        // Array as return type
        string[] reversed = reverseStrings(names);
        for (int i = 0; i < reversed.length; i = i + 1) {
            print("Reversed[" + i + "]: " + reversed[i]);
        }

        // Float array
        float[] floats = new float[3];
        floats[0] = 1.5;
        floats[1] = 2.5;
        floats[2] = 3.5;
        float fsum = floats[0] + floats[1] + floats[2];
        print("Float sum: " + fsum);

        // Array iteration with for loop
        print("Numbers:");
        for (int i = 0; i < 5; i = i + 1) {
            print("  " + numbers[i]);
        }

        print("Arrays test passed");
    }
}
