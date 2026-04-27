// MYT-213: Class.getMethods() does not walk the inheritance chain.
//
// EXPECTED:
//   getMethods() includes inherited start: true
//   getMethods() includes inherited stop:  true
//   getMethods() includes own honk:        true
//
// ACTUAL (broken):
//   getMethods() includes inherited start: false
//   getMethods() includes inherited stop:  false
//   getMethods() includes own honk:        true
// (getMethods returns the same set as getDeclaredMethods.)

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

class ParentVehicle {
    public function start(): void { print("start"); }
    public function stop(): void { print("stop"); }
}

class ChildCar extends ParentVehicle {
    public function honk(): void { print("honk"); }
}

Class cls = Class::forName("ChildCar");
Method[] all = cls.getMethods();

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

print("getMethods() includes inherited start: " + sawStart);
print("getMethods() includes inherited stop: " + sawStop);
print("getMethods() includes own honk: " + sawHonk);
