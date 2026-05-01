// An annotation declared with @Target([FIELD]) cannot be applied to a
// method body, even when that method throws inside a try/catch flow.
// The validator must reject this at compile time.
import * from "../../lib/exceptions/Exception.mt";

@Target([FIELD])
annotation FieldOnly { }

class Repo {
    @FieldOnly
    public function load(): void {
        throw new Exception("would throw if compiled");
    }
}

function main(): void {
    Repo r = new Repo();
    try {
        r.load();
    } catch (Exception e) {
        print("never compiles");
    }
}
main();
