// Arrays + Class Test 4: Class methods returning arrays
@Script

class ArrayFactory {
    fun createIntArray(size: Int, fillValue: Int): Int[] {
        let arr: Int[] = Int[size];
        let i: Int = 0;
        while (i < size) {
            arr[i] = fillValue;
            i = i + 1;
        }
        return arr;
    }

    fun createSequence(start: Int, count: Int): Int[] {
        let arr: Int[] = Int[count];
        let i: Int = 0;
        while (i < count) {
            arr[i] = start + i;
            i = i + 1;
        }
        return arr;
    }

    fun createStringArray(size: Int, prefix: String): String[] {
        let arr: String[] = String[size];
        let i: Int = 0;
        while (i < size) {
            arr[i] = prefix;
            i = i + 1;
        }
        return arr;
    }
}

class DataProcessor {
    field data: Int[];
    field size: Int;

    constructor(arr: Int[], s: Int) {
        this.data = arr;
        this.size = s;
    }

    fun filter(threshold: Int): Int[] {
        let count: Int = 0;
        let i: Int = 0;
        while (i < this.size) {
            if (this.data[i] > threshold) {
                count = count + 1;
            }
            i = i + 1;
        }

        let result: Int[] = Int[count];
        let j: Int = 0;
        i = 0;
        while (i < this.size) {
            if (this.data[i] > threshold) {
                result[j] = this.data[i];
                j = j + 1;
            }
            i = i + 1;
        }
        return result;
    }

    fun transform(multiplier: Int): Int[] {
        let result: Int[] = Int[this.size];
        let i: Int = 0;
        while (i < this.size) {
            result[i] = this.data[i] * multiplier;
            i = i + 1;
        }
        return result;
    }

    fun split(): Int[][] {
        let half: Int = this.size / 2;
        let result: Int[][] = Int[][2];
        result[0] = Int[half];
        result[1] = Int[this.size - half];

        let i: Int = 0;
        while (i < half) {
            result[0][i] = this.data[i];
            i = i + 1;
        }

        i = half;
        while (i < this.size) {
            result[1][i - half] = this.data[i];
            i = i + 1;
        }
        return result;
    }
}

print("ArrayFactory tests:");
let factory: ArrayFactory = ArrayFactory();

let filled: Int[] = factory.createIntArray(4, 42);
print("Filled array:");
let i: Int = 0;
while (i < 4) {
    print(filled[i]);
    i = i + 1;
}

let seq: Int[] = factory.createSequence(10, 5);
print("Sequence:");
i = 0;
while (i < 5) {
    print(seq[i]);
    i = i + 1;
}

let strings: String[] = factory.createStringArray(3, "test");
print("String array:");
i = 0;
while (i < 3) {
    print(strings[i]);
    i = i + 1;
}

print("DataProcessor tests:");
let data: Int[] = Int[6];
data[0] = 5;
data[1] = 15;
data[2] = 3;
data[3] = 25;
data[4] = 8;
data[5] = 30;

let processor: DataProcessor = DataProcessor(data, 6);

let filtered: Int[] = processor.filter(10);
print("Filtered (> 10):");
i = 0;
while (i < 3) {
    print(filtered[i]);
    i = i + 1;
}

let transformed: Int[] = processor.transform(2);
print("Transformed (* 2):");
i = 0;
while (i < 6) {
    print(transformed[i]);
    i = i + 1;
}

let parts: Int[][] = processor.split();
print("Split part 1:");
i = 0;
while (i < 3) {
    print(parts[0][i]);
    i = i + 1;
}
print("Split part 2:");
i = 0;
while (i < 3) {
    print(parts[1][i]);
    i = i + 1;
}
