/* SPDX-License-Identifier: GPL-3.0-only */
/* OCTODRONE: an organismic 8-voice drone synth for the M-VAVE FM-1 (README.md has the full map).
 *
 *   keys        the 8 sensors: the white keys F3..F4 are voices 1..8, and again G4..G5; a black key touches
 *               its two white neighbours
 *   SELECT      the pair (1-2, 3-4, 5-6, 7-8)          ALGORITHM  the pair's modulation: OFF, FM, LFO
 *   PRESETS     the panel P1..P8                      MASTER     volume
 *   HOME        PAIR: TUNE, TUNE, MOD, SHARP          ENV        QUADS: HOLD 1-4, HOLD 5-8, PITCH 1-4, PITCH 5-8
 *   LFO         HYPER LFO: FREQ A, FREQ B, AND/OR, LINK
 *   FX          DELAY: TIME 1, TIME 2, FEEDBACK, MIX (again: DELAY MOD: MOD 1, MOD 2, SELF/LFO, TRI/SQR)
 *   EDIT        DISTORTION: DRIVE, MIX, SPREAD, TOUCH
 *   SEL FAST (the pair)   GLO TOTAL FB   ARP VIBRATO   SEQ 34>56 (the four pairs in one loop)
 *   PLAY        LATCH: a key keeps its sensor touched  REC  let go of everything latched
 *   SAVE        keep the 8 panels (held 2 s: the factory panel back in this slot)
 *   OCT- OCT+   the octave of all the voices */
#include "fm1.h"
#include "od.h"

/* ---------------------------------------------------------------- colours */

#define C_BG RGB(14, 13, 12)
#define C_PANEL RGB(36, 34, 31)
#define C_TEXT RGB(232, 226, 214)
#define C_DIM RGB(128, 122, 112)
#define C_FAINT RGB(56, 52, 47)
#define C_AMBER RGB(255, 168, 40)
#define C_CYAN RGB(90, 200, 230)
#define C_RED RGB(240, 70, 50)
#define C_PLATE RGB(70, 72, 76)

static int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
static char *put(char *d, const char *s) {
  while (*s) *d++ = *s++;
  *d = 0;
  return d;
}
static char *put_int(char *d, int32_t v) {
  char b[12];
  format_int(b, v);
  return put(d, b);
}
static char *put_2(char *d, int32_t v) { /* two digits */
  *d++ = (char)('0' + (v / 10) % 10), *d++ = (char)('0' + v % 10), *d = 0;
  return d;
}
static int center_text(int cx, int y, const char *s, uint16_t c, int font) {
  return text(cx - text_width(s, font) / 2, y, s, c, font);
}

/* ---------------------------------------------------------------- the knobs and their pages */

enum { R_ABS, R_PAIR, R_VA, R_VB }; /* the param: as is, + the pair, + its first voice, + its second */
enum { F_NUM, F_TUNE, F_CENTS, F_HZ, F_MS, F_SW };
typedef struct {
  const char *label;
  int8_t param, rel, fmt;
  int16_t lo, hi;
  const char *sw0, *sw1; /* a switch's two names */
} knob_t;

