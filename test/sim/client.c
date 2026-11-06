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
