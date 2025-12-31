# Audiogram Safety Clamps and Limiter Path

This note documents how safety clamps are applied after audiogram-based EQ
fitting so that arbitrary frequency lists remain safe while sounding smooth.

## Post-EQ Safety Layer

1. **Limiter placement:** A stereo limiter is inserted directly after the
   per-ear EQ fitter and any mode overlays. This guarantees that every
   listening mode (ambient, music, theater, conversation, calls, tinnitus
   masking) inherits the same safety envelope.
2. **Soft knee with dual thresholds:**
   - `safe_gain_db` (default: 18 dB) sets an absolute cap for any reconstructed
     band once interpolation and fitting are applied.
   - `knee_start_db` (default: 15 dB) begins a 3 dB soft-knee region to avoid
     sudden compression.
3. **Timing constants:** Attack of 5 ms and release of 50 ms are used to avoid
   pumping while still reacting quickly to problem boosts; knee smoothing and
   release tails are applied independently per ear.
4. **Transition smoothing:** When mode overlays change, limiter state is
   crossfaded over 20 ms to prevent clicks while maintaining caps.
5. **Input validation feedback:** If interpolation would exceed
   `safe_gain_db + 3 dB`, the fitter pre-attenuates the target for that ear and
   records a warning so configuration tooling can surface the clamp event.

## Validation Expectations

- Frequencies must still follow the 100–12000 Hz, strictly increasing, unique
  rules from the schema. Threshold arrays must match the per-ear length.
- Octave-only profiles continue to pass untouched; limiter state is shared
  across schema versions and engages only when post-EQ gain would exceed the
  soft-knee region.
- Regression tests should confirm that limiter engagement does not alter
  octave-aligned references beyond tolerance while capping out-of-band gain in
  arbitrary-point cases.