enum { PG_PAIR, PG_QUAD, PG_LFO, PG_DELAY, PG_DMOD, PG_DIST, PG_COUNT };
static const char *const PAGE_NAMES[PG_COUNT] = {"PAIR", "QUADS", "HYPER LFO", "MOD DELAY", "DELAY MOD", "DISTORTION"};
static const knob_t KNOB[PG_COUNT][4] = {
    {{"TUNE", P_TUNE, R_VA, F_TUNE, 0, 0, 0, 0}, {"TUNE", P_TUNE, R_VB, F_TUNE, 0, 0, 0, 0},
     {"MOD", P_MOD, R_PAIR, F_NUM, 0, 127, 0, 0}, {"SHARP", P_SHARP, R_PAIR, F_NUM, 0, 127, 0, 0}},
    {{"HOLD1-4", P_HOLD, R_ABS, F_NUM, 0, 127, 0, 0}, {"HOLD5-8", P_HOLD + 1, R_ABS, F_NUM, 0, 127, 0, 0},
     {"PTCH1-4", P_PITCH, R_ABS, F_CENTS, -2400, 1200, 0, 0}, {"PTCH5-8", P_PITCH + 1, R_ABS, F_CENTS, -2400, 1200, 0, 0}},
    {{"FREQ A", P_LFO_A, R_ABS, F_HZ, 0, 127, 0, 0}, {"FREQ B", P_LFO_B, R_ABS, F_HZ, 0, 127, 0, 0},
     {"MODE", P_LFO_AND, R_ABS, F_SW, 0, 1, "OR", "AND"}, {"LINK", P_LFO_LINK, R_ABS, F_SW, 0, 1, "OFF", "ON"}},
    {{"TIME 1", P_DTIME, R_ABS, F_MS, 0, 127, 0, 0}, {"TIME 2", P_DTIME + 1, R_ABS, F_MS, 0, 127, 0, 0},
     {"FB", P_DFB, R_ABS, F_NUM, 0, 127, 0, 0}, {"MIX", P_DMIX, R_ABS, F_NUM, 0, 127, 0, 0}},
    {{"MOD 1", P_DMOD, R_ABS, F_NUM, 0, 127, 0, 0}, {"MOD 2", P_DMOD + 1, R_ABS, F_NUM, 0, 127, 0, 0},
     {"SOURCE", P_DSRC, R_ABS, F_SW, 0, 1, "SELF", "LFO"}, {"SHAPE", P_DSHAPE, R_ABS, F_SW, 0, 1, "TRI", "SQR"}},
    {{"DRIVE", P_DRIVE, R_ABS, F_NUM, 0, 127, 0, 0}, {"MIX", P_DIST_MIX, R_ABS, F_NUM, 0, 127, 0, 0},
     {"SPREAD", P_SPREAD, R_ABS, F_NUM, 0, 127, 0, 0}, {"TOUCH", P_TOUCH, R_ABS, F_NUM, 0, 127, 0, 0}},
};

static uint8_t cur, pair, page = PG_PAIR;

/* the tuning range of each voice, cents: 1-2 bass, 3-6 middle, 7-8 an octave up */
static const int16_t TUNE_LO[VOICES] = {2400, 2400, 3600, 3600, 3600, 3600, 4800, 4800};
static const int16_t TUNE_HI[VOICES] = {7200, 7200, 8400, 8400, 8400, 8400, 9600, 9600};

static int knob_param(const knob_t *k) {
  return k->param + (k->rel == R_PAIR ? pair : k->rel == R_VA ? 2 * pair : k->rel == R_VB ? 2 * pair + 1 : 0);
}
static void knob_range(const knob_t *k, int32_t *lo, int32_t *hi) {
  if (k->fmt == F_TUNE) {
    int v = knob_param(k) - P_TUNE;
    *lo = TUNE_LO[v], *hi = TUNE_HI[v];
  } else {
    *lo = k->lo, *hi = k->hi;
  }
}
static int32_t knob_get(const knob_t *k) { return PANEL[cur].v[knob_param(k)]; }
static void knob_turn(const knob_t *k, int32_t e) {
  int32_t lo, hi, a = e < 0 ? -e : e, d = e > 0 ? 1 : -1, v = knob_get(k), step;
  knob_range(k, &lo, &hi);
  if (k->fmt == F_SW) v = e > 0 ? hi : lo, step = 0;
  else if (k->fmt == F_TUNE) step = a > 1 ? 25 * a : 5;
  else if (k->fmt == F_CENTS) step = a > 1 ? 50 * a : 10;
  else step = a > 1 ? 4 * a : 1;
  PANEL[cur].v[knob_param(k)] = (int16_t)clampi(v + d * step, lo, hi);
}

