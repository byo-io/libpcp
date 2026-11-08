#pragma once

#include <byo/pcp/pcp.h>

/**
 * @brief A signed difference between two points in time, in milliseconds
 */
typedef int64_t pcp_duration_ms_t;

/**
 * @brief How long it is from `from` until `to`, negative if `to` comes first
 */
static inline pcp_duration_ms_t pcp_duration_between(pcp_time_ms_t from,
                                                     pcp_time_ms_t to) {
  return (pcp_duration_ms_t)to - (pcp_duration_ms_t)from;
}
