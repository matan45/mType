// MYT-110: applying a @Target([METHOD]) annotation to a parameter must be
// rejected at compile time with the existing @Target violation message.

import * from "../../lib/annotations/Targets.mt";

@Target([METHOD])
annotation MethodOnly { }

class Bad {
    public function foo(@MethodOnly int x): void {
    }
}
