#include "audiogram.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_LN10
#define M_LN10 2.302585092994046
#endif

#define MIN_POINTS_PER_EAR 2
#define AUDIOGRAM_MAX_DB_HL 120
#define AUDIOGRAM_MIN_DB_HL -20
#define AUDIOGRAM_MAX_GAIN_DB 24.0f
#define AUDIOGRAM_MIN_GAIN_DB -12.0f
#define AUDIOGRAM_HALF_GAIN 0.5f
#define SMOOTHING_WINDOW 3

static float clampf(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static float log10f_safe(float v) {
  if (v <= 0.0f) {
    return 0.0f;
  }
  return logf(v) / (float)M_LN10;
}

bool audiogram_validate_ear(const AudiogramEarProfile *ear, char *reason,
                            size_t reason_size) {
  if (!ear) {
    if (reason && reason_size)
      snprintf(reason, reason_size, "%s", "ear is NULL");
    return false;
  }
  if (ear->point_count < MIN_POINTS_PER_EAR) {
    if (reason && reason_size)
      snprintf(reason, reason_size, "%s", "not enough points");
    return false;
  }
  if (ear->point_count > AUDIOGRAM_MAX_POINTS_PER_EAR) {
    if (reason && reason_size)
      snprintf(reason, reason_size, "%s", "too many points");
    return false;
  }
  for (uint8_t i = 0; i < ear->point_count; ++i) {
    const uint16_t f = ear->frequencies_hz[i];
    const int16_t t = ear->thresholds_db_hl[i];
    if (f < AUDIOGRAM_MIN_FREQ_HZ || f > AUDIOGRAM_MAX_FREQ_HZ) {
      if (reason && reason_size)
        snprintf(reason, reason_size, "%s", "frequency out of range");
      return false;
    }
    if (t < AUDIOGRAM_MIN_DB_HL || t > AUDIOGRAM_MAX_DB_HL) {
      if (reason && reason_size)
        snprintf(reason, reason_size, "%s", "threshold out of range");
      return false;
    }
    if (i > 0) {
      if (ear->frequencies_hz[i] == ear->frequencies_hz[i - 1]) {
        if (reason && reason_size)
          snprintf(reason, reason_size, "%s", "duplicate frequency");
        return false;
      }
      if (ear->frequencies_hz[i] < ear->frequencies_hz[i - 1]) {
        if (reason && reason_size)
          snprintf(reason, reason_size, "%s", "frequencies not increasing");
        return false;
      }
    }
  }
  return true;
}

bool audiogram_validate_profile(const AudiogramProfile *profile, char *reason,
                                size_t reason_size) {
  if (!profile) {
    if (reason && reason_size)
      snprintf(reason, reason_size, "%s", "profile is NULL");
    return false;
  }
  if (profile->schema_version != 0 &&
      (profile->schema_version < AUDIOGRAM_MIN_SCHEMA_VERSION ||
       profile->schema_version > AUDIOGRAM_SCHEMA_VERSION)) {
    if (reason && reason_size)
      snprintf(reason, reason_size, "%s", "unsupported schema version");
    return false;
  }
  if (!audiogram_validate_ear(&profile->left, reason, reason_size)) {
    return false;
  }
  if (!audiogram_validate_ear(&profile->right, reason, reason_size)) {
    return false;
  }
  return true;
}

static float map_threshold_to_gain(int16_t threshold_db_hl) {
  float gain = threshold_db_hl * AUDIOGRAM_HALF_GAIN;
  return clampf(gain, AUDIOGRAM_MIN_GAIN_DB, AUDIOGRAM_MAX_GAIN_DB);
}

static void smooth_gain(float *gains, size_t count) {
  if (count < 3)
    return;
  float tmp[64];
  if (count > sizeof(tmp) / sizeof(tmp[0]))
    return;
  for (size_t i = 0; i < count; ++i) {
    float acc = 0.0f;
    float n = 0.0f;
    for (int win = -(SMOOTHING_WINDOW / 2); win <= (SMOOTHING_WINDOW / 2);
         ++win) {
      int idx = (int)i + win;
      if (idx < 0 || (size_t)idx >= count)
        continue;
      acc += gains[idx];
      n += 1.0f;
    }
    tmp[i] = acc / (n > 0.0f ? n : 1.0f);
  }
  memcpy(gains, tmp, count * sizeof(float));
}

int audiogram_interpolate_gain(const AudiogramEarProfile *ear,
                               const float *target_freqs, size_t target_count,
                               float *out_gain_db) {
  if (!ear || !target_freqs || !out_gain_db || target_count == 0)
    return -1;
  if (!audiogram_validate_ear(ear, NULL, 0))
    return -2;

  for (size_t i = 0; i < target_count; ++i) {
    float tfreq = target_freqs[i];
    float tlog = log10f_safe(tfreq);
    float lower_gain = map_threshold_to_gain(ear->thresholds_db_hl[0]);
    float upper_gain = map_threshold_to_gain(
        ear->thresholds_db_hl[ear->point_count - 1]);
    float lower_log = log10f_safe(ear->frequencies_hz[0]);
    float upper_log =
        log10f_safe(ear->frequencies_hz[ear->point_count - 1]);

    if (tfreq <= ear->frequencies_hz[0]) {
      out_gain_db[i] = lower_gain;
      continue;
    }
    if (tfreq >= ear->frequencies_hz[ear->point_count - 1]) {
      out_gain_db[i] = upper_gain;
      continue;
    }

    for (uint8_t j = 1; j < ear->point_count; ++j) {
      uint16_t f0 = ear->frequencies_hz[j - 1];
      uint16_t f1 = ear->frequencies_hz[j];
      if (tfreq >= f0 && tfreq <= f1) {
        float log0 = log10f_safe(f0);
        float log1 = log10f_safe(f1);
        float g0 = map_threshold_to_gain(ear->thresholds_db_hl[j - 1]);
        float g1 = map_threshold_to_gain(ear->thresholds_db_hl[j]);
        float ratio =
            (tlog - log0) / ((log1 - log0) > 0.0f ? (log1 - log0) : 1.0f);
        out_gain_db[i] = g0 + (g1 - g0) * ratio;
        break;
      }
    }
  }

  smooth_gain(out_gain_db, target_count);
  return (int)target_count;
}

int audiogram_build_iir_cfg(const AudiogramProfile *profile, bool left_ear,
                            IIR_CFG_T *out_cfg) {
  if (!profile || !out_cfg)
    return -1;
  if (!audiogram_validate_profile(profile, NULL, 0))
    return -2;

  static const float target_grid[] = {125.f,  250.f,  500.f,  750.f,  1000.f,
                                      1500.f, 2000.f, 3000.f, 4000.f, 6000.f,
                                      8000.f, 10000.f};
  const size_t target_count = sizeof(target_grid) / sizeof(target_grid[0]);
  float gains[target_count];
  const AudiogramEarProfile *ear = left_ear ? &profile->left : &profile->right;
  int written =
      audiogram_interpolate_gain(ear, target_grid, target_count, gains);
  if (written < 0)
    return written;

  memset(out_cfg, 0, sizeof(*out_cfg));
  out_cfg->gain0 = 0.0f;
  out_cfg->gain1 = 0.0f;
  out_cfg->num = (int)target_count;
  if (out_cfg->num > IIR_PARAM_NUM)
    out_cfg->num = IIR_PARAM_NUM;

  for (int i = 0; i < out_cfg->num; ++i) {
    out_cfg->param[i].type = IIR_TYPE_PEAK;
    out_cfg->param[i].gain = gains[i];
    out_cfg->param[i].fc = target_grid[i];
    out_cfg->param[i].Q = 1.2f;
  }
  return 0;
}
