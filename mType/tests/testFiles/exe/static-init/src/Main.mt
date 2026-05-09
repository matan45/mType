class LiteralConstants {
    public static final int INT_VALUE = 42;
    public static final string TEXT_VALUE = "ready";
    public static final float FLOAT_VALUE = 2.5;
    public static final bool BOOL_VALUE = true;
}

class ComputedConstants {
    public static final int COMPUTED_VALUE = LiteralConstants::INT_VALUE + 8;
    public static final string LABEL = LiteralConstants::TEXT_VALUE + "-go";
    public static int counter = ComputedConstants::COMPUTED_VALUE + 1;
}

class ChainedConstants {
    public static final int TOTAL = ComputedConstants::counter + LiteralConstants::INT_VALUE;
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        print("INT=" + LiteralConstants::INT_VALUE);
        print("TEXT=" + LiteralConstants::TEXT_VALUE);
        print("FLOAT=" + LiteralConstants::FLOAT_VALUE);
        print("BOOL=" + LiteralConstants::BOOL_VALUE);
        print("COMPUTED=" + ComputedConstants::COMPUTED_VALUE);
        print("LABEL=" + ComputedConstants::LABEL);
        print("COUNTER=" + ComputedConstants::counter);
        print("CHAINED=" + ChainedConstants::TOTAL);
        print("Static init test passed");
    }
}
