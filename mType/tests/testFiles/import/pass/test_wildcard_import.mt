// Test wildcard import syntax - imports all public symbols
import * from "wildcard_import_utils.mt"

function main() : void {
    // Should be able to use Point and Vector classes
    Point p = new Point();
    p.x = 3;
    p.y = 4;

    Vector v = new Vector();
    v.x = 1.0;
    v.y = 2.0;

    // Should be able to use createPoint function
    object pointObj = createPoint(5, 12);

    print("Point created successfully");
    print("Vector created successfully");
    print("Factory function works");
}

main();
