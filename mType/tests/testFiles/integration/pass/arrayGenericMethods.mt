// Arrays + Generics Test 3: Generic array methods (sort, search)
@Script

class ArrayUtils<T> {
    public function linearSearch(T[] arr, Int size, T target): Int {
        Int i = 0;
        while (i < size) {
            if (arr[i] == target) {
                return i;
            }
            i = i + 1;
        }
        return -1;
    }

    public function reverse(T[] arr, Int size): void {
        Int left = 0;
        Int right = size - 1;
        while (left < right) {
            T temp = arr[left];
            arr[left] = arr[right];
            arr[right] = temp;
            left = left + 1;
            right = right - 1;
        }
    }

    public function count(T[] arr, Int size, T target): Int {
        Int result = 0;
        Int i = 0;
        while (i < size) {
            if (arr[i] == target) {
                result = result + 1;
            }
            i = i + 1;
        }
        return result;
    }
}

ArrayUtils<Int> intUtils = ArrayUtils<Int>();
Int[] intArr = Int[7];
intArr[0] = 5;
intArr[1] = 10;
intArr[2] = 5;
intArr[3] = 20;
intArr[4] = 5;
intArr[5] = 30;
intArr[6] = 10;

print("Original array:");
Int i = 0;
while (i < 7) {
    print(intArr[i]);
    i = i + 1;
}

Int index = intUtils.linearSearch(intArr, 7, 20);
print("Index of 20:");
print(index);

Int count = intUtils.count(intArr, 7, 5);
print("Count of 5:");
print(count);

intUtils.reverse(intArr, 7);
print("Reversed array:");
i = 0;
while (i < 7) {
    print(intArr[i]);
    i = i + 1;
}

ArrayUtils<String> strUtils = ArrayUtils<String>();
String[] strArr = String[4];
strArr[0] = "hello";
strArr[1] = "world";
strArr[2] = "test";
strArr[3] = "hello";

Int strCount = strUtils.count(strArr, 4, "hello");
print("Count of 'hello':");
print(strCount);
