class Counter {
    int count;
    
    constructor(int initial) {
        count = initial;
    }
    
    function increment(): void {
        count = count + 1;
    }
    
    function getCount(): int {
        return count;
    }
}

Counter c1 = new Counter(5);
Counter c2 = c1; // Reference assignment

c1.increment();
print(c1.getCount()); // 6
print(c2.getCount()); // 6 (same object)

c2.increment();
print(c1.getCount()); // 7
print(c2.getCount()); // 7

Counter c3 = new Counter(10);
c2 = c3;

print(c1.getCount()); // 7
print(c2.getCount()); // 10
print(c3.getCount()); // 10