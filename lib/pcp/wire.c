#include "pcp/wire.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Parsing and validation on top of the generated codec (pcp_gen.h, from
// pcp.pkt): the PCP rules the schema can't express

static struct pcp_request_option pcp_packet_option(pcp_option_t const *src) {
  struct pcp_request_option result = {0};
  switch (src->type) {
  case PCP_OPTION_THIRD_PARTY:
    result.code = PCP_OPTION_CODE_THIRD_PARTY;
    memcpy(result.body.third_party.internal_ip, src->third_party.internal.addr,
           sizeof(result.body.third_party.internal_ip));
    break;
  case PCP_OPTION_FILTER:
    result.code = PCP_OPTION_CODE_FILTER;
    result.body.filter.prefix = src->filter.peers.prefix;
    result.body.filter.peer_port = src->filter.port;
    memcpy(result.body.filter.peer_ip, src->filter.peers.address.addr,
           sizeof(result.body.filter.peer_ip));
    break;
  }
  return result;
}

// Deletes (lifetime 0) can't carry FILTER. THIRD_PARTY has to stay though:
// without it the server looks for a mapping on our own address, doesn't find
// one and still answers SUCCESS.
static bool pcp_option_included(struct pcp_request const *request,
                                pcp_option_t const *option) {
  return option->type != PCP_OPTION_FILTER || request->lifetime > 0;
}

size_t pcp_packet_request_size(struct pcp_request const *request,
                               size_t count,
                               pcp_option_t const options[count]) {
  assert(request);
  assert(request->opcode == PCP_OPCODE_MAP ||
         request->opcode == PCP_OPCODE_PEER);
  assert(count == 0 || options);

  size_t size = pcp_request_encoded_size(request);
  auto const begin = options;
  auto const end = options + count;
  for (auto iter = begin; iter != end; ++iter) {
    if (pcp_option_included(request, iter)) {
      auto const option = pcp_packet_option(iter);
      size += pcp_request_option_encoded_size(&option);
    }
  }
  return size;
}

size_t pcp_packet_write_request(view_t dst,
                                struct pcp_request const *request,
                                size_t count,
                                pcp_option_t const options[count]) {
  assert(dst.size >= pcp_packet_request_size(request, count, options));

  size_t cursor = 0;
  [[maybe_unused]] auto status =
      pcp_request_encode(request, dst.data, dst.size, &cursor);
  assert(status == PCP_OK);

  // the options follow the request
  auto const begin = options;
  auto const end = options + count;
  for (auto iter = begin; iter != end; ++iter) {
    if (pcp_option_included(request, iter)) {
      auto const option = pcp_packet_option(iter);
      size_t written = 0;
      status = pcp_request_option_encode(&option, dst.data + cursor,
                                         dst.size - cursor, &written);
      assert(status == PCP_OK);
      cursor += written;
    }
  }

  return cursor;
}

pcp_optional_header_t pcp_packet_read_header(const_view_t src) {
  assert(src.data);

  pcp_optional_header_t result = {.has_value = false};
  result.has_value =
      pcp_response_header_decode(&result.value, src.data, src.size) == PCP_OK;
  return result;
}

pcp_optional_response_t
pcp_packet_read_response(struct pcp_response_header const *header) {
  assert(header);
  assert(header->version == PCP_VERSION);

  pcp_optional_response_t result = {.has_value = false};
  auto const out = &result.value;
  auto const body = header->rest;

  // silently drop anything malformed (RFC 6887 section 8.3): requests,
  // datagrams that aren't a multiple of 4 octets, and bodies that don't fit
  // their opcode, like errors without the request's body. Those can't be
  // matched to a mapping anyway.
  if (header->r != 1 || body.len % 4 != 0) {
    return result;
  }

  switch (header->opcode) {
  case PCP_OPCODE_ANNOUNCE: {
    struct pcp_announce_response announce;
    if (pcp_announce_response_decode(&announce, body.ptr, body.len) != PCP_OK) {
      return result;
    }
    out->lifetime = announce.lifetime;
    out->epoch = announce.epoch;
    break;
  }
  case PCP_OPCODE_MAP: {
    auto const map = &out->body.map_response;
    if (pcp_map_response_decode(map, body.ptr, body.len) != PCP_OK) {
      return result;
    }
    out->lifetime = map->lifetime;
    out->epoch = map->epoch;
    break;
  }
  case PCP_OPCODE_PEER: {
    auto const peer = &out->body.peer_response;
    if (pcp_peer_response_decode(peer, body.ptr, body.len) != PCP_OK) {
      return result;
    }
    out->lifetime = peer->lifetime;
    out->epoch = peer->epoch;
    break;
  }
  default:
    // only ANNOUNCE and responses to our own MAP/PEER requests matter
    return result;
  }

  out->opcode = header->opcode;
  out->result = header->result;
  result.has_value = true;
  return result;
}
