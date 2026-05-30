// MYT-370 regression fixture for ScriptAPI/native interop.
// Declares classes only; callbacks parse/register this file without running
// top-level script code, then call into it through ScriptAPI.

class Myt370Point {
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

class Myt370Constants {
    public static final int LEFT = 0;
    public static final int RIGHT = 1;
    public static final int MIDDLE = 2;
    public static final float SCALE = 1.5;
    public static final bool ENABLED = true;
    public static final Myt370Point POINT = new Myt370Point(3, 4);
    public static final int[] VALUES = [8, 13, 21];

    public static function staticRightViaClassAccess(): int {
        return Myt370Constants::RIGHT;
    }

    public static function staticScaleViaClassAccess(): float {
        return Myt370Constants::SCALE;
    }

    public static function staticEnabledViaClassAccess(): bool {
        return Myt370Constants::ENABLED;
    }

    public static function staticPointSumViaClassAccess(): int {
        Myt370Point point = Myt370Constants::POINT;
        return point.sum();
    }

    public static function staticArrayMiddleViaClassAccess(): int {
        int[] values = Myt370Constants::VALUES;
        return values[1];
    }
}

class Myt370Reader {
    public constructor() {}

    public function readRight(): int {
        return Myt370Constants::RIGHT;
    }

    public function readScale(): float {
        return Myt370Constants::SCALE;
    }

    public function readEnabled(): bool {
        return Myt370Constants::ENABLED;
    }

    public function readPointSum(): int {
        Myt370Point point = Myt370Constants::POINT;
        return point.sum();
    }

    public function readArrayLast(): int {
        int[] values = Myt370Constants::VALUES;
        return values[2];
    }
}
