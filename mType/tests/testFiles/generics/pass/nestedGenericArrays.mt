import * from "../../lib/primitives/Int.mt";
import * from "../../lib/collections/List.mt";

// Test nested generic arrays like Array<Array<Generic<T>>>
class Container<T> {
    T data;

    public function setData(T value): void {
        data = value;
    }

    public function getData(): T {
        return data;
    }
}

function main(): void {
    print("Testing nested generic arrays");

    // Test 1: 2D array of generic containers
    Container<Int>[][] grid = new Container<Int>[2][2];

    grid[0][0] = new Container<Int>();
    grid[0][0].setData(new Int(11));

    grid[0][1] = new Container<Int>();
    grid[0][1].setData(new Int(12));

    grid[1][0] = new Container<Int>();
    grid[1][0].setData(new Int(21));

    grid[1][1] = new Container<Int>();
    grid[1][1].setData(new Int(22));

    print("2D grid of containers created");
    print("grid[0][0]: " + grid[0][0].getData());
    print("grid[0][1]: " + grid[0][1].getData());
    print("grid[1][0]: " + grid[1][0].getData());
    print("grid[1][1]: " + grid[1][1].getData());

    // Test 2: Array of arrays containing Lists
    List<Int>[][] listGrid = new List<Int>[2][2];

    listGrid[0][0] = new List<Int>();
    listGrid[0][0].add(new Int(1));
    listGrid[0][0].add(new Int(2));

    listGrid[0][1] = new List<Int>();
    listGrid[0][1].add(new Int(3));

    listGrid[1][0] = new List<Int>();
    listGrid[1][0].add(new Int(4));
    listGrid[1][0].add(new Int(5));
    listGrid[1][0].add(new Int(6));

    listGrid[1][1] = new List<Int>();

    print("2D list grid created");
    print("listGrid[0][0] size: " + listGrid[0][0].size());
    print("listGrid[0][1] size: " + listGrid[0][1].size());
    print("listGrid[1][0] size: " + listGrid[1][0].size());
    print("listGrid[1][1] size: " + listGrid[1][1].size());

    // Test 3: Deeply nested generic
    Container<Container<Int>> nested = new Container<Container<Int>>();
    Container<Int> inner = new Container<Int>();
    inner.setData(new Int(999));
    nested.setData(inner);

    print("Deeply nested container value: " + nested.getData().getData());

    print("Nested generic arrays test completed");
}

main();
