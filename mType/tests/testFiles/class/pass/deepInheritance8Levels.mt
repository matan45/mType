// Test deep inheritance chain (8 levels)
// Tests MAX_INHERITANCE_DEPTH handling

class Level1 {
    int level;

    public function Level1() {
        level = 1;
    }

    public function getLevel(): int {
        return level;
    }

    public function identify(): string {
        return "Level1";
    }
}

class Level2 extends Level1 {
    public function Level2() {
        super();
        level = 2;
    }

    public function identify(): string {
        return "Level2";
    }
}

class Level3 extends Level2 {
    public function Level3() {
        super();
        level = 3;
    }

    public function identify(): string {
        return "Level3";
    }
}

class Level4 extends Level3 {
    public function Level4() {
        super();
        level = 4;
    }

    public function identify(): string {
        return "Level4";
    }
}

class Level5 extends Level4 {
    public function Level5() {
        super();
        level = 5;
    }

    public function identify(): string {
        return "Level5";
    }
}

class Level6 extends Level5 {
    public function Level6() {
        super();
        level = 6;
    }

    public function identify(): string {
        return "Level6";
    }
}

class Level7 extends Level6 {
    public function Level7() {
        super();
        level = 7;
    }

    public function identify(): string {
        return "Level7";
    }
}

class Level8 extends Level7 {
    public function Level8() {
        super();
        level = 8;
    }

    public function identify(): string {
        return "Level8";
    }
}

function main(): void {
    print("Testing deep inheritance (8 levels)");

    Level8 obj = new Level8();
    print("obj.getLevel(): " + obj.getLevel());
    print("obj.identify(): " + obj.identify());

    // Polymorphic references
    Level1 ref1 = obj;
    Level4 ref4 = obj;
    Level7 ref7 = obj;

    print("ref1.getLevel(): " + ref1.getLevel());
    print("ref4.getLevel(): " + ref4.getLevel());
    print("ref7.getLevel(): " + ref7.getLevel());

    print("ref1.identify(): " + ref1.identify());
    print("ref4.identify(): " + ref4.identify());
    print("ref7.identify(): " + ref7.identify());

    print("Deep inheritance test completed");
}

main();
