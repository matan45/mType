// Test: Chained casts with null intermediate values
class Level1 {
    public Level2? next;

    constructor(Level2? n) {
        this.next = n;
    }
}

class Level2 {
    public Level3? next;

    constructor(Level3? n) {
        this.next = n;
    }
}

class Level3 {
    public int value;

    constructor(int v) {
        this.value = v;
    }
}

// Test with full chain
Level1 level1 = new Level1(new Level2(new Level3(42)));
Level2 l2 = (Level2)level1.next;
Level3 l3 = l2 != null ? (Level3)l2.next : null;
int result1 = l3 != null ? l3.value : 0;
print(result1);

// Test with null in middle
Level1 level1Null = new Level1(null);
Level2 l2Null = (Level2)level1Null.next;
Level3 l3Null = l2Null != null ? (Level3)l2Null.next : null;
int result2 = l3Null != null ? l3Null.value : -1;
print(result2);

// Test casting nullable to nullable
Level2 nullableLevel2 = new Level2(null);
Level2 cast = (Level2)nullableLevel2;
print(cast != null);

// Expected output:
// 42
// -1
// true
