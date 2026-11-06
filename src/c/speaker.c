/* DQK.EXE resident module 25ff: the PC speaker and the Tandy/PCjr sound
 * chip at port 0xc0.  Start-up sets has_tandy from two calls through
 * RTLink vectors (1d99:155d, else 1d99:1553).  Silencing turns the speaker
 * gate off and, with the chip, sets its four channels to attenuation 15.
 * voice_set drives one voice: a voice below 4 goes to the chip when it is
 * present; otherwise a voice below 1 or above 3 goes to timer 2 and the
 * speaker gate, and voices 1..3 are dropped.  voice_note turns a note
 * number into a divisor for voice_set.
 *
 * EXE 0x0183f9..0x0185bb, one Turbo C large-model _TEXT segment starting at
 * byte 9 of paragraph 25ff.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * scrfont.c's module 25a7 ends with the retf at 25ff:0008, and palette.c's
 * module 261b starts at 0x0185bb with no stub between.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the
 * rest are placeholders.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined: speaker_init and speaker_done far-call speaker_silence, which
 * carries a prototype.  The port reads and writes are <dos.h>'s inportb
 * and outportb, which Turbo C expands to its inline in and out.
 *
 * _DATA is the two divisor tables at 9abc..9aeb, between scrfont.c's "FON"
 * (ending 9abb) and palette.c's literal at 9aec.  _BSS follows the statics'
 * names (Turbo C's symbol-table order), not their declaration order; these
 * names reproduce the shipped cdc0..cdc2 layout.  Only this module names
 * either.
 */

#include <dos.h>
#include "dqk.h"

extern int  far is_pcjr(void);                                   /* 1d99:155d, vector to overlay 4 2726:0004 */
extern int  far is_tandy(void);                                  /* 1d99:1553, vector to overlay 4 2726:0020 */

void far speaker_silence(void);

/* Divisors for the twelve notes from middle C (about 262 Hz) to B: timer
 * 2's (1193182 Hz / f) and the Tandy chip's (3579545 Hz / 32 / f). */
static unsigned spk_div[12] = {                                  /* 9abc */
    4561, 4305, 4063, 3835, 3620, 3417, 3225, 3044, 2873, 2712, 2560, 2416
};
static unsigned tandy_div[12] = {                                /* 9ad4 */
    427, 404, 381, 359, 339, 320, 302, 285, 269, 254, 240, 226
};
static char port61;                                              /* cdc0 */
static char spk_on;                                              /* cdc1 */
static char has_tandy;                                           /* cdc2 */

void far speaker_init(void)                                      /* 25ff:0009 */
{
    spk_on = 1;
    has_tandy = is_pcjr() || is_tandy();
    speaker_silence();
    if (has_tandy) {
        port61 = inportb(0x61);
        outportb(0x61, port61 | 0x60);
    }
}

void far speaker_done(void)                                      /* 25ff:0043 */
{
    speaker_silence();
    if (has_tandy)
        outportb(0x61, port61);             /* the census's weak `out` marker */
}

void far speaker_noop(void)                                      /* 25ff:0055 */
{
}

/* Unlike 0055, this stub has a push bp frame: it takes a parameter. */
void far speaker_argument_noop(int unused)                                /* 25ff:0056 */
{
}

/* Timer 2's gate and the speaker data bit (port 0x61 bits 0 and 1) off,
 * then each of the chip's four channels to attenuation 15, off. */
void far speaker_silence(void)                                      /* 25ff:005b */
{
    outportb(0x61, inportb(0x61) & 0xfc); /* the census's weak `in`/`out` markers */
    if (has_tandy) {
        outportb(0xc0, 0x9f);
        outportb(0xc0, 0xbf);
        outportb(0xc0, 0xdf);
        outportb(0xc0, 0xff);
    }
}

/* The chip takes volume's bits 3..6 as v and sets attenuation 15 - v.
 * A zero freq turns the speaker off. */
void far voice_set(int voice, unsigned freq, int volume)         /* 25ff:0079 */
{
    char hi;

    if (!spk_on)
        return;
    if (has_tandy && voice < 4) {
        volume = (volume >> 3) & 0xf;
        hi = voice << 5;
        outportb(0xc0, (freq & 0xf) | hi | 0x80); /* the census's weak `out` markers */
        if (voice < 3)
            outportb(0xc0, (freq >> 4) & 0x3f);
        outportb(0xc0, (0xf - volume) | hi | 0x90);
    } else if (voice < 1 || voice > 3) {
        hi = inportb(0x61) & 0xfc;          /* and its weak `in` marker */
        if (freq) {
            outportb(0x43, 0xb6);
            outportb(0x42, freq);
            outportb(0x42, freq >> 8);
            outportb(0x61, hi | 3);
        } else {
            outportb(0x61, hi);
        }
    }
}

/* The octave, note / 12, is taken relative to 5: below it the divisor
 * doubles, at most four times; above it, halves.  With the chip present,
 * the divisor is halved until it fits 10 bits, whichever table it came
 * from.  A negative volume is silence. */
void far voice_note(int voice, int note, int volume)             /* 25ff:011b */
{
    register unsigned freq;

    if (volume < 0) {
        freq = volume = 0;
    } else {
        if (has_tandy && voice < 3)
            freq = tandy_div[note % 12];
        else
            freq = spk_div[note % 12];
        note = note / 12 - 5;
        if (note < 0)
            freq <<= int_min(4, -note);
        else if (note)
            freq >>= note;
        if (has_tandy)
            while (freq >= 0x400)
                freq >>= 1;
    }
    voice_set(voice, freq, volume);
}

void far speaker_enable(char enabled)                                   /* 25ff:01b2 */
{
    spk_on = enabled;
    if (!enabled)
        speaker_silence();
}

char far speaker_enabled(void)                                      /* 25ff:01c7 */
{
    return spk_on;
}
