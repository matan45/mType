// Test flatMap on empty outer stream returns empty result
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing flatMap with empty outer:");

    ArrayList<ArrayList<Int>> outer = new ArrayList<ArrayList<Int>>();
    Int[] arr = outer.stream().flatMap<Int>(inner -> Streams::fromIterable<Int>(inner)).toArray();
    print("length: " + arr.length);

    print("Stream flatMap empty outer test passed!");
}
main();
