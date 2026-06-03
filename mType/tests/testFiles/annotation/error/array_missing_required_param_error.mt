// MYT-375: required array-valued annotation parameters must be supplied.

annotation RequiredValues {
    string[] names;
}

@RequiredValues
class Bad { }
