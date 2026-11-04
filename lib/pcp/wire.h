#pragma once

#include <pcp/ip.h>
#include <pcp/pcp.h>

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

typedef struct pcp_optional_response {
  bool has_value;
  struct pcp_response value;
} pcp_optional_response_t;

/**
 * @brief Parse a response received from the server
 *
 * Only checks the format of ANNOUNCE, MAP and PEER responses, not the result
 * code. Options are skipped.
 *
 * @param[in] src The received datagram
 *
 * @returns The parsed response, its views point into `src`. Nothing if `src`
 *          isn't a valid response and should be ignored.
 */
pcp_optional_response_t pcp_packet_read_response(const_view_t src);

typedef struct pcp_optional_header {
  bool has_value;
  struct pcp_response_header value;
} pcp_optional_header_t;

/**
 * @brief Parse the start of anything that may be a response, whatever its
 *        version
 *
 * For responses pcp_packet_read_response rejects, like UNSUPP_VERSION from a
 * server that speaks another version.
 *
 * @returns The header, or nothing if `src` is too short for one
 */
pcp_optional_header_t pcp_packet_read_header(const_view_t src);
