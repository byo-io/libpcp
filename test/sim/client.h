#pragma once

#include <pcp/pcp.h>

// The application side of the library: holds a session, asks for mappings and
// runs the event loop any application would. It's generic on purpose, and only
// knows pcp.h. Where datagrams go and what becomes of what it does is up to
// whoever provides the io, and nothing in here is specific to testing.

typedef struct sim_io {
  /// Send a datagram to the server, `data` is only valid during the call
  void (*send)(void *ctx, size_t size, uint8_t const data[size]);
  /// Handle anything else the application has to know about
  void (*on_event)(void *ctx, pcp_event_t const *event);
  /// The application asked for a mapping, and got `result` for it
  void (*on_requested)(void *ctx,
                       pcp_handle_or_error_t result,
                       pcp_protocol_t protocol,
                       pcp_port_t port);
  /// The application released `handle`
  void (*on_released)(void *ctx, pcp_handle_t handle);
  void *ctx;
} sim_io_t;

typedef struct sim_client {
  pcp_session_t *session;
  sim_io_t io;
  uint8_t send_buffer[PCP_PACKET_MAX_SIZE];
} sim_client_t;

bool sim_client_start(sim_client_t *client,
                      pcp_ip_address_t local,
                      pcp_rng_t rng,
                      sim_io_t io);

/**
 * @brief Ask for a mapping, like the application would
 *
 * @returns What the library returned, which the io hears about too
 */
pcp_handle_or_error_t sim_client_map(sim_client_t *client,
                                     pcp_time_s_t lifetime,
                                     pcp_protocol_t protocol,
                                     pcp_port_t port,
                                     pcp_socket_address_t external);

void sim_client_release(sim_client_t *client, pcp_handle_t handle);

/**
 * @brief Handle everything that's due at `now`
 *
 * @returns When to run again if no datagram arrives first, nothing if only a
 *          datagram can change anything
 */
pcp_optional_time_ms_t sim_client_run(sim_client_t *client, pcp_time_ms_t now);
