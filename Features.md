# Features

## Audiogram & Fitting
- **Supported formats:** Arbitrary sets of frequency points per ear (minimum two points). Frequencies must be strictly increasing, unique, and within 100–12000 Hz. Threshold values must match the frequency list and are interpreted as dB HL.
- **Conversion to target gain:** Points are interpolated in log-frequency space onto an internal DSP grid, then passed through multi-pass smoothing to avoid sharp transitions before clamping to safety bounds. Thresholds map to target gain using a conservative half-gain rule with configurable caps.
- **Application path:** The interpolated, smoothed curve forms the base compensation layer feeding the IIR/biquad EQ fitter per ear; the fitter honors section budgets per ear and falls back to a smoothed baseline when a requested filter set cannot be realized. The same calibrated base curve is reused across all listening modes (ambient, music, theater, conversation, calls, tinnitus masking) after per-bud calibration, with mode-specific EQ applied as a lightweight overlay instead of refitting per mode.
- **Safety constraints:** Frequency inputs outside 100–12000 Hz or unsorted/duplicate points are rejected. Post-EQ target gains are capped with a per-ear soft-knee limiter (knee at ~15 dB, hard cap at 18 dB) using 5 ms attack/50 ms release and a 20 ms state crossfade when modes change to avoid artifacts. Smoothing mitigates feedback risk and jagged responses.
- **Backward compatibility:** Standard octave-only audiograms remain valid and yield equivalent results after interpolation. Legacy configs continue to load; the schema supports versioning and migration for extended audiogram support.

### Example audiogram configuration (JSON)
```json
{
  "schema": "audiogram/v3",
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

See `documentation/audiogram_schema.md` for the full v3 schema details and
migration steps from v2 octave-only profiles.
