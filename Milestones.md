# Milestones

## Arbitrary Audiogram Points Support
- **Implementation tasks:**
  - Extend the audiogram configuration schema (version bump) to allow arbitrary per-ear frequency lists with validation and migration guidance.
  - Add firmware validation: strictly increasing, unique frequencies in the 100–12000 Hz range with matching dB HL thresholds and per-ear point count >= 2.
  - Implement log-frequency interpolation and smoothing to map audiogram points onto the internal DSP grid, then fit target gains into available EQ/biquad sections per ear.
  - Ensure the base audiogram compensation feeds all listening modes (ambient, music, theater, conversation, calls, tinnitus masking) and coexists with per-bud calibration and limiters.
  - Preserve backward compatibility so standard octave-only audiograms still load and sound the same within tolerance.
- **Acceptance criteria:**
  - Valid configs with non-octave points (e.g., 750, 1500, 3000 Hz) load and render without artifacts; invalid configs (duplicates, unsorted, out-of-range, length mismatches) are rejected safely.
  - Interpolated gain curves are smooth and capped to safety limits; limiter remains active after EQ.
  - Regression tests confirm octave-only audiograms are unchanged relative to prior behavior.
- **Performance constraints:**
  - Interpolation and smoothing must avoid heavy FFT paths and keep per-ear point counts bounded (e.g., max 32) so CPU, latency, and memory stay within existing audio pipeline budgets.
