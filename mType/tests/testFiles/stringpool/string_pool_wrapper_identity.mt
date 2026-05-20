// Test: String value-class wrapper preserves equality semantics on raw and wrapped paths
import * from "../lib/primitives/String.mt";

function main(): void {
    String a = new String("hello");
    String b = new String("hello");
    print("wrapper equals: " + a.equals(b));

    string ra = a.getValue();
    string rb = b.getValue();
    print("raw eq via getValue: " + (ra == rb));
    print("raw eq literal: " + (ra == "hello"));

    String concat = a.concat(new String(" world"));
    print("concat getValue eq literal: " + (concat.getValue() == "hello world"));

    String upper = a.toUpperCase();
    print("wrapped upper eq HELLO: " + upper.equals(new String("HELLO")));
}
main();

// Expected output:
// wrapper equals: true
// raw eq via getValue: true
// raw eq literal: true
// concat getValue eq literal: true
// wrapped upper eq HELLO: true
