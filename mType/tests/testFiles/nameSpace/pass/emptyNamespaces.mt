namespace empty1 {
}

namespace empty2 {
    namespace nested_empty {
    }
}

namespace has_content {
    int value = 123;
    
    function getValue(): int {
        return value;
    }
}

namespace also_empty {
    namespace deeply {
        namespace nested {
            namespace empty {
            }
            
            function deepFunction(): int {
                return 999;
            }
        }
    }
}

// Test that we can call functions from non-empty namespaces
int val = has_content::getValue();
print(val);

int deep = also_empty::deeply::nested::deepFunction();
print(deep);