#include "config_store.h"

#include "crc32.h"
#include <string.h>

#define CONFIG_STORE_ERR_BAD_PARAM (-1)
#define CONFIG_STORE_ERR_RANGE (-2)
#define CONFIG_STORE_ERR_CRC (-3)
#define CONFIG_STORE_ERR_EMPTY (-4)
#define CONFIG_STORE_ERR_STATE (-5)
#define CONFIG_STORE_ERR_SCHEMA (-6)

struct config_store_slot {
  struct config_blob blob;
  bool valid;
  bool temporary;
  uint32_t generation;
};

struct config_store_context {
  struct config_store_slot active;
  struct config_store_slot staged;
  struct config_store_slot backup;
  uint16_t last_error;
};

static struct config_store_context g_config_store[CONFIG_STORE_BUD_COUNT];

static void config_store_apply_header_defaults(struct config_blob_header *header) {
  if (header->schema_version == 0) {
    header->schema_version = CONFIG_STORE_SCHEMA_VERSION;
  }

  if (header->max_points_per_ear == 0) {
    header->max_points_per_ear = CONFIG_STORE_MAX_POINTS;
  }

  if (header->section_budget_bytes == 0 ||
      header->section_budget_bytes > CONFIG_STORE_MAX_SECTION_BYTES) {
    header->section_budget_bytes = CONFIG_STORE_MAX_SECTION_BYTES;
  }
}

uint32_t config_store_calculate_crc(const struct config_blob_header *header,
                                    const uint8_t *payload) {
  return crc32(0, payload, header->payload_len);
}

static int config_store_check_bounds(const struct config_blob_header *header,
                                     uint32_t blob_len) {
  if (header->schema_version == 0) {
    return CONFIG_STORE_ERR_BAD_PARAM;
  }

  if (header->schema_version != CONFIG_STORE_SCHEMA_VERSION) {
    return CONFIG_STORE_ERR_SCHEMA;
  }

  if (header->payload_len > CONFIG_STORE_MAX_PAYLOAD_BYTES) {
    return CONFIG_STORE_ERR_RANGE;
  }

  if (header->max_points_per_ear == 0 ||
      header->max_points_per_ear > CONFIG_STORE_MAX_POINTS) {
    return CONFIG_STORE_ERR_RANGE;
  }

  if (header->section_budget_bytes == 0 ||
      header->section_budget_bytes > CONFIG_STORE_MAX_SECTION_BYTES) {
    return CONFIG_STORE_ERR_RANGE;
  }

  if (blob_len < sizeof(*header) + header->payload_len) {
    return CONFIG_STORE_ERR_RANGE;
  }

  return 0;
}

static int config_store_validate_and_copy(const uint8_t *blob, uint32_t blob_len,
                                          struct config_blob *out_blob) {
  if (!blob || !out_blob) {
    return CONFIG_STORE_ERR_BAD_PARAM;
  }

  if (blob_len < sizeof(struct config_blob_header)) {
    return CONFIG_STORE_ERR_RANGE;
  }

  const struct config_blob_header *rx_header =
      (const struct config_blob_header *)blob;
  struct config_blob_header header = *rx_header;

  config_store_apply_header_defaults(&header);

  int bounds_res = config_store_check_bounds(&header, blob_len);
  if (bounds_res) {
    return bounds_res;
  }

  uint32_t payload_crc = config_store_calculate_crc(
      &header, blob + sizeof(struct config_blob_header));

  if (payload_crc != header.payload_crc32) {
    return CONFIG_STORE_ERR_CRC;
  }

  memcpy(&out_blob->header, &header, sizeof(header));
  memset(out_blob->payload, 0, sizeof(out_blob->payload));
  memcpy(out_blob->payload, blob + sizeof(struct config_blob_header),
         header.payload_len);

  return 0;
}

int config_store_validate_blob(const uint8_t *blob, uint32_t blob_len,
                               struct config_blob_header *out_header) {
  if (blob_len < sizeof(struct config_blob_header)) {
    return CONFIG_STORE_ERR_RANGE;
  }

  const struct config_blob_header *rx_header =
      (const struct config_blob_header *)blob;
  struct config_blob_header header = *rx_header;

  config_store_apply_header_defaults(&header);

  int bounds_res = config_store_check_bounds(&header, blob_len);
  if (bounds_res) {
    return bounds_res;
  }

  if (out_header) {
    memcpy(out_header, &header, sizeof(header));
  }

  return 0;
}

static struct config_store_context *config_store_get_ctx(config_store_bud_t bud) {
  if (bud >= CONFIG_STORE_BUD_COUNT) {
    return NULL;
  }
  return &g_config_store[bud];
}

void config_store_init(void) {
  for (int i = 0; i < CONFIG_STORE_BUD_COUNT; i++) {
    config_store_reset((config_store_bud_t)i);
  }
}

