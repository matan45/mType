import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Unbounded wildcard (?)
class Holder<T> {
    T data;

    public function setData(T d): void {
        data = d;
    }

    public function hasData(): Bool {
        return data != null;
    }
}

function checkHolder(Holder<Object> holder): void {
    if (holder.hasData()) {
        print("Has data");
    } else {
        print("No data");
    }
}

function main(): void {
    Holder<String> strHolder = new Holder<String>();
    strHolder.setData(new String("test"));

    Holder<Int> intHolder = new Holder<Int>();
    intHolder.setData(new Int(42));

    // Unbounded wildcard - can check any holder
    checkHolder(strHolder);
    checkHolder(intHolder);
}

main();
