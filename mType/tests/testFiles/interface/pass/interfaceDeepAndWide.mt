// Test deep and wide inheritance tree
// @Script

// Level 1
interface Level1A {
    func l1a(): Int;
}

interface Level1B {
    func l1b(): Int;
}

interface Level1C {
    func l1c(): Int;
}

// Level 2
interface Level2A extends Level1A {
    func l2a(): Int;
}

interface Level2B extends Level1B {
    func l2b(): Int;
}

interface Level2C extends Level1C {
    func l2c(): Int;
}

// Level 3
interface Level3A extends Level2A {
    func l3a(): Int;
}

interface Level3B extends Level2B {
    func l3b(): Int;
}

interface Level3C extends Level2C {
    func l3c(): Int;
}

// Level 4
interface Level4A extends Level3A {
    func l4a(): Int;
}

interface Level4B extends Level3B {
    func l4b(): Int;
}

interface Level4C extends Level3C {
    func l4c(): Int;
}

// Level 5
interface Level5 extends Level4A, Level4B, Level4C {
    func l5(): Int;
}

// Implementation of deepest interface
class DeepWideImpl implements Level5 {
    // Level 1
    func l1a(): Int { return 1; }
    func l1b(): Int { return 1; }
    func l1c(): Int { return 1; }

    // Level 2
    func l2a(): Int { return 2; }
    func l2b(): Int { return 2; }
    func l2c(): Int { return 2; }

    // Level 3
    func l3a(): Int { return 3; }
    func l3b(): Int { return 3; }
    func l3c(): Int { return 3; }

    // Level 4
    func l4a(): Int { return 4; }
    func l4b(): Int { return 4; }
    func l4c(): Int { return 4; }

    // Level 5
    func l5(): Int { return 5; }
}

var impl = new DeepWideImpl();

// Access from different levels
var level1: Level1A = impl;
print(level1.l1a());

var level2: Level2A = impl;
print(level2.l2a());

var level3: Level3A = impl;
print(level3.l3a());

var level4: Level4A = impl;
print(level4.l4a());

var level5: Level5 = impl;
print(level5.l5());

// Cross-cast test
var l1b: Level1B = level5 as Level1B;
print(l1b.l1b());

var l3c: Level3C = level2 as Level3C;
print(l3c.l3c());

print("Deep and wide interface test passed");
