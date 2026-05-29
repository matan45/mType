// A @Builder field whose type is itself a @Builder class: the inner object is
// built with its own builder and passed to the outer builder's setter. The
// builders don't auto-nest — the outer .address(...) setter just takes an
// Address argument.
@Builder
@Getter
class Address {
    private string city;
    private string zip;
}

@Builder
@Getter
class Person {
    private string name;
    private Address address;
}

Person p = Person::builder()
    .name("Bob")
    .address(Address::builder().city("NYC").zip("10001").build())
    .build();

print(p.getName());
print(p.getAddress().getCity());
print(p.getAddress().getZip());
