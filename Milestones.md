# Milestones

Sequenced plan with measurable acceptance criteria. Limiter last, calibration-first architecture and arbitrary audiogram points apply across all milestones.

## M0 — Baseline build/flash + logging
- **Scope:** Establish reproducible build/flash steps, enable runtime logging/telemetry plumbing.
- **Acceptance:** Build succeeds on target; device boots with logging for audio pipelines; panic-off command reachable.

## M1 — Config store + schema + protocol stubs
- **Scope:** Per-bud persistent store with versioning, CRC, atomic writes, rollback; command handlers for GET/SET/APPLY_TEMP/COMMIT/ROLLBACK/PANIC_OFF; schema version fields and migration hooks.
- **Acceptance:** Corrupted writes roll back; version mismatch prompts migration path; invalid configs rejected with errors; temporary apply does not persist after reboot.

## M2 — DSP core + limiter + ramps
- **Scope:** Shared DSP chain scaffolding with calibration EQ + audiogram EQ slots + mode overlay slots; compressors where needed; limiter last; gain ramps and headroom offsets; telemetry counters.
- **Acceptance:** Ordering enforced in code; limiter engagement counter works; ramps prevent pops on forced mode flips; CPU/overflow counters emit telemetry.

## M3 — Audiogram (arbitrary points) applied to A2DP + call downlink
- **Scope:** Validation for sorted unique 100–12000 Hz points (<=MAX_POINTS); log-frequency interpolation + smoothing; half_gain_v0 mapping; per-ear EQ fitting; base audiogram shared across modes; per-mode gain caps.
- **Acceptance:** Standard octave and mixed-point profiles (incl. 750/1500/3000 Hz) load; invalid profiles rejected safely; A2DP and call downlink audio use the fitted EQ with limiter last; regression keeps octave profiles unchanged within tolerance.

## M4 — Mode framework (Ambient/Music/Theater)
- **Scope:** Mode overlay system with ambient mix ratios, wind suppression hook, ducking options; music/theater overlays; ramped switching; documentation of policies.
- **Acceptance:** Mode changes apply overlays without clipping; ambient mix obeys ratios; theater disables ambient; limiter remains last.

## M5 — Call pipeline (uplink/downlink)
- **Scope:** Downlink speech compressor + presence EQ + limiter; uplink HPF + AGC + wind/NR + limiter (no audiogram); panic-off behavior validated.
- **Acceptance:** Downlink loudness consistent; uplink protected from clipping and wind bursts; panic-off mutes ambient/tinnitus instantly.

## M6 — Ambient pipeline improvements (wind/NR/conversation/outdoors)
- **Scope:** Wind detector with hysteresis; NR tiers per mode; conversational speech emphasis (1–4 kHz) with low latency; outdoors conservative HF gains; optional ambient music mix.
- **Acceptance:** Wind triggers adjust HPF/gain without pumping; NR aggressiveness matches mode; conversational latency kept minimal; limiter last.

## M7 — Tinnitus masker
- **Scope:** Shaped noise generator (white/pink/band/notched) per bud with level caps, modulation, fade-in/out; mode policy; panic-off integration.
- **Acceptance:** Masker respects caps and fades; disabled in calls by default; panic-off disables masker immediately; limiter still last.

## M8 — Calibration routines
- **Scope:** Minimal calibration (L/R trim, comfort ceiling, ambient gain ceiling); optional in-ear seal score and FR comp sweep; mic calibration (noise floor, NR/AGC seeds, wind test).
- **Acceptance:** Calibration results stored per bud and applied; seal score affects HF gain caps; FR comp filters generated when hardware supports; mic calibration seeds thresholds.

## M9 — Cross-bud coordination
- **Scope:** Feature-sharing (VAD state, noise estimates, gain states) between buds; stereo consistency; battery-aware coordination.
- **Acceptance:** Shared states stay in sync without raw audio streaming; coordinated ducking works; battery impact measured within budget.

## M10 — Optional binaural beamforming (stretch)
- **Scope:** Experimental binaural beamforming behind compile/runtime flag after feasibility proof.
- **Acceptance:** Feature flag defaults off; enabling demonstrates measurable directionality improvement without breaking latency/battery budgets.
