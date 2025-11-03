
// Test 5+ level deep hierarchy upcast
class Level0 {
    public function getLevel(): int {
        return 0;
    }
}

class Level1 extends Level0 {
    public function getLevel(): int {
        return 1;
    }
}

class Level2 extends Level1 {
    public function getLevel(): int {
        return 2;
    }
}

class Level3 extends Level2 {
    public function getLevel(): int {
        return 3;
    }
}

class Level4 extends Level3 {
    public function getLevel(): int {
        return 4;
    }
}

class Level5 extends Level4 {
    public function getLevel(): int {
        return 5;
    }
}

function main(): void {
    Level5 level5 = new Level5();

    // Upcast through entire hierarchy
    Level4 l4 = (Level4)level5;
    print(l4.getLevel());

    Level3 l3 = (Level3)level5;
    print(l3.getLevel());

    Level2 l2 = (Level2)level5;
    print(l2.getLevel());

    Level1 l1 = (Level1)level5;
    print(l1.getLevel());

    Level0 l0 = (Level0)level5;
    print(l0.getLevel());

    // Direct upcast to base
    Level0 base = (Level0)level5;
    print(base.getLevel());
}

main();
