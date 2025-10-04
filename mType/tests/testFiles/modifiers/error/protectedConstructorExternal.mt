// Test: Protected constructor access violation from external context
class BaseEntity {
    public int id;

    protected constructor(int entityID) {
        id = entityID;
    }
}

BaseEntity entity = new BaseEntity(100);  // ERROR: Cannot access protected constructor
