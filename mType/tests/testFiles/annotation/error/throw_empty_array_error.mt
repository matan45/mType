// Test: @Throw with an explicit empty exceptions list — should fail.
// Differs from empty_throw_error.mt (which omits parameters entirely).
// Expected: validateThrowAnnotation rejects an empty list with
// "must specify at least one exception class".
import * from "../../lib/exceptions/Exception.mt";

@Throw(exceptions = [])
function doWork(): void {
}

// This should never execute due to validation error
doWork();
