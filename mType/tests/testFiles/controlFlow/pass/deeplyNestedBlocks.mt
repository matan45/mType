// Stress-test parser and codegen with ~30 levels of nested plain { ... } blocks.
function deeplyNested(): int {
    int value = 0;
    {
        value = value + 1;
        {
            value = value + 1;
            {
                value = value + 1;
                {
                    value = value + 1;
                    {
                        value = value + 1;
                        {
                            value = value + 1;
                            {
                                value = value + 1;
                                {
                                    value = value + 1;
                                    {
                                        value = value + 1;
                                        {
                                            value = value + 1;
                                            {
                                                value = value + 1;
                                                {
                                                    value = value + 1;
                                                    {
                                                        value = value + 1;
                                                        {
                                                            value = value + 1;
                                                            {
                                                                value = value + 1;
                                                                {
                                                                    value = value + 1;
                                                                    {
                                                                        value = value + 1;
                                                                        {
                                                                            value = value + 1;
                                                                            {
                                                                                value = value + 1;
                                                                                {
                                                                                    value = value + 1;
                                                                                    {
                                                                                        value = value + 1;
                                                                                        {
                                                                                            value = value + 1;
                                                                                            {
                                                                                                value = value + 1;
                                                                                                {
                                                                                                    value = value + 1;
                                                                                                    {
                                                                                                        value = value + 1;
                                                                                                        {
                                                                                                            value = value + 1;
                                                                                                            {
                                                                                                                value = value + 1;
                                                                                                                {
                                                                                                                    value = value + 1;
                                                                                                                    {
                                                                                                                        value = value + 1;
                                                                                                                        {
                                                                                                                            value = value + 1;
                                                                                                                            print("Innermost reached");
                                                                                                                        }
                                                                                                                    }
                                                                                                                }
                                                                                                            }
                                                                                                        }
                                                                                                    }
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return value;
}

function main(): void {
    print("Testing deeply nested blocks");
    int result = deeplyNested();
    print("Result: " + result);
    print("Deeply nested blocks test completed");
}

main();
