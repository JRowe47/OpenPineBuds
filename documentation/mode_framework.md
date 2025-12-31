# Mode Framework (M4)

The mode framework layers ambient/music/theater overlays on top of the shared
calibration and audiogram stages introduced in earlier milestones. Every mode
shares the same calibrated base per ear and keeps the limiter as the terminal
stage. Mode switches are ramped to avoid pops, and headroom is reserved when
stacking overlays so limiter engagement remains predictable.

## Pipeline placement

The processing order is fixed to preserve the safety envelope:

1. **Calibration EQ** – per-bud trims and frequency-response compensation.
2. **Audiogram EQ** – interpolated per-ear compensation derived from validated
   audiogram points.
3. **Mode overlay EQ** – presets that add per-mode coloration and ambient
   policies.
4. **Compressors / NR (optional per mode)** – light DRC or NR before the
   limiter when enabled.
5. **Limiter** – soft-knee safety limiter (knee ~15 dB, cap 18 dB, 5 ms attack /
   50 ms release) always last.

`dsp_chain_request_ramp()` is called whenever the active mode changes so gains
crossfade over 20–50 ms instead of stepping. Panic-off uses
`dsp_chain_force_mute()` to bypass the ramp and guarantee an immediate mute.

## Mode presets

The configuration exposes three media-focused presets. All values are capped by
firmware safety limits and respect the shared headroom budget:

- **Ambient**
  - `ambient_mix_ratio`: 0.35–0.55 linear mix of mic feed into media path.
  - `wind_suppression`: boolean to engage the wind detector hook. When wind is
    detected, low-band HPF and mix ratio are tightened automatically.
  - `ducking`: optional speech-aware ducking depth (dB) and attack/release
    times to reduce media when speech is present.
  - Overlay EQ: mild presence lift (+2 dB @ 2–4 kHz) with low-shelf roll-off to
    reduce boominess.

- **Music**
  - `ambient_mix_ratio`: 0.05–0.10 (effectively media-first).
  - `ducking`: off by default; may be enabled for podcasts.
  - Overlay EQ: gentle smile curve (+2–3 dB at 80 Hz and 8 kHz) constrained by
    headroom so limiter margin is preserved.

- **Theater**
  - `ambient_mix_ratio`: 0.0 (ambient off) with the option to hard disable the
    ambient path for additional SNR.
  - `wind_suppression`: still available for rare outdoor playback cases.
  - Overlay EQ: presence-focused shaping (+3 dB @ 2 kHz, slight HF shelf) with
    no dynamic processing beyond the limiter.

Each preset stores an explicit headroom reservation (default 6 dB) that is
applied before overlays so total gain plus limiter threshold stays within safe
bounds.

## Switch logic and telemetry

- Mode switches update the overlay block pointer and request a gain ramp to the
  target overlay gain, keeping the limiter last.
- Ambient mix ratios are applied after the overlay EQ and before any optional
  compressor; mix changes are also ramped to avoid pumping.
- The wind suppression hook reports detector state; when active it raises the
  HPF corner and reduces ambient mix by 3–6 dB.
- Ducking uses the VAD flag from the ambient path to reduce media gain by the
  configured depth with attack/release times matching the ramp helper.
- Telemetry counters from `dsp_chain` (frames, limiter_engaged, clipping,
  underflow/overflow, CPU) are sampled per mode change to verify limiter
  placement and ramp behavior.

## Example configuration

```json
{
  "schema": "modes/v1",
  "headroom_db": 6.0,
  "presets": {
    "ambient": {
      "ambient_mix_ratio": 0.5,
      "wind_suppression": true,
      "ducking": { "depth_db": 6, "attack_ms": 40, "release_ms": 250 },
      "overlay_eq": "eq_presets/ambient_v1.json"
    },
    "music": {
      "ambient_mix_ratio": 0.08,
      "wind_suppression": false,
      "ducking": null,
      "overlay_eq": "eq_presets/music_v1.json"
    },
    "theater": {
      "ambient_mix_ratio": 0.0,
      "wind_suppression": true,
      "ducking": null,
      "overlay_eq": "eq_presets/theater_v1.json"
    }
  }
}
```

The EQ preset strings reference biquad tables stored alongside audiogram and
calibration tables. Apply logic validates all ratios and ducking depths before
activating the new preset; invalid values fall back to ambient defaults and log
an error.

## Acceptance validation

- **Ambient mix:** Measured mix ratio matches configuration within ±2% and ramps
  without clicks; wind activation reduces LF content and mix as specified.
- **Music/theater overlays:** Per-mode EQ responses respect headroom (limiter
  engagement does not increase over baseline) and ambient mix caps are obeyed
  (theater disables ambient by default).
- **Transitions:** Mode changes complete within the configured ramp duration
  without clipping or limiter dropouts; telemetry confirms limiter remains the
  last stage in all paths.
