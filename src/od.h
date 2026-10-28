/* SPDX-License-Identifier: GPL-3.0-only */
/* OCTODRONE: shared definitions. Eight voices in four pairs (1-2, 3-4, 5-6, 7-8) and two quads (1-4, 5-8). */
#ifndef OD_H
#define OD_H
#include <stdint.h>

#define VOICES 8
#define PAIRS 4
#define PRESETS 8

/* the panel: every knob and switch, in one array (a preset) */
enum {
  P_TUNE,                /* 8 voices: pitch in cents above MIDI note 0 */
  P_PITCH = P_TUNE + 8,  /* 2 quads: transposition, cents */
  P_MOD = P_PITCH + 2,   /* 4 pairs: modulation depth 0..127 */
  P_SHARP = P_MOD + 4,   /* 4 pairs: triangle .. square 0..127 */
  P_SRC = P_SHARP + 4,   /* 4 pairs: SRC_* */
  P_FAST = P_SRC + 4,    /* 4 pairs: 0 slow, 1 fast */
  P_HOLD = P_FAST + 4,   /* 2 quads: 0..127 */
  P_LFO_A = P_HOLD + 2,  /* hyper LFO: 0..127 (0.05 .. 400 Hz) */
  P_LFO_B,
  P_LFO_AND,             /* 0 OR (sum), 1 AND */
  P_LFO_LINK,            /* A modulates B */
  P_DTIME,               /* 2 lines: 0..127 (2 ms .. 0.9 s) */
  P_DMOD = P_DTIME + 2,  /* 2 lines: 0..127 */
  P_DSRC = P_DMOD + 2,   /* 0 SELF, 1 LFO */
  P_DSHAPE,              /* 0 triangle, 1 square */
  P_DFB,                 /* 0..127, self-oscillates past the middle */
  P_DMIX,
  P_DRIVE,
  P_DIST_MIX,
  P_VIBRATO,             /* 0/1 */
  P_TOTAL_FB,            /* 0/1: the output replaces the LFO */
  P_RING,                /* 0/1: the 34>56 switch, the four pairs in one loop */
  P_SPREAD,              /* 0..127: the delay lines left and right */
  P_TOUCH,               /* 0..127: how fast a touched sensor opens */
  P_COUNT
};
enum { SRC_OFF, SRC_FM, SRC_LFO };

typedef struct {
  char name[12];
  int16_t v[P_COUNT];
} panel_t;

extern panel_t PANEL[PRESETS];
extern const panel_t FACTORY[PRESETS];

/* dsp.c */
extern panel_t *volatile od_live; /* the panel the sound follows */
extern volatile uint8_t od_gate[VOICES]; /* the sensors touched (from loop) */
extern volatile int8_t od_oct;           /* global octave */
extern volatile uint16_t od_level[VOICES]; /* each voice's loudness, for the screen */
extern volatile int16_t od_scope[240];     /* the last output, for the screen */
extern volatile uint32_t od_scope_seq;     /* +1 each time the scope is filled */
void od_init(void);
void od_render(int32_t *out, uint32_t frames, const int32_t *in);
int od_pair_source(int pair); /* the pair that FM-modulates this one */
uint32_t od_lfo_inc(int v);   /* phase step of the hyper LFO at knob v */
uint32_t od_delay_samples(int v);
uint32_t od_cents_inc(int32_t cents);

#endif
