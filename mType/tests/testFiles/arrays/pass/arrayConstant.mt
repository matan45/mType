// Test array constants across scopes
print("Testing array constants");

class Constants {
    public int[] PRIMES;
    public string[] DAYS;

    public constructor() {
        this.PRIMES = new int[5];
        this.PRIMES[0] = 2;
        this.PRIMES[1] = 3;
        this.PRIMES[2] = 5;
        this.PRIMES[3] = 7;
        this.PRIMES[4] = 11;

        this.DAYS = new string[7];
        this.DAYS[0] = "Monday";
        this.DAYS[1] = "Tuesday";
        this.DAYS[2] = "Wednesday";
        this.DAYS[3] = "Thursday";
        this.DAYS[4] = "Friday";
        this.DAYS[5] = "Saturday";
        this.DAYS[6] = "Sunday";
    }

    public function getPrimes(): int[] {
        return this.PRIMES;
    }

    public function getDays(): string[] {
        return this.DAYS;
    }
}

Constants constants = new Constants();

print("Prime numbers:");
int[] primes = constants.getPrimes();
for (int i = 0; i < primes.length; i++) {
    print("  " + primes[i]);
}

print("Days of week:");
string[] days = constants.getDays();
for (int i = 0; i < days.length; i++) {
    print("  " + days[i]);
}

print("Array constants test completed");
