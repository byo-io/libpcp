#pragma once

#include <byo/pcp/pcp.h>

#include <stdalign.h>
#include <stddef.h>

// The size, alignment and field offsets of the public types, as one compiler
// sees them. The library is C23 and its users can be C11, so both have to agree
// on these.
#define PCP_LAYOUT(X)                                                          \
  X(sizeof(pcp_protocol_t))                                                    \
  X(sizeof(pcp_result_t))                                                      \
  X(sizeof(pcp_ip_address_t))                                                  \
  X(sizeof(pcp_optional_ip_address_t))                                         \
  X(sizeof(pcp_ip_network_t))                                                  \
  X(sizeof(pcp_socket_address_t))                                              \
  X(sizeof(pcp_optional_socket_address_t))                                     \
  X(sizeof(pcp_optional_time_ms_t))                                            \
  X(sizeof(pcp_allocator_t))                                                   \
  X(sizeof(pcp_rng_t))                                                         \
  X(sizeof(pcp_buffer_t))                                                      \
  X(sizeof(pcp_config_t))                                                      \
  X(sizeof(pcp_option_t))                                                      \
  X(sizeof(pcp_handle_or_error_t))                                             \
  X(sizeof(pcp_event_t))                                                       \
  X(alignof(pcp_event_t))                                                      \
  X(offsetof(pcp_event_t, failed.handle))                                      \
  X(offsetof(pcp_event_t, failed.result))                                      \
  X(offsetof(pcp_event_t, mapped.external))                                    \
  X(offsetof(pcp_option_t, filter.port))

/**
 * @brief The PCP_LAYOUT values from a file compiled as C11, in order
 */
extern size_t const layout_c11[];
