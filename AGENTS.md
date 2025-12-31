# Agents.md — OpenPineBuds “Hearing Assist + Modes” Project Guide

This repo is being modded to support **hearing-assist features (audiogram-driven)**, **media streaming listening modes**, and **strong phone-call performance**, while staying inside embedded constraints (latency, battery, CPU).

This is **not** intended to be a certified medical device. Treat it like a powerful audio tool that can absolutely hurt someone’s hearing if misused.

---

## 0) Safety and scope (read first)

### What we are building
- A **mode-based audio system** (ambient / music / theater / conversation / call) where a **per-user audiogram** (and per-bud calibration) *systematically* shapes all modes.
- Optional tinnitus masking (“sound therapy”) features.

### What we are NOT claiming
- No claim of medical efficacy.
- No claim of compliance with hearing-aid regulations (FDA/CE/etc).
- No guarantee of accurate dB HL ↔ dB SPL mapping (unless calibrated with proper equipment).

### Hard safety requirements (must-haves)
1) **Output limiter is last in the chain** (post-EQ, post-mix, post-masker).  
2) **Safe defaults** on first boot / after config reset (low gain, masker off, ambient gain capped).  
3) **Gain ramping** (no instant jumps) for mode changes and parameter updates.  
4) **Per-mode maximum gain caps**, especially for ambient/hearing-assist.  
5) **“Panic off” control**: a single action disables ambient amplification + tinnitus masker immediately.  
6) **No clipping allowed**: track headroom; enforce a digital limiter and conservative internal gain staging.  
7) **Hearing safety > sound quality**. If uncertain, reduce gain and limit.

### Human safety guidelines (for developers/users)
- Do not tune at high volumes. Use quiet environments for initial calibration.
- Assume hearing thresholds and comfort levels can change over time.
- Any tinnitus masking must start **very low**. People can accidentally worsen tinnitus with over-masking.

---

## 1) High-level architecture

### Core idea: “Audiogram + Calibration are the base layer”
All audio outputs are shaped by:

1) **Per-bud calibration** (hardware/fit compensation; L and R can differ)
2) **Per-bud audiogram profile** (L and R can differ)
3) Optional **tinnitus profile** (per-bud, per-user)
4) Then **mode overlay** (ambient/music/theater/conversation/call)
5) Then **final limiter**

Think of it like:
`DAC_out = Limiter( ModeDSP( AudiogramEQ( CalibrationEQ( Mix(sources) ) ) ) )`

Where `sources` can include:
- A2DP music decode
- HFP downlink (call audio from phone)
- mic passthrough (ambient / talk-through)
- tinnitus masker noise

### Performance principles
- Prefer **IIR/biquad** filters over FFT-heavy processing (battery + latency).
- Prefer **sharing features** (VAD states, noise estimates) between buds rather than raw mic streams, unless we can prove raw streaming is feasible and efficient.
- Any algorithm added must include:
  - CPU estimate
  - RAM estimate
  - expected latency impact
  - battery impact assumptions
  - fallback path

---

## 2) Reference audiogram data (dev profile)

**Audiometry (10/14/2020)** — AC, Insert Earphones, BC: B71

### Pure tone thresholds — Hearing Level (dB HL)

| Frequency (Hz) | Right (dB HL) | Left (dB HL) |
|---:|---:|---:|
| 125 | 10 | 5 |
| 250 | 10 | 5 |
| 500 | 15 | 10 |
| 1000 | 20 | 15 |
| 2000 | 35 | 25 |
| 4000 | 50 | 40 |
| 6000 | 60 | 50 |
| 8000 | 70 | 60 |

**Interpretation (engineering, not clinical):** sloping high-frequency loss; Right is worse than Left above ~2 kHz. This strongly motivates:
- high-frequency emphasis for speech clarity
- careful feedback avoidance in ambient/hearing-assist modes
- conservative gain limits in 2–8 kHz unless calibration/feedback control is robust

### Immittance (10/14/2020)
- Probe tone: 226 Hz
- Tymps: Type A (R/L)

Reflex Screening values were provided but appear nonstandard/ambiguous in units (0.75/0.50/0.25). Keep as metadata only unless verified.

