// Complex inheritance scenarios with namespaces

class Entity {
    int id;
    string name;

    public constructor(int entityId, string entityName) {
        id = entityId;
        name = entityName;
    }

    public function getName() : string {
        return name;
    }

    public function getId() : int {
        return id;
    }
}


class GameObject {
    Entity entity;
    final string TYPE = "GameObject";

    public constructor(int id, string name) {
        entity = new Entity(id, name);
    }

    public function getEntityInfo() : string {
        return TYPE + " : " + entity.getName() + " [" + entity.getId() + "]";
    }
}



// Test inheritance-like composition with namespaces
Entity baseEntity = new Entity(1, "BaseEntity");
GameObject gameObj = new GameObject(2, "Player");

print(baseEntity.getName());
print(gameObj.getEntityInfo());