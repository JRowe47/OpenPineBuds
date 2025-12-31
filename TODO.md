# TODO

Actionable tasks mapped to features (A–K) and milestones (M0–M10). Check items as they land; add file/function pointers as implementations materialize.

## M0 — Baseline build/flash + logging
- [ ] (M0) Verify `build.sh`/`Makefile` paths produce firmware image; document exact target + toolchain versions.
- [ ] (M0) Enable logging for audio pipelines; ensure panic-off command path is reachable via UART/USB and/or BLE console.

## M1 — Config store + schema + protocol stubs
- [ ] (M1, A1/A3/K1) Implement per-bud persistent store with CRC and atomic commit in `config/` layer; add rollback-on-failure path.
- [ ] (M1, A1) Add schema version fields and migration hooks; bound MAX_POINTS (e.g., 32) and section budgets in validation helpers.
- [ ] (M1, A3/K1) Stub transport commands (`GET_DEVICE_INFO`, `GET_CONFIG`, `SET_CONFIG` chunked, `APPLY_TEMP`, `COMMIT`, `ROLLBACK`, `PANIC_OFF`) in UART/BLE command handlers (see `apps/` command server stubs).

## M2 — DSP core + limiter + ramps
- [ ] (M2, C1/C2) Build shared DSP chain module (calibration EQ -> audiogram EQ -> mode overlay -> compressors/NR -> limiter) under `apps/` or `platform/` audio pipeline; enforce limiter last.
- [ ] (M2, C2) Add telemetry counters for CPU, underflow/overflow, clipping/limiter events; expose via logging/command.
- [ ] (M2, C1) Implement gain ramps/headroom offsets for mode/config updates to avoid pops; ensure panic-off bypasses ramps for instant mute.

## M3 — Audiogram arbitrary points applied to A2DP + call downlink
- [ ] (M3, B1/B2/B3) Add validation for sorted unique 100–12000 Hz arrays (<=MAX_POINTS) and matching thresholds in audiogram loader (`config` or `apps` fitting code).
- [ ] (M3, B2) Implement log-frequency interpolation + smoothing + slope limiting; map thresholds to gain using `half_gain_v0` with per-mode caps.
- [ ] (M3, B3) Fit interpolated gains to EQ sections per ear; keep octave-only regression tests; apply base audiogram in A2DP and HFP downlink chains with limiter last.

## M4 — Mode framework (Ambient/Music/Theater)
- [ ] (M4, D1/D2/D3) Add mode overlay presets (ambient/music/theater) in `config/modes.json` and pipeline switch logic; include ambient mix ratios, wind suppression hook, ducking option.
- [ ] (M4, C1/D1) Ensure mode transitions ramp gains and preserve headroom; document policy mapping in code comments/docs.

## M5 — Call pipeline (uplink/downlink)
- [ ] (M5, F1/F2) Wire downlink compressor + presence EQ + limiter; uplink HPF + AGC + wind/NR + limiter (no audiogram) in HFP paths (see `apps/hfp` or similar).
- [ ] (M5, F2) Validate panic-off disables ambient/tinnitus and leaves uplink unclipped; add call-mode tests/logs.

## M6 — Ambient pipeline improvements
- [ ] (M6, G1/H1/E1/E2) Implement wind detector with hysteresis (low-band energy/coherence) and adjust HPF/gain/NR thresholds; tune conversational vs outdoors NR.
- [ ] (M6, E1/E2) Add speech-focused EQ and compression for conversational mode with latency budget; outdoors mode enforces conservative HF gain caps and optional ambient music mix.

## M7 — Tinnitus masker
- [ ] (M7, I1/I2) Add masker generator (white/pink/band/notched) per bud with level caps, modulation, fade-in/out; integrate mode policy (default off for calls) and panic-off mute.
- [ ] (M7, I2) Route masker through limiter after mix; add safety validation for masker levels in config loader.

## M8 — Calibration routines
- [ ] (M8, J1/J2/J3) Implement minimal calibration (L/R trim, comfort/ambient ceilings) UI/commands; optional in-ear seal score + FR comp sweep if hardware allows (see `platform` mic APIs).
- [ ] (M8, J3) Add mic calibration routines for noise floor estimation and wind trigger validation; store per-bud results with CRC.

## M9 — Cross-bud coordination
- [ ] (M9, H2) Define feature-sharing messages for VAD state/noise estimates/gain states; avoid raw audio streaming; integrate into existing link if available.
- [ ] (M9, D1/E1) Ensure stereo consistency for ducking/NR decisions; measure battery impact.

## M10 — Optional binaural beamforming
- [ ] (M10, H3) Prototype binaural beamforming behind compile/runtime flag; document latency/battery budget and disable by default.

## Validation & Tooling
- [ ] (All) Add config validation tests (valid/invalid arbitrary audiogram cases, CRC failures, rollback) under existing test harness.
- [ ] (All) Provide example JSON/CBOR configs (including arbitrary-point audiograms and tinnitus presets) alongside protocol docs.
- [ ] (All) Document telemetry/log commands for counters (CPU, clipping, limiter use, wind state).

## Known Unknowns / Risks
- Availability of internal/feedback mics for in-ear calibration and beamforming.
- Existing ANC hooks or DSP acceleration on target SOC.
- Companion transport preference (BLE vs UART/USB) and MTU limits for chunked config updates.
- Precise DSP section budgets (number of biquads/peaks per ear) and CPU headroom on the chosen MCU.
