// MYT-324: JIT/OSR regression for a combat-tick loop repeatedly calling
// registry-style accessors (`valid`, `getUnit`, `hasOrder`, `getOrder`) while
// reading and writing object-array fields. Pre-fix the JIT path could crash
// after OSR tier-up; --no-jit completed.

class Unit {
    public int faction;
    public float hp;
    public float attackDamage;
    public float cooldownLeft;
}

class AttackOrder {
    public int targetEntity;
}

class FakeRegistry {
    public Unit[] units;
    public AttackOrder[] orders;
    public bool[] hasOrderFlag;
    public bool[] alive;
    public int cap;

    public constructor(int n) {
        this.cap = n;
        this.units = new Unit[n];
        this.orders = new AttackOrder[n];
        this.hasOrderFlag = new bool[n];
        this.alive = new bool[n];

        int i = 0;
        while (i < n) {
            this.hasOrderFlag[i] = false;
            this.alive[i] = false;
            i = i + 1;
        }
    }

    public function spawn(int e, Unit u): void {
        this.units[e] = u;
        this.alive[e] = true;
    }

    public function getUnit(int e): Unit {
        return this.units[e];
    }

    public function hasOrder(int e): bool {
        return this.hasOrderFlag[e];
    }

    public function getOrder(int e): AttackOrder {
        return this.orders[e];
    }

    public function setOrder(int e, AttackOrder a): void {
        this.orders[e] = a;
        this.hasOrderFlag[e] = true;
    }

    public function clearOrder(int e): void {
        this.hasOrderFlag[e] = false;
    }

    public function emplaceUnit(int e, Unit u): void {
        this.units[e] = u;
    }

    public function valid(int e): bool {
        if (e < 0) {
            return false;
        }
        if (e >= this.cap) {
            return false;
        }
        return this.alive[e];
    }
}

FakeRegistry reg = new FakeRegistry(8);

int idx = 0;
while (idx < 6) {
    Unit u = new Unit();
    if (idx < 4) {
        u.faction = 1;
        u.hp = 30.0;
        u.attackDamage = 0.0;
    } else {
        u.faction = 2;
        u.hp = 40.0;
        u.attackDamage = 8.0;
    }
    u.cooldownLeft = 0.0;
    reg.spawn(idx, u);
    idx = idx + 1;
}

AttackOrder firstOrder = new AttackOrder();
firstOrder.targetEntity = 1;
reg.setOrder(4, firstOrder);

AttackOrder secondOrder = new AttackOrder();
secondOrder.targetEntity = 2;
reg.setOrder(5, secondOrder);

int attackChecks = 0;
int targetSum = 0;
int clearCount = 0;

function combatTick(FakeRegistry r, float dt): void {
    int e = 0;
    while (e < r.cap) {
        if (r.valid(e)) {
            Unit u = r.getUnit(e);
            if (u.attackDamage > 0.0) {
                attackChecks = attackChecks + 1;
                if (u.cooldownLeft > 0.0) {
                    u.cooldownLeft = u.cooldownLeft - dt;
                    if (u.cooldownLeft < 0.0) {
                        u.cooldownLeft = 0.0;
                    }
                    r.emplaceUnit(e, u);
                }

                int target = 0;
                if (r.hasOrder(e)) {
                    AttackOrder ao = r.getOrder(e);
                    if (r.valid(ao.targetEntity)) {
                        target = ao.targetEntity;
                    } else {
                        r.clearOrder(e);
                        clearCount = clearCount + 1;
                    }
                }
                targetSum = targetSum + target;
            }
        }
        e = e + 1;
    }
}

int N = 1000;
int i = 0;
while (i < N) {
    combatTick(reg, 0.01666667);
    i = i + 1;
}

print("myt324 ticks=" + N + " attackChecks=" + attackChecks + " targetSum=" + targetSum + " clears=" + clearCount);
