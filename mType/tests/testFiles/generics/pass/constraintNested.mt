// Test: Nested generic constraints
// Expected: Should compile and run successfully
import * from "../../lib/collections/ArrayList.mt";

interface Comparable<T> {
    function compareTo(T other): int;
}

class Point implements Comparable<Point> {
    private int x;
    private int y;

    constructor(int xCoord, int yCoord) {
        this.x = xCoord;
        this.y = yCoord;
    }

    public function compareTo(Point other): int {
        int distThis = this.x * this.x + this.y * this.y;
        int distOther = other.x * other.x + other.y * other.y;

        if (distThis < distOther) {
            return -1;
        } else if (distThis > distOther) {
            return 1;
        }
        return 0;
    }

    public function getX(): int {
        return this.x;
    }

    public function getY(): int {
        return this.y;
    }
}

// Generic class with constrained parameter
class SortedArrayList<T extends Comparable<T>> {
    private ArrayList<T> items;

    constructor() {
        this.items = new ArrayList<T>();
    }

    public function add(T item): void {
        this.items.add(item);
    }

    public function get(int index): T {
        return this.items.get(index);
    }
}

// Nested generic with constraint - ArrayList of SortedArrayLists
class Container<T extends Comparable<T>> {
    private ArrayList<SortedArrayList<T>> ArrayLists;

    constructor() {
        this.ArrayLists = new ArrayList<SortedArrayList<T>>();
    }

    public function addArrayList(SortedArrayList<T> ArrayList): void {
        this.ArrayLists.add(ArrayList);
    }

    public function getArrayList(int index): SortedArrayList<T> {
        return this.ArrayLists.get(index);
    }
}

// Test nested constraints
Container<Point> container = new Container<Point>();

SortedArrayList<Point> ArrayList1 = new SortedArrayList<Point>();
ArrayList1.add(new Point(1, 2));
ArrayList1.add(new Point(3, 4));

SortedArrayList<Point> ArrayList2 = new SortedArrayList<Point>();
ArrayList2.add(new Point(5, 6));

container.addArrayList(ArrayList1);
container.addArrayList(ArrayList2);

Point p = container.getArrayList(0).get(0);
print("Point: (" + p.getX() + ", " + p.getY() + ")");

print("Nested constraint test passed!");