### Speech (10/14/2020)
Right:
- SDT 45 dB HL
- SRT 90 dB HL
- WRS 80%
- MCL 75 dB HL
- UCL 85 dB HL

Left:
- SDT 85 dB HL
- SRT 48 dB HL
- WRS 90%
- (MCL/UCL not specified)

**Note:** some of these fields look internally inconsistent (e.g., SDT vs SRT ordering). Treat **pure-tone thresholds** as the primary input unless the speech section is re-verified.

### Derived “starter” gain targets (NOT a prescription)
A simple engineering starting point is a half-gain rule:
`target_gain_dB ≈ 0.5 × threshold_dBHL` with:
- low-frequency gain limited (reduce occlusion/boom)
- hard caps for safety and feedback risk

For the reference profile this implies roughly:
- Right: +0…10 dB up to 1 kHz, rising to ~+17.5 dB at 2 kHz, ~+25 dB at 4 kHz, ~+35 dB at 8 kHz (before caps)
- Left: slightly less than right across 2–8 kHz

We will implement something *like* this initially, then refine using in-ear calibration and subjective tuning.

---

## 3) Modes and required behaviors

### Media streaming (A2DP)
**Ambient / pass-through mode**
- Mix: `music + ambient_mics`
- Must include: wind suppression, low-frequency control, safe ambient gain caps
- Optional: “speech-aware ducking” (when nearby speech detected, duck music smoothly)

**Music mode**
- Ambient mix: off (or very low if explicitly enabled)
- EQ tuned for music enjoyment, but audiogram still applied (with a “music” variant allowed)

**Theater mode**
- Ambient mix: 0
- Prioritize immersion; allow stronger bass extension if safe
- If ANC exists and is stable, this mode can enable it (later milestone)

### Hearing / ambient dominant modes
**Conversational**
- Ambient mic dominant (talk-through)
- Voice clarity emphasis (1–4 kHz) done via *compression + modest EQ*, not just raw treble boost
- Must keep latency low; must not create “robot voice” artifacts

**Outdoors / ambient music**
- Moderate ambient mix
- Strong wind suppression
- Conservative high-frequency gain to reduce wind hiss

### Phone calls (HFP)
Call mode is two separate pipelines:
- **Uplink** (your mic → phone): noise reduction, AGC/limiter, wind suppression, echo control
- **Downlink** (phone → your ears): audiogram shaping + speech compression + limiter

---

## 4) Feature checklist

### Core foundations
- [ ] Per-bud persistent storage (calibration/audiogram/tinnitus)
- [ ] Versioned config schema + migration path
- [ ] Atomic updates + rollback (don’t brick configs on power loss)
- [ ] Mode state machine (A2DP vs HFP vs idle; per-mode presets)
- [ ] Output limiter + gain ramping everywhere

### Audiogram-driven hearing assist
- [ ] Per-ear audiogram ingestion
- [ ] Audiogram → EQ “prescription” generator (starter: half-gain; later: more advanced fitting rules)
- [ ] Multi-band compression (WDRC-style) for ambient/hearing modes
- [ ] Feedback-risk controls (gain caps, notch strategy, adaptive constraints)

### Ambient / pass-through features
- [ ] Ambient talk-through routing
- [ ] Wind detection + suppression
- [ ] Basic noise reduction (low compute)
- [ ] Conversational enhancement (speech emphasis + optional VAD-based gating/ducking)

### Media streaming modes
- [ ] Ambient mix mode (music + ambient)
- [ ] Music EQ preset(s) (enjoyment-focused)
- [ ] Theater mode (no ambient; optional ANC later)
- [ ] Audiogram modulation consistently applied across all modes

### Phone-call performance
- [ ] Uplink processing: HPF, AGC, wind suppression, noise suppression
- [ ] Downlink processing: audiogram EQ + speech-focused compression
- [ ] Robust switching between A2DP/HFP without pops/clicks

### Tinnitus mitigation
- [ ] Noise generator (white → shaped to pink/band-limited)
- [ ] Tunable profile (frequency, bandwidth, level, modulation optional)
- [ ] Strict level caps + “panic off”
- [ ] Per-mode enable policy (e.g., allow in ambient + music; disable by default in calls)

