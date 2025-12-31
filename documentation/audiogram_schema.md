# Audiogram Configuration Schema (v3)

Schema v3 formalizes support for arbitrary per-ear frequency lists while
remaining backward compatible with v2 octave-aligned data. The schema string
maps to the numeric `schema_version` stored in `AudiogramProfile` (for example,
`"audiogram/v3"` sets `schema_version` to `3`).

## Field Requirements

- **Top-level keys**: `schema` (string), `left`, `right`, optional
  `max_points_per_ear` (integer hint; capped at 32 by the firmware).
- **Per ear (`left` / `right`)**:
  - `frequencies_hz`: Array of unique, strictly increasing frequency points in
    Hz. Accepts any length between 2 and 32 inclusive per ear; lists do not have
    to match between ears.
  - `thresholds_db_hl`: Array of dB HL thresholds matching the length of
    `frequencies_hz` for that ear.
- **Safety bounds**: Frequencies must be within 100–12000 Hz. Thresholds are
  clamped in firmware to safe gain values using the half-gain rule.

## v3 Example (asymmetric frequency lists)

```json
{
  "schema": "audiogram/v3",
  "max_points_per_ear": 32,
  "left": {
    "frequencies_hz": [125, 250, 500, 750, 1000, 1500, 2000, 3000, 4500, 6000, 8000, 12000],
    "thresholds_db_hl": [5, 10, 15, 20, 25, 25, 30, 35, 42, 45, 50, 55]
  },
  "right": {
    "frequencies_hz": [250, 500, 1000, 2000, 3000, 4000, 6000, 9000],
    "thresholds_db_hl": [0, 5, 12, 18, 22, 30, 38, 45]
  }
}
```

## Migration from v2 to v3

1. Update the `schema` string to `"audiogram/v3"` (numeric schema version `3`).
2. Keep existing octave-spaced points as-is; they remain valid under v3.
3. If using distinct left/right measurements, set `frequencies_hz` and
   `thresholds_db_hl` independently per ear. Ensure lengths match per ear and
   stay within the 2–32 point limit.
4. Optional: retain or add `max_points_per_ear` to document upstream limits;
   firmware enforces the 32-point maximum regardless of this hint.
5. Validate that all frequencies remain within 100–12000 Hz and are strictly
   increasing without duplicates.

## Backward compatibility (v2 octave-only profiles)

- v2 profiles that omit a `schema` string default to `audiogram/v2` and keep
  their octave-spaced frequency lists. They load unchanged under the v3 parser
  as long as the lists remain sorted, unique, and within 100–12000 Hz.
- A v2 example (still valid under v3):

  ```json
  {
    "schema": "audiogram/v2",
    "left": {
      "frequencies_hz": [125, 250, 500, 1000, 2000, 4000, 8000],
      "thresholds_db_hl": [0, 5, 10, 15, 20, 25, 30]
    },
    "right": {
      "frequencies_hz": [125, 250, 500, 1000, 2000, 4000, 8000],
      "thresholds_db_hl": [2, 4, 8, 12, 18, 22, 28]
    }
  }
  ```

- To migrate the above to v3, change the `schema` string to
  `"audiogram/v3"` and keep the octave points; the interpolation pipeline
  yields the same response within tolerance while unlocking the option to add
  non-octave points later.

## Application Across Listening Modes

The firmware computes a single calibrated base compensation curve per ear from
the validated audiogram and interpolation pipeline. This base curve is shared
by every listening mode to avoid refitting per mode:

- The base curve is calibrated per bud and cached as the starting point for
  ambient, music, theater, conversation, calls, and tinnitus-masking modes.
- Mode-specific EQ presets apply as overlays after the base curve, preserving
  the same underlying compensation everywhere.
- Final soft-knee limiters (knee ~15 dB, cap 18 dB, 5 ms attack/50 ms release)
  remain engaged after the overlays to maintain safe gain caps and are
  crossfaded over 20 ms between modes to avoid artifacts. See
  `documentation/audiogram_safety.md` for detailed safety-layer behavior.
