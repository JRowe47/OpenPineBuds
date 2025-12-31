# Companion Protocol Stubs (M1)

This document describes the UART/BLE command stubs introduced for milestone M1.
They focus on the per-bud configuration transport path and surface the request
and response payloads for host integration and testing. All payload examples use
little-endian integer layout.

## Command Summary

| Command name | Purpose | Payload (request) | Payload (response) |
| --- | --- | --- | --- |
| `get_dev` | Report device-level config status and bounds. | None. | `config_device_info_payload` (see below). |
| `get_cfg` | Fetch the current or staged blob for one bud. | `config_get_request` | `config_get_response` with status and blob header/payload. |
| `set_cfg` | Chunked write into the staging buffer. | `config_set_chunk_header` + `chunk_size` bytes | Status only (0 success or error code in response CRC field). |
| `apply_tmp` | Apply staged blob without committing. | `config_commit_request` | Status only. |
| `commit_cfg` | Atomically commit staged blob to active and bump generation. | `config_commit_request` | Status only. |
| `rollback` | Restore the last committed snapshot. | `config_commit_request` | Status only. |
| `panic_off` | Reset per-bud stores to safe defaults. | None. | Status only. |

`config_protocol_register_uart()` registers these names with the existing
`hal_cmd` dispatcher when `__PC_CMD_UART__` is enabled. BLE/GATT handlers can
reuse the same payloads.

## Payload Structures

```c
struct config_get_request {
  uint8_t bud;          // 0 = left, 1 = right
  uint8_t staged_only;  // 1 to fetch staged instead of active
};

struct config_get_response {
  uint8_t status;                       // 0 success, otherwise error
  struct config_blob_header header;     // schema, budgets, payload length
  uint8_t payload[CONFIG_STORE_MAX_PAYLOAD_BYTES];
};

struct config_set_chunk_header {
  uint8_t bud;            // 0 = left, 1 = right
  uint8_t flags;          // bit0: final chunk
  uint16_t total_size;    // full blob size (header + payload)
  uint16_t chunk_offset;  // byte offset from start of blob
  uint16_t chunk_size;    // bytes that follow this header
};

struct config_commit_request {
  uint8_t bud;            // 0 = left, 1 = right
};
```

## Error Handling

- Non-zero error codes are returned in the command status/CRC field and echoed
  in `config_get_response.status` for fetch requests.
- Validation failures while staging a blob set `last_error` for that bud and
  reject the transfer. Bounds checks cover schema version (non-zero and matching
  the compiled `CONFIG_STORE_SCHEMA_VERSION`), `max_points_per_ear <= 32`,
  `section_budget_bytes <= 2048`, payload length within 64 bytes, and CRC
  mismatches.
- Chunk uploads enforce sequential offsets and total size limits; violating
  these constraints aborts the transfer.
- `panic_off` clears active, staged, and backup slots for both buds so that
  subsequent reads return empty/default headers until new data is staged.

## Device Info Response

The `config_device_info_payload` returned by `get_dev` reports the firmware-side
limits and per-bud status:

- `bud_count`: Always 2 (left/right buds).
- `schema_version`, `max_points`, `section_budget_bytes`: Build-time bounds the
  loader enforces during validation.
- Per bud: `has_active`, `has_staged`, `has_backup`, `temporary_active`,
  `active_generation`, `staged_generation`, and `last_error` (non-zero when the
  last staging attempt failed validation).

## Persistence and Safety Notes

- The config store keeps an active, staged, and backup slot per bud with
  generation counters. `commit` copies staged to active and saves the previous
  active as backup to enable rollback.
- `apply_tmp` swaps staged into the active slot but marks it as temporary; a
  subsequent `commit` clears the temporary flag, while `rollback` restores the
  backup snapshot.
- CRC validation is mandatory for every staged blob. Requests must provide a
  `config_blob_header` followed by payload bytes whose CRC matches
  `payload_crc32`.
