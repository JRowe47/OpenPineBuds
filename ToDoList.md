# To-Do List

## Arbitrary Audiogram Points Support
- [x] Update audiogram configuration schema to support arbitrary per-ear frequency lists (version bump with migration guidance).
- [x] Enforce validation rules: strictly increasing, unique frequencies within 100–12000 Hz; threshold counts match frequencies; minimum two points per ear.
- [x] Implement log-frequency interpolation onto the internal DSP grid with smoothing to avoid sharp transitions.
- [x] Fit interpolated target gains into available EQ/biquad sections per ear, ensuring section budgets per ear are respected and falling back to safe defaults if fitting fails.
- [x] Ensure base audiogram compensation feeds all listening modes (ambient, music, theater, conversation, calls, tinnitus masking) after per-bud calibration.
- [ ] Apply safety clamps/limiters after EQ to cap gains and preserve smooth transitions.
- [ ] Preserve backward compatibility so octave-only audiograms load unchanged; provide schema versioning and migration path.
- [ ] Add regression tests for legacy octave-only audiograms and new non-octave cases (e.g., 750, 1500, 3000 Hz).
- [ ] Document example configurations and validation errors for arbitrary frequency lists.

## Performance & Reliability
- [ ] Keep interpolation/smoothing lightweight (avoid heavy FFT paths) and cap per-ear point counts (e.g., max 32) to meet CPU/latency/memory budgets.
- [ ] Verify limiter remains active after EQ when arbitrary audiogram points are used.
- [ ] Profile interpolation and fitting stages to confirm performance constraints are met.

## Tooling & Configuration
- [ ] Provide validation/migration tooling for schema updates, including version checks.
- [ ] Add config schema tests that cover both valid and invalid cases (duplicates, unsorted points, out-of-range frequencies, length mismatches).

## Documentation
- [ ] Update feature documentation to describe arbitrary audiogram support, validation rules, and safety constraints.
- [ ] Include updated JSON examples illustrating arbitrary frequency lists and thresholds per ear.
- [ ] Note compatibility expectations and safe operating ranges in user-facing docs.
