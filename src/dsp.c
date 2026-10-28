/* SPDX-License-Identifier: GPL-3.0-only */
/* OCTODRONE's sound, after the signal flow of an organismic drone synth (integers only):
 *
 *   8 voices: triangle oscillators, SHARP folds them toward square; each pair is frequency-modulated by
 *   another pair (12<->34, 56<->78; the RING switch closes 12->34->56->78->12), by the hyper LFO, or by
 *   the instrument's own output (TOTAL FB). Sensors open an envelope (SLOW or FAST); HOLD keeps a quad
 *   sounding. VIBRATO: every voice has its own.
 *     -> MOD DELAY: two lines, cross-fed, their time modulated by themselves (SELF) or the LFO
 *     -> DISTORTION: DRIVE, MIX -> out (MASTER is the volume) */
#include "fm1.h"
#include "od.h"

#define DLEN 40000 /* a delay line: 0.9 s */
static int16_t dline[2][DLEN] POOL;

panel_t *volatile od_live;
volatile uint8_t od_gate[VOICES];
volatile int8_t od_oct;
volatile uint16_t od_level[VOICES];
volatile int16_t od_scope[240];
volatile uint32_t od_scope_seq;

typedef struct {
  uint32_t ph, vph, vinc;
  int32_t env, amp, y, drift;
} voice_t;
static voice_t V[VOICES];
static int32_t hold_lvl[2][2], pair_sig[PAIRS]; /* [quad][fast] */
static uint32_t lfo_a, lfo_b;
static int32_t tfb;               /* the last output, for TOTAL FB */
static uint32_t dw;               /* the delay lines' write position */
static int32_t dcur[2];           /* their time now, samples x 256 (it glides toward the knob) */
static int32_t dout[2], dlp[2], dmods[2];
static int32_t dist_lp[2], dc_x[2], dc_y[2], dry_x, dry_y, oc_x[2], oc_y[2];
static uint32_t scope_n, scope_dec;
static int32_t scope_last;
static uint8_t scope_armed;

static int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
static int32_t tri16(uint32_t ph) { /* a triangle over the phase, -32767 .. 32767 */
  int32_t t = (int32_t)(ph >> 16);
  return (t < 32768 ? t : 65535 - t) * 2 - 32767;
}

/* base x 2^(oct / 65536), for oct >= 0 (within ~0.2 %) */
static uint32_t pow2_scale(uint32_t base, uint32_t oct_q16) {
  uint32_t i = oct_q16 >> 16, f = oct_q16 & 0xFFFFu;
  uint32_t m = 65536u + (uint32_t)(((uint64_t)f * (43024u + ((22512u * f) >> 16))) >> 16);
  uint64_t r = ((uint64_t)base * m) >> 16;
  return (uint32_t)(r << i);
}

uint32_t od_lfo_inc(int v) { /* 0.05 Hz .. 410 Hz over 13 octaves */
  v = clampi(v, 0, 127);
  return pow2_scale(4870u, (uint32_t)v * 13u * 65536u / 127u);
}
uint32_t od_delay_samples(int v) { /* x 256: 2 ms .. 0.9 s */
  v = clampi(v, 0, 127);
  uint32_t d = pow2_scale(88u * 256u, (uint32_t)v * 577630u / 127u); /* 8.814 octaves */
  return d > (uint32_t)(DLEN - 4) * 256u ? (uint32_t)(DLEN - 4) * 256u : d;
}
uint32_t od_cents_inc(int32_t c) {
  c = clampi(c, 0, 12699);
  int n = c / 100, f = c % 100;
  uint32_t a = note_inc(n), b = note_inc(n + 1);
  uint32_t d = b - a; /* no 64-bit division on the chip */
  return a + d / 100u * (uint32_t)f + d % 100u * (uint32_t)f / 100u;
}

int od_pair_source(int p) {
  static const uint8_t TWO_LOOPS[PAIRS] = {1, 0, 3, 2}, RING[PAIRS] = {3, 0, 1, 2};
  panel_t *pn = od_live;
  return (pn && pn->v[P_RING]) ? RING[p] : TWO_LOOPS[p];
}

