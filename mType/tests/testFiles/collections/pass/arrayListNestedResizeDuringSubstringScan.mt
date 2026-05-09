// MYT-287: resizing ArrayList<ArrayList<T>> during a substring-driven loop
// must not corrupt the loop's scalar locals.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function countByIndexOf(string buffer): int {
    int count = 0;
    int searchFrom = 0;
    int n = strLength(buffer);

    while (searchFrom < n) {
        string remaining = substring(buffer, searchFrom, n - searchFrom);
        int rel = indexOf(remaining, "\n");
        if (rel < 0) {
            searchFrom = n;
        } else {
            count = count + 1;
            searchFrom = searchFrom + rel + 1;
        }
    }

    return count;
}

function scanReportWithNestedResize(string buffer): string {
    ArrayList<ArrayList<Int>> lines = new ArrayList<ArrayList<Int>>();
    int lineCount = 0;
    int i = 0;
    int n = strLength(buffer);

    while (i < n) {
        if (substring(buffer, i, 1) == "\n") {
            ArrayList<Int> line = new ArrayList<Int>();
            line.add(new Int(lineCount));
            lines.add(line);
            lineCount = lineCount + 1;
        }
        i = i + 1;
    }

    return "outer size: " + lines.size() +
        "\nfinal i: " + i +
        "\nsubstring count: " + lineCount;
}

function main(): void {
    string buffer = "";
    for (int i = 0; i < 25; i++) {
        buffer = buffer + "x\n";
    }

    string report = "";
    for (int i = 0; i < 150; i++) {
        report = scanReportWithNestedResize(buffer);
    }

    int indexOfCount = countByIndexOf(buffer);

    print(report);
    print("indexOf count: " + indexOfCount);
    print("counts match: " + (indexOf(report, "substring count: " + indexOfCount) >= 0));
}

main();
