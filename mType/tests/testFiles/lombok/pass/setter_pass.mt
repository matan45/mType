// @Setter generates setX(value) mutators (final fields are skipped).
// @NoArgsConstructor provides the zero-arg constructor used here.
@NoArgsConstructor
@Getter
@Setter
class Settings {
    private int level = 0;
    private string mode = "auto";
}

Settings s = new Settings();
s.setLevel(5);
s.setMode("manual");
print(s.getLevel());
print(s.getMode());
