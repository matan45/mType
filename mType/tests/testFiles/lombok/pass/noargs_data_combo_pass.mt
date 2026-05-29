// A class with only non-final fields under @Data: getters + setters + toString
// are generated, and the required-args constructor degenerates to a no-arg one
// (no final fields), so `new Account()` is available.
@Data
class Account {
    private string owner = "none";
    private int balance = 0;
}

Account a = new Account();
a.setOwner("Alice");
a.setBalance(100);
print(a.getOwner());
print(a.getBalance());
print(a.toString());
