// MYT-109 (3a): parser accepts meta-annotations on an annotation declaration.
// Verifies `@X annotation Name { ... }` parses — built-in @Retention / @Target
// are recognised so validation doesn't reject the usage.

@Retention(RUNTIME)
@Target([METHOD])
annotation Logged {
    string level = "info";
}

print("meta-annotated annotation Logged declared");
