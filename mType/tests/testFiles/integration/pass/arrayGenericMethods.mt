// Arrays + Generics Test 3: Generic array methods (sort, search)
@Script

class ArrayUtils<T> {
    fun linearSearch(arr: T[], size: Int, target: T): Int {
        let i: Int = 0;
        while (i < size) {
            if (arr[i] == target) {
                return i;
            }
            i = i + 1;
        }
        return -1;
    }

    fun reverse(arr: T[], size: Int): Void {
        let left: Int = 0;
        let right: Int = size - 1;
        while (left < right) {
            let temp: T = arr[left];
            arr[left] = arr[right];
            arr[right] = temp;
            left = left + 1;
            right = right - 1;
        }
    }

    fun count(arr: T[], size: Int, target: T): Int {
        let result: Int = 0;
        let i: Int = 0;
        while (i < size) {
            if (arr[i] == target) {
                result = result + 1;
            }
            i = i + 1;
        }
        return result;
    }
}

let intUtils: ArrayUtils<Int> = ArrayUtils<Int>();
let intArr: Int[] = Int[7];
intArr[0] = 5;
intArr[1] = 10;
intArr[2] = 5;
intArr[3] = 20;
intArr[4] = 5;
intArr[5] = 30;
intArr[6] = 10;

print("Original array:");
let i: Int = 0;
while (i < 7) {
    print(intArr[i]);
    i = i + 1;
}

let index: Int = intUtils.linearSearch(intArr, 7, 20);
print("Index of 20:");
print(index);

let count: Int = intUtils.count(intArr, 7, 5);
print("Count of 5:");
print(count);

intUtils.reverse(intArr, 7);
print("Reversed array:");
i = 0;
while (i < 7) {
    print(intArr[i]);
    i = i + 1;
}

let strUtils: ArrayUtils<String> = ArrayUtils<String>();
let strArr: String[] = String[4];
strArr[0] = "hello";
strArr[1] = "world";
strArr[2] = "test";
strArr[3] = "hello";

let strCount: Int = strUtils.count(strArr, 4, "hello");
print("Count of 'hello':");
print(strCount);
