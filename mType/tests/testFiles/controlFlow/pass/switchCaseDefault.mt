// Test 1: match case
int x = 2;
switch (x) {
    case 1:
        print(100);
		break;
    case 2:
        print(200);
		break;
    case 3:
        print(300);
		break;
    default:
        print(999);
		break;
}

// Test 2: no match (default case)
int y = 5;
switch (y) {
    case 1:
        print(100);
		 break;
    case 2:
        print(200);
		 break;
    case 3:
        print(300);
		 break;
    default:
        print(999);
		break;
}
print("Test passed"); // Test completed