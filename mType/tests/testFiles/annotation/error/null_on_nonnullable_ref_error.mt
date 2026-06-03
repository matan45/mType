// MYT-375: a non-nullable reference parameter rejects an explicit null value
// at usage (distinct from the nullable-primitive-at-declaration parser error).
annotation Label {
    string text;
}

@Label(text = null)
class Bad { }
