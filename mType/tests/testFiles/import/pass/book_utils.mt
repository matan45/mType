class Book {
    static string TEST = "Book class imported successfully";
    
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