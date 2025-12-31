#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_STORE_MAX_POINTS 32
#define CONFIG_STORE_MAX_SECTION_BYTES 2048
#define CONFIG_STORE_MAX_PAYLOAD_BYTES 64
#define CONFIG_STORE_SCHEMA_VERSION 1

#define CONFIG_STORE_MAX_BLOB_SIZE                                            \
  (sizeof(struct config_blob_header) + CONFIG_STORE_MAX_PAYLOAD_BYTES)

typedef enum {
  CONFIG_STORE_BUD_LEFT = 0,
  CONFIG_STORE_BUD_RIGHT = 1,
  CONFIG_STORE_BUD_COUNT
} config_store_bud_t;

struct config_blob_header {
  uint16_t schema_version;
  uint16_t data_version;
  uint16_t max_points_per_ear;
  uint16_t section_budget_bytes;
  uint32_t payload_len;
  uint32_t payload_crc32;
};

struct config_blob {
  struct config_blob_header header;
  uint8_t payload[CONFIG_STORE_MAX_PAYLOAD_BYTES];
};

struct config_store_status {
  bool has_active;
  bool has_staged;
  bool has_backup;
  bool temporary_active;
  uint32_t active_generation;
  uint32_t staged_generation;
  uint16_t schema_version;
  uint16_t last_error;
};

void config_store_init(void);
void config_store_reset(config_store_bud_t bud);

int config_store_stage(config_store_bud_t bud, const uint8_t *blob,
                       uint32_t blob_len);
int config_store_apply_temp(config_store_bud_t bud);
int config_store_commit(config_store_bud_t bud);
int config_store_rollback(config_store_bud_t bud);

int config_store_get_active_blob(config_store_bud_t bud, const uint8_t **blob,
                                 uint32_t *blob_len);
int config_store_get_staged_blob(config_store_bud_t bud, const uint8_t **blob,
                                 uint32_t *blob_len);
int config_store_get_status(config_store_bud_t bud,
                            struct config_store_status *status);

uint32_t config_store_calculate_crc(const struct config_blob_header *header,
                                    const uint8_t *payload);
int config_store_validate_blob(const uint8_t *blob, uint32_t blob_len,
                               struct config_blob_header *out_header);

#ifdef __cplusplus
}
#endif

