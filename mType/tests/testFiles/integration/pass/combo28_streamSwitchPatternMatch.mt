// Combo 28: Stream forEach + match on heterogeneous element types
// Tests: ArrayList<Object> streamed and pattern-matched element-by-element

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/Bool.mt";

function describe(Object x): string {
    string result = "";
    match (x) {
        case Int i -> {
            result = "int=" + i.getValue();
        }
        case String s -> {
            result = "str=" + s.getValue();
        }
        case Float f -> {
            result = "float=" + f.getValue();
        }
        case Bool b -> {
            result = "bool=" + b.getValue();
        }
        default -> {
            result = "unknown";
        }
    }
    return result;
}

function main(): void {
    print("=== Combo 28: Stream + Switch + Pattern Match ===");

    ArrayList<Object> mixed = new ArrayList<Object>();
    mixed.add(new Int(7));
    mixed.add(new String("hi"));
    mixed.add(new Float(1.5));
    mixed.add(new Bool(true));
    mixed.add(new Int(42));
    mixed.add(new String("end"));

    print("--- Stream forEach pattern dispatch ---");
    mixed.stream().forEach(x -> print(describe(x)));

    print("--- Plain forEach for sanity ---");
    int idx = 0;
    for (Object x : mixed) {
        print(idx + ": " + describe(x));
        idx = idx + 1;
    }

    print("=== Combo 28 Complete ===");
}

main();
