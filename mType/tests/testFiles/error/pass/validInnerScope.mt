function getName(): void {
    int x = 5;

    {
        // Inner scoping is allowed
        int y = 10;
        x = x + y;
		print(x);
    }

    // Another inner scope
    {
        int z = 20;
        x = x + z;
		print(x);
    }
}
getName();