#include "pcp/epoch.h"

#include <assert.h>

#include "duration.h"

// how far apart the two clocks may move, on top of 1/16 of drift
static constexpr pcp_duration_ms_t PCP_EPOCH_SLACK = 2000;

static pcp_duration_ms_t pcp_epoch_delta(pcp_time_s_t from, pcp_time_s_t to) {
  return pcp_duration_between((pcp_time_ms_t)from * 1000,
                              (pcp_time_ms_t)to * 1000);
}

static bool pcp_epoch_valid(pcp_epoch_t const *epoch,
                            pcp_time_s_t client,
                            pcp_time_s_t server) {
  if (!epoch->has_previous) {
    return true;
  }

  auto const client_delta = pcp_epoch_delta(epoch->client, client);
  auto const server_delta = pcp_epoch_delta(epoch->server, server);

  // the server's clock went backwards
  if (server_delta < -PCP_EPOCH_SLACK) {
    return false;
  }

  // both clocks should have moved by roughly the same amount
  return client_delta + PCP_EPOCH_SLACK >= server_delta - server_delta / 16 &&
         server_delta + PCP_EPOCH_SLACK >= client_delta - client_delta / 16;
}

bool pcp_epoch_update(pcp_epoch_t *epoch,
                      pcp_time_ms_t now,
                      pcp_time_s_t server) {
  assert(epoch);

  auto const client = (pcp_time_s_t)(now / 1000);
  assert(!epoch->has_previous || client >= epoch->client);

  auto const valid = pcp_epoch_valid(epoch, client, server);
  *epoch = (pcp_epoch_t){
      .has_previous = true,
      .client = client,
      .server = server,
  };
  return valid;
}
