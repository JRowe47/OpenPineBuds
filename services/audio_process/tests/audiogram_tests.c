#include "audiogram.h"
#include "dsp_chain.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Minimal stubs for hardware-specific dependencies.
uint32_t hal_fast_sys_timer_get(void) {
  static uint32_t tick = 0;
  tick += 160;
  return tick;
}

uint32_t hal_cmu_get_crystal_freq(void) { return 24000000; }

static const float TARGET_GRID[] = {125.f,  250.f,  500.f,  750.f,  1000.f,
                                    1500.f, 2000.f, 3000.f, 4000.f, 6000.f,
                                    8000.f, 10000.f};

static AudiogramProfile make_flat_octave_profile(void) {
  AudiogramProfile p = {0};
  p.schema_version = AUDIOGRAM_SCHEMA_VERSION;
  uint16_t freqs[] = {125,  250,  500,  1000, 2000,
                      4000, 6000, 8000};
  for (size_t i = 0; i < sizeof(freqs) / sizeof(freqs[0]); ++i) {
    p.left.frequencies_hz[i] = freqs[i];
    p.right.frequencies_hz[i] = freqs[i];
    p.left.thresholds_db_hl[i] = 20;
    p.right.thresholds_db_hl[i] = 20;
  }
  p.left.point_count = sizeof(freqs) / sizeof(freqs[0]);
  p.right.point_count = p.left.point_count;
  return p;
}

static AudiogramProfile make_mixed_point_profile(void) {
  AudiogramProfile p = {0};
  p.schema_version = AUDIOGRAM_SCHEMA_VERSION;
  uint16_t freqs[] = {125,  250,  500,  750,  1000, 1500,
                      2000, 3000, 4000, 6000, 8000, 10000};
  for (size_t i = 0; i < sizeof(freqs) / sizeof(freqs[0]); ++i) {
    p.left.frequencies_hz[i] = freqs[i];
    p.right.frequencies_hz[i] = freqs[i];
    p.left.thresholds_db_hl[i] = (int16_t)(i * 5);
    p.right.thresholds_db_hl[i] = (int16_t)(i * 5);
  }
  p.left.point_count = sizeof(freqs) / sizeof(freqs[0]);
  p.right.point_count = p.left.point_count;
  p.left.threshold_count = p.left.point_count;
  p.right.threshold_count = p.left.point_count;
  return p;
}

static void test_octave_profile_regression(void) {
  AudiogramProfile profile = make_flat_octave_profile();
  float gains[sizeof(TARGET_GRID) / sizeof(TARGET_GRID[0])] = {0};

  assert(audiogram_validate_profile(&profile, NULL, 0));
  int written = audiogram_interpolate_gain(&profile.left, TARGET_GRID,
                                           sizeof(TARGET_GRID) /
                                               sizeof(TARGET_GRID[0]),
                                           gains);
  assert(written == (int)(sizeof(TARGET_GRID) / sizeof(TARGET_GRID[0])));
  for (size_t i = 0; i < sizeof(gains) / sizeof(gains[0]); ++i) {
    assert(fabsf(gains[i] - 10.0f) < 0.001f);
  }
}

static void test_mixed_points_fit_and_interp(void) {
  AudiogramProfile profile = make_mixed_point_profile();
  float gains[sizeof(TARGET_GRID) / sizeof(TARGET_GRID[0])] = {0};
  IIR_CFG_T cfg = {0};
  const float expected[] = {1.875f, 2.917f, 5.000f, 7.500f, 10.000f, 12.500f,
                            15.000f, 17.500f, 19.889f, 21.889f, 23.222f,
                            23.750f};

  assert(audiogram_validate_profile(&profile, NULL, 0));
  int written = audiogram_interpolate_gain(&profile.left, TARGET_GRID,
                                           sizeof(TARGET_GRID) /
                                               sizeof(TARGET_GRID[0]),
                                           gains);
  assert(written == (int)(sizeof(TARGET_GRID) / sizeof(TARGET_GRID[0])));

  for (size_t i = 0; i < sizeof(gains) / sizeof(gains[0]); ++i) {
    float delta = fabsf(gains[i] - expected[i]);
    assert(delta < 0.05f);
  }

  assert(audiogram_build_iir_cfg(&profile, true, &cfg) == 0);
  assert(cfg.num == (int)(sizeof(TARGET_GRID) / sizeof(TARGET_GRID[0])));
}

static void test_excessive_point_budget_rejected(void) {
  AudiogramEarProfile ear = {0};
  ear.point_count = AUDIOGRAM_MAX_POINTS_PER_EAR + 1;
  uint16_t freq = AUDIOGRAM_MIN_FREQ_HZ;
  for (size_t i = 0; i < ear.point_count && i < AUDIOGRAM_MAX_POINTS_PER_EAR;
       ++i) {
    ear.frequencies_hz[i] = freq;
    ear.thresholds_db_hl[i] = 0;
    freq += 10;
  }
  assert(!audiogram_validate_ear(&ear, NULL, 0));
}

static void test_target_bin_cap(void) {
  AudiogramProfile profile = make_flat_octave_profile();
  float tiny_grid[AUDIOGRAM_MAX_TARGET_BINS + 1];
  for (size_t i = 0; i < sizeof(tiny_grid) / sizeof(tiny_grid[0]); ++i) {
    tiny_grid[i] = 100.0f + (float)i;
  }
  float gains[AUDIOGRAM_MAX_TARGET_BINS + 1];
  assert(audiogram_interpolate_gain(&profile.left, tiny_grid,
                                    sizeof(tiny_grid) /
                                        sizeof(tiny_grid[0]),
                                    gains) == -3);
}

static void test_limiter_remains_last(void) {
  const char *order[] = {"calibration_eq", "audiogram_eq", "limiter"};
  DspChainState state;
  bool ok = dsp_chain_init(&state, order, 3, 3.0f);
  assert(ok);
  assert(state.limiter_last);

  dsp_chain_mark_limiter(&state);
  assert(state.counters.limiter_engaged == 1);
}

static void profile_interpolation_speed(void) {
  AudiogramProfile profile = make_mixed_point_profile();
  float gains[sizeof(TARGET_GRID) / sizeof(TARGET_GRID[0])];
  const size_t iterations = 2000;
  clock_t start = clock();
  for (size_t i = 0; i < iterations; ++i) {
    int ret = audiogram_interpolate_gain(&profile.left, TARGET_GRID,
                                         sizeof(TARGET_GRID) /
                                             sizeof(TARGET_GRID[0]),
                                         gains);
    assert(ret > 0);
  }
  clock_t end = clock();
  double elapsed_ms =
      (double)(end - start) * 1000.0 / (double)CLOCKS_PER_SEC;
  printf("Interpolated %zu iterations in %.3f ms (%.3f us/iter)\n",
         iterations, elapsed_ms, (elapsed_ms * 1000.0) / (double)iterations);
  assert(elapsed_ms < 50.0);
}

int main(void) {
  test_octave_profile_regression();
  test_mixed_points_fit_and_interp();
  test_excessive_point_budget_rejected();
  test_target_bin_cap();
  test_limiter_remains_last();
  profile_interpolation_speed();

  printf("All audiogram and limiter tests passed.\n");
  return 0;
}
