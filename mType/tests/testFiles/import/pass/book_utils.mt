class Book {
    public static string TEST = "Book class imported successfully";
    
    string title;
    string author;
    int pages;
    
    constructor(string t, string a, int p) {
        title = t;
        author = a;
        pages = p;
    }
    
    public function getInfo(): string {
        return title + " by " + author;
    }
}