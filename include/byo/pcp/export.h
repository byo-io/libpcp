#pragma once

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
