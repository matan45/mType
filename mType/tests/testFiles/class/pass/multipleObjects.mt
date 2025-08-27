class Book {
    string title;
    string author;
    int pages;
    
    constructor(string t, string a, int p) {
        title = t;
        author = a;
        pages = p;
    }
    
    function getInfo(): string {
        return title + " by " + author;
    }
}

Book book1 = new Book("1984", "Orwell", 328);
Book book2 = new Book("Dune", "Herbert", 688);
Book book3 = new Book("Foundation", "Asimov", 244);

print(book1.getInfo());
print(book2.getInfo());
print(book3.getInfo());

print(book1.pages); // 328
print(book2.pages); // 688
print(book3.pages); // 244