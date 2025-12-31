#include "config_protocol.h"

#include "config_store.h"
#include <stdbool.h>
#include <string.h>

#ifdef __PC_CMD_UART__
#include "hal_cmd.h"
extern void hal_cmd_set_res_playload(uint8_t *data, int len);
#endif

#define CONFIG_PROTOCOL_OK 0
#define CONFIG_PROTOCOL_ERR_BAD_PARAM (-1)
#define CONFIG_PROTOCOL_ERR_LENGTH (-2)

#define CONFIG_PROTOCOL_FLAG_FINAL_CHUNK 0x01

struct config_transfer_state {
  uint16_t expected_total;
  uint16_t received_total;
  uint8_t buffer[CONFIG_STORE_MAX_BLOB_SIZE];
};

static struct config_transfer_state g_transfer_state[CONFIG_STORE_BUD_COUNT];

struct config_device_info_payload {
  uint8_t bud_count;
  uint8_t schema_version;
  uint8_t max_points;
  uint16_t section_budget_bytes;
  struct {
    uint8_t has_active;
    uint8_t has_staged;
    uint8_t has_backup;
    uint8_t temporary_active;
    uint32_t active_generation;
    uint32_t staged_generation;
    uint16_t last_error;
  } buds[CONFIG_STORE_BUD_COUNT];
} __attribute__((packed));

struct config_get_request {
  uint8_t bud;
  uint8_t staged_only;
} __attribute__((packed));

struct config_get_response {
  uint8_t status;
  struct config_blob_header header;
  uint8_t payload[CONFIG_STORE_MAX_PAYLOAD_BYTES];
} __attribute__((packed));

struct config_set_chunk_header {
  uint8_t bud;
  uint8_t flags;
  uint16_t total_size;
  uint16_t chunk_offset;
  uint16_t chunk_size;
} __attribute__((packed));

struct config_commit_request {
  uint8_t bud;
} __attribute__((packed));

static void config_protocol_reset_transfer(config_store_bud_t bud) {
  g_transfer_state[bud].expected_total = 0;
  g_transfer_state[bud].received_total = 0;
  memset(g_transfer_state[bud].buffer, 0, sizeof(g_transfer_state[bud].buffer));
}

void config_protocol_init(void) {
  config_store_init();
  for (int i = 0; i < CONFIG_STORE_BUD_COUNT; i++) {
    config_protocol_reset_transfer((config_store_bud_t)i);
  }
}

#ifdef __PC_CMD_UART__
static int config_protocol_send_blob_response(config_store_bud_t bud,
                                              bool staged_only) {
  const uint8_t *blob = NULL;
  uint32_t blob_len = 0;
  int res = staged_only ? config_store_get_staged_blob(bud, &blob, &blob_len)
                        : config_store_get_active_blob(bud, &blob, &blob_len);

  struct config_get_response response = {0};
  response.status = res;

  if (res == 0 && blob_len <= sizeof(response)) {
    memcpy(&response.header, blob, sizeof(struct config_blob_header));
    memcpy(response.payload, blob + sizeof(struct config_blob_header),
           response.header.payload_len);
    hal_cmd_set_res_playload((uint8_t *)&response,
                             sizeof(struct config_get_response));
  }

  return res;
}

static int config_protocol_get_device_info(void) {
  struct config_device_info_payload payload = {0};
  payload.bud_count = CONFIG_STORE_BUD_COUNT;
  payload.schema_version = CONFIG_STORE_SCHEMA_VERSION;
  payload.max_points = CONFIG_STORE_MAX_POINTS;
  payload.section_budget_bytes = CONFIG_STORE_MAX_SECTION_BYTES;

  for (int i = 0; i < CONFIG_STORE_BUD_COUNT; i++) {
    struct config_store_status status = {0};
    config_store_get_status((config_store_bud_t)i, &status);
    payload.buds[i].has_active = status.has_active;
    payload.buds[i].has_staged = status.has_staged;
    payload.buds[i].has_backup = status.has_backup;
    payload.buds[i].temporary_active = status.temporary_active;
    payload.buds[i].active_generation = status.active_generation;
    payload.buds[i].staged_generation = status.staged_generation;
    payload.buds[i].last_error = status.last_error;
  }

  hal_cmd_set_res_playload((uint8_t *)&payload, sizeof(payload));
  return CONFIG_PROTOCOL_OK;
}

static int config_protocol_handle_get(uint8_t *buf, uint32_t len) {
  if (len < sizeof(struct config_get_request)) {
    return CONFIG_PROTOCOL_ERR_LENGTH;
  }

  struct config_get_request request = {0};
  memcpy(&request, buf, sizeof(request));

  if (request.bud >= CONFIG_STORE_BUD_COUNT) {
    return CONFIG_PROTOCOL_ERR_BAD_PARAM;
  }

  return config_protocol_send_blob_response(
      (config_store_bud_t)request.bud, request.staged_only != 0);
}

