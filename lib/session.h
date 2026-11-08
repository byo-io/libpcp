#pragma once

#include <byo/pcp/pcp.h>

#include "mapping.h"
#include "pcp/epoch.h"

typedef struct pcp_session {
  pcp_config_t config;
  pcp_epoch_t epoch;
  pcp_mapping_t *mappings;
  pcp_handle_t handles;
} pcp_session_t;