static void note_name(char *d, int32_t c) { /* A1+12 */
  static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  int32_t n = (c + 50) / 100, dev = c - n * 100;
  d = put_int(put(d, NN[n % 12]), n / 12 - 1);
  if (dev) put_int(put(d, dev > 0 ? "+" : ""), dev);
}
static void knob_text(char *d, const knob_t *k) {
  int32_t v = knob_get(k);
  switch (k->fmt) {
    case F_TUNE: note_name(d, v); break;
    case F_CENTS: { /* semitones, one decimal */
      int32_t a = v < 0 ? -v : v;
      d = put(d, v < 0 ? "-" : v > 0 ? "+" : "");
      d = put_int(d, a / 100), *d++ = '.', *d = 0;
      put_int(d, (a % 100) / 10);
      break;
    }
    case F_HZ: {
      uint32_t h = od_lfo_inc(v) / 974u; /* Hz x 100 */
      if (h < 1000) d = put_2(put(put_int(d, (int32_t)(h / 100)), "."), (int32_t)(h % 100));
      else if (h < 10000) d = put_int(put(put_int(d, (int32_t)(h / 100)), "."), (int32_t)(h / 10 % 10));
      else d = put_int(d, (int32_t)(h / 100));
      put(d, "Hz");
      break;
    }
    case F_MS: put(put_int(d, (int32_t)(od_delay_samples(v) / 11290u)), "ms"); break;
    case F_SW: put(d, v ? k->sw1 : k->sw0); break;
    default: put_int(d, v); break;
  }
}

/* ---------------------------------------------------------------- the keys: sensors */

static uint8_t key_mask[KEY_COUNT];   /* the voices a key touches */
static uint8_t touch_n[VOICES], latch; /* keys and MIDI notes on each voice; voices latched */
static uint8_t latch_mode;

