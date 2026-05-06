// Edge: three-file dependency where Sub.mt imports Super.mt, then this file
// imports Sub.mt. No cycle, but tests cross-module class resolution and
// vtable construction across two import boundaries.
import { Sub } from "./inherit3/Sub.mt";

function main(): void {
    Sub s = new Sub();
    print(s.tag());
}

main();
