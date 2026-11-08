#pragma once

#include <byo/pcp/pcp.h>

typedef struct pcp_epoch {
  bool has_previous;
  // the time the previous response was received
  pcp_time_s_t client;
  // the server's epoch time from the previous response
  pcp_time_s_t server;
} pcp_epoch_t;

/**
 * @brief Check a response's epoch time against the previous one, then store it
 *
 * @param[in] epoch  The previous epoch, updated in either case
 * @param[in] now    The time the response was received
 * @param[in] server The epoch time from the response
 *
 * @returns false if the server lost its state (e.g. it restarted)
 */
bool pcp_epoch_update(pcp_epoch_t *epoch,
                      pcp_time_ms_t now,
                      pcp_time_s_t server);
