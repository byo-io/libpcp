#pragma once

#include <stdint.h>

/**
 * @brief Marks a function as part of the shared library's interface
 *
 * The library is built with hidden visibility, so only functions marked with
 * this are exported.
 */
#if defined(__GNUC__) || defined(__clang__)
#define PCP_EXPORT __attribute__((visibility("default")))
#else
#define PCP_EXPORT
#endif

// The headers work from C11 on, while the library itself is C23. These keep
// C23's checks where they're available, with the same layout either way.
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) ||              \
    defined(__cplusplus)
/**
 * @brief constexpr for constants that work in constant expressions from C23 on
 */
#define PCP_CONSTEXPR constexpr
/**
 * @brief An enum one byte wide, e.g. PCP_UINT8_ENUM(pcp_result) {...}; also
 *        declares pcp_result_t
 *
 * Before C23 the type is a plain uint8_t and the values are ordinary enum
 * constants. From C23 on it's an enum with uint8_t as its underlying type,
 * which is compatible with uint8_t.
 */
#define PCP_UINT8_ENUM(tag)                                                    \
  enum tag : uint8_t;                                                          \
  typedef enum tag tag##_t;                                                    \
  enum tag : uint8_t
#else
#define PCP_CONSTEXPR const
#define PCP_UINT8_ENUM(tag)                                                    \
  typedef uint8_t tag##_t;                                                     \
  enum tag
#endif
