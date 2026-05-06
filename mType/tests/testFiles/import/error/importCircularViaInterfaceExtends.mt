// Edge: circular import expressed via interface inheritance. IfaceA extends
// IfaceB; IfaceB extends IfaceA — and they live in separate files so the
// import graph is also cyclic. Either the import-cycle detector or the
// interface-resolution layer must reject this.
import { IA } from "./interfaceCycle/IfaceA.mt";

function main(): void {
    print("should not reach here");
}

main();
