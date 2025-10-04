class Counter {
    public static int count = 0;
    public int id;

    public constructor() {
        count = count + 1;
        id = count;
    }

    public function getId(): int {
        return id;
    }
}

Counter c1 = new Counter();
print(c1.getId()); // 1
print(Counter::count); // 1

Counter c2 = new Counter();
print(c2.getId()); // 2
print(Counter::count); // 2

Counter c3 = new Counter();
print(c3.getId()); // 3
print(Counter::count); // 3