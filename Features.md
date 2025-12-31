# Features

This document specifies the audio system features, interfaces, constraints, and acceptance criteria for the mode-based buds. Audiogram data is interpreted as **dB HL** and sits on top of per-bud calibration with a final limiter for safety.

## Audiogram Data Is dB HL, Not SPL
- Audiogram thresholds are user hearing levels (dB HL) and not absolute SPL. Calibration establishes the device reference and cannot be bypassed by audiogram inputs.
- Frequency points must be strictly increasing, unique, and constrained to 100–12000 Hz (MAX_POINTS e.g., 32). Threshold counts must match frequency counts per ear.
- Validation clamps unsafe inputs and rejects malformed configs; migration paths preserve backward compatibility for octave-only inputs.

## Feature A — Config System (per-bud, versioned, safe)
### What it does
Provides per-bud persistent configuration with schema versioning, atomic updates, rollback, and validation to enforce safety limits.
### Inputs/Outputs
- Inputs: config blobs (e.g., JSON/CBOR) delivered via transport (BLE GATT and/or UART/USB command channel), per-bud storage.
- Outputs: validated runtime structures for calibration/audiogram/tinnitus/modes, device info responses, status/error codes.
### Config parameters
- Schema/version fields with migration notes.
- Per-bud namespaces (`L`, `R`) stored independently with CRCs/checksums and monotonic version counters.
- Update commands: `GET_DEVICE_INFO`, `GET_CONFIG`, `SET_CONFIG` (chunked), `APPLY_TEMP`, `COMMIT`, `ROLLBACK`, `PANIC_OFF`.
- Safety caps: immutable firmware limits for gains, masker levels, and EQ budgets.
### Constraints
- Atomic writes with rollback on power loss; CRC validated on load/write.
- Storage-size bounded; reject configs exceeding MAX_POINTS or section budgets.
- Latency target: config apply should not stall audio pipelines; use double-buffered apply/commit.
### Acceptance criteria & tests
- Corrupted or partial writes roll back to last good config; CRC failures force safe defaults.
- Version mismatch triggers migration or rejection with explicit error.
- Transport commands verified with positive/negative tests (bad CRC, truncated chunk, invalid schema).

## Feature B — Arbitrary Audiogram Points (per ear)
### What it does
Supports arbitrary per-ear frequency sets while keeping the base audiogram layer reusable across all modes.
### Inputs/Outputs
- Inputs: per-ear frequency arrays (>=2, <=MAX_POINTS) and matching threshold arrays (dB HL).
- Outputs: smoothed target gain curves per ear on DSP grid, fitted EQ sections per ear, validation errors for invalid inputs.
### Config parameters
- Frequency range 100–12000 Hz, strictly increasing and unique.
- Thresholds in dB HL; optional per-band/user trims in 1 dB steps; fitting method `half_gain_v0` (documented mapping).
- Mode gain caps: separate caps for hearing-assist vs music; limiter cap 18 dB.
### Constraints
- Interpolation in log-frequency domain followed by smoothing/slope limiting to avoid EQ spikes.
- Per-ear independence; supports legacy octave lists unchanged.
- Avoid heavy FFT; keep CPU budget low and memory bounded.
### Acceptance criteria & tests
- Valid mixed-point configs (e.g., include 750/1500/3000 Hz) load and render smooth curves without artifacts.
- Invalid inputs (unsorted, duplicate, out-of-range, mismatched lengths) are rejected with safe fallback.
- Regression: octave-only profiles produce previous-equivalent responses.

## Feature C — DSP Core (shared blocks + safety)
### What it does
Provides shared DSP chain blocks usable by A2DP, HFP downlink, and ambient pipelines with unified safety behavior.
### Inputs/Outputs
- Inputs: decoded PCM streams (A2DP), mic streams (ambient/uplink), downlink voice (HFP), tinnitus noise requests.
- Outputs: processed PCM to DAC; telemetry counters; limiter state; panic-off mute.
### Config parameters
- EQ sections for calibration, audiogram, and mode overlays (IIR-first, parametric peak/shelving).
- Compressors (per mode), limiter parameters (soft knee ~15 dB, cap 18 dB, 5 ms attack/50 ms release).
- Gain ramps for mode transitions and config updates; headroom management offsets.
### Constraints
- Low latency for talk-through; headroom maintained when stacking EQs; limiter must be last.
- Telemetry counters for CPU usage, underflow/overflow, clipping/limiter engagement; optional battery impact estimates.
### Acceptance criteria & tests
- Chain ordering enforced: calibration EQ -> audiogram EQ -> mode overlay -> compressors/NR -> limiter.
- Ramping verified to prevent pops on rapid mode changes; limiter engagement counters increment on saturation.
- CPU budget measurements recorded per mode.

