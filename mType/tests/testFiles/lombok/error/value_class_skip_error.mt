// Value classes are skipped by Lombok synthesis (like generic/abstract), so
// @Getter generates no getCents() for Money and the call fails to resolve.
@Getter
value class Money {
    private int cents;

    public constructor(int cents) {
        this.cents = cents;
    }
}

Money m = new Money(100);
print(m.getCents());
