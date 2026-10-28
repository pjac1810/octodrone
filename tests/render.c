/* SPDX-License-Identifier: GPL-3.0-only */
/* Host test for OCTODRONE: plays each factory panel (sensors touched one after another, then pairs, then
 * let go) to build/render-out/N-NAME.wav, measures the CPU, then fuzzes every knob.   tests/run.sh */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "od.h"

int32_t sine(uint32_t phase) {
  int32_t x = (int32_t)phase >> 16, ax = x < 0 ? -x : x, y, ay;
  y = (x * (32768 - ax)) >> 13;
  ay = y < 0 ? -y : y;
  y += ((((y * ay) >> 15) - y) * 7373) >> 15;
  return y > 32767 ? 32767 : y < -32767 ? -32767 : y;
}
static const uint32_t INC_C9[12] = {815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
                                    1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};
uint32_t note_inc(int note) {
  if (note < 0) note = 0;
  if (note > 127) note = 127;
  return INC_C9[note % 12] >> (10 - note / 12);
}
static uint32_t rs = 0x1234567u;
uint32_t random32(void) {
  rs ^= rs << 13, rs ^= rs >> 17, rs ^= rs << 5;
  return rs;
}

#define MAXF (44100 * 30)
static int16_t buf[MAXF * 2];
static uint32_t pos;
static int fail;

static void render(uint32_t frames) {
  static int32_t blk[512];
  while (frames) {
    uint32_t n = frames > 256 ? 256 : frames;
    memset(blk, 0, sizeof blk);
    od_render(blk, n, 0);
    for (uint32_t i = 0; i < 2 * n; i++) {
      if (blk[i] > 32767 || blk[i] < -32767) fail = 1;
      if (pos * 2 + i < MAXF * 2) buf[pos * 2 + i] = (int16_t)blk[i];
    }
    pos += n, frames -= n;
  }
}

static void save(const char *dir, const char *name) {
  char path[600];
  snprintf(path, sizeof path, "%s/%s.wav", dir, name);
  for (char *c = path + strlen(dir) + 1; *c; c++)
    if (*c == ' ') *c = '_';
  FILE *f = fopen(path, "wb");
  uint32_t frames = pos < MAXF ? pos : MAXF, data = frames * 4, riff = 36 + data, rate = 44100, br = rate * 4, fmt = 16;
  uint16_t pcm = 1, ch = 2, al = 4, bits = 16;
  fwrite("RIFF", 1, 4, f), fwrite(&riff, 4, 1, f), fwrite("WAVEfmt ", 1, 8, f), fwrite(&fmt, 4, 1, f);
  fwrite(&pcm, 2, 1, f), fwrite(&ch, 2, 1, f), fwrite(&rate, 4, 1, f), fwrite(&br, 4, 1, f);
  fwrite(&al, 2, 1, f), fwrite(&bits, 2, 1, f), fwrite("data", 1, 4, f), fwrite(&data, 4, 1, f);
  fwrite(buf, 2, frames * 2, f);
  fclose(f);
  double sq = 0, dc = 0;
  int peak = 0, clip = 0;
  for (uint32_t i = 0; i < frames * 2; i++) {
    int v = abs(buf[i]);
    sq += (double)v * v, peak = v > peak ? v : peak, clip += v >= 32700, dc += buf[i];
  }
  printf("%-16s peak %6d rms %6.0f dc %6.0f clip %.3f%%\n", name, peak, sqrt(sq / (frames * 2)), dc / (frames * 2),
         100.0 * clip / (frames * 2));
  pos = 0;
}

static void gates(uint8_t m) {
  for (int v = 0; v < VOICES; v++) od_gate[v] = (uint8_t)(m >> v & 1u);
}

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : ".";
  for (int i = 0; i < PRESETS; i++) PANEL[i] = FACTORY[i];
  od_init();
  for (int k = 0; k < PRESETS; k++) {
    od_live = &PANEL[k];
    gates(0), render(2048);
    clock_t t0 = clock();
    /* one sensor after another, each 1.2 s, overlapping */
    for (int v = 0; v < VOICES; v++) gates((uint8_t)(3u << v)), render(44100 * 12 / 10);
    /* pairs together, then two quads, then let go */
    gates(0x11), render(44100 * 2);
    gates(0x0F), render(44100 * 2);
    gates(0xF0), render(44100 * 2);
    gates(0), render(44100 * 5);
    double ms = (clock() - t0) * 1000.0 / CLOCKS_PER_SEC, secs = pos / 44100.0;
    char n[40];
    snprintf(n, sizeof n, "%d-%s", k + 1, PANEL[k].name);
    printf("  (%5.1f ms CPU for %.1f s) ", ms, secs);
    save(dir, n);
  }

  /* fuzz: any knob anywhere, any sensors, any time */
  srand(7);
  od_live = &PANEL[0];
  for (int b = 0; b < 40000; b++) {
    int r = rand() % 8;
    if (r == 0) gates((uint8_t)rand());
    else if (r == 1) od_oct = (int8_t)(rand() % 5 - 2);
    else if (r == 2) od_live = &PANEL[rand() % PRESETS];
    else PANEL[rand() % PRESETS].v[rand() % P_COUNT] = (int16_t)(rand() % 14000 - 2000);
    render(1 + rand() % 256);
    pos = 0;
  }
  printf("fuzz: 40000 blocks, %s\n", fail ? "FAILED" : "ok");
  return fail;
}
