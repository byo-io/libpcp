#include "server.h"

#include <string.h>
#include <unity.h>

#include "pcp_gen.h"

static constexpr uint8_t VERSION = 2;
static constexpr uint32_t ERROR_LIFETIME = 30;

static_assert(SIM_SERVER_RESPONSE_SIZE ==
              PCP_RESPONSE_HEADER_MIN_SIZE + PCP_MAP_RESPONSE_MIN_SIZE);

// what a generated decoder can't know is wrong, it already checks the rest of
// the format
static void check_request(ip_address_t const *source,
                          struct pcp_request const *request) {
  TEST_ASSERT_EQUAL_MESSAGE(PCP_OPCODE_MAP, request->opcode,
                            "only MAP is simulated");
  TEST_ASSERT_MESSAGE(
      memcmp(request->client_ip, source->addr, sizeof(source->addr)) == 0,
      "client address doesn't match the source address");
  TEST_ASSERT_MESSAGE(request->body.map_body.protocol != ALL &&
                          request->body.map_body.internal_port != 0,
                      "mappings of all protocols or ports aren't simulated");
}

static void expire_leases(sim_server_t *server, pcp_time_ms_t now) {
  for (size_t i = 0; i < SIM_SERVER_MAX_LEASES; ++i) {
    auto const lease = &server->leases[i];
    if (lease->used && lease->expires <= now) {
      lease->used = false;
      ++server->stats.expired;
    }
  }
}

static sim_lease_t *
find_lease(sim_server_t *server, uint8_t protocol, port_t internal_port) {
  for (size_t i = 0; i < SIM_SERVER_MAX_LEASES; ++i) {
    auto const lease = &server->leases[i];
    if (lease->used && lease->protocol == protocol &&
        lease->internal_port == internal_port) {
      return lease;
    }
  }
  return nullptr;
}

static bool port_is_free(sim_server_t const *server, port_t port) {
  if (port == 0) {
    return false;
  }

  for (size_t i = 0; i < SIM_SERVER_MAX_LEASES; ++i) {
    auto const lease = &server->leases[i];
    if (lease->used && lease->external.port == port) {
      return false;
    }
  }
  return true;
}

// takes the suggested port if it can, the suggested address never matters since
// the server only has the one
static port_t allocate_port(sim_server_t *server, port_t suggested) {
  if (port_is_free(server, suggested)) {
    return suggested;
  }

  while (!port_is_free(server, server->next_port)) {
    ++server->next_port;
  }
  return server->next_port++;
}

static sim_lease_t *create_lease(sim_server_t *server,
                                 struct pcp_map_body const *map) {
  for (size_t i = 0; i < SIM_SERVER_MAX_LEASES; ++i) {
    auto const lease = &server->leases[i];
    if (lease->used) {
      continue;
    }

    *lease = (sim_lease_t){
        .used = true,
        .protocol = (pcp_protocol_t)map->protocol,
        .internal_port = map->internal_port,
        .external =
            {
                .ip = server->external_ip,
                .port = allocate_port(server, map->external_port),
            },
    };
    memcpy(lease->nonce, map->nonce, sizeof(lease->nonce));
    return lease;
  }

  TEST_FAIL_MESSAGE("the simulated server ran out of leases");
  return nullptr;
}

static void set_external(struct pcp_map_response *body,
                         socket_address_t const *external) {
  body->external_port = external->port;
  memcpy(body->external_ip, external->ip.addr, sizeof(body->external_ip));
}

// updates the leases for `request`, and `body` with what the response says
static pcp_result_t apply_request(sim_server_t *server,
                                  pcp_time_ms_t now,
                                  struct pcp_request const *request,
                                  struct pcp_map_response *body) {
  auto const map = &request->body.map_body;
  auto lease = find_lease(server, map->protocol, map->internal_port);

  // only the nonce that created a mapping can change it, section 11.3
  if (lease && memcmp(lease->nonce, map->nonce, sizeof(lease->nonce)) != 0) {
    body->lifetime = ERROR_LIFETIME;
    return NOT_AUTHORIZED;
  }

  // a delete succeeds whether or not there's anything to delete, section 15
  if (request->lifetime == 0) {
    if (lease) {
      set_external(body, &lease->external);
      lease->used = false;
      ++server->stats.deleted;
    }
    return SUCCESS;
  }

  if (lease) {
    ++server->stats.renewed;
  } else {
    lease = create_lease(server, map);
    ++server->stats.created;
  }

  lease->expires = now + (pcp_time_ms_t)request->lifetime * 1000;
  set_external(body, &lease->external);
  return SUCCESS;
}

// the encoders take the header and what follows it separately, see pcp.pkt
static size_t write_response(pcp_result_t result,
                             struct pcp_map_response const *body,
                             uint8_t response[SIM_SERVER_RESPONSE_SIZE]) {
  uint8_t rest[PCP_MAP_RESPONSE_MIN_SIZE];
  size_t rest_size = 0;
  TEST_ASSERT_EQUAL(
      PCP_OK, pcp_map_response_encode(body, rest, sizeof(rest), &rest_size));

  struct pcp_response_header const header = {
      .version = VERSION,
      .r = 1,
      .opcode = PCP_OPCODE_MAP,
      .result = result,
      .rest = {.ptr = rest, .len = rest_size},
  };
  size_t size = 0;
  TEST_ASSERT_EQUAL(PCP_OK, pcp_response_header_encode(&header, response,
                                                       SIM_SERVER_RESPONSE_SIZE,
                                                       &size));
  return size;
}

void sim_server_begin(sim_server_t *server,
                      ip_address_t external_ip,
                      pcp_time_ms_t now) {
  *server = (sim_server_t){
      .external_ip = external_ip,
      .next_port = 40000,
      .started = now,
  };
}

sim_server_reply_t sim_server_on_datagram(sim_server_t *server,
                                          pcp_time_ms_t now,
                                          ip_address_t source,
                                          size_t size,
                                          uint8_t const data[size]) {
  struct pcp_request request;
  TEST_ASSERT_EQUAL_MESSAGE(PCP_OK, pcp_request_decode(&request, data, size),
                            "malformed request, or one with options, which "
                            "aren't simulated yet");
  check_request(&source, &request);

  expire_leases(server, now);
  ++server->stats.requests;

  // responses echo the request's body, section 11.4
  auto const map = &request.body.map_body;
  struct pcp_map_response body = {
      .lifetime = request.lifetime,
      .epoch = (uint32_t)((now - server->started) / 1000),
      .protocol = map->protocol,
      .internal_port = map->internal_port,
      .external_port = map->external_port,
  };
  memcpy(body.nonce, map->nonce, sizeof(body.nonce));
  memcpy(body.external_ip, map->external_ip, sizeof(body.external_ip));

  sim_server_reply_t reply = {
      .requested_lifetime = request.lifetime,
      .result = apply_request(server, now, &request, &body),
  };
  reply.size = write_response(reply.result, &body, reply.data);
  return reply;
}

sim_lease_t const *sim_server_find_lease(sim_server_t *server,
                                         pcp_time_ms_t now,
                                         pcp_protocol_t protocol,
                                         port_t internal_port) {
  expire_leases(server, now);
  return find_lease(server, protocol, internal_port);
}
