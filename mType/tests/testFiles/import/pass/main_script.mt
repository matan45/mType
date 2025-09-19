        import "math_utils.mt";
        import "string_utils.mt";
	import "book_utils.mt";
        import "math_utils.mt";  // Safe duplicate - will be ignored
        
        native function print(int n): void;
        native function print(string s): void;
        
        // Use imported functions
        int sum = add(10, 20);
        print(sum);
		
		Book book1 = new Book("1984", "Orwell", 328);
        print(book1.getInfo());
		print(Book::TEST);
		
        int product = multiply(5, 6);
        print(product);
        
        int processed = processNumber(7);
        print(processed);
        
        // Use imported variables
        print(MATH_CONSTANT);
        print(GREETING);