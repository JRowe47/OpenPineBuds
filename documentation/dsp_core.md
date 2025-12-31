# DSP Core Pipeline (M2)

This milestone introduces a reusable DSP chain scaffold that mirrors the
calibration → audiogram → mode overlay → compressor → limiter flow required by
the audio system specifications. The goal is to keep the limiter as the final
stage, provide predictable headroom while stacking EQ layers, and surface
telemetry for safety/diagnostics.

## Chain ordering

The chain order is declared explicitly so the limiter cannot be accidentally
reordered. The stage list is validated on init and will assert if the limiter is
not last:

1. `calibration_eq` – per-bud trims and frequency response compensation.
2. `audiogram_eq` – per-ear interpolation/smoothing from the audiogram grid.
3. `mode_overlay` – per-mode boosts/cuts (ambient/music/theater, etc.).
4. `compressor` – optional DRC/NR elements ahead of the limiter.
5. `limiter` – soft-knee safety limiter, always the terminal block.

## Ramping and headroom management

`services/audio_process/dsp_chain.c` tracks a block-level gain value in dB and
applies a headroom offset before the samples hit the processing chain. The
ramping helper enables smooth transitions when modes or configurations change
and can be bypassed with `audio_process_force_panic_off()` to enforce an instant
mute during panic-off events.

## Telemetry counters

Telemetry is gathered for each processed frame:

- CPU time per frame (`last_frame_us`) and running total cycles.
- Limiter engagement count (incremented on every limiter block run).
- Clipping, overflow, and underflow counters for postmortem diagnostics.

Use `audio_process_get_telemetry()` to snapshot the counters for logging or host
reports. The counters are stored alongside the ramp/headroom state so reporting
can correlate limiter activity with headroom consumption.
