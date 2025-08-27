// For loop variables cannot be static
for (static int i = 0; i < 5; i++) {
    print(i);
}

print("This should not be reached");