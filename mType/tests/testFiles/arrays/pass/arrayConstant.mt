// Test array constants across scopes
print("Testing array constants");

class Constants {
    int[] PRIMES;
    string[] DAYS;

    Constants() {
        PRIMES = new int[5];
        PRIMES[0] = 2;
        PRIMES[1] = 3;
        PRIMES[2] = 5;
        PRIMES[3] = 7;
        PRIMES[4] = 11;

        DAYS = new string[7];
        DAYS[0] = "Monday";
        DAYS[1] = "Tuesday";
        DAYS[2] = "Wednesday";
        DAYS[3] = "Thursday";
        DAYS[4] = "Friday";
        DAYS[5] = "Saturday";
        DAYS[6] = "Sunday";
    }

    int[] getPrimes() {
        return PRIMES;
    }

    string[] getDays() {
        return DAYS;
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
