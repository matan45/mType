// Test: an @Script subclass whose inheritance chain provides onStart and
// onDestroy but NOT onUpdate(float) must still be rejected — the hierarchy
// walk must not over-accept.

public class PartialLifecycleBase {
    public constructor() { }

    public function onStart(): void {
    }

    public function onDestroy(): void {
    }
}

@Script
public class MissingUpdateScript extends PartialLifecycleBase {
    public constructor() : super() { }
}
