// Test: Getter/Setter pattern with private fields
class Temperature {
    private float celsius;

    public Temperature(float c) {
        celsius = c;
    }

    public float getCelsius() {
        return celsius;
    }

    public void setCelsius(float c) {
        celsius = c;
    }

    public float getFahrenheit() {
        return (celsius * 9.0 / 5.0) + 32.0;
    }
}

Temperature temp = new Temperature(0.0);
print(temp.getCelsius());     // Expected: 0
print(temp.getFahrenheit());  // Expected: 32

temp.setCelsius(100.0);
print(temp.getCelsius());     // Expected: 100
print(temp.getFahrenheit());  // Expected: 212
