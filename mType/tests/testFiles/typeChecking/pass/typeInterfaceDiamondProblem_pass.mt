// Test diamond inheritance resolution with type checking
interface A {
    function getValue(): int;
}

interface B extends A {
    function getValueB(): int;
}

interface C extends A {
    function getValueC(): int;
}

interface D extends B, C {
    function getValueD(): int;
}

class DiamondImpl implements D {
    int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }

    public function getValueB(): int {
        return this.value * 2;
    }

    public function getValueC(): int {
        return this.value * 3;
    }

    public function getValueD(): int {
        return this.value * 4;
    }
}

DiamondImpl impl = new DiamondImpl(5);
print(impl.getValue());
print(impl.getValueB());
print(impl.getValueC());
print(impl.getValueD());
print("Diamond inheritance resolution successful");
