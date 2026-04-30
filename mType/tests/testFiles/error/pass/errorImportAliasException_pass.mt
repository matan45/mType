// Test: an exception type imported with an alias is throwable and
// catchable through that alias only.
import * from "../../lib/exceptions/Exception.mt";
import {CustomException as Boom} from "./modules/CustomException.mt";

function main(): void {
    try {
        throw new Boom("aliased");
    } catch (Boom e) {
        print("alias caught: " + e.getMessage());
    }
    print("done");
}
main();
