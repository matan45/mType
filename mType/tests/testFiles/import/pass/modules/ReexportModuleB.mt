// Module B imports from A
import { ReexportedClass } from "./ReexportModuleA.mt";

// Module B can use it
class ModuleBClass {
    public function useReexported(): string {
        ReexportedClass obj = new ReexportedClass();
        return obj.getValue();
    }
}
