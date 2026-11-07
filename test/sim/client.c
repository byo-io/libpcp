#include "client.h"

bool sim_client_start(sim_client_t *client,
                      ip_address_t local,
                      pcp_rng_t rng,
                      sim_io_t io) {
  client->io = io;
  client->session = pcp_session_begin((pcp_config_t){
      .local = local,
      .allocator = {0},
      .rng = rng,
      .buffer = {.size = sizeof(client->send_buffer),
                 .data = client->send_buffer},
  });
  return client->session != nullptr;
}

pcp_handle_or_error_t sim_client_map(sim_client_t *client,
                                     pcp_time_s_t lifetime,
                                     pcp_protocol_t protocol,
                                     port_t port,
                                     socket_address_t external) {
  auto const result = pcp_session_map_request(
      client->session, lifetime, protocol, port, external, 0, nullptr);
  client->io.on_requested(client->io.ctx, result, protocol, port);
  return result;
}

void sim_client_release(sim_client_t *client, pcp_handle_t handle) {
  pcp_session_release(client->session, handle);
  client->io.on_released(client->io.ctx, handle);
}

pcp_optional_time_ms_t sim_client_run(sim_client_t *client, pcp_time_ms_t now) {
  while (true) {
    auto const event = pcp_session_poll(client->session, now);
    switch (event.type) {
    case PCP_EVENT_TIMEOUT:
      return event.timeout.deadline;
    case PCP_EVENT_SEND:
      client->io.send(client->io.ctx, event.send.size, event.send.data);
      break;
    default:
      client->io.on_event(client->io.ctx, &event);
      break;
    }
  }
}
