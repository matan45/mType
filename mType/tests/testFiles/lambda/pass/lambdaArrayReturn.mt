// Lambda returning arrays test
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

print("=== Array Return Test ===");

// Lambda returning int array
Function<Int, Int[]> rangeGenerator = n -> {
    Int[] result = new Int[n.getValue()];
    for (int i = 0; i < n.getValue(); i = i + 1) {
        result[i] = i + 1;
    }
    return result;
};

Int[] arr1 = rangeGenerator.apply(5);
print("Range of 5:");
for (int i = 0; i < arr1.length; i = i + 1) {
    print(arr1[i].toString());
}

// Lambda returning String array
Function<Int, String[]> stringArrayGen = count -> {
    String[] result = new String[count.getValue()];
    for (int i = 0; i < count.getValue(); i = i + 1) {
        result[i] = "Item-" + i;
    }
    return result;
};

String[] arr2 = stringArrayGen.apply(3);
print("String array:");
for (int i = 0; i < arr2.length; i = i + 1) {
    print(arr2[i]);
}

// Lambda transforming arrays
Function<int[], int[]> doubler = input -> {
    int[] result = new int[input.length];
    for (int i = 0; i < input.length; i = i + 1) {
        result[i] = input[i] * 2;
    }
    return result;
};

int[] original = [1, 2, 3, 4];
int[] doubled = doubler.apply(original);
print("Doubled:");
for (int i = 0; i < doubled.length; i = i + 1) {
    print(doubled[i]);
}

print("Array return complete");
