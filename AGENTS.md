# Project Agent Notes

This repository does not currently include nested agent instructions. Follow these guardrails when updating documentation and code.
- Before starting work, review `ToDoList.md` and check items off as you complete them; add subtasks when new work is discovered.

## Configuration model
- Audiogram inputs must accept any number of frequency points per ear (minimum of two).
- Frequencies must be strictly increasing, unique, and constrained to a safe listening range (100–12000 Hz).
- Threshold arrays must match the frequency list length; thresholds are expressed in dB HL and should be sanity-checked/clamped to safe limits before use.
- Audiogram points are interpolated in logarithmic-frequency space, then lightly smoothed before being fit onto the internal DSP target grid.
- The interpolated target curve feeds the IIR/biquad fitting pipeline per ear so that all listening modes share the same base compensation layer.

## Feature checklist
- [x] Arbitrary audiogram frequency-point ingestion (per ear)
- [x] Validation + schema versioning/migration
- [x] Log-frequency interpolation + smoothing of target gain
- [x] Fitting target gain to available EQ sections (IIR/biquads) per ear
- [ ] Regression: standard octave-only audiograms remain supported and produce equivalent results

## Milestones
- Add a milestone for “Arbitrary Audiogram Points Support” with acceptance criteria that include validation of arbitrary points, interpolation onto the DSP grid with smoothing, safe clamping of gains, and confirmation that legacy octave-only profiles behave the same as before.
