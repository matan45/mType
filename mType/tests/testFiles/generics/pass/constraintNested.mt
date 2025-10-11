// Test: Nested generic constraints
// Expected: Should compile and run successfully
import * from "../../lib/collections/List.mt";

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
class SortedList<T extends Comparable<T>> {
    private List<T> items;

    constructor() {
        this.items = new List<T>();
    }

    public function add(T item): void {
        this.items.add(item);
    }

    public function get(int index): T {
        return this.items.get(index);
    }
}

// Nested generic with constraint - List of SortedLists
class Container<T extends Comparable<T>> {
    private List<SortedList<T>> lists;

    constructor() {
        this.lists = new List<SortedList<T>>();
    }

    public function addList(SortedList<T> list): void {
        this.lists.add(list);
    }

    public function getList(int index): SortedList<T> {
        return this.lists.get(index);
    }
}

// Test nested constraints
Container<Point> container = new Container<Point>();

SortedList<Point> list1 = new SortedList<Point>();
list1.add(new Point(1, 2));
list1.add(new Point(3, 4));

SortedList<Point> list2 = new SortedList<Point>();
list2.add(new Point(5, 6));

container.addList(list1);
container.addList(list2);

Point p = container.getList(0).get(0);
print("Point: (" + p.getX() + ", " + p.getY() + ")");

print("Nested constraint test passed!");
