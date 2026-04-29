// Test: Class.getMethods() includes parent class's public methods.
// `getDeclaredMethods()` returns only methods declared directly on the class;
// `getMethods()` walks the inheritance chain and returns the public set.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

class ParentVehicle {
    public function start(): void {
        print("starting");
    }

    public function stop(): void {
        print("stopping");
    }
}

class ChildCar extends ParentVehicle {
    public function honk(): void {
        print("honk");
    }
}

Class childClass = Class::forName("ChildCar");

// getDeclaredMethods: only honk
Method[] declared = childClass.getDeclaredMethods();
print("declared count: " + declared.length);
for (Method m : declared) {
    print("declared: " + m.getName());
}

// getMethods: includes start, stop, honk (public set, parent + child)
Method[] all = childClass.getMethods();
bool sawStart = false;
bool sawStop = false;
bool sawHonk = false;
for (Method m : all) {
    if (m.getName() == "start") {
        sawStart = true;
    }
    if (m.getName() == "stop") {
        sawStop = true;
    }
    if (m.getName() == "honk") {
        sawHonk = true;
    }
}
print("inherited start: " + sawStart);
print("inherited stop: " + sawStop);
print("own honk: " + sawHonk);

print("Test passed");
