/* DQK.EXE resident module 28fc: digitized sounds.  A sound is a resource
 * item: a word sample rate, a long sample count, then the samples.
 * sample_library_set switches item lookup from a bare fcache slot to an item of
 * the library opened in a slot, sample_sequence_play hands a list of items, each
 * cut by a start and a length in 1/200 s units, to 10d7:10df, which plays
 * them on the PC speaker a number of times until a key is pressed, and
 * sample_play_rate hands it one item at a rate its caller gives.  Both do
 * nothing when module 25ff's 25ff:01c7 returns 0, and both pass the rate as
 * the timer divisor 1193181 / rate, rounded (0x1234dd, the PIT input clock).
 *
 * EXE 0x01b3c4..0x01b7f7, one Turbo C large-model _TEXT segment starting at
 * byte 4 of paragraph 28fc.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * fcache.c's module 27fa ends with the retf at 0x01b3c3, and library.c's
 * module 293f starts at 0x01b7f7 with no stub between.
 *
 * The two empty routines at 28fc:0004 and 0005 are claimed here, as 2999
 * opens with two such stubs: nothing in the image calls or points at them,
 * and the bytes cannot tell a head of this module from a tail of fcache.c's.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the
 * rest are placeholders chosen from use.
 *
 * in_library (_DATA 9f80) and lib_slot (_BSS da8e) are read by nothing
 * outside this module.
 */

#include "dqk.h"

struct Clip {                   /* one item queued by sample_sequence_play          */
    long len;                   /* +0  samples to play                      */
    unsigned rate;              /* +4  Hz, then the timer divisor           */
    char far *data;             /* +6  first sample                         */
    char arg;                   /* +a  passed through to 10d7:10df          */
};

extern void far huge_move(void far *src, void far *dst, unsigned n); /* 00d7:08c2 */

static char in_library = 0;                                      /* 9f80 */
static char lib_slot;                                            /* da8e */

/* Empty init of the library init/done pair at the module head (plausible). */
void far digisnd_init(void)                                      /* 28fc:0004 */
{
}

/* Empty done of the library init/done pair at the module head (plausible). */
void far digisnd_done(void)                                      /* 28fc:0005 */
{
}

void far sample_library_set(int slot)                                  /* 28fc:0006 */
{
    lib_slot = slot;
    in_library = 1;
}

/* The arguments after count come in fours -- item, start, length, arg --
 * ended by a negative item; a start or length of 0 or less leaves the
 * item's own.  clip[] holds 25 entries but the loop admits n up to 25, a
 * 26th, as shipped. */
void far sample_sequence_play(int count, ...)                            /* 28fc:0016 */
{
    struct Clip clip[25];
    long div;
    int far *argument_cursor;
    char far *sample_data;
    short rep;
    short step;
    register int sample_index, inner_index;

    if (!speaker_enabled())
        return;
    argument_cursor = &count;
    argument_cursor++;
    for (sample_index = 0; *argument_cursor >= 0 && sample_index <= 25; sample_index++) {
        if (in_library)
            sample_data = lib_item(slot_base(lib_slot), *argument_cursor++);
        else
            sample_data = slot_base(*argument_cursor++);
        huge_move(sample_data, &clip[sample_index].rate, 2);
        clip[sample_index].rate = swap16(clip[sample_index].rate);
        step = clip[sample_index].rate / 200;
        clip[sample_index].rate = div = ((clip[sample_index].rate >> 1) + 0x1234ddL) / clip[sample_index].rate;
        sample_data = huge_add(sample_data, 2L);
        huge_move(sample_data, &clip[sample_index].len, 4);
        clip[sample_index].len = swap32(clip[sample_index].len);
        clip[sample_index].data = huge_add(sample_data, 4L);
        if (*argument_cursor > 0)
            clip[sample_index].data = huge_add(clip[sample_index].data, *argument_cursor * step);
        argument_cursor++;
        if (*argument_cursor > 0)
            clip[sample_index].len = (long)*argument_cursor * step;
        argument_cursor++;
        clip[sample_index].arg = *argument_cursor;
        argument_cursor++;
    }
    for (rep = 0; rep < count; rep++) {
        for (inner_index = 0; inner_index < sample_index; inner_index++) {
            /* len goes as its low word only, as shipped (push es:[bx]) */
            speaker_play(clip[inner_index].data, clip[inner_index].len, clip[inner_index].rate, clip[inner_index].arg);
            if (key_pressed())
                break;
        }
        if (key_pressed())
            break;
    }
}

/* The census's `rot` marker, `33 d2 d1 fa d1 d8` at 28fc:03ac, is Turbo C's
 * inline (long)div >> 1: xor dx,dx; sar dx,1; rcr ax,1. */
void far sample_play_rate(int item, unsigned short rate, int arg)    /* 28fc:035a */
{
    long len;
    unsigned short div;
    void far *data;
    short unused;               /* referenced below only to keep its [bp-6] slot */
    char far *sample_data;

    (void)unused;
    if (!speaker_enabled())
        return;
    if (in_library)
        sample_data = lib_item(slot_base(lib_slot), item);
    else
        sample_data = slot_base(item);
    div = rate;
    div = (((long)div >> 1) + 0x1234ddL) / div;
    sample_data = huge_add(sample_data, 2L);
    huge_move(sample_data, &len, 4);
    len = swap32(len);
    data = huge_add(sample_data, 4L);
    speaker_play(data, len, div, arg);
}
