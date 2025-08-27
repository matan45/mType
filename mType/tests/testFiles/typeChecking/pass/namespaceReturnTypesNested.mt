namespace outer {
    namespace inner {
        function getValue(): int {
            return 100;
        }
    }
    
    function getInnerValue(): int {
        return inner::getValue();
    }
}

int value1 = outer::inner::getValue();
int value2 = outer::getInnerValue();

print("Nested namespace test passed");