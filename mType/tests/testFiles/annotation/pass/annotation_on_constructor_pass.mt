// MYT-108: annotation applied to a constructor. Validation runs during class
// registration; the class should construct normally and the annotation is
// carried on the runtime ConstructorDefinition for future reflection APIs.

annotation DefaultCtor {
    bool isDefault = true;
}

class Config {
    public int x;

    @DefaultCtor
    public constructor() {
        this.x = 7;
    }
}

Config c = new Config();
print(c.x);
