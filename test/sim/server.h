#pragma once

#include <pcp/pcp.h>

// A PCP server (RFC 6887) for the simulation. It tracks real leases and only
// knows the client through the bytes it sends. It shares the library's
// generated wire format (pcp_gen.h), test/unit/wire.c is what pins that to the
// RFC's layout.
//
// A request no real server should have to accept fails the test instead of
// getting an error response. Refusing a well-formed request is regular server
// behavior, and gets a response.
//
// For now it serves a single client's MAP requests without options.

static constexpr size_t SIM_SERVER_MAX_LEASES = 16;
static constexpr size_t SIM_SERVER_RESPONSE_SIZE = 60;

typedef struct sim_lease {
  bool used;
  uint8_t nonce[12];
  pcp_protocol_t protocol;
  port_t internal_port;
  socket_address_t external;
  pcp_time_ms_t expires;
} sim_lease_t;

typedef struct sim_server_stats {
  size_t requests;
  size_t created;
  size_t renewed;
  size_t deleted;
  /// Leases that ran out instead of being renewed or deleted
  size_t expired;
} sim_server_stats_t;

typedef struct sim_server {
  ip_address_t external_ip;
  port_t next_port;
  pcp_time_ms_t started;
  sim_lease_t leases[SIM_SERVER_MAX_LEASES];
  sim_server_stats_t stats;
} sim_server_t;

void sim_server_start(sim_server_t *server,
                      ip_address_t external_ip,
                      pcp_time_ms_t now);

typedef struct sim_server_reply {
  /// What the request asked for and what came of it, for the timeline
  pcp_time_s_t requested_lifetime;
  pcp_result_t result;
  /// The response to send back
  size_t size;
  uint8_t data[SIM_SERVER_RESPONSE_SIZE];
} sim_server_reply_t;

sim_server_reply_t sim_server_on_datagram(sim_server_t *server,
                                          pcp_time_ms_t now,
                                          ip_address_t source,
                                          size_t size,
                                          uint8_t const data[size]);

/**
 * @returns The lease that's live at `now`, or nullptr if there is none
 */
sim_lease_t const *sim_server_find_lease(sim_server_t *server,
                                         pcp_time_ms_t now,
                                         pcp_protocol_t protocol,
                                         port_t internal_port);