static int white_index(int note) { /* white keys from C-1, black keys -1 */
  static const int8_t W[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
  return W[note % 12] < 0 ? -1 : (note / 12) * 7 + W[note % 12];
}
static uint8_t note_mask(int note) {
  int f = white_index(KEY_FIRST_NOTE), w = white_index(note);
  if (w >= 0) return (uint8_t)(1u << (((w - f) % 8 + 8) % 8));
  return (uint8_t)(note_mask(note - 1) | note_mask(note + 1));
}
static void touch(uint8_t m, int on) {
  for (int v = 0; v < VOICES; v++)
    if (m >> v & 1u) {
      if (on) touch_n[v]++;
      else if (touch_n[v]) touch_n[v]--;
    }
}
static void update_gates(void) {
  for (int v = 0; v < VOICES; v++) od_gate[v] = (uint8_t)(touch_n[v] || (latch >> v & 1u));
}

/* ---------------------------------------------------------------- state */

#define STORE_MAGIC (0x4F443100u + P_COUNT) /* "OD1" and the panel's size */
static struct {
  uint32_t magic;
  panel_t pan[PRESETS];
} store;
_Static_assert(sizeof store <= SETTINGS_MAX, "the panels must fit the settings' flash");

static uint32_t saved_t, save_down, restored_t;
static uint8_t save_done;
static uint32_t sh_title = ~0u, sh_pairs = ~0u, sh_sens = ~0u, sh_knobs = ~0u, sh_foot = ~0u, scope_seen, scope_t;
static uint32_t led_k = ~0u, led_b = ~0u;
static uint8_t drawn;
static int8_t span13[27]; /* a disc's half widths */

/* ---------------------------------------------------------------- drawing */

#define SCOPE_Y 20
#define SCOPE_H 48
#define PAIR_Y 72
#define SENS_Y 134 /* the sensors' centre */
#define KNOB_Y 158
#define FOOT_Y 202

static void draw_title(void) {
  rect(0, 0, SCREEN_W, 18, C_PANEL);
  text(6, 1, "OCTODRONE", C_AMBER, FONT_SMALL);
  char b[20], *d = put(b, "P");
  *d++ = (char)('1' + cur), *d = 0;
  put(put(d, " "), PANEL[cur].name);
  text(SCREEN_W - 6 - text_width(b, FONT_SMALL), 1, b, C_TEXT, FONT_SMALL);
}

static void draw_scope(void) {
  rect(0, SCOPE_Y, SCREEN_W, SCOPE_H, C_BG);
  for (int x = 0; x < SCREEN_W; x += 8) pixel(x, SCOPE_Y + SCOPE_H / 2, C_FAINT);
  int c = SCOPE_Y + SCOPE_H / 2, py = c;
  for (int x = 0; x < SCREEN_W; x++) {
    int y = c - ((int)od_scope[x] * (SCOPE_H / 2 - 1)) / 32768;
    int a = y < py ? y : py, b = y < py ? py : y;
    if (x == 0) a = b = y;
    rect(x, a, 1, b - a + 1, C_AMBER);
    py = y;
  }
}

static void draw_pairs(void) {
  panel_t *p = &PANEL[cur];
  rect(0, PAIR_Y - 2, SCREEN_W, 42, C_BG);
  for (int i = 0; i < PAIRS; i++) {
    int x = 2 + i * 60, sel = i == pair;
    rect(x, PAIR_Y, 56, 38, sel ? C_PANEL : RGB(24, 23, 21));
    if (sel) frame(x, PAIR_Y, 56, 38, C_AMBER);
    char b[12] = {(char)('1' + 2 * i), '-', (char)('2' + 2 * i), 0};
    text(x + 4, PAIR_Y + 2, b, sel ? C_AMBER : C_TEXT, FONT_SMALL);
    text(x + 52 - text_width(p->v[P_FAST + i] ? "F" : "S", FONT_SMALL), PAIR_Y + 2, p->v[P_FAST + i] ? "F" : "S",
         p->v[P_FAST + i] ? C_RED : C_DIM, FONT_SMALL);
    int s = p->v[P_SRC + i];
    if (s == SRC_FM) {
      int q = od_pair_source(i);
      char f[8] = {'F', 'M', ' ', (char)('1' + 2 * q), (char)('2' + 2 * q), 0};
      put(b, f);
    } else {
      put(b, s == SRC_LFO ? (p->v[P_TOTAL_FB] ? "T.FB" : "LFO") : "OFF");
    }
    text(x + 4, PAIR_Y + 17, b, s == SRC_OFF ? C_DIM : s == SRC_FM ? C_CYAN : C_AMBER, FONT_SMALL);
    rect(x + 4, PAIR_Y + 34, 48, 2, C_FAINT);
    rect(x + 4, PAIR_Y + 34, p->v[P_MOD + i] * 48 / 127, 2, C_CYAN);
  }
}

static uint16_t level_colour(int l) { /* 0..15: a dark plate .. glowing */
  return RGB(70 + l * 185 / 15, 72 + l * 96 / 15, 76 - l * 40 / 15);
}
static int sensor_x(int v) { return v < 4 ? 17 + v * 30 : 133 + (v - 4) * 30; }
static void draw_sensor(int v, int l) {
  int cx = sensor_x(v);
  rect(cx - 14, SENS_Y - 14, 29, 29, C_BG);
  uint16_t c = level_colour(l);
  for (int dy = -13; dy <= 13; dy++) rect(cx - span13[dy + 13], SENS_Y + dy, 2 * span13[dy + 13] + 1, 1, c);
  char b[2] = {(char)('1' + v), 0};
  center_text(cx + 1, SENS_Y - 8, b, l > 9 ? C_BG : C_TEXT, FONT_SMALL);
  rect(cx - 6, SENS_Y + 16, 13, 3, (latch >> v & 1u) ? C_AMBER : C_BG);
}

static void draw_knobs(void) {
  rect(0, KNOB_Y - 2, SCREEN_W, FOOT_Y - KNOB_Y + 1, C_BG);
  for (int i = 0; i < 4; i++) {
    const knob_t *k = &KNOB[page][i];
    int cx = i * 60 + 30;
    char b[16];
    if (k->fmt == F_TUNE) put_int(put(b, "TUNE "), knob_param(k) - P_TUNE + 1);
    else put(b, k->label);
    center_text(cx, KNOB_Y, b, C_DIM, FONT_SMALL);
    knob_text(b, k);
    center_text(cx, KNOB_Y + 16, b, C_TEXT, FONT_SMALL);
    int32_t lo, hi, v = knob_get(k), w = 46;
    knob_range(k, &lo, &hi);
    rect(cx - w / 2, KNOB_Y + 35, w, 3, C_FAINT);
    rect(cx - w / 2, KNOB_Y + 35, (clampi(v, lo, hi) - lo) * w / (hi - lo), 3, C_AMBER);
  }
}

static void draw_foot(uint32_t now) {
  panel_t *p = &PANEL[cur];
  rect(0, FOOT_Y, SCREEN_W, SCREEN_H - FOOT_Y, C_BG);
  line(0, FOOT_Y, SCREEN_W - 1, FOOT_Y, C_FAINT);
  text(6, FOOT_Y + 4, PAGE_NAMES[page], C_AMBER, FONT_SMALL);
  if (now - saved_t < 1500 && saved_t) text(SCREEN_W - 6 - text_width("SAVED", FONT_SMALL), FOOT_Y + 4, "SAVED", C_AMBER, FONT_SMALL);
  else if (now - restored_t < 1500 && restored_t)
    text(SCREEN_W - 6 - text_width("FACTORY", FONT_SMALL), FOOT_Y + 4, "FACTORY", C_AMBER, FONT_SMALL);
  int x = 6;
  x = text(x, FOOT_Y + 22, "VIB", p->v[P_VIBRATO] ? C_TEXT : C_FAINT, FONT_SMALL) + 8;
  x = text(x, FOOT_Y + 22, "T.FB", p->v[P_TOTAL_FB] ? C_RED : C_FAINT, FONT_SMALL) + 8;
  x = text(x, FOOT_Y + 22, "34>56", p->v[P_RING] ? C_CYAN : C_FAINT, FONT_SMALL) + 8;
  x = text(x, FOOT_Y + 22, "LATCH", latch_mode ? C_AMBER : C_FAINT, FONT_SMALL) + 8;
  char b[10], *d = put(b, "OCT");
  d = put(d, od_oct > 0 ? "+" : od_oct < 0 ? "" : " ");
  put_int(d, od_oct);
  text(SCREEN_W - 6 - text_width(b, FONT_SMALL), FOOT_Y + 22, b, od_oct ? C_TEXT : C_DIM, FONT_SMALL);
}

/* ---------------------------------------------------------------- setup and loop */

void setup(void) {
  if (settings_load(&store, sizeof store) || store.magic != STORE_MAGIC) {
    for (int i = 0; i < PRESETS; i++) PANEL[i] = FACTORY[i];
  } else {
    for (int i = 0; i < PRESETS; i++) PANEL[i] = store.pan[i], PANEL[i].name[11] = 0;
  }
  for (int k = 0; k < KEY_COUNT; k++) key_mask[k] = note_mask(KEY_FIRST_NOTE + k);
  for (int dy = -13; dy <= 13; dy++) {
    int w = 0;
    while ((w + 1) * (w + 1) + dy * dy <= 13 * 13 + 6) w++;
    span13[dy + 13] = (int8_t)w;
  }
  od_live = &PANEL[0];
  od_init();
  led_backlight(1);
  clear(C_BG);
}

static void save_all(void) {
  store.magic = STORE_MAGIC;
  for (int i = 0; i < PRESETS; i++) store.pan[i] = PANEL[i];
  settings_save(&store, sizeof store);
}

void loop(void) {
  uint32_t now = millis();
  panel_t *p = &PANEL[cur];
  int32_t e;

  /* ---- buttons */
  static const uint8_t PAGE_BTN[] = {BTN_HOME, BTN_ENV, BTN_LFO};
  for (int i = 0; i < 3; i++)
    if (button_pressed(PAGE_BTN[i])) page = (uint8_t)i;
  if (button_pressed(BTN_FX)) page = page == PG_DELAY ? PG_DMOD : PG_DELAY;
  if (button_pressed(BTN_EDIT)) page = PG_DIST;
  if (button_pressed(BTN_SEL)) p->v[P_FAST + pair] = !p->v[P_FAST + pair];
  if (button_pressed(BTN_GLO)) p->v[P_TOTAL_FB] = !p->v[P_TOTAL_FB];
  if (button_pressed(BTN_ARP)) p->v[P_VIBRATO] = !p->v[P_VIBRATO];
  if (button_pressed(BTN_SEQ)) p->v[P_RING] = !p->v[P_RING];
  if (button_pressed(BTN_PLAY)) {
    latch_mode ^= 1;
    if (!latch_mode) latch = 0;
  }
  if (button_pressed(BTN_REC)) latch = 0;
  if (button_pressed(BTN_SAVE)) save_down = now, save_done = 0;
  if (button_held(BTN_SAVE) && !save_done && now - save_down > 2000) {
    PANEL[cur] = FACTORY[cur], save_done = 1, restored_t = now ? now : 1;
  }
  if (button_released(BTN_SAVE) && !save_done) save_all(), saved_t = now ? now : 1;
  if (button_pressed(BTN_OCT_DOWN) || button_pressed(BTN_OCT_UP)) {
    if (button_held(BTN_OCT_DOWN) && button_held(BTN_OCT_UP)) od_oct = 0;
    else od_oct = (int8_t)clampi(od_oct + (button_pressed(BTN_OCT_UP) ? 1 : -1), -2, 2);
  }

  /* ---- encoders */
  if ((e = encoder(ENC_SELECT))) pair = (uint8_t)((pair + PAIRS * 8 + e % PAIRS) % PAIRS);
  if ((e = encoder(ENC_ALGORITHM))) p->v[P_SRC + pair] = (int16_t)((p->v[P_SRC + pair] + 3 + (e > 0 ? 1 : -1)) % 3);
  if ((e = encoder(ENC_PRESETS))) {
    cur = (uint8_t)((cur + PRESETS * 8 + e % PRESETS) % PRESETS);
    p = &PANEL[cur], od_live = p;
  }
  for (int i = 0; i < 4; i++)
    if ((e = encoder(ENC_KNOB1 + i))) knob_turn(&KNOB[page][i], e);

  /* ---- keys and MIDI: the sensors */
  for (int k = 0; k < KEY_COUNT; k++) {
    if (key_pressed(k)) {
      if (latch_mode) latch ^= key_mask[k];
      else touch(key_mask[k], 1);
    }
    if (key_released(k) && !latch_mode) touch(key_mask[k], 0);
  }
  if (button_pressed(BTN_PLAY)) /* the keys held go out with the change */
    for (int v = 0; v < VOICES; v++) touch_n[v] = 0;
  uint8_t st, d1, d2;
  while (midi_read(&st, &d1, &d2)) {
    int t = st & 0xF0;
    if (t == 0x90 && d2) touch(note_mask(d1), 1);
    else if (t == 0x80 || (t == 0x90 && !d2)) touch(note_mask(d1), 0);
    else if (t == 0xB0 && d1 == 123) /* all notes off */
      for (int v = 0; v < VOICES; v++) touch_n[v] = 0;
  }
  update_gates();

  /* ---- LEDs: the keys of the voices sounding */
  uint8_t sounding = 0;
  for (int v = 0; v < VOICES; v++)
    if (od_level[v] > 1200) sounding |= (uint8_t)(1u << v);
  uint32_t lk = 0, lb = 0;
  for (int k = 0; k < KEY_COUNT; k++)
    if ((key_mask[k] & sounding) && white_index(KEY_FIRST_NOTE + k) >= 0) lk |= 1u << k;
  lb |= 1u << (page == PG_PAIR ? BTN_HOME : page == PG_QUAD ? BTN_ENV : page == PG_LFO ? BTN_LFO : page == PG_DIST ? BTN_EDIT : BTN_FX);
  if (p->v[P_FAST + pair]) lb |= 1u << BTN_SEL;
  if (p->v[P_TOTAL_FB]) lb |= 1u << BTN_GLO;
  if (p->v[P_VIBRATO]) lb |= 1u << BTN_ARP;
  if (p->v[P_RING]) lb |= 1u << BTN_SEQ;
  if (latch_mode) lb |= 1u << BTN_PLAY;
  if (latch) lb |= 1u << BTN_REC;
  if (lk != led_k) {
    for (int i = 0; i < KEY_COUNT; i++)
      if ((lk ^ led_k) >> i & 1u) led_key(i, lk >> i & 1u);
    led_k = lk;
  }
  if (lb != led_b) {
    for (int i = 0; i < BTN_COUNT; i++)
      if ((lb ^ led_b) >> i & 1u) led_button(i, lb >> i & 1u);
    led_b = lb;
  }

  /* ---- the screen, only what changed */
  if (!drawn) clear(C_BG), drawn = 1;
  uint32_t h = cur;
  for (int i = 0; i < 11 && p->name[i]; i++) h = h * 131u + (uint8_t)p->name[i];
  if (h != sh_title) sh_title = h, draw_title();
  if (od_scope_seq != scope_seen && now - scope_t >= 70) scope_seen = od_scope_seq, scope_t = now, draw_scope();
  h = pair | (uint32_t)p->v[P_RING] << 2 | (uint32_t)p->v[P_TOTAL_FB] << 3;
  for (int i = 0; i < PAIRS; i++) h = h * 1031u + (uint32_t)(p->v[P_FAST + i] * 3 + p->v[P_SRC + i]) * 128u + (uint32_t)p->v[P_MOD + i];
  if (h != sh_pairs) sh_pairs = h, draw_pairs();
  h = latch;
  for (int v = 0; v < VOICES; v++) h = h * 17u + (od_level[v] >> 11);
  if (h != sh_sens) {
    sh_sens = h;
    for (int v = 0; v < VOICES; v++) draw_sensor(v, od_level[v] >> 11);
  }
  h = (uint32_t)page | (uint32_t)pair << 3 | (uint32_t)cur << 5;
  for (int i = 0; i < 4; i++) h = h * 257u + (uint32_t)(knob_get(&KNOB[page][i]) + 3000);
  if (h != sh_knobs) sh_knobs = h, draw_knobs();
  h = (uint32_t)page | (uint32_t)p->v[P_VIBRATO] << 3 | (uint32_t)p->v[P_TOTAL_FB] << 4 | (uint32_t)p->v[P_RING] << 5 |
      (uint32_t)latch_mode << 6 | (uint32_t)(od_oct + 2) << 7 | (uint32_t)(now - saved_t < 1500) << 10 |
      (uint32_t)(now - restored_t < 1500) << 11;
  if (h != sh_foot) sh_foot = h, draw_foot(now);
}

/* ---------------------------------------------------------------- the sound */

void audio(int32_t *out, uint32_t frames) {
#if FOMA_USB_AUDIO
  od_render(out, frames, audio_in()); /* a computer's audio goes in like the EXT IN jack */
#else
  od_render(out, frames, 0);
#endif
}
