// Integration Test 03: Abstract Classes with Generic Constraints
// Tests: Abstract classes + Generics (T extends Interface) + Final methods + Static members

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Comparable interface for generic constraints
interface Comparable<T> {
    function compareTo(T other): int;
}

// Data classes implementing Comparable
class DataItem implements Comparable<DataItem> {
    private int value;
    private string name;

    constructor(int val, string n) {
        this.value = val;
        this.name = n;
    }

    public function compareTo(DataItem other): int {
        if (this.value < other.getValue()) {
            return -1;
        } else if (this.value > other.getValue()) {
            return 1;
        }
        return 0;
    }

    public function getValue(): int {
        return this.value;
    }

    public function getName(): string {
        return this.name;
    }
}

class Score implements Comparable<Score> {
    private int points;

    constructor(int p) {
        this.points = p;
    }

    public function compareTo(Score other): int {
        if (this.points < other.getPoints()) {
            return -1;
        } else if (this.points > other.getPoints()) {
            return 1;
        }
        return 0;
    }

    public function getPoints(): int {
        return this.points;
    }
}

// Abstract generic class with constraint
abstract class Container<T extends Comparable<T>> {
    protected T item;
    protected static int instanceCount = 0;

    constructor(T value) {
        this.item = value;
        Container::instanceCount = Container::instanceCount + 1;
    }

    // Abstract method to be implemented
    abstract function process(): String;

    // Final method cannot be overridden
    public final function compare(T other): int {
        return this.item.compareTo(other);
    }

    // Concrete method can be overridden
    public function describe(): String {
        return new String("Container holding item");
    }

    // Static method
    public static function getCount(): int {
        return Container::instanceCount;
    }
}

// Concrete implementation for DataItem
class DataContainer extends Container<DataItem> {
    private string label;

    constructor(DataItem item, string lbl) : super(item) {
        this.label = lbl;
    }

    // Must implement abstract method
    public function process(): String {
        return new String(this.label + ": " + super.item.getName() + " = " + super.item.getValue());
    }

    // Override concrete method
    @Override
    public function describe(): String {
        return new String("DataContainer with label: " + this.label);
    }
}

// Concrete implementation for Score
class ScoreContainer extends Container<Score> {
    constructor(Score score) : super(score) {}

    // Must implement abstract method
    public function process(): String {
        return new String("Score: " + super.item.getPoints());
    }

    // Keep default describe method (don't override)
}

// Another abstract layer
abstract class SortableContainer<T extends Comparable<T>> extends Container<T> {
    constructor(T value) : super(value) {}

    // Add another abstract method
    abstract function sort(T other): bool;

    // Final method using generic constraint
    public final function isGreaterThan(T other): bool {
        return super.item.compareTo(other) > 0;
    }
}

// Concrete sortable container
class SortableDataContainer extends SortableContainer<DataItem> {
    constructor(DataItem item) : super(item) {}

    public function process(): String {
        return new String("Sortable: " + super.item.getName());
    }

    public function sort(DataItem other): bool {
        return this.compare(other) < 0;
    }
}

// Main test execution
print("=== Test 03: Abstract Classes with Generic Constraints ===");

// Test 1: DataContainer with generic constraint
DataItem item1 = new DataItem(100, "Alpha");
DataItem item2 = new DataItem(50, "Beta");

DataContainer container1 = new DataContainer(item1, "First");
print(container1.process().getValue());
print(container1.describe().getValue());

// Test 2: ScoreContainer with generic constraint
Score score1 = new Score(95);
Score score2 = new Score(87);

ScoreContainer scoreContainer = new ScoreContainer(score1);
print(scoreContainer.process().getValue());
print(scoreContainer.describe().getValue());

// Test 3: Compare using final method
int compResult = container1.compare(item2);
if (compResult > 0) {
    print("item1 is greater than item2");
} else if (compResult < 0) {
    print("item1 is less than item2");
} else {
    print("Items are equal");
}

// Test 4: Static method
print("Total containers created: " + Container::getCount());

// Test 5: Sortable container
DataItem item3 = new DataItem(75, "Gamma");
SortableDataContainer sortable = new SortableDataContainer(item3);
print(sortable.process().getValue());

bool shouldSortBefore = sortable.sort(item1);
if (shouldSortBefore) {
    print("Gamma sorts before Alpha");
} else {
    print("Gamma sorts after Alpha");
}

// Test 6: isGreaterThan final method
DataItem item4 = new DataItem(120, "Delta");
SortableDataContainer sortable2 = new SortableDataContainer(item4);

if (sortable2.isGreaterThan(item1)) {
    print("Delta is greater than Alpha");
} else {
    print("Delta is not greater than Alpha");
}

// Test 7: Static count after all creations
print("Final container count: " + Container::getCount());

print("=== Test 03 Complete ===");
