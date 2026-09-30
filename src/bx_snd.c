/* BX SND - software audio synthesis and WAV export. See bx_snd.h. */
#include "bx_snd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

bx_snd_ctx_t g_bx_snd;

double bx_snd_note_freq(int midi) {
    return 440.0 * pow(2.0, (double)(midi - 69) / 12.0);
}

static int ci_equal_ss(const char *a, const char *b) {
    while (*a && *b) {
        int ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

/* "C4", "C#4", "Bb3", "A-2" -> MIDI note. Returns -1 on error. */
int bx_snd_note_from_name(const char *name) {
    if (!name || !*name) return -1;
    static const char *const names[12] = { "C", "C#", "D", "D#", "E", "F",
                                           "F#", "G", "G#", "A", "A#", "B" };
    char pitch[4];
    int pi = 0;
    while (*name && !(*name >= '0' && *name <= '9')) {
        if (pi + 1 < (int)sizeof pitch) pitch[pi++] = *name;
        name++;
    }
    pitch[pi] = 0;
    if (pi == 0) return -1;
    if (!*name || *name < '0' || *name > '9') return -1;
    int show = *name - '0';

    if (!strcmp(pitch, "Cb")) strcpy(pitch, "B");
    else if (!strcmp(pitch, "Db")) strcpy(pitch, "C#");
    else if (!strcmp(pitch, "Eb")) strcpy(pitch, "D#");
    else if (!strcmp(pitch, "Fb")) strcpy(pitch, "E");
    else if (!strcmp(pitch, "Gb")) strcpy(pitch, "F#");
    else if (!strcmp(pitch, "Ab")) strcpy(pitch, "G#");
    else if (!strcmp(pitch, "Bb")) strcpy(pitch, "A#");

    for (int i = 0; i < 12; i++)
        if (!strcmp(pitch, names[i]))
            return (show + 1) * 12 + i;   /* C4 = 60 */
    return -1;
}

static int16_t *mix_ensure(uint32_t samples) {
    bx_snd_mix_t *m = &g_bx_snd.mix;
    if (m->samples < samples) {
        int16_t *p = (int16_t *)realloc(m->mix, samples * sizeof *p);
        if (!p) return NULL;
        memset(p + m->samples, 0, (samples - m->samples) * sizeof *p);
        m->mix = p;
        m->samples = samples;
    }
    return m->mix;
}

void bx_snd_init(uint32_t rate) {
    bx_snd_ctx_t *s = &g_bx_snd;
    if (!rate) rate = BX_SND_DEFAULT_RATE;
    s->rate = rate;
    s->voice_count = 0;
    free(s->mix.mix);
    memset(&s->mix, 0, sizeof s->mix);
    memset(s->voices, 0, sizeof s->voices);
}

static int voice_add(bx_snd_wave_t wave, double freq, double amp, double dur,
                     double a, double d, double s, double r, uint32_t start) {
    bx_snd_ctx_t *ctx = &g_bx_snd;
    if (ctx->voice_count >= BX_SND_MAX_VOICES) return -1;
    bx_snd_voice_t *v = &ctx->voices[ctx->voice_count++];
    memset(v, 0, sizeof *v);
    v->wave = wave;
    v->freq = freq;
    v->amp = amp;
    v->a = a;
    v->d = d;
    v->s = s;
    v->r = r;
    v->dur = dur > 0 ? (uint32_t)(dur * ctx->rate) : 0;
    v->start = start;
    v->phase = 0;
    v->t = 0;
    return 0;
}

int bx_snd_tone(bx_snd_wave_t wave, double freq, double amp, double dur,
                double a, double d, double s, double r) {
    return voice_add(wave, freq, amp, dur, a, d, s, r, 0);
}

int bx_snd_note(bx_snd_wave_t wave, int midi, double amp, double dur,
                double a, double d, double s, double r) {
    return voice_add(wave, bx_snd_note_freq(midi), amp, dur, a, d, s, r, 0);
}

int bx_snd_melody(const char *notes, double dur, bx_snd_wave_t wave,
                  double amp, double offset,
                  double a, double d, double s, double r) {
    int count = 0;
    const char *p = notes;
    while (p && *p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',') p++;
        if (!*p) break;
        char tok[16];
        int i = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != ','
               && i + 1 < (int)sizeof tok)
            tok[i++] = *p++;
        tok[i] = 0;
        int midi = bx_snd_note_from_name(tok);
        if (midi < 0) return -1;
        uint32_t start = (uint32_t)((count * dur + offset) * g_bx_snd.rate);
        if (voice_add(wave, bx_snd_note_freq(midi), amp, dur, a, d, s, r, start) != 0)
            return -1;
        count++;
    }
    return count;
}

int bx_snd_clear(void) {
    g_bx_snd.voice_count = 0;
    return 0;
}

int bx_snd_voice_count(void) {
    return g_bx_snd.voice_count;
}

/* Sample a waveform at cycle offset 0..1. */
static double wave_sample(bx_snd_wave_t wave, double ph) {
    switch (wave) {
    case BX_SND_WAVE_SINE:   return sin(2.0 * 3.14159265358979323846 * ph);
    case BX_SND_WAVE_SQUARE: return ph < 0.5 ? 1.0 : -1.0;
    case BX_SND_WAVE_SAW:    return 2.0 * ph - 1.0;
    case BX_SND_WAVE_TRI:    return ph < 0.5 ? 4.0 * ph - 1.0 : 3.0 - 4.0 * ph;
    case BX_SND_WAVE_NOISE:  return (double)(rand() % 20001) / 10000.0 - 1.0;
    }
    return 0.0;
}

