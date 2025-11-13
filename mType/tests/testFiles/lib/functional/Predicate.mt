/**
 * Represents a predicate (boolean-valued function) of one argument.
 * Used for filtering operations like filter().
 *
 * @param T the type of the input to the predicate
 *
 * Example:
 *   Predicate<int> isPositive = (int x) => x > 0;
 *   bool result = isPositive.test(5); // true
 */
interface Predicate<T> {
    /**
     * Evaluates this predicate on the given argument.
     *
     * @param t the input argument
     * @return true if the input argument matches the predicate, otherwise false
     */
    function test(T t): bool;

    /**
     * Returns a composed predicate that represents a short-circuiting logical
     * AND of this predicate and another.
     *
     * @param other a predicate that will be logically-ANDed with this predicate
     * @return a composed predicate that represents the short-circuiting logical
     *         AND of this predicate and the other predicate
     */
    function and(Predicate<T> other): Predicate<T>;

    /**
     * Returns a composed predicate that represents a short-circuiting logical
     * OR of this predicate and another.
     *
     * @param other a predicate that will be logically-ORed with this predicate
     * @return a composed predicate that represents the short-circuiting logical
     *         OR of this predicate and the other predicate
     */
    function or(Predicate<T> other): Predicate<T>;

    /**
     * Returns a predicate that represents the logical negation of this predicate.
     *
     * @return a predicate that represents the logical negation of this predicate
     */
    function negate(): Predicate<T>;
}
