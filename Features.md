# Feature Checklist

Derived from the project guide, these features track the hearing-assist and listening-mode roadmap.

## Core Foundations
- Per-bud persistent storage (calibration/audiogram/tinnitus)
- Versioned config schema with migration and rollback
- Atomic updates that survive power loss
- Mode state machine (A2DP vs HFP vs idle; per-mode presets)
- Output limiter everywhere with gain ramping

## Audiogram-Driven Hearing Assist
- Per-ear audiogram ingestion
- Audiogram → EQ “prescription” generator (starter: half-gain, later more advanced)
- Multi-band compression (WDRC-style) for ambient/hearing modes
- Feedback-risk controls (gain caps, notch strategy, adaptive constraints)

## Ambient / Pass-Through
- Ambient talk-through routing
- Wind detection and suppression
- Basic low-compute noise reduction
- Conversational enhancement (speech emphasis plus optional VAD-based gating/ducking)

## Media Streaming Modes
- Ambient mix mode (music + ambient)
- Music EQ preset(s) aimed at enjoyment
- Theater mode without ambient mix (ANC optional later)
- Audiogram modulation applied across all modes

## Phone-Call Performance
- Uplink processing: HPF, AGC, wind suppression, noise suppression
- Downlink processing: audiogram EQ plus speech-focused compression
- Robust switching between A2DP and HFP without artifacts

## Tinnitus Mitigation
- Noise generator (white to pink/band-limited)
- Tunable masker profile (frequency, bandwidth, level, optional modulation)
- Strict level caps with “panic off”
- Per-mode enable policy (default off in calls)

## Advanced / Optional
- Cross-bud coordination (share VAD/noise estimates, not raw audio)
- Binaural beamforming (only if feasible on TWS link and CPU budget)
- Personalized in-ear response measurement when a feedback mic is available