### Advanced / optional
- [ ] Cross-bud coordination (share VAD/noise estimates, not raw audio)
- [ ] Binaural beamforming (only if proven feasible on the TWS link + CPU budget)
- [ ] Personalized in-ear response measurement (if internal feedback mic is accessible)

---

## 5) Configuration model (recommended)

### Conceptual layers
1) **Per-bud calibration**: compensates manufacturing variance + fit response + mic sensitivity
2) **Per-bud audiogram**: per-ear thresholds and fitting parameters
3) **Per-bud tinnitus profile**: masker parameters
4) **User mode presets**: “music/theater/ambient/conversation/call” overlays

### Recommended config objects
- `calibration_{L|R}.json`
- `audiogram_{L|R}.json`
- `tinnitus_{L|R}.json`
- `modes.json` (shared across both buds for the same user, unless explicitly separated)

Example (shape, not final):

```json
{
  "schema_version": 1,
  "bud_id": "BLE_ADDR_OR_SERIAL",
  "calibration": {
    "dac_headroom_db": 6.0,
    "output_trim_db": -1.5,
    "fr_compensation_iir": [
      {"type":"PEAK","fc_hz":1800,"q":1.0,"gain_db":2.0}
    ],
    "mic": {
      "external_sensitivity_db": 0.0,
      "internal_sensitivity_db": 0.0,
      "noise_floor_dbfs": -70.0
    },
    "fit": {
      "seal_score": 0.0,
      "last_measured_utc": "2025-01-01T00:00:00Z"
    }
  }
}
```

```json
{
  "schema_version": 1,
  "audiogram": {
    "date": "2020-10-14",
    "units": "dB_HL",
    "thresholds_hz": [125,250,500,1000,2000,4000,6000,8000],
    "right_dbhl": [10,10,15,20,35,50,60,70],
    "left_dbhl":  [ 5, 5,10,15,25,40,50,60],
    "fitting": {
      "method": "half_gain_v0",
      "low_freq_gain_cap_db": 6.0,
      "high_freq_gain_cap_db": 20.0,
      "compression": {
        "enabled": true,
        "bands_hz": [[125,500],[500,2000],[2000,8000]],
        "ratios": [1.5, 2.0, 2.5],
        "attack_ms": 10,
        "release_ms": 150
      }
    }
  }
}
```

```json
{
  "schema_version": 1,
  "tinnitus": {
    "enabled": false,
    "type": "band_limited_noise",
    "center_hz": 6000,
    "bandwidth_hz": 1500,
    "level_dbfs": -45,
    "modulation": {
      "enabled": false,
      "rate_hz": 0.2,
      "depth_db": 3.0
    },
    "safety": {
      "max_level_dbfs": -35,
      "fade_in_ms": 2000,
      "panic_off_supported": true
    }
  }
}
```

---

## 6) Calibration process (hardware-appropriate)

We need two kinds of calibration:

1) **Relative calibration** (works without lab gear): makes L/R consistent and prevents dangerous gain.
2) **In-ear response calibration** (best-effort, if internal/feedback mic access exists): measures your ear+bud response and improves accuracy.

### 6.1 Minimal calibration (no extra equipment)
Goal: safe, usable, consistent between buds.

**Step A — Baseline sanity**
- Reset configs to safe defaults
- Verify no clipping at max phone volume in each mode (music, ambient, call)
- Verify limiter engages smoothly (no hard distortion)

**Step B — L/R balance trim**
- Play 1 kHz tone centered (same signal to L/R)
- User adjusts a “balance trim” until sound image is centered
- Store as `output_trim_db` per bud

**Step C — Comfort ceiling**
- Play wideband pink noise at a low starting level
- User increases until reaching “comfortably loud, not loud”
- Store this as `user_mcl_reference` (engineering reference), then apply conservative caps below it

**Step D — Ambient gain ceiling**
- In a quiet room, enable ambient/talk-through
- Increase ambient gain until user says “natural but not loud”
- Hard-cap ambient gain at or below this value; store per bud

