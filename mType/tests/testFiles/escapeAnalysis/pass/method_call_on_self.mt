class Counter {
    public int count;
    public constructor(int start) {
        this.count = start;
    }
    public function bump(): int {
        this.count = this.count + 1;
        return this.count;
    }
}

Counter c = new Counter(10);
print(c.bump());
print(c.bump());
print(c.count);