void config_store_reset(config_store_bud_t bud) {
  struct config_store_context *ctx = config_store_get_ctx(bud);
  if (!ctx) {
    return;
  }

  memset(ctx, 0, sizeof(*ctx));

  ctx->active.blob.header.schema_version = CONFIG_STORE_SCHEMA_VERSION;
  ctx->active.blob.header.max_points_per_ear = CONFIG_STORE_MAX_POINTS;
  ctx->active.blob.header.section_budget_bytes = CONFIG_STORE_MAX_SECTION_BYTES;
  ctx->active.valid = false;
  ctx->staged.valid = false;
  ctx->backup.valid = false;
  ctx->last_error = 0;
}

int config_store_stage(config_store_bud_t bud, const uint8_t *blob,
                       uint32_t blob_len) {
  struct config_store_context *ctx = config_store_get_ctx(bud);
  if (!ctx) {
    return CONFIG_STORE_ERR_BAD_PARAM;
  }

  struct config_blob parsed_blob = {0};
  int res = config_store_validate_and_copy(blob, blob_len, &parsed_blob);
  if (res) {
    ctx->last_error = (uint16_t)(-res);
    return res;
  }

  ctx->staged.blob = parsed_blob;
  ctx->staged.valid = true;
  ctx->staged.temporary = false;
  ctx->staged.generation = ctx->active.generation + 1;
  ctx->last_error = 0;

  return 0;
}

int config_store_apply_temp(config_store_bud_t bud) {
  struct config_store_context *ctx = config_store_get_ctx(bud);
  if (!ctx || !ctx->staged.valid) {
    return CONFIG_STORE_ERR_STATE;
  }

  ctx->backup = ctx->active;
  ctx->active = ctx->staged;
  ctx->active.temporary = true;
  ctx->active.valid = true;

  return 0;
}

int config_store_commit(config_store_bud_t bud) {
  struct config_store_context *ctx = config_store_get_ctx(bud);
  if (!ctx) {
    return CONFIG_STORE_ERR_BAD_PARAM;
  }

  if (!ctx->staged.valid && !ctx->active.temporary) {
    return CONFIG_STORE_ERR_STATE;
  }

  if (ctx->staged.valid) {
    ctx->backup = ctx->active;
    ctx->active = ctx->staged;
    ctx->staged.valid = false;
  }

  ctx->active.temporary = false;
  ctx->active.valid = true;

  return 0;
}

int config_store_rollback(config_store_bud_t bud) {
  struct config_store_context *ctx = config_store_get_ctx(bud);
  if (!ctx) {
    return CONFIG_STORE_ERR_BAD_PARAM;
  }

  if (!ctx->backup.valid) {
    return CONFIG_STORE_ERR_EMPTY;
  }

  ctx->active = ctx->backup;
  ctx->staged.valid = false;
  ctx->active.temporary = false;

  return 0;
}

static int config_store_get_blob(const struct config_store_slot *slot,
                                 const uint8_t **blob, uint32_t *blob_len) {
  if (!slot->valid) {
    return CONFIG_STORE_ERR_EMPTY;
  }

  if (blob) {
    *blob = (const uint8_t *)&slot->blob;
  }

  if (blob_len) {
    *blob_len = sizeof(struct config_blob_header) + slot->blob.header.payload_len;
  }

  return 0;
}

int config_store_get_active_blob(config_store_bud_t bud, const uint8_t **blob,
                                 uint32_t *blob_len) {
  struct config_store_context *ctx = config_store_get_ctx(bud);
  if (!ctx) {
    return CONFIG_STORE_ERR_BAD_PARAM;
  }

  return config_store_get_blob(&ctx->active, blob, blob_len);
}

int config_store_get_staged_blob(config_store_bud_t bud, const uint8_t **blob,
                                 uint32_t *blob_len) {
  struct config_store_context *ctx = config_store_get_ctx(bud);
  if (!ctx) {
    return CONFIG_STORE_ERR_BAD_PARAM;
  }

  return config_store_get_blob(&ctx->staged, blob, blob_len);
}

int config_store_get_status(config_store_bud_t bud,
                            struct config_store_status *status) {
  struct config_store_context *ctx = config_store_get_ctx(bud);
  if (!ctx || !status) {
    return CONFIG_STORE_ERR_BAD_PARAM;
  }

  status->has_active = ctx->active.valid;
  status->has_staged = ctx->staged.valid;
  status->has_backup = ctx->backup.valid;
  status->temporary_active = ctx->active.temporary;
  status->active_generation = ctx->active.generation;
  status->staged_generation = ctx->staged.generation;
  status->schema_version = ctx->active.blob.header.schema_version;
  status->last_error = ctx->last_error;

  return 0;
}
