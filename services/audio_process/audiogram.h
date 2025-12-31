#ifndef __AUDIOGRAM_H__
#define __AUDIOGRAM_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "iir_process.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIOGRAM_SCHEMA_VERSION 3
#define AUDIOGRAM_MIN_SCHEMA_VERSION 2
#define AUDIOGRAM_MAX_POINTS_PER_EAR 32
#define AUDIOGRAM_MAX_TARGET_BINS 16
#define AUDIOGRAM_MIN_FREQ_HZ 100
#define AUDIOGRAM_MAX_FREQ_HZ 12000

typedef struct {
  uint16_t frequencies_hz[AUDIOGRAM_MAX_POINTS_PER_EAR];
  int16_t thresholds_db_hl[AUDIOGRAM_MAX_POINTS_PER_EAR];
  // `point_count` tracks the number of frequency samples provided. When
  // `threshold_count` is zero, it is treated as matching `point_count` for
  // backward compatibility with legacy callers that only fill one count.
  uint8_t point_count;
  uint8_t threshold_count;
} AudiogramEarProfile;

typedef struct {
  // Schema version corresponds to the numeric suffix of the config string
  // (e.g., "audiogram/v3" -> schema_version = 3). Versions 2–3 are
  // accepted, and 0 indicates an unspecified legacy profile.
  uint8_t schema_version;
  AudiogramEarProfile left;
  AudiogramEarProfile right;
} AudiogramProfile;

// Validation helpers
bool audiogram_validate_ear(const AudiogramEarProfile *ear, char *reason,
                            size_t reason_size);
bool audiogram_validate_profile(const AudiogramProfile *profile, char *reason,
                                size_t reason_size);

// Interpolate an audiogram ear profile onto the supplied target frequencies
// (Hz) using log-frequency interpolation with smoothing. The output array must
// be at least target_count long. Returns number of written bins or negative on
// error.
int audiogram_interpolate_gain(const AudiogramEarProfile *ear,
                               const float *target_freqs, size_t target_count,
                               float *out_gain_db);

// Fit a validated audiogram profile into an EQ configuration for one ear. The
// returned configuration uses peak filters centered on an internal target grid
// with smoothed gains capped for safety.
int audiogram_build_iir_cfg(const AudiogramProfile *profile, bool left_ear,
                            IIR_CFG_T *out_cfg);

#ifdef __cplusplus
}
#endif

#endif // __AUDIOGRAM_H__
