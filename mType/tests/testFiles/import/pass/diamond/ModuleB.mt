import { SharedClass } from "./ModuleD.mt";

class ClassB {
    public function useShared(): string {
        SharedClass shared = new SharedClass();
        return "B: " + shared.getValue();
    }
}
