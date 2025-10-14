// Test Generic Type Arrays with SIMD and SoA Optimizations
// Generic arrays automatically benefit from optimizations when instantiated
// with primitive types (SIMD) or object types (SoA)
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/Bool.mt";
// ============================================
// Generic Container Classes
// ============================================

class Container<T> {
    public T[] items;
    public int size;

    public constructor(int capacity) {
        this.items = new T[capacity];
        this.size = 0;
    }

    public function add(T item): void {
        this.items[this.size] = item;
        this.size = this.size + 1;
    }

    public function get(int index): T {
        return this.items[index];
    }

    public function getSize(): int {
        return this.size;
    }
}

class Pair<K, V> {
    public K key;
    public V value;

    public constructor(K key, V value) {
        this.key = key;
        this.value = value;
    }
}

class Grid<T> {
    public T[][] data;
    public int rows;
    public int cols;

    public constructor(int rows, int cols) {
        this.data = new T[rows][cols];
        this.rows = rows;
        this.cols = cols;
    }

    public function set(int row, int col, T value): void {
        this.data[row][col] = value;
    }

    public function get(int row, int col): T {
        return this.data[row][col];
    }
}

class DataPoint {
    public int id;
    public string label;
    public float value;

    public constructor(int id, string label, float value) {
        this.id = id;
        this.label = label;
        this.value = value;
    }
}

// ============================================
// 1D Generic Arrays - SIMD Optimization
// ============================================

// Test 1.1: Generic Container with int (SIMD optimized for size >= 16)
Container<Int> intContainer = new Container<Int>(20);
for (int i = 0; i < 20; i = i + 1) {
    intContainer.add(new Int(i * 5));
}

print("Int Container[5]: " + intContainer.get(5).toString()); // Expected: 25
print("Int Container Size: " + intContainer.getSize()); // Expected: 20

// Test 1.2: Generic Container with float (SIMD optimized)
Container<Float> floatContainer = new Container<Float>(25);
for (int i = 0; i < 25; i = i + 1) {
    floatContainer.add(new Float(i * 2.5));
}

print("Float Container[10]: " + floatContainer.get(10).toString()); // Expected: 25.0

// Test 1.3: Generic Container with bool (SIMD optimized)
Container<Bool> boolContainer = new Container<Bool>(30);
for (int i = 0; i < 30; i = i + 1) {
    boolContainer.add(new Bool(i % 3 == 0));
}

int trueBoolCount = 0;
for (int i = 0; i < boolContainer.getSize(); i = i + 1) {
    if (boolContainer.get(i)) {
        trueBoolCount = trueBoolCount + 1;
    }
}
print("True Bool Count in Container: " + trueBoolCount); // Expected: 10

// Test 1.4: Generic Container with string (StringPool optimized)
Container<String> stringContainer = new Container<String>(20);
for (int i = 0; i < 20; i = i + 1) {
    if (i % 4 == 0) {
        stringContainer.add(new String("Red"));
    }
    if (i % 4 == 1) {
        stringContainer.add(new String("Green"));
    }
    if (i % 4 == 2) {
        stringContainer.add(new String("Blue"));
    }
    if (i % 4 == 3) {
        stringContainer.add(new String("Yellow"));
    }
}

print("String Container[4]: " + stringContainer.get(4).toString()); // Expected: Red

// ============================================
// 1D Generic Arrays - SoA Optimization for Objects
// ============================================

// Test 2.1: Generic Container with DataPoint objects (SoA optimized)
Container<DataPoint> dataContainer = new Container<DataPoint>(25);
for (int i = 0; i < 25; i = i + 1) {
    dataContainer.add(new DataPoint(100 + i, "Data" + i, i * 10.5));
}

print("DataPoint Container[10] ID: " + dataContainer.get(10).id);      // Expected: 110
print("DataPoint Container[10] Label: " + dataContainer.get(10).label); // Expected: Data10
print("DataPoint Container[10] Value: " + dataContainer.get(10).value); // Expected: 105.0

// Calculate average value (field access benefits from SoA)
float totalValue = 0.0;
for (int i = 0; i < dataContainer.getSize(); i = i + 1) {
    totalValue = totalValue + dataContainer.get(i).value;
}
float avgValue = totalValue / dataContainer.getSize();
print("Average DataPoint Value: " + avgValue); // Expected: 126.0

// ============================================
// 2D Generic Arrays - Multi-dimensional Optimization
// ============================================

// Test 3.1: Generic Grid with Int (2D SIMD optimization)
Grid<Int> intGrid = new Grid<Int>(5, 5);
for (int i = 0; i < 5; i = i + 1) {
    for (int j = 0; j < 5; j = j + 1) {
        intGrid.set(i, j, new Int(i * 5 + j));
    }
}

print("Int Grid[3][2]: " + intGrid.get(3, 2).toString()); // Expected: 17

