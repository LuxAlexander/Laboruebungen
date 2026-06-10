/**
 * @author MIC Lab Team
 * @brief Bit manipulation macros
 *
 */

#ifndef bit_h_tie8AeRuf7unoh7l
#define bit_h_tie8AeRuf7unoh7l

/** @return a word with only n-th bit set. */
#define BIT(n) (1ull << (n))
/** Raise n-th bit in word. */
#define BIT_SET(word, n) ((word) |= BIT(n))
/** Clear n-th bit in word. */
#define BIT_CLR(word, n) ((word) &= ~BIT(n))
/** Toggle the n-th bit in word. */
#define BIT_TOGGLE(word, n) ((word) ^= BIT(n))
/** Mask out given bit in word. */
#define BIT_MASK(word, n) ((word) & BIT(n))

/** @return value "1" if n-th bit of word is set and zero otherwise. */
#define BIT_IS_SET(word, n) (!!BIT_MASK(word, n))


#endif
