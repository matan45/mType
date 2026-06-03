// MYT-376: a folded array whose elements are of mixed primitive kinds (string
// + int) is rejected.

annotation Names {
    string[] tags;
}

@Names(tags = ["a" + "b", 5])
class Target { }
