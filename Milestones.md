# Milestones

## Arbitrary Audiogram Points Support
- **Implementation tasks:**
  - Extend the audiogram configuration schema (version bump) to allow arbitrary per-ear frequency lists with validation and migration guidance.
  - Add firmware validation: strictly increasing, unique frequencies in the 100–12000 Hz range with matching dB HL thresholds and per-ear point count >= 2.
  - Implemented log-frequency interpolation and multi-pass smoothing to map audiogram points onto the internal DSP grid; fit target gains into available EQ/biquad sections per ear with section-budget awareness and safe fallbacks.
  - Ensure the base audiogram compensation feeds all listening modes (ambient, music, theater, conversation, calls, tinnitus masking) and coexists with per-bud calibration and limiters, with mode-specific EQ applied as an overlay rather than refitting per mode.
  - Added a post-EQ per-ear soft-knee limiter (knee ~15 dB, cap 18 dB, 5 ms attack/50 ms release, 20 ms mode crossfade) to clamp gains while keeping transitions smooth.
  - Preserve backward compatibility so standard octave-only audiograms still load and sound the same within tolerance; document schema versioning and migration from v2 octave profiles to v3 arbitrary-frequency profiles.
- **Acceptance criteria:**
  - Valid configs with non-octave points (e.g., 750, 1500, 3000 Hz) load and render without artifacts; invalid configs (duplicates, unsorted, out-of-range, length mismatches) are rejected safely.
  - Legacy octave-only profiles continue to parse under `audiogram/v2` (or default) schema strings and migrate cleanly to v3 when requested without altering output beyond tolerance.
  - Interpolated gain curves are smooth and capped to safety limits; limiter remains active after EQ.
  - Regression tests confirm octave-only audiograms are unchanged relative to prior behavior.
- **Performance constraints:**
  - Interpolation and smoothing must avoid heavy FFT paths and keep per-ear point counts bounded (e.g., max 32) so CPU, latency, and memory stay within existing audio pipeline budgets.
