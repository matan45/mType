// Comprehensive nested loop break/continue limits and edge cases

// Test 1: Deeply nested loops (4 levels) with break propagation
print("Test 1: 4-level nested loops with break");
for (int a = 1; a <= 2; a = a + 1) {
    print("Level 1: " + a);
    for (int b = 1; b <= 3; b = b + 1) {
        print("  Level 2: " + b);
        for (int c = 1; c <= 3; c = c + 1) {
            print("    Level 3: " + c);
            for (int d = 1; d <= 3; d = d + 1) {
                if (d == 2) {
                    print("      Breaking level 4 at d=2");
                    break; // Only breaks innermost loop
                }
                print("      Level 4: " + d);
            }
            print("    Continuing level 3");
        }
        print("  Continuing level 2");
    }
    print("Continuing level 1");
}

// Test 2: Multiple breaks at different nesting levels
print("Test 2: Multiple breaks at different levels");
for (int i = 1; i <= 3; i = i + 1) {
    print("Outer i=" + i);
    for (int j = 1; j <= 4; j = j + 1) {
        if (i == 2 && j == 3) {
            print("  Breaking inner at i=2, j=3");
            break; // Break inner loop when i=2
        }
        print("  Inner j=" + j);
        for (int k = 1; k <= 3; k = k + 1) {
            if (k == 2 && j == 2) {
                print("    Breaking deepest at k=2, j=2");
                break; // Break deepest loop
            }
            print("    Deepest k=" + k);
        }
    }
}

// Test 3: Deeply nested loops with continue propagation
print("Test 3: 4-level nested loops with continue");
for (int x = 1; x <= 2; x = x + 1) {
    for (int y = 1; y <= 3; y = y + 1) {
        for (int z = 1; z <= 3; z = z + 1) {
            for (int w = 1; w <= 4; w = w + 1) {
                if (w == 2) {
                    continue; // Skip w=2 in innermost loop
                }
                if (w == 3 && z == 2) {
                    continue; // Skip w=3 when z=2
                }
                print("Nested: x=" + x + " y=" + y + " z=" + z + " w=" + w);
            }
        }
    }
}

// Test 4: Mixed while and for loops with breaks
print("Test 4: Mixed loop types with breaks");
for (int outer = 1; outer <= 2; outer = outer + 1) {
    print("For loop: " + outer);
    int inner = 1;
    while (inner <= 4) {
        if (inner == 3 && outer == 2) {
            print("  Breaking while at inner=3, outer=2");
            break;
        }
        print("  While loop: " + inner);
        
        for (int deep = 1; deep <= 3; deep = deep + 1) {
            if (deep == 2) {
                print("    Breaking for at deep=2");
                break;
            }
            print("    Deep for: " + deep);
        }
        
        inner = inner + 1;
    }
}

// Test 5: Maximum nesting depth test (5 levels)
print("Test 5: Maximum nesting depth (5 levels)");
for (int l1 = 1; l1 <= 2; l1 = l1 + 1) {
    for (int l2 = 1; l2 <= 2; l2 = l2 + 1) {
        for (int l3 = 1; l3 <= 2; l3 = l3 + 1) {
            for (int l4 = 1; l4 <= 2; l4 = l4 + 1) {
                for (int l5 = 1; l5 <= 2; l5 = l5 + 1) {
                    if (l1 == 2 && l2 == 1 && l3 == 2 && l4 == 1 && l5 == 2) {
                        print("Deep break condition met");
                        break;
                    }
                    print("5-Deep: " + l1 + l2 + l3 + l4 + l5);
                }
            }
        }
    }
}

// Test 6: Complex continue patterns in nested loops
print("Test 6: Complex continue patterns");
for (int p = 1; p <= 3; p = p + 1) {
    for (int q = 1; q <= 4; q = q + 1) {
        if (q % 2 == 0) {
            continue; // Skip even q values
        }
        for (int r = 1; r <= 3; r = r + 1) {
            if (r == 2 && p == 2) {
                continue; // Skip r=2 when p=2
            }
            if (p == 3 && q == 3 && r == 3) {
                continue; // Skip specific combination
            }
            print("Continue pattern: p=" + p + " q=" + q + " r=" + r);
        }
    }
}

// Test 7: Break and continue in same nested structure
print("Test 7: Mixed break and continue");
for (int m = 1; m <= 3; m = m + 1) {
    for (int n = 1; n <= 5; n = n + 1) {
        if (n == 1) {
            continue; // Skip n=1
        }
        if (n == 4) {
            print("  Breaking at n=4 for m=" + m);
            break; // Break at n=4
        }
        
        for (int o = 1; o <= 4; o = o + 1) {
            if (o == 2) {
                continue; // Skip o=2
            }
            if (o == 4 && m == 3) {
                break; // Break innermost when m=3, o=4
            }
            print("  Mixed: m=" + m + " n=" + n + " o=" + o);
        }
    }
}

