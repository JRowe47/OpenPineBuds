# Ambient Pipeline (M6)

The ambient pipeline extends the mode framework with wind-aware detection,
speech-first processing for conversation, and conservative outdoor tuning. The
chain still honors the calibration → audiogram → overlay → dynamics → limiter
ordering and ramps any parameter changes to avoid pumping or clicks.

## Wind detector with hysteresis

- **Detector:** Low-band energy ratio (50–250 Hz) combined with short-term
  coherence between paired ambient mics. The detector raises a wind flag when
  both metrics exceed thresholds for 60 ms and clears it only after 200 ms of
  stable recovery.
- **Adjustments when active:**
  - Increase HPF corner to 150–180 Hz and reduce low-shelf boost.
  - Drop ambient mix by 3–6 dB and tighten NR attack.
  - Clamp per-ear overlay gains to preserve limiter headroom.
- **Fail-safe:** Detector state is rate-limited so mode flips cannot toggle HPF
  every frame; panic-off bypasses ramps and mutes immediately.

## Mode overlays added for M6

- **Conversation:** Low-latency speech focus with +2 to +4 dB lift from 1–4 kHz
  and a fast soft-knee compressor (attack 3–5 ms, release 80–120 ms, ratio up
  to 2.5:1). Ambient mix defaults to 0.35 with optional ducking when VAD fires.
- **Outdoors:** Wind-biased variant of ambient. HPF defaults to 120–150 Hz,
  high-frequency shelving is capped, and NR uses a medium tier with slower
  release to avoid pumping. Ambient mix is clamped to 0.25 unless the detector
  is clear.
- **Ambient music mix (optional):** Retains a light NR tier and medium HPF, and
  crossfades to the conversation stack when speech is detected, returning to the
  music overlay after a 300 ms hold.

## Noise reduction and isolation tiers

- **By mode:**
  - *Ambient/Outdoors:* Medium NR (6–8 dB max) with wind-aware HPF corner
    adjustments.
  - *Conversation:* Low-latency NR (4–6 dB) that keeps the presence band intact
    and defers heavy suppression to the limiter safety margin.
  - *Music/Theater:* NR off by default; theater keeps ambient disabled.
- **Voice isolation:** Optional narrow presence enhancer engaged only when VAD
  is stable for >120 ms to avoid chatter-induced pumping.
- **Limiter last:** All NR and isolation filters feed the shared limiter; gain
  ramps allocate 6 dB of headroom before overlays to keep limiter engagement
  predictable.

## Cross-bud sharing hooks

- Exchange detector states (wind, VAD, NR tier) at ~10 Hz to keep stereo
  behavior aligned without streaming audio.
- Each bud retains autonomy; remote flags are debounced and can only tighten
  processing (e.g., never lowering HPF or NR when the peer reports wind).
- Logging includes local/peer states plus limiter engagement so tuning can
  verify that shared decisions do not destabilize the safety chain.

## Validation checklist

- Wind detector transitions honor hysteresis and do not oscillate when wind is
  near threshold; HPF and mix ramps complete within 50 ms without clicks.
- Conversation mode maintains <10 ms additional latency while providing the
  documented presence lift and compression behavior.
- Outdoors mode limits HF gain and NR changes so limiter engagement does not
  exceed the ambient baseline; panic-off still mutes immediately with limiter
  last.
