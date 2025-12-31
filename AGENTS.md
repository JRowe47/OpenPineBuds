# Project Agent Notes

This repository does not currently include nested agent instructions. Follow these guardrails when updating documentation and code.
- Before starting work, review `ToDoList.md` and check items off as you complete them; add subtasks when new work is discovered.

## Architecture Layers
1) **Calibration layer (per bud):** hardware trims, FR compensation filters, seal/fit metadata, mic calibration; must run first in every pipeline.
2) **Audiogram layer (per user, per ear):** arbitrary frequency-point audiograms in dB HL, validated and interpolated (log-frequency) with smoothing before fitting into EQ; shared across all modes.
3) **Mode overlay layer:** per-application overlays (music/ambient/theater/conversation/outdoors/call/tinnitus) composed on top of calibration + audiogram with ramped transitions and headroom management.
4) **Limiter layer:** final post-chain soft-knee limiter (knee ~15 dB, cap 18 dB, 5 ms attack/50 ms release, ~20 ms crossfade) that cannot be bypassed; panic-off must also mute/disable masker and ambient gain immediately.

## Safety Requirements
- Limiter is the last stage in every output chain (A2DP, HFP downlink, ambient/talk-through, tinnitus mix) and always enabled.
- Enforce gain caps and internal headroom when stacking calibration + audiogram + overlays; apply ramping for mode/config changes.
- Validation rejects unsafe configs (frequency ranges, gains, masker levels); rollback on failed updates; panic-off path is non-blocking.
- Wind/NR/AGC/ANC hooks must preserve intelligibility and avoid pumping; tinnitus masker respects hard level caps and fade-in/out.

## Feature Checklist (A–K)
- [ ] A Config System: per-bud persistent store with versioned schemas, atomic commits/rollback, CRC/validation, safety caps.
- [ ] A Config Objects: calibration/audiogram/tinnitus/mode overlay JSON with hard caps and migration notes.
- [ ] A Transport/API: BLE GATT and/or UART/USB commands for GET/SET/APPLY_TEMP/COMMIT/ROLLBACK/PANIC_OFF.
- [ ] B Arbitrary Audiogram Points: sorted unique 100–12000 Hz, thresholds in dB HL, log-domain interpolation + smoothing, half_gain_v0 mapping, per-mode gain caps, per-ear independence.
- [ ] C DSP Core: shared EQ/compressor/noise generator/limiter chain with headroom and gain ramps; telemetry counters for CPU/overflow/clipping; limiter last.
- [ ] D Media Streaming Modes (A2DP): Ambient mix with wind suppression and ducking; Music mode; Theater mode; all with audiogram base layer.
- [ ] E Hearing Assist Modes: Conversational low-latency speech-focused processing; Outdoors with wind-safe tuning; optional ambient music mix.
- [ ] F Phone Call Quality (HFP): Downlink speech processing with audiogram; uplink HPF+AGC+wind/NR+limiter (no audiogram) with low artifacts.
- [ ] G Wind Suppression: detector + hysteresis driving HPF/gain/NR adjustments without pumping.
- [ ] H Noise Reduction / Voice Isolation: basic NR; cross-bud feature sharing; optional binaural beamforming flag.
- [ ] I Tinnitus Masking: shaped noise generator per bud with modulation, safety caps, fade, mode policy, panic-off aware.
- [ ] J Calibration: minimal trims and ceilings; in-ear seal score + FR comp + feedback risk; mic calibration routines.
- [ ] K Companion Protocol: documented commands, JSON/CBOR schemas, and reference profiles; buds generate test signals.

## Milestones (must match `Milestones.md`)
- **M0** Baseline build/flash + logging hooks.
- **M1** Config store/schema/protocol stubs (per-bud, versioned, safe updates).
- **M2** DSP core blocks with limiter/ramping/headroom + instrumentation.
- **M3** Arbitrary audiogram ingestion/interpolation/fitting applied to A2DP + call downlink.
- **M4** Mode framework (Ambient/Music/Theater overlays).
- **M5** Call pipeline (HFP downlink/uplink safety + processing).
- **M6** Ambient pipeline improvements (wind/NR/conversation/outdoors tuning).
- **M7** Tinnitus masker generator + safety policy.
- **M8** Calibration routines (minimal then in-ear/mic).
- **M9** Cross-bud coordination and feature sharing.
- **M10** Optional binaural beamforming stretch goal.

## Reference Audiogram (Example)
Use anonymized sample data only. Audiogram thresholds are **dB HL** (not SPL). Frequencies must be strictly increasing and within 100–12000 Hz.

- **Standard octave points (2020-10-14 sample):**
  - Right ear dB HL: 125:10, 250:10, 500:15, 1000:20, 2000:35, 4000:50, 6000:60, 8000:70.
  - Left ear dB HL: 125:5, 250:5, 500:10, 1000:15, 2000:25, 4000:40, 6000:50, 8000:60.
- **Extended points example:** include additional 750, 1500, 3000 Hz entries per ear (interpolated or measured) to illustrate arbitrary point support.

## Notes on Arbitrary Audiogram Points
- Explicitly support arbitrary per-ear point sets with MAX_POINTS (e.g., 32) defined; enforce sorted/unique constraints and matching thresholds.
- Interpolate in log-frequency space with smoothing before EQ fitting; document validation errors and migration guidance.
