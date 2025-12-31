# Call Pipeline (M5)

The call pipeline specializes the shared DSP chain for HFP traffic. It keeps
calibration and audiogram placement consistent with media modes while adding
voice-focused processing for both downlink and uplink paths. Limiter-last order
and panic-off behavior remain mandatory.

## Downlink path (to the listener)

Downlink audio uses the calibrated+audiogram base with an additional presence
shaping block and speech compressor before the limiter:

1. **Calibration EQ** – per-bud trims and FR compensation.
2. **Audiogram EQ** – interpolated per-ear compensation from validated
   audiogram points (downlink only).
3. **Presence EQ overlay** – gentle upper-mid lift (e.g., +2–3 dB at 2–4 kHz)
   to preserve intelligibility without touching the limiter headroom budget.
4. **Speech compressor** – low ratio (1.5–2:1) with ~5 ms attack / 80–120 ms
   release to stabilize voice loudness ahead of the limiter.
5. **Limiter** – soft-knee limiter (knee ~15 dB, cap 18 dB, 5 ms attack / 50 ms
   release) remains the terminal block.

Downlink processing shares the same ramp helper used by media modes so presence
EQ and compressor engagement transitions over ~30–50 ms instead of stepping. The
limiter engagement counter should not increase versus media playback after
headroom reservation is applied.

## Uplink path (mic to network)

Uplink audio omits the audiogram entirely and prioritizes protection from wind
and clipping before the limiter:

1. **Mic high-pass filter (HPF)** – removes rumble; raises corner frequency when
   the wind detector is active.
2. **Wind/NR gate** – toggles light NR or bypass based on wind state without
   introducing pumping.
3. **AGC** – slow/fast pair with speech-preserving time constants; target level
   set to leave 6–8 dB of headroom for the limiter.
4. **Optional ENC/beamforming hook** – reserved slot for future cross-bud
   coordination without changing stage order.
5. **Limiter** – same limiter parameters as downlink; must remain last.

Panic-off bypasses any ramping and immediately mutes ambient/tinnitus streams
while keeping uplink gain stable to avoid transient clipping; uplink limiter
still guards unexpected bursts.

## Safety, validation, and logging

- Stage order is validated during pipeline init; any attempt to insert blocks
  after the limiter triggers an assert.
- Audiogram application is explicitly disabled on uplink; config load rejects
  uplink audiogram references.
- Panic-off path invokes the same `dsp_chain_force_mute()` used by media modes
  and reports a telemetry event for postmortem analysis.
- Telemetry records limiter engagement, clipping counts, AGC gain state, and
  wind detector state for both paths; call-mode snapshots are logged on call
  setup/teardown for QA.

## Acceptance checks

- **Downlink consistency:** Presence EQ and compressor maintain talker loudness
  while limiter engagement stays within media-mode baseline; audiogram response
  matches validated profile.
- **Uplink protection:** HPF + AGC + limiter prevent clipping during loud voice
  or wind bursts; wind detector adjustments do not introduce audible pumping.
- **Panic-off behavior:** Ambient/tinnitus are muted instantly; uplink remains
  stable; limiter stays last in both chains.
