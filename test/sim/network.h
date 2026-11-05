#pragma once

#include <pcp/pcp.h>

// The network between the client and the server. It carries opaque datagrams
// and knows nothing about what's in them.

/// A length of time in milliseconds, pcp_time_ms_t being a point in time
typedef pcp_time_ms_t sim_duration_ms_t;

static constexpr size_t SIM_NETWORK_MAX_IN_FLIGHT = 64;
static constexpr size_t SIM_NETWORK_MAX_DATAGRAM_SIZE = 1500;

typedef enum sim_endpoint {
  SIM_ENDPOINT_CLIENT,
  SIM_ENDPOINT_SERVER,
} sim_endpoint_t;

typedef struct sim_datagram {
  pcp_time_ms_t deliver_at;
  sim_endpoint_t to;
  size_t size;
  uint8_t data[SIM_NETWORK_MAX_DATAGRAM_SIZE];
} sim_datagram_t;

typedef struct sim_network {
  /// Each way
  sim_duration_ms_t latency;
  size_t in_flight;
  sim_datagram_t datagrams[SIM_NETWORK_MAX_IN_FLIGHT];
} sim_network_t;

void sim_network_begin(sim_network_t *network, sim_duration_ms_t latency);

void sim_network_send(sim_network_t *network,
                      pcp_time_ms_t now,
                      sim_endpoint_t to,
                      size_t size,
                      uint8_t const data[size]);

/**
 * @brief Take the next datagram that has arrived by `now`
 *
 * @returns Whether there was one
 */
bool sim_network_receive(sim_network_t *network,
                         pcp_time_ms_t now,
                         sim_datagram_t *out);

/**
 * @brief Get when the next datagram arrives
 *
 * @returns Whether anything is in flight
 */
bool sim_network_next_arrival(sim_network_t const *network, pcp_time_ms_t *out);

size_t sim_network_in_flight(sim_network_t const *network);
