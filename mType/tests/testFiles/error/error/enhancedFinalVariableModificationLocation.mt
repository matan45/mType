// Test enhanced final variable modification exception with location info
namespace math {
    final double pi = 3.14159;
}

math::pi = 2.5;  // Line 5: Should show final variable modification error with location
print("This should not execute");