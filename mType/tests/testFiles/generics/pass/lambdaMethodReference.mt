import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Method reference pattern with generics
class Converter<T, R> {
    function(T): R converter;

    public function setConverter(function(T): R conv): void {
        converter = conv;
    }

    public function convert(T input): R {
        return converter(input);
    }
}

class Formatter {
    public static function intToString(Int x): String {
        return new String("Number: " + x);
    }

    public static function stringToInt(String s): Int {
        return new Int(100);
    }
}

function main(): void {
    Converter<Int, String> conv1 = new Converter<Int, String>();
    conv1.setConverter(Formatter.intToString);
    print(conv1.convert(new Int(42)));

    Converter<String, Int> conv2 = new Converter<String, Int>();
    conv2.setConverter(Formatter.stringToInt);
    print("Converted: " + conv2.convert(new String("test")));
}

main();
