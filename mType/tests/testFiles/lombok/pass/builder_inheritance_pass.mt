// @Builder over an inheriting class: the builder mirrors inherited + own
// fields, and build() calls the all-args constructor (which forwards the
// inherited field to super(...)). Getters are inherited from the @Getter parent.
@AllArgsConstructor
@Getter
class Base {
    protected int id;
}

@Builder
@Getter
class Derived extends Base {
    private string label;
}

Derived d = Derived::builder().id(7).label("widget").build();
print(d.getId());
print(d.getLabel());
