class VecLike {
    private float y;

    public constructor(float y) {
        this.y = y;
    }
}

class RaycastHitLike {
    private string label;

    public constructor(string label) {
        this.label = label;
    }

    public function getLabel(): string {
        return this.label;
    }
}

class PhysicsLike {
    public static function raycastHit(VecLike origin, VecLike direction, float maxDistance): RaycastHitLike {
        return new RaycastHitLike("default");
    }

    public static function raycastHit(VecLike origin, VecLike direction, float maxDistance, string layers): RaycastHitLike {
        return new RaycastHitLike(layers);
    }
}

class BareFieldRaycastTest {
    private VecLike rayDir;

    public constructor() {
        rayDir = new VecLike(-1.0);
    }

    public function run(): string {
        VecLike rayOrigin = new VecLike(500.0);
        RaycastHitLike hit = PhysicsLike::raycastHit(rayOrigin, rayDir, 1000.0, "Static");
        return hit.getLabel();
    }
}

BareFieldRaycastTest test = new BareFieldRaycastTest();
print(test.run());
