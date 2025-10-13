class Counter {
    int count;

    constructor() {
        this.count = 0;
    }

    public function reset(): void {
        print("Instance reset called");
    }

    public static function reset(): void {
        print("Static reset called");
    }
}

Counter c = new Counter();
c.reset();
Counter::reset();
