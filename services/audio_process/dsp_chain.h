#pragma once

#include <stdbool.h>
#include <stdint.h>

// Maximum number of processing stages tracked in the chain telemetry.
#define DSP_CHAIN_MAX_STAGES 8

// Telemetry counters used for on-device reporting.
typedef struct {
  uint32_t frames_processed;
  uint32_t limiter_engaged;
  uint32_t clipped_samples;
  uint32_t underflow_events;
  uint32_t overflow_events;
  uint64_t total_cpu_cycles;
  uint32_t last_frame_us;
} DspChainCounters;

// Block-level ramping and headroom state. Gains are stored in dB to make it
// clear how much headroom is being reserved when stacking EQ and mode overlays.
typedef struct {
  float current_gain_db;
  float target_gain_db;
  float step_db;
  uint32_t remaining_frames;
  float headroom_db;
  float applied_gain_db;
} DspChainRamp;

typedef struct {
  const char *stages[DSP_CHAIN_MAX_STAGES];
  uint8_t stage_count;
  bool limiter_last;
  DspChainRamp ramp;
  DspChainCounters counters;
  uint32_t frame_samples;
  uint32_t frame_start_fast_ticks;
} DspChainState;

// Initialize the chain with an explicit stage order (limiter must be last).
bool dsp_chain_init(DspChainState *state, const char **stage_order,
                    uint8_t stage_count, float headroom_db);

// Prepare telemetry for a new audio block and return the linear gain that
// should be applied for ramping + headroom management.
float dsp_chain_begin_frame(DspChainState *state, uint32_t frame_samples);

// Complete telemetry for the current frame using the fast timer.
void dsp_chain_finish_frame(DspChainState *state);

// Ramping utilities.
void dsp_chain_request_ramp(DspChainState *state, float target_gain_db,
                            uint32_t frame_count);
void dsp_chain_force_mute(DspChainState *state);

// Telemetry helpers.
void dsp_chain_mark_limiter(DspChainState *state);
void dsp_chain_mark_clipping(DspChainState *state, uint32_t clipped);
void dsp_chain_mark_underflow(DspChainState *state);
void dsp_chain_mark_overflow(DspChainState *state);
void dsp_chain_get_counters(const DspChainState *state, DspChainCounters *out);

