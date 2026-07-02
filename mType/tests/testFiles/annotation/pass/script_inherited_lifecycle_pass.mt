// Test: @Script accepts lifecycle hooks inherited from a base class.
// The validator must resolve onStart/onUpdate(float)/onDestroy through the
// inheritance chain (engine Behaviour-style base), not only own-declared
// methods. Runtime dispatch of inherited + overridden hooks is verified
// through the printed output.

public class LifecycleBase {
    public constructor() { }

    public function onStart(): void {
        print("base onStart");
    }

    public function onUpdate(float deltaTime): void {
        print("base onUpdate");
    }

    public function onDestroy(): void {
        print("base onDestroy");
    }
}

@Script
public class InheritedLifecycleScript extends LifecycleBase {
    public constructor() : super() { }

    @Override
    public function onUpdate(float deltaTime): void {
        print("derived onUpdate");
    }
}

InheritedLifecycleScript script = new InheritedLifecycleScript();
script.onStart();
script.onUpdate(0.016);
script.onDestroy();
