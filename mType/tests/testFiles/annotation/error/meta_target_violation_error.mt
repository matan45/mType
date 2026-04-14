// MYT-109 (3a): @Target([METHOD]) annotation applied to a class must fail.

@Target([METHOD])
annotation MethodOnly { }

@MethodOnly
class Oops { }
