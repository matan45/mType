// Pass test: Final class with static and instance members

final class FinalUtility {
    // Static members
    public static final int MAX_VALUE = 100;
    static int counter = 0;

    // Instance members
    int id;
    string name;

    constructor(string n) {
        counter = counter + 1;
        id = counter;
        name = n;
    }

    public function getInfo(): string {
        return name;
    }

    public static function getCounter(): int {
        return counter;
    }
}

FinalUtility util1 = new FinalUtility("First");
print(util1.getInfo());
print(FinalUtility::getCounter());

FinalUtility util2 = new FinalUtility("Second");
print(util2.getInfo());
print(FinalUtility::getCounter());
print(FinalUtility::MAX_VALUE);
