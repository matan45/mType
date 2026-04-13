class ArrayHelper {
    public static function createIntArray(): int[] {
        int[] arr = new int[3];
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        return arr;
    }

    public static function sum(int[] arr): int {
        int total = 0;
        for (int i = 0; i < arr.length; i = i + 1) {
            total = total + arr[i];
        }
        return total;
    }
}

int[] nums = ArrayHelper::createIntArray();
print(nums.length);
print(ArrayHelper::sum(nums));
