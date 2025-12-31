# Development Milestones

## M0 — Build + Flash Baseline
- Reproducible toolchain builds
- Reliable firmware flashing
- Logging/telemetry for CPU, buffer health, and battery estimates (if available)

## M1 — Config Storage + Schema
- Per-bud persistent config store
- Versioning, migration, and rollback
- Simple transport (UART/USB, BLE, or file-based injection)

## M2 — DSP Core + Limiter
- Shared DSP core library (EQ, compressor, limiter, ramps)
- Hard limiter validated to prevent clipping

## M3 — Audiogram EQ Integration
- Audiogram config ingestion
- Audiogram → IIR generator
- Applied to A2DP playback and call downlink

## M4 — Mode Framework
- Ambient, Music, and Theater presets
- Click-free mode switching
- Consistent audiogram modulation across modes

## M5 — Call Pipeline Quality
- Uplink processing (HPF, AGC, NR hooks)
- Downlink speech shaping and compression
- Robust A2DP ↔ HFP transitions

## M6 — Ambient Improvements
- Wind suppression
- Basic noise reduction
- Conversational mode with speech emphasis and optional ducking

## M7 — Tinnitus Masking
- Noise generator and filters
- Configurable with safety caps and panic-off control
- Per-mode policy (default off in calls)

## M8 — Calibration Routines
- Minimal calibration implemented
- Optional in-ear measurement using internal mic with compensation EQ fitting

## M9 — Cross-Bud Coordination (Optional)
- Share VAD/noise estimates
- Coordinated ducking and gain state

## M10 — Advanced Binaural / Beamforming (Conditional)
- Attempt only if bandwidth, latency, and battery budgets are acceptable and benefits are proven
