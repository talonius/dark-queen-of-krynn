/* DQK.EXE resident module 2093: small game services.  The Esc flag, set
 * when Esc or ` is read while demo_active is on; zeroing a block; leaving
 * the game; reading a key, where Ins and Del (0x152, 0x153) also call
 * into module 2023 and other extended keys come back less 0x80;
 * allocating and freeing records in the pool at item_record_pool, passing 0x3d
 * as the word strutil.c's pool_alloc and pool_free name unused; and
 * formatting a byte, an int or an unsigned long into a static buffer.
 *
 * EXE 0x012d35..0x012f2d, one Turbo C large-model _TEXT segment starting at
 * byte 5 of paragraph 2093.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * The lone retf at 0x012d35 (misc_unit_init) is far-called as 1093:0005 from
 * 0x037e89.  Module 20b2 starts at 0x012f2d with no stub between.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  A name another module
 * gives an address is reused from there, this module's own mod1093_ names
 * included; the rest are placeholders chosen from use.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined: input_pending_clear far-calls input_press_discard, so that one carries a prototype.
 */

#include "dqk.h"

void far input_press_discard(void);

static char b_buf[10];                                           /* a86e */
static char i_buf[12];                                           /* a878 */
static char l_buf[12];                                           /* a884 */
static char esc_hit;                                             /* a890 */

/* The translated Pascal unit's empty initialization block (DKK gameio.pas). */
void far misc_unit_init(void)                                         /* 2093:0005 */
{
}

void far esc_set(char enabled)                                        /* 2093:0006 */
{
    esc_hit = enabled;
}

char far esc_get(void)                                           /* 2093:0011 */
{
    return esc_hit;
}

void far mem_zero(void far *buffer, int byte_count)                        /* 2093:0015 */
{
    mem_fill(buffer, byte_count, 0);
}

void far game_exit(void)                                         /* 2093:002d */
{
    slot_close(0);
    game_sound_stop();
    sys_shutdown();
    dos_call(0x4c00);
}

unsigned char far game_key_get(void)                             /* 2093:0078 */
{
    short key;

    if (demo_active) {
        if (key_poll())
            key = key_get();
        else
            key = 0;
        if (key == 0x1b || key == 0x60)
            esc_set(1);
    } else
        key = key_get();
    if (key == 0x152)
        sound_toggle();
    if (key == 0x153)
        music_toggle();
    if (key != 0)
        while (key_poll()) {
            key = key_get();
            if (key == 0x152)
                sound_toggle();
            if (key == 0x153)
                music_toggle();
            if (demo_active && (key == 0x1b || key == 0x60))
                esc_set(1);
        }
    if (key >= 0x100)
        key -= 0x80;
    return key;
}

void far input_pending_clear(void)                                      /* 2093:0130 */
{
    input_press_discard();
}

void far input_press_discard(void)                                          /* 2093:0136 */
{
    int unused_mouse_x;
    int unused_mouse_y;

    if (key_poll())
        game_key_get();
    mouse_press(7, &unused_mouse_x, &unused_mouse_y);
}

void far *far rec_alloc(void)                                    /* 2093:0162 */
{
    void far *record;

    pool_alloc(&record, 0x3d, &item_record_pool);
    return record;
}

void far rec_free(void far * far *record_ref)                            /* 2093:0188 */
{
    pool_free(*record_ref, 0x3d, &item_record_pool);
}

char far *far byte_str(unsigned char value)                          /* 2093:01a7 */
{
    int_to_str(value, b_buf);
    return b_buf;
}

char far *far int_str(int value)                                     /* 2093:01c3 */
{
    int_to_str(value, i_buf);
    return i_buf;
}

char far *far ulong_str(unsigned long value)                         /* 2093:01dc */
{
    str_format(l_buf, "%lu", value);
    return l_buf;
}
