import { SharedClass } from "./ModuleD.mt";

class ClassC {
    public function useShared(): string {
        SharedClass shared = new SharedClass();
        return "C: " + shared.getValue();
    }
}