/* ADSR at a single sample, given stage breakpoints in samples. */
static double env_at(const bx_snd_voice_t *v, uint32_t t, uint32_t n) {
    if (n == 0) return 0.0;
    uint32_t a = (uint32_t)(v->a * n);
    uint32_t d = (uint32_t)(v->d * n);
    uint32_t r = (uint32_t)(v->r * n);
    uint32_t dstart = a;
    uint32_t dlen = d ? d : 1;
    uint32_t rstart = n > r ? n - r : 0;
    if (t < a) return (double)t / (a ? a : 1);
    if (t < dstart + dlen) {
        double x = (double)(t - dstart) / dlen;
        return 1.0 + x * (v->s - 1.0);
    }
    if (t < rstart) return v->s;
    if (t >= n) return 0.0;
    double x = (double)(t - rstart) / (n - rstart);
    return v->s * (1.0 - x);
}

uint32_t bx_snd_render(void) {
    bx_snd_ctx_t *ctx = &g_bx_snd;
    uint32_t n = 0;
    for (int i = 0; i < ctx->voice_count; i++) {
        bx_snd_voice_t *v = &ctx->voices[i];
        uint32_t len = v->dur ? v->dur
                              : (uint32_t)((v->a + v->d + v->r) * ctx->rate) + 1;
        uint32_t end = v->start + len;
        if (end > n) n = end;
    }
    if (!mix_ensure(n)) return 0;
    memset(ctx->mix.mix, 0, n * sizeof *ctx->mix.mix);

    double per = 1.0 / (double)ctx->rate;
    for (int i = 0; i < ctx->voice_count; i++) {
        bx_snd_voice_t *v = &ctx->voices[i];
        uint32_t len = v->dur ? v->dur
                              : (uint32_t)((v->a + v->d + v->r) * ctx->rate) + 1;
        double phase = v->phase;
        for (uint32_t t = 0; t < len; t++) {
            double env = env_at(v, t, len);
            double w = wave_sample(v->wave, phase);
            phase += v->freq * per;
            phase -= (double)(long)phase;
            ctx->mix.mix[v->start + t] += (int16_t)(v->amp * env * w * 30000.0);
        }
        v->phase = phase;
        v->t += len;
    }
    ctx->mix.used = n;
    return n;
}

uint32_t bx_snd_rate(void) {
    return g_bx_snd.rate ? g_bx_snd.rate : BX_SND_DEFAULT_RATE;
}

uint32_t bx_snd_samples(void) {
    return g_bx_snd.mix.used;
}

int bx_snd_wav(const char *path) {
    if (!path || !*path) return -1;
    uint32_t n = g_bx_snd.mix.used;
    if (!g_bx_snd.mix.mix) n = 0;
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    uint32_t rate = g_bx_snd.rate ? g_bx_snd.rate : BX_SND_DEFAULT_RATE;
    uint32_t data_size = n * 2;
    unsigned char hdr[44];
    memset(hdr, 0, sizeof hdr);
    memcpy(hdr, "RIFF", 4);
    hdr[4] = (unsigned char)((data_size + 36) & 0xFF);
    hdr[5] = (unsigned char)(((data_size + 36) >> 8) & 0xFF);
    hdr[6] = (unsigned char)(((data_size + 36) >> 16) & 0xFF);
    hdr[7] = (unsigned char)(((data_size + 36) >> 24) & 0xFF);
    memcpy(hdr + 8, "WAVEfmt ", 8);
    hdr[16] = 16;                       /* PCM chunk size */
    hdr[20] = 1; hdr[21] = 0;           /* PCM = 1 */
    hdr[22] = 1; hdr[23] = 0;           /* mono */
    hdr[24] = (unsigned char)(rate & 0xFF);
    hdr[25] = (unsigned char)((rate >> 8) & 0xFF);
    hdr[26] = (unsigned char)((rate >> 16) & 0xFF);
    hdr[27] = (unsigned char)((rate >> 24) & 0xFF);
    uint32_t byte_rate = rate * 2;
    hdr[28] = (unsigned char)(byte_rate & 0xFF);
    hdr[29] = (unsigned char)((byte_rate >> 8) & 0xFF);
    hdr[30] = (unsigned char)((byte_rate >> 16) & 0xFF);
    hdr[31] = (unsigned char)((byte_rate >> 24) & 0xFF);
    hdr[32] = 2; hdr[33] = 0;           /* block align = 2 */
    hdr[34] = 16; hdr[35] = 0;          /* bits per sample */
    memcpy(hdr + 36, "data", 4);
    hdr[40] = (unsigned char)(data_size & 0xFF);
    hdr[41] = (unsigned char)((data_size >> 8) & 0xFF);
    hdr[42] = (unsigned char)((data_size >> 16) & 0xFF);
    hdr[43] = (unsigned char)((data_size >> 24) & 0xFF);
    fwrite(hdr, 44, 1, f);
    if (n) fwrite(g_bx_snd.mix.mix, n * 2, 1, f);
    int rc = ferror(f) ? -1 : 0;
    fclose(f);
    return rc;
}