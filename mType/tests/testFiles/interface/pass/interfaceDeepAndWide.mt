// Test deep and wide inheritance tree
// @Script

// Level 1
interface Level1A {
    function l1a(): int;
}

interface Level1B {
    function l1b(): int;
}

interface Level1C {
    function l1c(): int;
}

// Level 2
interface Level2A extends Level1A {
    function l2a(): int;
}

interface Level2B extends Level1B {
    function l2b(): int;
}

interface Level2C extends Level1C {
    function l2c(): int;
}

// Level 3
interface Level3A extends Level2A {
    function l3a(): int;
}

interface Level3B extends Level2B {
    function l3b(): int;
}

interface Level3C extends Level2C {
    function l3c(): int;
}

// Level 4
interface Level4A extends Level3A {
    function l4a(): int;
}

interface Level4B extends Level3B {
    function l4b(): int;
}

interface Level4C extends Level3C {
    function l4c(): int;
}

// Level 5
interface Level5 extends Level4A, Level4B, Level4C {
    function l5(): int;
}

// Implementation of deepest interface
class DeepWideImpl implements Level5 {
    // Level 1
    public function l1a(): int { return 1; }
    public function l1b(): int { return 1; }
    public function l1c(): int { return 1; }

    // Level 2
    public function l2a(): int { return 2; }
    public function l2b(): int { return 2; }
    public function l2c(): int { return 2; }

    // Level 3
    public function l3a(): int { return 3; }
    public function l3b(): int { return 3; }
    public function l3c(): int { return 3; }

    // Level 4
    public function l4a(): int { return 4; }
    public function l4b(): int { return 4; }
    public function l4c(): int { return 4; }

    // Level 5
    public function l5(): int { return 5; }
}

DeepWideImpl impl = new DeepWideImpl();

// Access from different levels
Level1A level1 = impl;
print(level1.l1a());

Level2A level2 = impl;
print(level2.l2a());

Level3A level3 = impl;
print(level3.l3a());

Level4A level4 = impl;
print(level4.l4a());

Level5 level5 = impl;
print(level5.l5());

// Cross-cast test
Level1B l1b = (Level1B)level5;
print(l1b.l1b());

Level3C l3c = (Level3C)level2;
print(l3c.l3c());

print("Deep and wide interface test passed");
