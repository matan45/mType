// Test: @Script class for C++ interop via callMethod
// Run with: mType --test-script-objects script_callmethod_test.mt

@Script
public class ScriptTest {
    private int selfId;

    constructor() {
        selfId = 0;
    }

    public function onStart(): void {
        print("onStart called");
    }

    public function onUpdate(float deltaTime): void {
    }

    public function onDestroy(): void {
    }
}
