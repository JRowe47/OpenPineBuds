# Features

## Audiogram & Fitting
- **Supported formats:** Arbitrary sets of frequency points per ear (minimum two points). Frequencies must be strictly increasing, unique, and within 100–12000 Hz. Threshold values must match the frequency list and are interpreted as dB HL.
- **Conversion to target gain:** Points are interpolated in log-frequency space onto an internal DSP grid, smoothed to avoid sharp transitions, and clamped to safety bounds. Thresholds map to target gain using a conservative half-gain rule with configurable caps.
- **Application path:** The interpolated, smoothed curve forms the base compensation layer feeding the IIR/biquad EQ fitter per ear. The same base curve modulates all listening modes (ambient, music, theater, conversation, calls, tinnitus masking) after per-bud calibration, with mode-specific EQ applied as an overlay.
- **Safety constraints:** Frequency inputs outside 100–12000 Hz or unsorted/duplicate points are rejected. Target gains are capped to safe values with a limiter after EQ, and transitions ramp to avoid sudden steps. Smoothing mitigates feedback risk and jagged responses.
- **Backward compatibility:** Standard octave-only audiograms remain valid and yield equivalent results after interpolation. Legacy configs continue to load; the schema supports versioning and migration for extended audiogram support.

### Example audiogram configuration (JSON)
```json
{
  "schema": "audiogram/v2",
  "left": {
    "frequencies_hz": [125, 250, 500, 750, 1000, 1500, 2000, 3000, 4000, 6000, 8000],
    "thresholds_db_hl": [5, 10, 15, 20, 25, 25, 30, 35, 40, 45, 50]
  },
  "right": {
    "frequencies_hz": [125, 250, 500, 750, 1000, 1500, 2000, 3000, 4000, 6000, 8000],
    "thresholds_db_hl": [0, 5, 10, 15, 20, 20, 25, 30, 35, 40, 45]
  }
}
```
