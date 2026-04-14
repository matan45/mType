// Test: empty annotation declaration `annotation Marker { }` parses and runs.
// Spec: MYT-108 §7a case 1 (annotation_declare_empty.mt).
// M1 only verifies that the parser recognizes the `annotation` keyword and
// produces an AnnotationDeclarationNode. Usage as `@Marker` and reflection
// round-trip arrive in later milestones.

annotation Marker { }

print("annotation Marker declared");