// Test 3.2: Generic Grid with Float (2D SIMD optimization)
Grid<Float> floatGrid = new Grid<Float>(4, 6);
for (int i = 0; i < 4; i = i + 1) {
    for (int j = 0; j < 6; j = j + 1) {
        floatGrid.set(i, j, new Float(i * 2.5 + j * 0.5));
    }
}

print("Float Grid[2][4]: " + floatGrid.get(2, 4).toString()); // Expected: 7.0

// Test 3.3: Generic Grid with string (2D StringPool optimization)
Grid<String> stringGrid = new Grid<String>(5, 5);
for (int i = 0; i < 5; i = i + 1) {
    for (int j = 0; j < 5; j = j + 1) {
        if ((i + j) % 2 == 0) {
            stringGrid.set(i, j, new String("Even"));
        }
        if ((i + j) % 2 == 1) {
            stringGrid.set(i, j, new String("Odd"));
        }
    }
}

print("String Grid[1][1]: " + stringGrid.get(1, 1).toString()); // Expected: Even

// Test 3.4: Generic Grid with DataPoint objects (2D SoA optimization)
Grid<DataPoint> dataGrid = new Grid<DataPoint>(4, 5);
for (int i = 0; i < 4; i = i + 1) {
    for (int j = 0; j < 5; j = j + 1) {
        int idx = i * 5 + j;
        dataGrid.set(i, j, new DataPoint(200 + idx, "Grid" + idx, idx * 5.5));
    }
}

print("DataGrid[2][3] ID: " + dataGrid.get(2, 3).id);      // Expected: 213
print("DataGrid[2][3] Label: " + dataGrid.get(2, 3).label); // Expected: Grid13

// ============================================
// Generic Pair Arrays - Mixed Optimization
// ============================================

// Test 4.1: Array of Pair<Int, string> - 20 elements (SoA for Pair objects)
Pair<Int, String>[] pairs = new Pair<Int, String>[20];
for (int i = 0; i < 20; i = i + 1) {
    pairs[i] = new Pair<Int, String>(new Int(i), new String("Value" + i));
}

print("Pair[10] Key: " + pairs[10].key.toString());     // Expected: 10
print("Pair[10] Value: " + pairs[10].value.toString()); // Expected: Value10

// Test 4.2: Array of Pair<string, Int> - 25 elements
Pair<String, Int>[] reversePairs = new Pair<String, Int>[25];
for (int i = 0; i < 25; i = i + 1) {
    reversePairs[i] = new Pair<String, Int>(new String("Key" + i), new Int(i * 100));
}

print("ReversePair[15] Key: " + reversePairs[15].key.toString());     // Expected: Key15
print("ReversePair[15] Value: " + reversePairs[15].value.toString()); // Expected: 1500

// Test 4.3: Array of Pair<Float, Bool> - 30 elements
Pair<Float, Bool>[] floatBoolPairs = new Pair<Float, Bool>[30];
for (int i = 0; i < 30; i = i + 1) {
    floatBoolPairs[i] = new Pair<Float, Bool>(new Float(i * 1.5), new Bool(i % 2 == 0));
}

print("FloatBoolPair[20] Key: " + floatBoolPairs[20].key.toString());     // Expected: 30.0
print("FloatBoolPair[20] Value: " + floatBoolPairs[20].value.toString()); // Expected: true

// ============================================
// Nested Generic Arrays - Complex Optimization
// ============================================

// Test 5.1: Container of Container<Int> (nested SIMD optimization)
Container<Container<Int>> nestedContainer = new Container<Container<Int>>(5);
for (int i = 0; i < 5; i = i + 1) {
    Container<Int> inner = new Container<Int>(20);
    for (int j = 0; j < 20; j = j + 1) {
        inner.add(new Int(i * 100 + j));
    }
    nestedContainer.add(inner);
}

print("NestedContainer[2][10]: " + nestedContainer.get(2).get(10).toString()); // Expected: 210

// ============================================
// Large Generic Arrays - Maximum Optimization Benefit
// ============================================

// Test 6.1: Large generic Int array - 100 elements (SIMD)
Container<Int> largeIntContainer = new Container<Int>(100);
for (int i = 0; i < 100; i = i + 1) {
    largeIntContainer.add(new Int(i * i));
}

print("Large Int Container[50]: " + largeIntContainer.get(50).toString()); // Expected: 2500

// Test 6.2: Large generic object array - 100 elements (SoA)
Container<DataPoint> largeDataContainer = new Container<DataPoint>(100);
for (int i = 0; i < 100; i = i + 1) {
    largeDataContainer.add(new DataPoint(500 + i, "Large" + i, i * 12.5));
}

print("Large Data Container[75] ID: " + largeDataContainer.get(75).id); // Expected: 575

// Calculate sum of all IDs (benefits from SoA field access)
int totalIds = 0;
for (int i = 0; i < largeDataContainer.getSize(); i = i + 1) {
    totalIds = totalIds + largeDataContainer.get(i).id;
}
print("Total IDs in Large Container: " + totalIds); // Expected: 54950

print("Generic Arrays Optimization Test Completed!");
