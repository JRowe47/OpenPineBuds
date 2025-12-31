# Firmware Development TODO (Consolidated)

Combined backlog sourced from the previous to-do lists, feature specs (A–K), and milestone tracker (M0–M10). Completed items are preserved. Tasks are ordered to support a reasonable implementation progression for the new firmware.

## M0 — Baseline Build/Flash & Logging
- [x] Verify `build.sh`/`Makefile` outputs the expected firmware image; document exact target and toolchain versions (see build verification notes in `documentation.md`).
- [x] Enable logging for audio pipelines; confirm `PANIC_OFF` command path works over UART/USB and/or BLE console.

## M1 — Config Store, Schemas, & Transport Stubs (Feature A/K)
- [x] Implement per-bud persistent store with CRC, atomic commit, and rollback in the `config/` layer.
- [x] Add schema version fields, migration hooks, and bounds for MAX_POINTS/section budgets in validation helpers.
- [x] Stub transport commands (`GET_DEVICE_INFO`, `GET_CONFIG`, `SET_CONFIG` chunked, `APPLY_TEMP`, `COMMIT`, `ROLLBACK`, `PANIC_OFF`) in UART/BLE command handlers.
- [x] Document request/response formats and error handling for the companion protocol.

## M2 — DSP Core, Limiter, & Ramps (Feature C)
- [x] Build shared DSP chain (calibration EQ → audiogram EQ → mode overlay → compressors/NR → limiter) within the audio pipeline.
- [x] Enforce limiter-last ordering and add telemetry counters (CPU, underflow/overflow, clipping/limiter events).
- [x] Implement gain ramps and headroom offsets for mode/config updates; ensure panic-off bypasses ramps for instant mute.

## M3 — Arbitrary Audiogram Points Applied to A2DP + Call Downlink (Feature B)
- [x] Update audiogram configuration schema to support arbitrary per-ear frequency lists with versioned migration guidance.
- [x] Enforce validation rules: strictly increasing unique frequencies within 100–12000 Hz; threshold counts match; minimum two points per ear.
- [x] Implement log-frequency interpolation onto the internal DSP grid with smoothing to avoid sharp transitions.
- [x] Fit interpolated target gains into EQ sections per ear with safe fallbacks when budgets are exceeded.
- [x] Ensure base audiogram compensation feeds all listening modes (ambient, music, theater, conversation, calls, tinnitus masking) after per-bud calibration.
- [x] Apply safety clamps/limiters after EQ to cap gains and preserve smooth transitions.
- [x] Preserve backward compatibility so octave-only audiograms load unchanged; provide schema versioning and migration path.
- [x] Add regression tests for legacy octave-only audiograms and new non-octave cases (e.g., 750, 1500, 3000 Hz).
- [x] Document example configurations and validation errors for arbitrary frequency lists.
- [x] Keep interpolation/smoothing lightweight (avoid heavy FFT paths) and cap per-ear point counts to meet CPU/latency/memory budgets.
- [x] Verify limiter remains active after EQ when arbitrary audiogram points are used.
- [x] Profile interpolation and fitting stages to confirm performance constraints are met.

## M4 — Mode Framework (Ambient/Music/Theater) (Feature D)
- [x] Add mode overlay presets (ambient/music/theater) in configuration and pipeline switch logic, including ambient mix ratios, wind suppression hook, and ducking option.
- [x] Ensure mode transitions ramp gains, preserve headroom, and respect audiogram and limiter placement.
- [x] Validate ambient mix behavior, music/theater overlays, and wind suppression acceptance criteria.

## M5 — Call Pipeline (Uplink/Downlink) (Feature F)
- [ ] Wire downlink compressor + presence EQ + limiter; apply audiogram only to downlink.
- [ ] Wire uplink HPF + AGC + wind/NR + limiter (no audiogram) within HFP paths.
- [ ] Validate panic-off disables ambient/tinnitus and keeps uplink unclipped; add call-mode tests/logs.

## M6 — Ambient Pipeline Improvements (Features E, G, H)
- [x] Implement wind detector with hysteresis (low-band energy/coherence) and adjust HPF/gain/NR thresholds.
- [x] Add speech-focused EQ and compression for conversational mode with minimal latency; tune outdoors mode for wind suppression and conservative HF gain caps.
- [x] Ensure wind suppression and NR avoid pumping; verify mode transitions are ramped and safe.
- [x] Improve NR/voice isolation tiers and optional cross-bud feature sharing hooks.

## M7 — Tinnitus Masker (Feature I)
- [ ] Add masker generator (white/pink/band/notched) per bud with level caps, modulation, and fade-in/out; integrate default-off call policy and panic-off mute.
- [ ] Route masker through limiter after mix; add safety validation for masker levels in config loader.

## M8 — Calibration Routines (Feature J)
- [ ] Implement minimal calibration commands/UI (L/R trim, comfort/ambient ceilings); optional in-ear seal score and FR compensation sweep when hardware allows.
- [ ] Add mic calibration routines for noise floor estimation and wind trigger validation; store per-bud results with CRC and versioning.

## M9 — Cross-Bud Coordination (Feature H/D/E)
- [ ] Define feature-sharing messages for VAD state/noise estimates/gain states without streaming raw audio.
- [ ] Ensure stereo consistency for ducking/NR decisions; measure battery impact.

## M10 — Optional Binaural Beamforming (Stretch) (Feature H)
- [ ] Prototype binaural beamforming behind compile/runtime flag; document latency/battery budget and keep disabled by default.

## Companion Protocol & Tuning Workflow (Feature K)
- [ ] Provide example host script to read/write configs, trigger measurements (tone/sweep/noise), and test rollback/panic-off end-to-end.
- [ ] Publish reference profiles, including arbitrary-point audiograms and tinnitus presets.

## Validation, Tooling, & Documentation
- [ ] Add config validation tests (valid/invalid arbitrary audiogram cases, CRC failures, rollback) under the existing test harness.
- [ ] Provide validation/migration tooling for schema updates, including version checks and schema tests for valid/invalid cases (duplicates, unsorted points, out-of-range frequencies, length mismatches).
- [ ] Document telemetry/log commands for counters (CPU, clipping, limiter use, wind state) and acceptance criteria coverage.
- [ ] Keep feature documentation updated with safety constraints, compatibility expectations, and JSON examples.
