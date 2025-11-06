// Arrays + Class Test 4: Class methods returning arrays
@Script

class ArrayFactory {
    function createIntArray(size: Int, fillValue: Int): Int[] {
        Int[] arr = Int[size];
        Int i = 0;
        while (i < size) {
            arr[i] = fillValue;
            i = i + 1;
        }
        return arr;
    }

    function createSequence(start: Int, count: Int): Int[] {
        Int[] arr = Int[count];
        Int i = 0;
        while (i < count) {
            arr[i] = start + i;
            i = i + 1;
        }
        return arr;
    }

    function createStringArray(size: Int, prefix: String): String[] {
        String[] arr = String[size];
        Int i = 0;
        while (i < size) {
            arr[i] = prefix;
            i = i + 1;
        }
        return arr;
    }
}

class DataProcessor {
    Int[] data;
    Int size;

    constructor(arr: Int[], s: Int) {
        this.data = arr;
        this.size = s;
    }

    function filter(threshold: Int): Int[] {
        Int count = 0;
        Int i = 0;
        while (i < this.size) {
            if (this.data[i] > threshold) {
                count = count + 1;
            }
            i = i + 1;
        }

        Int[] result = Int[count];
        Int j = 0;
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

    function transform(multiplier: Int): Int[] {
        Int[] result = Int[this.size];
        Int i = 0;
        while (i < this.size) {
            result[i] = this.data[i] * multiplier;
            i = i + 1;
        }
        return result;
    }

    function split(): Int[][] {
        Int half = this.size / 2;
        Int[][] result = Int[][2];
        result[0] = Int[half];
        result[1] = Int[this.size - half];

        Int i = 0;
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
ArrayFactory factory = ArrayFactory();

Int[] filled = factory.createIntArray(4, 42);
print("Filled array:");
Int i = 0;
while (i < 4) {
    print(filled[i]);
    i = i + 1;
}

Int[] seq = factory.createSequence(10, 5);
print("Sequence:");
i = 0;
while (i < 5) {
    print(seq[i]);
    i = i + 1;
}

String[] strings = factory.createStringArray(3, "test");
print("String array:");
i = 0;
while (i < 3) {
    print(strings[i]);
    i = i + 1;
}

print("DataProcessor tests:");
Int[] data = Int[6];
data[0] = 5;
data[1] = 15;
data[2] = 3;
data[3] = 25;
data[4] = 8;
data[5] = 30;

DataProcessor processor = DataProcessor(data, 6);

Int[] filtered = processor.filter(10);
print("Filtered (> 10):");
i = 0;
while (i < 3) {
    print(filtered[i]);
    i = i + 1;
}

Int[] transformed = processor.transform(2);
print("Transformed (* 2):");
i = 0;
while (i < 6) {
    print(transformed[i]);
    i = i + 1;
}

Int[][] parts = processor.split();
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