static int config_protocol_handle_set(uint8_t *buf, uint32_t len) {
  if (len < sizeof(struct config_set_chunk_header)) {
    return CONFIG_PROTOCOL_ERR_LENGTH;
  }

  struct config_set_chunk_header header = {0};
  memcpy(&header, buf, sizeof(header));

  if (header.bud >= CONFIG_STORE_BUD_COUNT) {
    return CONFIG_PROTOCOL_ERR_BAD_PARAM;
  }

  if (header.total_size > CONFIG_STORE_MAX_BLOB_SIZE ||
      header.chunk_size > CONFIG_STORE_MAX_BLOB_SIZE) {
    config_protocol_reset_transfer((config_store_bud_t)header.bud);
    return CONFIG_PROTOCOL_ERR_LENGTH;
  }

  if (header.chunk_offset + header.chunk_size > header.total_size) {
    config_protocol_reset_transfer((config_store_bud_t)header.bud);
    return CONFIG_PROTOCOL_ERR_LENGTH;
  }

  uint32_t payload_len = len - sizeof(header);
  if (payload_len != header.chunk_size) {
    config_protocol_reset_transfer((config_store_bud_t)header.bud);
    return CONFIG_PROTOCOL_ERR_LENGTH;
  }

  struct config_transfer_state *state = &g_transfer_state[header.bud];
  if (state->expected_total == 0) {
    state->expected_total = header.total_size;
    state->received_total = 0;
  }

  if (header.total_size != state->expected_total ||
      header.chunk_offset != state->received_total) {
    config_protocol_reset_transfer((config_store_bud_t)header.bud);
    return CONFIG_PROTOCOL_ERR_BAD_PARAM;
  }

  memcpy(state->buffer + header.chunk_offset, buf + sizeof(header),
         header.chunk_size);
  state->received_total += header.chunk_size;

  if (header.flags & CONFIG_PROTOCOL_FLAG_FINAL_CHUNK) {
    int stage_res = config_store_stage((config_store_bud_t)header.bud,
                                       state->buffer, state->expected_total);
    config_protocol_reset_transfer((config_store_bud_t)header.bud);
    return stage_res;
  }

  return CONFIG_PROTOCOL_OK;
}

static int config_protocol_handle_apply_temp(uint8_t *buf, uint32_t len) {
  if (len < sizeof(struct config_commit_request)) {
    return CONFIG_PROTOCOL_ERR_LENGTH;
  }

  struct config_commit_request request = {0};
  memcpy(&request, buf, sizeof(request));

  if (request.bud >= CONFIG_STORE_BUD_COUNT) {
    return CONFIG_PROTOCOL_ERR_BAD_PARAM;
  }

  return config_store_apply_temp((config_store_bud_t)request.bud);
}

static int config_protocol_handle_commit(uint8_t *buf, uint32_t len) {
  if (len < sizeof(struct config_commit_request)) {
    return CONFIG_PROTOCOL_ERR_LENGTH;
  }

  struct config_commit_request request = {0};
  memcpy(&request, buf, sizeof(request));

  if (request.bud >= CONFIG_STORE_BUD_COUNT) {
    return CONFIG_PROTOCOL_ERR_BAD_PARAM;
  }

  return config_store_commit((config_store_bud_t)request.bud);
}

static int config_protocol_handle_rollback(uint8_t *buf, uint32_t len) {
  if (len < sizeof(struct config_commit_request)) {
    return CONFIG_PROTOCOL_ERR_LENGTH;
  }

  struct config_commit_request request = {0};
  memcpy(&request, buf, sizeof(request));

  if (request.bud >= CONFIG_STORE_BUD_COUNT) {
    return CONFIG_PROTOCOL_ERR_BAD_PARAM;
  }

  return config_store_rollback((config_store_bud_t)request.bud);
}

static int config_protocol_handle_panic_off(uint8_t *buf, uint32_t len) {
  (void)buf;
  (void)len;
  for (int i = 0; i < CONFIG_STORE_BUD_COUNT; i++) {
    config_store_reset((config_store_bud_t)i);
  }
  return CONFIG_PROTOCOL_OK;
}

static int config_protocol_handle_device_info(uint8_t *buf, uint32_t len) {
  (void)buf;
  (void)len;
  return config_protocol_get_device_info();
}

static const struct {
  const char *name;
  int (*handler)(uint8_t *, uint32_t);
} kConfigCommands[] = {
    {"get_dev", config_protocol_handle_device_info},
    {"get_cfg", config_protocol_handle_get},
    {"set_cfg", config_protocol_handle_set},
    {"apply_tmp", config_protocol_handle_apply_temp},
    {"commit_cfg", config_protocol_handle_commit},
    {"rollback", config_protocol_handle_rollback},
    {"panic_off", config_protocol_handle_panic_off},
};

void config_protocol_register_uart(void) {
  for (unsigned int i = 0; i < sizeof(kConfigCommands) / sizeof(kConfigCommands[0]);
       i++) {
    hal_cmd_register((char *)kConfigCommands[i].name, kConfigCommands[i].handler);
  }
}
#else
void config_protocol_register_uart(void) {}
#endif

