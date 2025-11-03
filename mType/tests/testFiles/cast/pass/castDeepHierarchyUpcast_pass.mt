@Script
// Test 5+ level deep hierarchy upcast
class Level0 {
    fn getLevel(): Int {
        return 0;
    }
}

class Level1 extends Level0 {
    fn getLevel(): Int {
        return 1;
    }
}

class Level2 extends Level1 {
    fn getLevel(): Int {
        return 2;
    }
}

class Level3 extends Level2 {
    fn getLevel(): Int {
        return 3;
    }
}

class Level4 extends Level3 {
    fn getLevel(): Int {
        return 4;
    }
}

class Level5 extends Level4 {
    fn getLevel(): Int {
        return 5;
    }
}

fn main() {
    let level5: Level5 = new Level5();

    // Upcast through entire hierarchy
    let l4: Level4 = level5 as Level4;
    print(l4.getLevel());

    let l3: Level3 = level5 as Level3;
    print(l3.getLevel());

    let l2: Level2 = level5 as Level2;
    print(l2.getLevel());

    let l1: Level1 = level5 as Level1;
    print(l1.getLevel());

    let l0: Level0 = level5 as Level0;
    print(l0.getLevel());

    // Direct upcast to base
    let base: Level0 = level5 as Level0;
    print(base.getLevel());
}
