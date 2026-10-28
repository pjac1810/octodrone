/* SPDX-License-Identifier: GPL-3.0-only */
/* OCTODRONE's eight factory panels. Tunings in cents above MIDI note 0 (A1 = 3300, A2 = 4500 ...). */
#include "od.h"

#define TUNES(a, b, c, d, e, f, g, h) \
  [P_TUNE] = a, [P_TUNE + 1] = b, [P_TUNE + 2] = c, [P_TUNE + 3] = d, [P_TUNE + 4] = e, [P_TUNE + 5] = f, [P_TUNE + 6] = g, [P_TUNE + 7] = h
#define TWO(f, a, b) [f] = a, [f + 1] = b
#define FOUR(f, a, b, c, d) [f] = a, [f + 1] = b, [f + 2] = c, [f + 3] = d

panel_t PANEL[PRESETS];

const panel_t FACTORY[PRESETS] = {
    {"DRONE",
     {TUNES(3300, 4000, 4500, 5200, 5700, 6000, 6400, 6900), TWO(P_PITCH, 0, 0), FOUR(P_MOD, 0, 0, 0, 0),
      FOUR(P_SHARP, 20, 30, 40, 30), FOUR(P_SRC, SRC_OFF, SRC_OFF, SRC_OFF, SRC_OFF), FOUR(P_FAST, 0, 0, 0, 0),
      TWO(P_HOLD, 0, 0), [P_LFO_A] = 40, [P_LFO_B] = 55, [P_LFO_AND] = 0, [P_LFO_LINK] = 0, TWO(P_DTIME, 70, 85),
      TWO(P_DMOD, 10, 15), [P_DSRC] = 1, [P_DSHAPE] = 0, [P_DFB] = 50, [P_DMIX] = 45, [P_DRIVE] = 0, [P_DIST_MIX] = 0,
      [P_VIBRATO] = 0, [P_TOTAL_FB] = 0, [P_RING] = 0, [P_SPREAD] = 50, [P_TOUCH] = 60}},
    {"CATHEDRAL",
     {TUNES(2600, 3300, 3800, 4500, 5000, 5300, 5700, 6200), TWO(P_PITCH, 0, 0), FOUR(P_MOD, 14, 10, 0, 8),
      FOUR(P_SHARP, 10, 18, 26, 34), FOUR(P_SRC, SRC_LFO, SRC_FM, SRC_OFF, SRC_LFO), FOUR(P_FAST, 0, 0, 0, 0),
      TWO(P_HOLD, 72, 0), [P_LFO_A] = 22, [P_LFO_B] = 31, [P_LFO_AND] = 0, [P_LFO_LINK] = 1, TWO(P_DTIME, 108, 120),
      TWO(P_DMOD, 18, 24), [P_DSRC] = 1, [P_DSHAPE] = 0, [P_DFB] = 68, [P_DMIX] = 62, [P_DRIVE] = 10, [P_DIST_MIX] = 20,
      [P_VIBRATO] = 1, [P_TOTAL_FB] = 0, [P_RING] = 0, [P_SPREAD] = 90, [P_TOUCH] = 85}},
    {"SWARM",
     {TUNES(3600, 3612, 4800, 4807, 5500, 5512, 6000, 6709), TWO(P_PITCH, 0, 0), FOUR(P_MOD, 42, 36, 30, 46),
      FOUR(P_SHARP, 50, 40, 60, 45), FOUR(P_SRC, SRC_FM, SRC_FM, SRC_FM, SRC_FM), FOUR(P_FAST, 0, 0, 0, 0),
      TWO(P_HOLD, 0, 0), [P_LFO_A] = 50, [P_LFO_B] = 63, [P_LFO_AND] = 0, [P_LFO_LINK] = 0, TWO(P_DTIME, 76, 92),
      TWO(P_DMOD, 30, 26), [P_DSRC] = 0, [P_DSHAPE] = 0, [P_DFB] = 56, [P_DMIX] = 48, [P_DRIVE] = 30, [P_DIST_MIX] = 40,
      [P_VIBRATO] = 1, [P_TOTAL_FB] = 0, [P_RING] = 1, [P_SPREAD] = 70, [P_TOUCH] = 55}},
    {"APOCALYPSE",
     {TUNES(3100, 3800, 4300, 5000, 5500, 5800, 6200, 6700), TWO(P_PITCH, 0, 0), FOUR(P_MOD, 46, 40, 52, 36),
      FOUR(P_SHARP, 80, 70, 90, 60), FOUR(P_SRC, SRC_LFO, SRC_FM, SRC_LFO, SRC_FM), FOUR(P_FAST, 0, 0, 0, 0),
      TWO(P_HOLD, 0, 0), [P_LFO_A] = 60, [P_LFO_B] = 72, [P_LFO_AND] = 1, [P_LFO_LINK] = 1, TWO(P_DTIME, 60, 75),
      TWO(P_DMOD, 40, 50), [P_DSRC] = 0, [P_DSHAPE] = 1, [P_DFB] = 74, [P_DMIX] = 55, [P_DRIVE] = 105, [P_DIST_MIX] = 100,
      [P_VIBRATO] = 0, [P_TOTAL_FB] = 1, [P_RING] = 0, [P_SPREAD] = 60, [P_TOUCH] = 40}},
    {"HYPER PULSE",
     {TUNES(3300, 4500, 5200, 5700, 6000, 6400, 6900, 7600), TWO(P_PITCH, 0, 0), FOUR(P_MOD, 64, 0, 52, 20),
      FOUR(P_SHARP, 100, 60, 110, 80), FOUR(P_SRC, SRC_LFO, SRC_OFF, SRC_LFO, SRC_LFO), FOUR(P_FAST, 1, 0, 1, 0),
      TWO(P_HOLD, 0, 0), [P_LFO_A] = 70, [P_LFO_B] = 80, [P_LFO_AND] = 1, [P_LFO_LINK] = 1, TWO(P_DTIME, 66, 80),
      TWO(P_DMOD, 22, 30), [P_DSRC] = 1, [P_DSHAPE] = 1, [P_DFB] = 52, [P_DMIX] = 40, [P_DRIVE] = 40, [P_DIST_MIX] = 50,
      [P_VIBRATO] = 0, [P_TOTAL_FB] = 0, [P_RING] = 0, [P_SPREAD] = 80, [P_TOUCH] = 20}},
    {"TAPE BENDS",
     {TUNES(3300, 4000, 4500, 4900, 5200, 5700, 6400, 6900), TWO(P_PITCH, 0, 0), FOUR(P_MOD, 0, 8, 0, 6),
      FOUR(P_SHARP, 30, 20, 40, 25), FOUR(P_SRC, SRC_OFF, SRC_LFO, SRC_OFF, SRC_LFO), FOUR(P_FAST, 0, 0, 0, 0),
      TWO(P_HOLD, 0, 0), [P_LFO_A] = 30, [P_LFO_B] = 38, [P_LFO_AND] = 0, [P_LFO_LINK] = 0, TWO(P_DTIME, 40, 56),
      TWO(P_DMOD, 70, 60), [P_DSRC] = 0, [P_DSHAPE] = 0, [P_DFB] = 66, [P_DMIX] = 70, [P_DRIVE] = 20, [P_DIST_MIX] = 30,
      [P_VIBRATO] = 0, [P_TOTAL_FB] = 0, [P_RING] = 0, [P_SPREAD] = 100, [P_TOUCH] = 50}},
    {"BELLS",
     {TUNES(3600, 4300, 6200, 6900, 7400, 7600, 7900, 8100), TWO(P_PITCH, 0, 0), FOUR(P_MOD, 0, 30, 0, 24),
      FOUR(P_SHARP, 0, 0, 10, 0), FOUR(P_SRC, SRC_OFF, SRC_FM, SRC_OFF, SRC_FM), FOUR(P_FAST, 1, 1, 1, 1),
      TWO(P_HOLD, 0, 0), [P_LFO_A] = 45, [P_LFO_B] = 50, [P_LFO_AND] = 0, [P_LFO_LINK] = 0, TWO(P_DTIME, 92, 104),
      TWO(P_DMOD, 12, 16), [P_DSRC] = 1, [P_DSHAPE] = 0, [P_DFB] = 60, [P_DMIX] = 50, [P_DRIVE] = 0, [P_DIST_MIX] = 0,
      [P_VIBRATO] = 1, [P_TOTAL_FB] = 0, [P_RING] = 0, [P_SPREAD] = 110, [P_TOUCH] = 10}},
    {"INIT",
     {TUNES(3300, 4000, 4500, 5200, 5700, 6400, 6900, 7600), TWO(P_PITCH, 0, 0), FOUR(P_MOD, 0, 0, 0, 0),
      FOUR(P_SHARP, 0, 0, 0, 0), FOUR(P_SRC, SRC_OFF, SRC_OFF, SRC_OFF, SRC_OFF), FOUR(P_FAST, 0, 0, 0, 0),
      TWO(P_HOLD, 0, 0), [P_LFO_A] = 50, [P_LFO_B] = 60, [P_LFO_AND] = 0, [P_LFO_LINK] = 0, TWO(P_DTIME, 80, 90),
      TWO(P_DMOD, 0, 0), [P_DSRC] = 1, [P_DSHAPE] = 0, [P_DFB] = 30, [P_DMIX] = 0, [P_DRIVE] = 0, [P_DIST_MIX] = 0,
      [P_VIBRATO] = 0, [P_TOTAL_FB] = 0, [P_RING] = 0, [P_SPREAD] = 40, [P_TOUCH] = 40}},
};
