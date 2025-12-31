#include "dsp_chain.h"
#include "hal_timer.h"
#include <math.h>
#include <string.h>

#ifndef M_LN10
#define M_LN10 2.302585092994046
#endif

static float db_to_linear(float db) { return powf(10.0f, db / 20.0f); }

static void reset_counters(DspChainCounters *counters) {
  memset(counters, 0, sizeof(*counters));
}

static void reset_ramp(DspChainRamp *ramp, float headroom_db) {
  ramp->current_gain_db = 0.0f;
  ramp->target_gain_db = 0.0f;
  ramp->step_db = 0.0f;
  ramp->remaining_frames = 0;
  ramp->headroom_db = headroom_db;
  ramp->applied_gain_db = -headroom_db;
}

bool dsp_chain_init(DspChainState *state, const char **stage_order,
                    uint8_t stage_count, float headroom_db) {
  if (!state || !stage_order || stage_count == 0 ||
      stage_count > DSP_CHAIN_MAX_STAGES)
    return false;

  memset(state, 0, sizeof(*state));
  for (uint8_t i = 0; i < stage_count; ++i) {
    state->stages[i] = stage_order[i];
  }
  state->stage_count = stage_count;
  state->limiter_last = stage_order[stage_count - 1] &&
                        strcmp(stage_order[stage_count - 1], "limiter") == 0;
  reset_ramp(&state->ramp, headroom_db);
  reset_counters(&state->counters);
  return state->limiter_last;
}

float dsp_chain_begin_frame(DspChainState *state, uint32_t frame_samples) {
  if (!state)
    return 1.0f;

  state->frame_samples = frame_samples;
  state->frame_start_fast_ticks = hal_fast_sys_timer_get();
  state->counters.frames_processed++;
  state->ramp.applied_gain_db = state->ramp.current_gain_db -
                                state->ramp.headroom_db;

  if (state->ramp.remaining_frames) {
    state->ramp.current_gain_db += state->ramp.step_db;
    state->ramp.remaining_frames--;
  }

  return db_to_linear(state->ramp.applied_gain_db);
}

void dsp_chain_finish_frame(DspChainState *state) {
  if (!state)
    return;

  uint32_t end_ticks = hal_fast_sys_timer_get();
  uint32_t elapsed_ticks = end_ticks - state->frame_start_fast_ticks;
  state->counters.total_cpu_cycles += elapsed_ticks;
  state->counters.last_frame_us = FAST_TICKS_TO_US(elapsed_ticks);
}

void dsp_chain_request_ramp(DspChainState *state, float target_gain_db,
                            uint32_t frame_count) {
  if (!state)
    return;

  state->ramp.target_gain_db = target_gain_db;
  if (frame_count == 0) {
    state->ramp.current_gain_db = target_gain_db;
    state->ramp.remaining_frames = 0;
    state->ramp.step_db = 0.0f;
    return;
  }
  state->ramp.remaining_frames = frame_count;
  state->ramp.step_db =
      (target_gain_db - state->ramp.current_gain_db) / (float)frame_count;
}

void dsp_chain_force_mute(DspChainState *state) {
  if (!state)
    return;
  state->ramp.current_gain_db = -80.0f;
  state->ramp.target_gain_db = -80.0f;
  state->ramp.remaining_frames = 0;
  state->ramp.step_db = 0.0f;
}

void dsp_chain_mark_limiter(DspChainState *state) {
  if (!state)
    return;
  state->counters.limiter_engaged++;
}

void dsp_chain_mark_clipping(DspChainState *state, uint32_t clipped) {
  if (!state)
    return;
  state->counters.clipped_samples += clipped;
}

void dsp_chain_mark_underflow(DspChainState *state) {
  if (!state)
    return;
  state->counters.underflow_events++;
}

void dsp_chain_mark_overflow(DspChainState *state) {
  if (!state)
    return;
  state->counters.overflow_events++;
}

void dsp_chain_get_counters(const DspChainState *state, DspChainCounters *out) {
  if (!state || !out)
    return;
  memcpy(out, &state->counters, sizeof(*out));
}

