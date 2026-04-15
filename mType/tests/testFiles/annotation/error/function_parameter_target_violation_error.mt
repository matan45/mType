// MYT-110: applying a @Target([METHOD]) annotation to a top-level function
// parameter must be rejected. Exercises the FunctionRegistrar parameter
// validation path (distinct from the class-method path covered by
// parameter_target_violation_error.mt).

import * from "../../lib/annotations/Targets.mt";

@Target([METHOD])
annotation MethodOnly { }

function bad(@MethodOnly int x): void {
}
