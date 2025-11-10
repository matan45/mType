// Switch statement with many cases - testing performance with 100+ cases

// Test with 100 cases
int testValue = 75;
switch (testValue) {
    case 0: print(0); break;
    case 1: print(1); break;
    case 2: print(2); break;
    case 3: print(3); break;
    case 4: print(4); break;
    case 5: print(5); break;
    case 6: print(6); break;
    case 7: print(7); break;
    case 8: print(8); break;
    case 9: print(9); break;
    case 10: print(10); break;
    case 11: print(11); break;
    case 12: print(12); break;
    case 13: print(13); break;
    case 14: print(14); break;
    case 15: print(15); break;
    case 16: print(16); break;
    case 17: print(17); break;
    case 18: print(18); break;
    case 19: print(19); break;
    case 20: print(20); break;
    case 21: print(21); break;
    case 22: print(22); break;
    case 23: print(23); break;
    case 24: print(24); break;
    case 25: print(25); break;
    case 26: print(26); break;
    case 27: print(27); break;
    case 28: print(28); break;
    case 29: print(29); break;
    case 30: print(30); break;
    case 31: print(31); break;
    case 32: print(32); break;
    case 33: print(33); break;
    case 34: print(34); break;
    case 35: print(35); break;
    case 36: print(36); break;
    case 37: print(37); break;
    case 38: print(38); break;
    case 39: print(39); break;
    case 40: print(40); break;
    case 41: print(41); break;
    case 42: print(42); break;
    case 43: print(43); break;
    case 44: print(44); break;
    case 45: print(45); break;
    case 46: print(46); break;
    case 47: print(47); break;
    case 48: print(48); break;
    case 49: print(49); break;
    case 50: print(50); break;
    case 51: print(51); break;
    case 52: print(52); break;
    case 53: print(53); break;
    case 54: print(54); break;
    case 55: print(55); break;
    case 56: print(56); break;
    case 57: print(57); break;
    case 58: print(58); break;
    case 59: print(59); break;
    case 60: print(60); break;
    case 61: print(61); break;
    case 62: print(62); break;
    case 63: print(63); break;
    case 64: print(64); break;
    case 65: print(65); break;
    case 66: print(66); break;
    case 67: print(67); break;
    case 68: print(68); break;
    case 69: print(69); break;
    case 70: print(70); break;
    case 71: print(71); break;
    case 72: print(72); break;
    case 73: print(73); break;
    case 74: print(74); break;
    case 75: print(75); break;
    case 76: print(76); break;
    case 77: print(77); break;
    case 78: print(78); break;
    case 79: print(79); break;
    case 80: print(80); break;
    case 81: print(81); break;
    case 82: print(82); break;
    case 83: print(83); break;
    case 84: print(84); break;
    case 85: print(85); break;
    case 86: print(86); break;
    case 87: print(87); break;
    case 88: print(88); break;
    case 89: print(89); break;
    case 90: print(90); break;
    case 91: print(91); break;
    case 92: print(92); break;
    case 93: print(93); break;
    case 94: print(94); break;
    case 95: print(95); break;
    case 96: print(96); break;
    case 97: print(97); break;
    case 98: print(98); break;
    case 99: print(99); break;
    case 100: print(100); break;
    case 101: print(101); break;
    case 102: print(102); break;
    case 103: print(103); break;
    case 104: print(104); break;
    case 105: print(105); break;
    default:
        print("Default case");
        break;
}

// Test with first case
int testFirst = 0;
switch (testFirst) {
    case 0: print("Matched first case"); break;
    case 1: print(1); break;
    case 2: print(2); break;
    case 3: print(3); break;
    case 4: print(4); break;
    case 5: print(5); break;
    case 6: print(6); break;
    case 7: print(7); break;
    case 8: print(8); break;
    case 9: print(9); break;
    case 10: print(10); break;
    default: print("Default"); break;
}

// Test with last case before default
int testLast = 105;
switch (testLast) {
    case 100: print(100); break;
    case 101: print(101); break;
    case 102: print(102); break;
    case 103: print(103); break;
    case 104: print(104); break;
    case 105: print("Matched last case"); break;
    default: print("Default"); break;
}

// Test with default (no match)
int testDefault = 999;
switch (testDefault) {
    case 100: print(100); break;
    case 101: print(101); break;
    case 102: print(102); break;
    case 103: print(103); break;
    case 104: print(104); break;
    case 105: print(105); break;
    default: print("Matched default with no case"); break;
}

print("Many cases test completed");
