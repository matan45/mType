// Complex inheritance scenarios with namespaces

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
    

        class GameObject {
            Entity entity;
            final string TYPE = "GameObject";
            
            constructor(int id, string name) {
                entity = new Entity(id, name);
            }
            
            function getEntityInfo(): string {
                return TYPE + ": " + entity.getName() + " [" + toString(entity.getId()) + "]";
            }
        }
    


// Test inheritance-like composition with namespaces
Entity baseEntity = new Entity(1, "BaseEntity");
GameObject gameObj = new GameObject(2, "Player");

print(baseEntity.getName());
print(gameObj.getEntityInfo());