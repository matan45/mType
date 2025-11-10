// Error: Cannot cast nullable int to non-nullable int without null check
int? nullableInt = null;
int nonNullInt = (int)nullableInt;