## Feature D — Media Streaming Modes (A2DP applications)
### What it does
Defines listening modes for media playback with optional ambient mix and audiogram compensation.
### Inputs/Outputs
- Inputs: A2DP decoded audio; ambient mic for mix/wind detection; optional VAD for ducking.
- Outputs: Mixed/processed PCM to DAC with limiter.
### Config parameters
- Ambient mode: mix ratio for ambient, wind suppression enable, speech-aware ducking depth and time constants.
- Music mode: minimal ambient, music EQ overlay, optional “music variant” fitting curve.
- Theater mode: ambient mix disabled, optional ANC hook flag, limiter only dynamic control.
### Constraints
- Audiogram layer always active post-calibration; headroom preserved.
- Ducking and wind suppression must avoid pumping; latency acceptable for media (> talk-through).
### Acceptance criteria & tests
- Mode switching preserves ramped transitions; limiter last.
- Ambient mix behaves per configuration; music/theater respect overlays and gain caps.
- Wind suppression verified to reduce LF noise without harming tonal balance.

## Feature E — Hearing Assist / Ambient Dominant Modes
### What it does
Supports ambient-first modes optimized for intelligibility and safety.
### Inputs/Outputs
- Inputs: Ambient mic streams (mono/stereo), optional wind detector, NR modules.
- Outputs: Low-latency passthrough audio with processing and limiter.
### Config parameters
- Conversational: speech-focused EQ (1–4 kHz), compression ratios, NR aggressiveness, minimal latency target.
- Outdoors: strong wind suppression, conservative HF gain caps, optional ambient music mix.
- Shared: ramp times, gain caps, ducking policies.
### Constraints
- Latency minimized; avoid heavy processing paths; headroom maintained.
- Wind suppression avoids audible pumping via hysteresis; NR tuned per mode.
### Acceptance criteria & tests
- Conversational mode improves intelligibility metrics (subjective/objective) while maintaining low latency.
- Outdoors mode attenuates wind bursts and caps HF gain; limiter still last.
- Mode transitions are ramped and safe.

## Feature F — Phone Call Quality (HFP applications)
### What it does
Provides downlink and uplink processing pipelines tailored for voice calls.
### Inputs/Outputs
- Inputs: Downlink voice stream; uplink mic audio.
- Outputs: Downlink processed audio with audiogram; uplink processed audio without audiogram.
### Config parameters
- Downlink: speech-focused compressor settings, presence shaping EQ overlay, limiter parameters.
- Uplink: HPF cutoff, AGC thresholds, wind suppression/NR toggles, limiter parameters, ENC enable flag.
### Constraints
- Audiogram applies to downlink only; uplink must avoid coloration from audiogram.
- Preserve intelligibility; avoid “underwater” artifacts; limiter last in both chains.
### Acceptance criteria & tests
- Downlink maintains loudness consistency across voices with limiter protection.
- Uplink avoids clipping and manages wind/noise; verifies panic-off mutes ambient/tinnitus immediately.

## Feature G — Wind Suppression (shared)
### What it does
Detects wind and adapts filters/gains to reduce wind noise.
### Inputs/Outputs
- Inputs: Mic signals (single or dual-mic coherence), low-band energy estimators.
- Outputs: Wind state flag, adjusted HPF cutoff, ambient gain scaling, NR threshold adjustments.
### Config parameters
- Detector thresholds (low-band energy/coherence), hysteresis times, HPF frequency limits, gain reduction amounts.
### Constraints
- Lightweight computation; avoid perceptible pumping; coordinate with NR and compressors.
### Acceptance criteria & tests
- Detector triggers on wind scenarios and releases smoothly; adjustments reduce wind noise without harming speech more than defined limits.

## Feature H — Noise Reduction / Voice Isolation
### What it does
Provides tunable NR tiers with optional cross-bud coordination and stretch goal beamforming.
### Inputs/Outputs
- Inputs: Mic signals, optional VAD/shared state between buds.
- Outputs: Noise-suppressed audio and coordination metadata (VAD state, noise estimates).
### Config parameters
- NR aggressiveness per mode (conversation > outdoors > ambient mix), VAD thresholds, feature-sharing toggles, beamforming enable flag.
### Constraints
- Embedded-friendly (M4F-class) compute budget; cross-bud sharing passes features not raw audio; battery-first design.
### Acceptance criteria & tests
- NR improves SNR within acceptable artifact bounds per mode; cross-bud coordination maintains stereo consistency; beamforming gated behind feature flag with feasibility proof.

