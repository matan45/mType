// MYT-370: static final fields must be initialized before Class::FIELD reads.

class Myt370ClassPoint {
    public int x;
    public int y;

    public constructor(int px, int py) {
        x = px;
        y = py;
    }

    public function sum(): int {
        return x + y;
    }
}

class Myt370ClassConstants {
    public static final int LEFT = 0;
    public static final int RIGHT = 1;
    public static final int MIDDLE = 2;
    public static final float SCALE = 1.5;
    public static final bool ENABLED = true;
    public static final Myt370ClassPoint POINT = new Myt370ClassPoint(3, 4);
    public static final int[] VALUES = [8, 13, 21];

    public static function rightViaClassAccess(): int {
        return Myt370ClassConstants::RIGHT;
    }

    public static function scaleViaClassAccess(): float {
        return Myt370ClassConstants::SCALE;
    }

    public static function enabledViaClassAccess(): bool {
        return Myt370ClassConstants::ENABLED;
    }

    public static function pointSumViaClassAccess(): int {
        Myt370ClassPoint point = Myt370ClassConstants::POINT;
        return point.sum();
    }

    public static function arrayMiddleViaClassAccess(): int {
        int[] values = Myt370ClassConstants::VALUES;
        return values[1];
    }
}

class Myt370ClassReader {
    public constructor() {}

    public function readRight(): int {
        return Myt370ClassConstants::RIGHT;
    }

    public function readScale(): float {
        return Myt370ClassConstants::SCALE;
    }

    public function readEnabled(): bool {
        return Myt370ClassConstants::ENABLED;
    }

    public function readPointSum(): int {
        Myt370ClassPoint point = Myt370ClassConstants::POINT;
        return point.sum();
    }

    public function readArrayLast(): int {
        int[] values = Myt370ClassConstants::VALUES;
        return values[2];
    }
}

print("LEFT=" + Myt370ClassConstants::LEFT);
print("RIGHT=" + Myt370ClassConstants::RIGHT);
print("MIDDLE=" + Myt370ClassConstants::MIDDLE);
print("SCALE=" + Myt370ClassConstants::SCALE);
print("ENABLED=" + Myt370ClassConstants::ENABLED);

Myt370ClassPoint point = Myt370ClassConstants::POINT;
print("POINT.sum=" + point.sum());

int[] values = Myt370ClassConstants::VALUES;
print("VALUES[0]=" + values[0]);
print("VALUES[1]=" + values[1]);
print("VALUES[2]=" + values[2]);

print("static.RIGHT=" + Myt370ClassConstants::rightViaClassAccess());
print("static.SCALE=" + Myt370ClassConstants::scaleViaClassAccess());
print("static.ENABLED=" + Myt370ClassConstants::enabledViaClassAccess());
print("static.POINT.sum=" + Myt370ClassConstants::pointSumViaClassAccess());
print("static.VALUES[1]=" + Myt370ClassConstants::arrayMiddleViaClassAccess());

Myt370ClassReader reader = new Myt370ClassReader();
print("reader.RIGHT=" + reader.readRight());
print("reader.SCALE=" + reader.readScale());
print("reader.ENABLED=" + reader.readEnabled());
print("reader.POINT.sum=" + reader.readPointSum());
print("reader.VALUES[2]=" + reader.readArrayLast());
