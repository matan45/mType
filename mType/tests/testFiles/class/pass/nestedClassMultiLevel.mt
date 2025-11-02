// Test: Multi-level nested classes
// Expected: Pass - demonstrates nested classes at multiple levels

class Level1 {
    private int value1 = 1;

    public constructor() {
        print("Level1 constructor");
    }

    class Level2 {
        private int value2 = 2;

        public constructor() {
            print("Level2 constructor, outer value1=" + Level1.this.value1);
        }

        class Level3 {
            private int value3 = 3;

            public constructor() {
                print("Level3 constructor");
                print("  Level2.value2=" + Level2.this.value2);
                print("  Level1.value1=" + Level1.this.value1);
            }

            public void displayAll() {
                print("Level3.displayAll():");
                print("  value3=" + this.value3);
                print("  value2=" + Level2.this.value2);
                print("  value1=" + Level1.this.value1);
            }
        }

        public void createLevel3() {
            Level3 l3 = new Level3();
            l3.displayAll();
        }
    }

    public void createLevel2() {
        Level2 l2 = new Level2();
        l2.createLevel3();
    }
}

// Test multi-level nesting
print("Test 1: Create through outer class");
Level1 l1 = new Level1();
l1.createLevel2();

print("\nTest 2: Direct creation of Level2");
Level1.Level2 l2 = new Level1.Level2();

print("\nTest 3: Direct creation of Level3");
Level1.Level2.Level3 l3 = new Level1.Level2.Level3();
l3.displayAll();