## Feature I — Tinnitus Masking
### What it does
Generates shaped noise per bud with strict safety controls and mode policies.
### Inputs/Outputs
- Inputs: Masker configuration per bud (type, center/bandwidth, level, modulation), enable/disable commands.
- Outputs: Noise stream mixed with program audio through limiter; panic-off immediately mutes masker and ambient gain.
### Config parameters
- Noise types: white/pink/band-limited/notched; center frequency/bandwidth; level caps (dbFS); modulation depth/rate; fade-in/out durations; enabled policies per mode.
### Constraints
- Safety caps cannot be overridden; limiter still last; fade behavior avoids clicks; default OFF.
### Acceptance criteria & tests
- Masker respects level caps and fades; disabled in call modes unless explicitly allowed; panic-off stops masker instantly.

## Feature J — Calibration (hardware-appropriate)
### What it does
Captures per-bud calibration data to normalize hardware variance and fit quality.
### Inputs/Outputs
- Inputs: Calibration routines (balance trim, sweeps, seal probes), mic measurements.
- Outputs: Calibration EQ/trim parameters stored per bud; seal/fit metadata; feedback-risk metrics.
### Config parameters
- L/R balance trim (around 1 kHz), comfort ceiling reference, ambient gain ceiling.
- Optional in-ear sweep parameters for FR compensation biquads; seal score thresholds; feedback-risk caps on HF gain.
- Mic calibration: noise floor estimation, NR/AGC seed thresholds, wind test routine.
### Constraints
- Minimal calibration works without lab gear; optional advanced routines if internal mics available.
- Calibration results stored per bud with versioning and validation.
### Acceptance criteria & tests
- Balance trim and ceiling stored and applied in chains; in-ear sweep (if available) yields FR comp filters; seal score recorded and influences gain caps; mic calibration seeds NR/AGC thresholds.

## Feature K — Companion App / Tuning Workflow (protocol hooks)
### What it does
Defines a lightweight companion protocol instead of a full app, enabling tuning and measurement from a phone/host.
### Inputs/Outputs
- Inputs: Protocol commands from phone (BLE GATT or UART/USB), config payloads, measurement requests.
- Outputs: Device info, measurement responses, example reference profiles, status codes.
### Config parameters
- Command set: discovery, `GET_DEVICE_INFO`, `GET_CONFIG`, `SET_CONFIG`, `APPLY_TEMP`, `COMMIT`, `ROLLBACK`, `PANIC_OFF`, measurement triggers (tone/sweep/noise generation), profile selection.
- Payloads: JSON/CBOR schemas for calibration/audiogram/tinnitus/modes; reference profiles including arbitrary-point audiograms.
### Constraints
- Keep UI simple; buds generate test signals; transport reliable with chunking and ACK/NAK.
### Acceptance criteria & tests
- Protocol documented with request/response formats; example host script can read/write configs and trigger measurements; rollback/panic-off tested end-to-end.

## Example Audiogram Configurations
### Standard octave audiogram (dB HL)
```json
{
  "schema": "audiogram/v3",
  "right": {
    "frequencies_hz": [125, 250, 500, 1000, 2000, 4000, 6000, 8000],
    "thresholds_db_hl": [10, 10, 15, 20, 35, 50, 60, 70]
  },
  "left": {
    "frequencies_hz": [125, 250, 500, 1000, 2000, 4000, 6000, 8000],
    "thresholds_db_hl": [5, 5, 10, 15, 25, 40, 50, 60]
  }
}
```

### Non-standard points example (includes 750, 1500, 3000 Hz)
```json
{
  "schema": "audiogram/v3",
  "right": {
    "frequencies_hz": [125, 250, 500, 750, 1000, 1500, 2000, 3000, 4000, 6000, 8000],
    "thresholds_db_hl": [10, 10, 15, 18, 20, 28, 35, 45, 50, 60, 70]
  },
  "left": {
    "frequencies_hz": [125, 250, 500, 750, 1000, 1500, 2000, 3000, 4000, 6000, 8000],
    "thresholds_db_hl": [5, 5, 10, 12, 15, 22, 25, 35, 40, 50, 60]
  }
}
```

These examples are anonymized and intended for testing documentation and protocol tooling only.
