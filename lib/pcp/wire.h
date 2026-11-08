#pragma once

#include <byo/pcp/ip.h>
#include <byo/pcp/pcp.h>

#include "pcp_gen.h"
#include "view.h"

/// The version of PCP spoken here (RFC 6887)
static constexpr uint8_t PCP_VERSION = 2;

/**
 * @brief Size of the request pcp_packet_write_request() would write
 */
size_t pcp_packet_request_size(struct pcp_request const *request,
                               size_t count,
                               pcp_option_t const options[count]);

/**
 * @brief Write a MAP or PEER request and its options
 *
 * @returns The size of the request
 */
size_t pcp_packet_write_request(view_t dst,
                                struct pcp_request const *request,
                                size_t count,
                                pcp_option_t const options[count]);

typedef struct pcp_optional_header {
  bool has_value;
  struct pcp_response_header value;
} pcp_optional_header_t;

/**
 * @brief Parse the header of anything received from the server, the first
 *        step of reading a response
 *
 * Reads any version, so the caller can tell a version 2 response from a
 * server that speaks another one, like NAT-PMP's UNSUPP_VERSION. Checks
 * nothing else.
 *
 * @param[in] src The received datagram
 *
 * @returns The header, its `rest` points into `src`. Nothing if `src` is too
 *          short for one.
 */
pcp_optional_header_t pcp_packet_read_header(const_view_t src);

/**
 * @brief A version 2 response to one of our requests, or an ANNOUNCE
 */
typedef struct pcp_response {
  enum pcp_opcode opcode;
  uint8_t result;
  uint32_t lifetime;
  uint32_t epoch;
  /// Set for MAP and PEER
  union {
    struct pcp_map_response map_response;
    struct pcp_peer_response peer_response;
  } body;
} pcp_response_t;

typedef struct pcp_optional_response {
  bool has_value;
  pcp_response_t value;
} pcp_optional_response_t;

/**
 * @brief Parse the rest of a version 2 response, the second step of reading
 *        one
 *
 * Only checks the format of ANNOUNCE, MAP and PEER responses, not the result
 * code. Options are skipped.
 *
 * @param[in] header From pcp_packet_read_header, with version PCP_VERSION
 *
 * @returns The parsed response, its views point into the datagram. Nothing if
 *          it isn't a valid response and should be ignored.
 */
pcp_optional_response_t
pcp_packet_read_response(struct pcp_response_header const *header);