/* a soft clip: +-1.5 in (Q15) -> +-1 out, smooth */
static int32_t soft(int32_t x) {
  x = clampi(x, -49151, 49151);
  int32_t x3 = (int32_t)(((int64_t)x * x >> 15) * x >> 15);
  return x - x3 * 4 / 27;
}

void od_init(void) {
  for (int v = 0; v < VOICES; v++) {
    V[v].ph = random32();
    V[v].vph = random32();
    V[v].vinc = (uint32_t)((4300u + (uint32_t)v * 290u + (uint32_t)(v & 1) * 130u) * 97391u / 1000u);
  }
  for (int i = 0; i < 2; i++) dcur[i] = 256 * 4410, dmods[i] = 0;
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < DLEN; j++) dline[i][j] = 0;
}

void od_render(int32_t *out, uint32_t frames, const int32_t *in) {
  panel_t *p = od_live;
  if (!p || !frames) return;
  const int16_t *k = p->v;

  /* ---- per block: pitches, envelopes, the pairs' settings */
  int32_t T = (int32_t)(frames * 10000u / SAMPLE_RATE); /* the block, 0.1 ms */
  if (T < 1) T = 1;
  int32_t tau_slow = (int32_t)pow2_scale(50u, (uint32_t)clampi(k[P_TOUCH], 0, 127) * 478413u / 127u); /* 5 ms .. 0.8 s */
  int32_t tau_fast = tau_slow / 6 < 30 ? 30 : tau_slow / 6 > 150 ? 150 : tau_slow / 6;
  for (int q = 0; q < 2; q++) { /* HOLD: the quad's floor, as a loudness; a FAST pair needs the knob higher */
    int32_t h = clampi(k[P_HOLD + q], 0, 127);
    int32_t ts = h > 44 ? (h - 44) * 32767 / 83 : 0, tf = h > 80 ? (h - 80) * 32767 / 47 : 0;
    hold_lvl[q][0] += (ts - hold_lvl[q][0]) / 8;
    hold_lvl[q][1] += (tf - hold_lvl[q][1]) / 8;
  }
  int32_t amp0[VOICES], astep[VOICES]; /* x 256 */
  int32_t inc[VOICES];
  for (int v = 0; v < VOICES; v++) {
    voice_t *s = &V[v];
    int pr = v >> 1, q = v >> 2, fast = k[P_FAST + pr] != 0;
    /* pitch: the knob, the quad, the octave; a slow drift and the vibrato */
    int32_t c = k[P_TUNE + v] + k[P_PITCH + q] + od_oct * 1200;
    uint32_t i0 = od_cents_inc(c);
    s->drift += ((int32_t)(random32() >> 22) - 512) - s->drift / 64;
    int64_t ii = (int64_t)i0 + (((int64_t)i0 * s->drift) >> 21);
    s->vph += s->vinc * frames;
    if (k[P_VIBRATO]) ii += ((int64_t)i0 * sine(s->vph)) >> 21; /* +-1.5 % */
    inc[v] = (int32_t)ii;
    /* the envelope */
    int32_t target = od_gate[v] ? 32767 : 0, tau = od_gate[v] ? (fast ? tau_fast : tau_slow) : (fast ? 900 : 16000);
    int32_t cf = (int32_t)(65536 * T / (tau + T / 2));
    if (cf > 65536) cf = 65536;
    s->env += (int32_t)(((int64_t)(target - s->env) * cf) >> 16);
    if (!od_gate[v] && s->env < 16) s->env = 0;
    int32_t a = (int32_t)(((int64_t)s->env * s->env) >> 15), hl = hold_lvl[q][fast];
    if (hl > a) a = hl;
    amp0[v] = s->amp * 256;
    astep[v] = (a - s->amp) * 256 / (int32_t)frames;
    s->amp = a;
    od_level[v] = (uint16_t)a;
  }
  int32_t fm_abs[PAIRS], fm_rel[PAIRS], gain[PAIRS], src[PAIRS], srcp[PAIRS];
  for (int pr = 0; pr < PAIRS; pr++) {
    int32_t m = clampi(k[P_MOD + pr], 0, 127), sh = clampi(k[P_SHARP + pr], 0, 127);
    fm_abs[pr] = (m * m * 283) >> 10; /* up to ~1.5 kHz of deviation */
    fm_rel[pr] = m * m * 3 / 2;       /* up to +-74 % of the pitch */
    gain[pr] = 256 + sh * sh * 3840 / 16129;
    src[pr] = k[P_SRC + pr];
    srcp[pr] = od_pair_source(pr);
  }
  uint32_t ia = od_lfo_inc(k[P_LFO_A]), ib = od_lfo_inc(k[P_LFO_B]);
  int and_mode = k[P_LFO_AND] != 0, link = k[P_LFO_LINK] != 0, total = k[P_TOTAL_FB] != 0;
  int32_t dtgt[2], dk[2];
  for (int i = 0; i < 2; i++) {
    dtgt[i] = (int32_t)od_delay_samples(k[P_DTIME + i]);
    int32_t m = clampi(k[P_DMOD + i], 0, 127);
    dk[i] = m * m;
  }
  int dself = k[P_DSRC] == 0, dsq = k[P_DSHAPE] != 0;
  int32_t fb = clampi(k[P_DFB], 0, 127) * 340; /* 1.0 a bit past the middle, 1.32 at the top */
  int32_t mixq = clampi(k[P_DMIX], 0, 127) * 258, spq = clampi(k[P_SPREAD], 0, 127) * 258;
  int32_t drive = (int32_t)pow2_scale(256u, (uint32_t)clampi(k[P_DRIVE], 0, 127) * 393216u / 127u); /* x1 .. x64 */
  int32_t dmix = clampi(k[P_DIST_MIX], 0, 127) * 258;

  for (uint32_t n = 0; n < frames; n++) {
    /* ---- the hyper LFO: two squares, summed (OR) or ANDed; LINK: A bends B */
    lfo_a += ia;
    int32_t tri_a = tri16(lfo_a);
    lfo_b += link ? (uint32_t)((int64_t)ib + (((int64_t)ib * tri_a) >> 17)) : ib;
    int32_t tri_b = tri16(lfo_b);
    int sa = (lfo_a >> 31) ? 1 : 0, sb = (lfo_b >> 31) ? 1 : 0;
    int32_t lfo = and_mode ? (sa && sb ? 32767 : -32767) : (sa + sb - 1) * 32767;
    int32_t lfo_tri = (tri_a + tri_b) / 2;

    /* ---- the voices */
    int32_t dry = 0, ps[PAIRS] = {0, 0, 0, 0};
    for (int v = 0; v < VOICES; v++) {
      voice_t *s = &V[v];
      int pr = v >> 1;
      int64_t f = inc[v];
      if (src[pr] == SRC_FM) f += (int64_t)pair_sig[srcp[pr]] * fm_abs[pr];
      else if (src[pr] == SRC_LFO) {
        if (total) f += (int64_t)tfb * fm_abs[pr];
        else f += ((int64_t)inc[v] * ((lfo * fm_rel[pr]) >> 15)) >> 15;
      }
      if (f < 0) f = 0;
      if (f > (1 << 30)) f = 1 << 30;
      s->ph += (uint32_t)f;
      int32_t y = clampi((tri16(s->ph) * gain[pr]) >> 8, -32767, 32767);
      int32_t a = (amp0[v] + astep[v] * (int32_t)n) >> 8;
      int32_t o = (y * a) >> 15;
      s->y = y;
      dry += o, ps[pr] += o;
    }
    for (int pr = 0; pr < PAIRS; pr++) pair_sig[pr] = ps[pr] / 2;
    dry /= 4;
    { /* DC out: a voice stalled by a deep FM holds still */
      int32_t hp = dry - dry_x + (int32_t)(((int64_t)dry_y * 32700 + 16384) >> 15);
      dry_x = dry, dry_y = hp, dry = clampi(hp, -65000, 65000);
    }
    if (in) dry = clampi(dry + (in[2 * n] + in[2 * n + 1]) / 4, -65000, 65000);

    /* ---- MOD DELAY */
    for (int i = 0; i < 2; i++) {
      dcur[i] += (dtgt[i] - dcur[i]) / 4096; /* the time glides: turning TIME bends the pitch */
      int32_t m = dself ? dout[i] : dsq ? lfo : lfo_tri;
      dmods[i] += (m - dmods[i]) / 32;
      int32_t sm = (dmods[i] * dk[i]) >> 15;
      int32_t d = dcur[i] + (int32_t)(((int64_t)dcur[i] * sm * 29) >> 20);
      d = clampi(d, 256, (DLEN - 3) * 256);
      uint32_t di = (uint32_t)d >> 8, fr = (uint32_t)d & 255u;
      uint32_t r0 = dw >= di ? dw - di : dw + DLEN - di, r1 = r0 ? r0 - 1 : DLEN - 1;
      int32_t s0 = dline[i][r0], s1 = dline[i][r1];
      dout[i] = s0 + (((s1 - s0) * (int32_t)fr) >> 8);
    }
    for (int i = 0; i < 2; i++) { /* cross-fed: each line takes the other's output */
      int32_t o = (int32_t)(((int64_t)dout[1 - i] * fb) >> 15);
      dlp[i] += (o - dlp[i]) / 2;
      dline[i][dw] = (int16_t)clampi(soft(dry + dlp[i]), -32767, 32767);
    }
    dw = dw + 1 >= DLEN ? 0 : dw + 1;
    int32_t wl = (dout[0] * (32767 + spq) + dout[1] * (32767 - spq)) >> 16;
    int32_t wr = (dout[1] * (32767 + spq) + dout[0] * (32767 - spq)) >> 16;
    int32_t ch[2];
    ch[0] = ((dry * (32767 - mixq)) >> 15) + ((wl * mixq) >> 15);
    ch[1] = ((dry * (32767 - mixq)) >> 15) + ((wr * mixq) >> 15);

    /* ---- DISTORTION */
    for (int c = 0; c < 2; c++) {
      int32_t x = clampi((int32_t)(((int64_t)ch[c] * drive) >> 8) + 1500, -400000, 400000);
      int32_t y = soft(x);
      dist_lp[c] += ((y - dist_lp[c]) * 15) >> 5;
      int32_t dz = dist_lp[c] - dc_x[c] + (int32_t)(((int64_t)dc_y[c] * 32604 + 16384) >> 15); /* DC out (the bias) */
      dc_x[c] = dist_lp[c], dc_y[c] = dz;
      int32_t dd = (dz * 21000) >> 15;
      int32_t s = ((ch[c] * (32767 - dmix)) >> 15) + ((dd * dmix) >> 15);
      int32_t hp = s - oc_x[c] + (int32_t)(((int64_t)oc_y[c] * 32700 + 16384) >> 15); /* DC out (a self-bent delay) */
      oc_x[c] = s, oc_y[c] = hp;
      ch[c] = clampi(soft(hp), -32767, 32767);
    }
    tfb = (ch[0] + ch[1]) / 2;
    out[2 * n] += ch[0];
    out[2 * n + 1] += ch[1];

    /* ---- the scope: every 4th sample, from a rising zero crossing */
    if (++scope_dec >= 4) {
      scope_dec = 0;
      if (!scope_armed) {
        if ((scope_last < 0 && tfb >= 0) || ++scope_n > 2000) scope_armed = 1, scope_n = 0;
      } else {
        od_scope[scope_n] = (int16_t)tfb;
        if (++scope_n >= 240) scope_n = 0, scope_armed = 0, od_scope_seq++;
      }
      scope_last = tfb;
    }
  }
}