// Test 8: Extreme nesting with alternating break/continue
print("Test 8: Extreme nesting with alternating controls");
for (int e1 = 1; e1 <= 2; e1 = e1 + 1) {
    for (int e2 = 1; e2 <= 3; e2 = e2 + 1) {
        if (e2 == 2 && e1 == 1) {
            continue; // Continue at level 2
        }
        
        for (int e3 = 1; e3 <= 3; e3 = e3 + 1) {
            if (e3 == 3) {
                break; // Break at level 3
            }
            
            for (int e4 = 1; e4 <= 4; e4 = e4 + 1) {
                if (e4 % 2 == 1) {
                    continue; // Continue odd values at level 4
                }
                print("Extreme: " + e1 + e2 + e3 + e4);
            }
        }
    }
}

// Test 9: Nested loops with conditional break/continue chains
print("Test 9: Conditional break/continue chains");
for (int chain1 = 1; chain1 <= 3; chain1 = chain1 + 1) {
    for (int chain2 = 1; chain2 <= 4; chain2 = chain2 + 1) {
        bool shouldBreak = false;
        bool shouldContinue = false;
        
        // Complex conditional logic
        if (chain1 == 1 && chain2 == 3) {
            shouldContinue = true;
        }
        if (chain1 == 2 && chain2 == 4) {
            shouldBreak = true;
        }
        
        if (shouldContinue) {
            print("  Continuing chain1=" + chain1 + " chain2=" + chain2);
            continue;
        }
        if (shouldBreak) {
            print("  Breaking chain1=" + chain1 + " chain2=" + chain2);
            break;
        }
        
        for (int chain3 = 1; chain3 <= 3; chain3 = chain3 + 1) {
            if (chain1 + chain2 + chain3 > 7) {
                break; // Break based on sum
            }
            print("  Chain result: " + (chain1 * 100 + chain2 * 10 + chain3));
        }
    }
}

// Test 10: Performance stress test with many iterations
print("Test 10: Performance stress test");
int iterations = 0;
for (int stress1 = 1; stress1 <= 10; stress1 = stress1 + 1) {
    if (stress1 > 5) {
        break; // Limit outer iterations
    }
    
    for (int stress2 = 1; stress2 <= 20; stress2 = stress2 + 1) {
        if (stress2 % 3 == 0) {
            continue; // Skip multiples of 3
        }
        if (stress2 > 15) {
            break; // Limit inner iterations
        }
        
        for (int stress3 = 1; stress3 <= 10; stress3 = stress3 + 1) {
            if (stress3 % 2 == 0) {
                continue; // Skip even numbers
            }
            if (stress3 > 7) {
                break; // Limit deepest iterations
            }
            
            iterations = iterations + 1;
            if (iterations > 100) {
                print("Stress limit reached at " + iterations);
                break;
            }
        }
    }
}
print("Total stress iterations: " + iterations);

// Test 11: Edge case - immediate break/continue
print("Test 11: Immediate break/continue");
for (int immediate = 1; immediate <= 5; immediate = immediate + 1) {
    for (int inner_immediate = 1; inner_immediate <= 5; inner_immediate = inner_immediate + 1) {
        if (inner_immediate == 1) {
            if (immediate % 2 == 0) {
                continue; // Immediate continue
            } else {
                break; // Immediate break
            }
        }
        print("Should not reach here often: " + immediate + "," + inner_immediate);
    }
}

// Test 12: Multiple nested while loops
print("Test 12: Multiple nested while loops");
int w1 = 1;
while (w1 <= 3) {
    print("While level 1: " + w1);
    int w2 = 1;
    while (w2 <= 4) {
        if (w2 == 3 && w1 == 2) {
            print("  Breaking w2 at 3 when w1=2");
            break;
        }
        if (w2 == 2) {
            w2 = w2 + 1;
            continue;
        }
        
        int w3 = 1;
        while (w3 <= 3) {
            if (w3 == 2) {
                w3 = w3 + 1;
                continue;
            }
            print("  While deep: w1=" + w1 + " w2=" + w2 + " w3=" + w3);
            w3 = w3 + 1;
        }
        
        w2 = w2 + 1;
    }
    w1 = w1 + 1;
}

print("All nested loop break/continue limit tests completed");