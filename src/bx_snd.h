/* BX SND - software audio synthesis and WAV export.
 *
 * A small mixer maintains a set of voices. Every voice renders a waveform at
 * a frequency with a per-note ADSR envelope into a shared 16-bit mono mix
 * buffer at a fixed sample rate (22050 by default, so the buffer stays small
 * and tests run quickly). high.snd.wav writes the mix out as a standard RIFF
 * WAV file.
 *
 * The whole thing is printf-free and heap-light where it matters, and it lives
 * off the main input thread so nothing about the interpreter depends on audio
 * timing.
 */
#ifndef BX_SND_H
#define BX_SND_H

#include <stdint.h>

#define BX_SND_DEFAULT_RATE 22050u

typedef enum {
    BX_SND_WAVE_SINE = 0,
    BX_SND_WAVE_SQUARE = 1,
    BX_SND_WAVE_SAW = 2,
    BX_SND_WAVE_TRI = 3,
    BX_SND_WAVE_NOISE = 4
} bx_snd_wave_t;

typedef struct {
    uint32_t rate;        /* samples per second */
    int16_t *mix;         /* 16-bit mono mix buffer, size = samples */
    uint32_t samples;     /* current buffer size */
    uint32_t used;        /* highest written sample + 1 */
} bx_snd_mix_t;

typedef struct {
    bx_snd_wave_t wave;
    double freq;          /* Hz */
    double amp;           /* 0..1 */
    double a, d, s, r;    /* ADSR: seconds */
    uint32_t dur;         /* total samples (0 = until envelope finishes) */
    uint32_t start;       /* delay in samples before this voice sounds */
    double phase;
    uint32_t t;           /* rendered samples so far */
} bx_snd_voice_t;

#define BX_SND_MAX_VOICES 64

typedef struct {
    uint32_t rate;
    uint32_t fade;                      /* transition samples near start (tests) */
    bx_snd_voice_t voices[BX_SND_MAX_VOICES];
    int32_t  voice_count;
    bx_snd_mix_t mix;
} bx_snd_ctx_t;

extern bx_snd_ctx_t g_bx_snd;

/* Setup -------------------------------------------------------------------- */

/* Reset the whole system. rate 0 keeps the current rate (default 22050). */
void bx_snd_init(uint32_t rate);

/* Voices ------------------------------------------------------------------- */

/* Add a voice. dur is in seconds. Returns 0 on success. */
int  bx_snd_tone(bx_snd_wave_t wave, double freq, double amp, double dur,
                 double a, double d, double s, double r);
/* MIDI note (60 = middle C) convenience wrapper around bx_snd_tone. */
int  bx_snd_note(bx_snd_wave_t wave, int midi, double amp, double dur,
                 double a, double d, double s, double r);
int  bx_snd_clear(void);
int  bx_snd_voice_count(void);

/* Friendly layer ------------------------------------------------------------ */

/* Schedule one note per whitespace-separated name in "C#4 Ab5 E4" style.
 * Each note starts dur seconds after the previous one, plus offset seconds.
 * Returns the number of notes scheduled, or -1 on a bad note name. */
int  bx_snd_melody(const char *notes, double dur, bx_snd_wave_t wave,
                   double amp, double offset,
                   double a, double d, double s, double r);

/* Envelope helpers --------------------------------------------------------- */

/* A frequency table used by melody parsing and tone tests. */
double bx_snd_note_freq(int midi);        /* 440 * 2^((midi-69)/12) */
int    bx_snd_note_from_name(const char *name);       /* "C4" -> 60, -1 on error */

/* Mixing ------------------------------------------------------------------- */

/* Render all current voices into the mix buffer. Returns new sample count. */
uint32_t bx_snd_render(void);
uint32_t bx_snd_samples(void);   /* how many samples the mix currently holds */
uint32_t bx_snd_rate(void);      /* current sample rate */

/* Export the current mix as a 16-bit mono WAV. Returns 0 on success. */
int bx_snd_wav(const char *path);

#endif