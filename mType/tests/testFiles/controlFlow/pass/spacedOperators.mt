// Test all spaced operators
int x = 5;
int y = 10;

// Test spaced comparison operators
if(x = = 5) print("spaced equals works");
if(x ! = 10) print("spaced not equals works");
if(x < = y) print("spaced less equals works");
if(y > = x) print("spaced greater equals works");

// Test spaced increment/decrement operators
x + +;
print("after spaced increment: " + x);
y - -;
print("after spaced decrement: " + y);

// Test spaced assignment operators
x + = 5;
print("after spaced plus assign: " + x);
x - = 2;
print("after spaced minus assign: " + x);
x * = 3;
print("after spaced multiply assign: " + x);
x / = 2;
print("after spaced divide assign: " + x);
x % = 4;
print("after spaced modulo assign: " + x);

// Test spaced logical operators
bool a = true;
bool b = false;
if(a & & !b) print("spaced AND works");
if(b | | a) print("spaced OR works");

// Test with various whitespace
if(x  =  = 15) print("multiple spaces equals works");
if(x +	= 0) print("tab-spaced plus assign works");  // Note: this has a tab character