### 6.2 In-ear calibration (preferred, if internal mic is available)
If the hardware/SDK exposes an **internal (in-ear) mic** (often used for hybrid ANC feedback), we can do much better.

**Step A — Seal / fit score**
- Play a low-frequency probe tone/sweep (e.g., 50–500 Hz)
- Record internal mic response
- Infer seal quality; store `seal_score`
- If seal is poor: reduce bass boost and/or warn in app/UI

**Step B — Frequency response measurement**
- Generate a logarithmic sine sweep or multi-tone stimulus at safe level
- Record internal mic signal
- Compute response curve (relative is fine)
- Fit a small set of IIR filters to compensate (e.g., 3–8 biquads)
- Store as `fr_compensation_iir` in calibration

**Step C — Feedback-risk metric**
- Measure loop gain proxy: how much internal mic sees when output changes in 2–8 kHz
- If feedback risk high: reduce allowable high-frequency gain caps automatically

### 6.3 Microphone calibration (for ambient + calls)
- Measure mic noise floor in quiet (external mics)
- Set AGC/NR thresholds accordingly
- Run wind test procedure:
  - detect low-frequency turbulence signature
  - confirm wind suppression engages without “pumping”

### 6.4 Calibration acceptance criteria
- No audible pops on entering/leaving ambient mode
- Ambient latency subjectively tolerable (no obvious “delay echo” when speaking)
- Call uplink not clipping in loud speech
- Call downlink intelligible at conservative volume
- Tinnitus masker default OFF; when ON, fade-in and remains subtle

---

## 7) Development milestones (suggested)

### M0 — Build + flash baseline
- Toolchain builds reproducibly
- Firmware flashes reliably
- Logging/telemetry enabled (CPU, buffer underflows, battery estimates if available)

### M1 — Config storage + schema
- Per-bud persistent config store
- Versioning, migration, rollback
- Simple transport: UART/USB, BLE, or file-based injection

### M2 — DSP core + limiter
- Shared DSP core library (EQ + compressor + limiter + ramps)
- Hard limiter validated (no clipping)

### M3 — Audiogram EQ integration
- Audiogram config ingestion
- Audiogram → IIR generator
- Applies to A2DP playback and call downlink

### M4 — Mode framework
- Ambient / Music / Theater implemented as presets
- Mode switching is click-free
- Audiogram modulates all modes consistently

### M5 — Call pipeline quality
- Uplink processing integrated (HPF, AGC, NR hooks)
- Downlink speech shaping + compression integrated
- Robust A2DP ↔ HFP transitions

### M6 — Ambient improvements
- Wind suppression
- Basic noise reduction
- Conversational mode (speech emphasis + optional ducking)

### M7 — Tinnitus masking
- Noise generator + filters
- Configurable, safe caps, panic-off
- Per-mode policy (default off in calls)

### M8 — Calibration routines
- Minimal calibration implemented
- If possible: in-ear measurement using internal mic + compensation EQ fitting

### M9 — Cross-bud coordination (optional)
- Share VAD/noise estimates
- Coordinated ducking and gain state

### M10 — Advanced binaural / beamforming (only if feasible)
- Only attempt after proving:
  - bandwidth and latency are acceptable
  - battery impact is tolerable
  - subjective benefit is real

---

## 8) Testing expectations (don’t skip)

### Objective checks
- Buffer underflow/overflow counters
- Worst-case CPU usage in each mode
- Latency measurement (ambient path)
- Distortion / clipping detection via internal metrics (peak stats)

### Subjective checks
- Quiet-room speech (ambient/conversation)
- Windy outdoors walking test
- Music enjoyment test (music mode)
- Call test in quiet + noise + wind

### Safety regression tests
- Limiter always active
- No mode can exceed gain caps
- “Panic off” always works
- Config corruption cannot enable dangerous defaults

---

## 9) How to contribute (agent/human)
- Keep changes small and testable.
- Prefer feature flags for new DSP blocks.
- Document all new parameters (units, ranges, defaults, safety caps).
- If you add a mode, add acceptance tests and safe defaults.
- Never merge a change that can cause sudden volume jumps.

---

End of Agents.md
