// Complex inheritance scenarios with namespaces
namespace baseTypes {
    class Entity {
        int id;
        string name;
        
        constructor(int entityId, string entityName) {
            id = entityId;
            name = entityName;
        }
        
        function getName(): string {
            return name;
        }
        
        function getId(): int {
            return id;
        }
    }
    
    namespace advanced {
        class GameObject {
            baseTypes::Entity entity;
            final string TYPE = "GameObject";
            
            constructor(int id, string name) {
                entity = new baseTypes::Entity(id, name);
            }
            
            function getEntityInfo(): string {
                return TYPE + ": " + entity.getName() + " [" + toString(entity.getId()) + "]";
            }
        }
    }
}

// Test inheritance-like composition with namespaces
baseTypes::Entity baseEntity = new baseTypes::Entity(1, "BaseEntity");
baseTypes::advanced::GameObject gameObj = new baseTypes::advanced::GameObject(2, "Player");

print(baseEntity.getName());
print(gameObj.getEntityInfo());