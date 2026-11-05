#include "network.h"

#include <string.h>
#include <unity.h>

// the next datagram to arrive, the one sent first if it's a tie
static size_t next_datagram(sim_network_t const *network) {
  size_t next = 0;
  for (size_t i = 1; i < network->in_flight; ++i) {
    if (network->datagrams[i].deliver_at <
        network->datagrams[next].deliver_at) {
      next = i;
    }
  }
  return next;
}

void sim_network_begin(sim_network_t *network, sim_duration_ms_t latency) {
  network->latency = latency;
  network->in_flight = 0;
}

void sim_network_send(sim_network_t *network,
                      pcp_time_ms_t now,
                      sim_endpoint_t to,
                      size_t size,
                      uint8_t const data[size]) {
  TEST_ASSERT_LESS_THAN_size_t_MESSAGE(SIM_NETWORK_MAX_IN_FLIGHT,
                                       network->in_flight,
                                       "too many datagrams in flight");
  TEST_ASSERT_LESS_OR_EQUAL_size_t_MESSAGE(
      SIM_NETWORK_MAX_DATAGRAM_SIZE, size,
      "datagram too large for the network");

  auto const datagram = &network->datagrams[network->in_flight++];
  datagram->deliver_at = now + network->latency;
  datagram->to = to;
  datagram->size = size;
  memcpy(datagram->data, data, size);
}

bool sim_network_receive(sim_network_t *network,
                         pcp_time_ms_t now,
                         sim_datagram_t *out) {
  if (network->in_flight == 0) {
    return false;
  }

  auto const next = next_datagram(network);
  if (network->datagrams[next].deliver_at > now) {
    return false;
  }

  *out = network->datagrams[next];
  memmove(&network->datagrams[next], &network->datagrams[next + 1],
          (network->in_flight - next - 1) * sizeof(sim_datagram_t));
  --network->in_flight;
  return true;
}

bool sim_network_next_arrival(sim_network_t const *network,
                              pcp_time_ms_t *out) {
  if (network->in_flight == 0) {
    return false;
  }

  *out = network->datagrams[next_datagram(network)].deliver_at;
  return true;
}

size_t sim_network_in_flight(sim_network_t const *network) {
  return network->in_flight;
}
