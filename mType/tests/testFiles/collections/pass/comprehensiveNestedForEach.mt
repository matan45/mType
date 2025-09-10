// Comprehensive demonstration of nested for-each loops with all supported scenarios

print("=== Comprehensive Nested For-Each Demo ===");

// Test 1: Primitive types in nested arrays
print("\\n1. Nested Arrays with Primitive Types:");
Array<Array<int>> numbers = new Array<Array<int>>();

Array<int> row1 = new Array<int>();
row1.add(10);
row1.add(20);

Array<int> row2 = new Array<int>();
row2.add(30);
row2.add(40);

numbers.add(row1);
numbers.add(row2);

print("Matrix values:");
for (Array<int> row : numbers) {
    for (int value : row) {
        print("  Number:", value);
    }
}

// Test 2: Custom classes in nested arrays
print("\\n2. Nested Arrays with Custom Classes:");

class Book {
    string title;
    string author;
    
    constructor(string bookTitle, string bookAuthor) {
        title = bookTitle;
        author = bookAuthor;
    }
    
    function getTitle(): string {
        return title;
    }
    
    function getAuthor(): string {
        return author;
    }
}

Array<Array<Book>> libraries = new Array<Array<Book>>();

Array<Book> fiction = new Array<Book>();
fiction.add(new Book("1984", "Orwell"));
fiction.add(new Book("Dune", "Herbert"));

Array<Book> nonfiction = new Array<Book>();
nonfiction.add(new Book("Sapiens", "Harari"));
nonfiction.add(new Book("Cosmos", "Sagan"));

libraries.add(fiction);
libraries.add(nonfiction);

print("Library books:");
for (Array<Book> shelf : libraries) {
    for (Book book : shelf) {
        print("  Book:", book.getTitle(), "by", book.getAuthor());
    }
}

// Test 3: Mixed nested iteration patterns
print("\\n3. Complex Nested Patterns:");

Array<string> genres = new Array<string>();
genres.add("Fiction");
genres.add("Non-Fiction");

int shelfIndex = 0;
for (string genre : genres) {
    print("Genre:", genre);
    Array<Book> currentShelf = libraries.get(shelfIndex);
    for (Book book : currentShelf) {
        print("  ->", book.getTitle());
    }
    shelfIndex = shelfIndex + 1;
}

print("\\n=== All nested for-each scenarios working perfectly! ===");