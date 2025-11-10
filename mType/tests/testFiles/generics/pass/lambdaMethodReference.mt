import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Method reference pattern with generics
interface ConverterFunc<T, R> {
    function convert(T input): R;
}

class Converter<T, R> {
    ConverterFunc<T, R> converter;

    public function setConverter(ConverterFunc<T, R> conv): void {
        converter = conv;
    }

    public function convert(T input): R {
        return converter.convert(input);
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
    conv1.setConverter(x -> Formatter::intToString(x));
    print(conv1.convert(new Int(42)).toString());

    Converter<String, Int> conv2 = new Converter<String, Int>();
    conv2.setConverter(s -> Formatter::stringToInt(s));
    print("Converted: " + conv2.convert(new String("test")).toString());
}

main();
