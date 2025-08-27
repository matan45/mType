// Test using directive order dependency and last-wins behavior
// If the language allows multiple using directives, test order precedence

namespace first {
    function getValue(): int {
        return 100;
    }
    
    int priority = 1;
}

namespace second {
    function getValue(): int {
        return 200;
    }
    
    int priority = 2;
}

namespace third {
    function getValue(): int {
        return 300;
    }
    
    int priority = 3;
}

// Test different orders of using directives
// If the language allows this, the last using directive should take precedence


// These should use the third namespace (last using directive wins)
int result1 = first::getValue();  // Should return 300
print(result1);

int prio1 = second::priority;  // Should be 3
print(prio1);

// Qualified access should still work
int result2 = first::getValue();   // Should return 100
print(result2);

int result3 = second::getValue();  // Should return 200  
print(result3);

int prio2 = first::priority;  // Should be 1
print(prio2);

int prio3 = second::priority; // Should be 2
print(prio3);