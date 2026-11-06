/* DQK.EXE overlay 04 module 264f: the title pictures and the closing
 * animation.
 * title_show draws title screens 1 to 8 from the title1 library in turn,
 * waiting up to 800 ticks on each; a space moves on, Esc leaves at once, and any other key
 * ends the sequence early.  ending_show plays the three final1 pictures
 * frame by frame, with pauses between them.
 *
 * EXE 0x0383cb..0x038e53, one Turbo C large-model _TEXT segment starting at
 * byte 0xb of paragraph 264f in RTLink section 4 (payload 0x37910, link
 * segment 25a4).  The first 0xb bytes of paragraph 264f are the tail of
 * module 25de, and the last 3 bytes of the range lie in paragraph 26f8,
 * whose module starts at 26f8:0003.  Built with Turbo C 2.01,
 * tcc -ml -O -Z -d.  Functions are defined in shipped order; bake consumes
 * them in that order.  Sections 5 to 8 hold other bytes at the 25a4 link
 * segment.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  Callee names are placeholders chosen from use.  title_show is reached through
 * the vector stub 1d99:1157, from gamemain.c, and
 * ending_show through 1d99:134b, called from section 3 at 0x3296a; no
 * vector and no far call reaches titleseq_unit_init.
 *
 * _DATA is three literals, DGROUP 6dc2..6dd0.  _BSS is empty.
 */

#include "dqk.h"

/* The translated Pascal unit's empty initialization block (DKK intro.pas). */
void far titleseq_unit_init(void)                                   /* 264f:000b */
{
}

char far title_wait(unsigned ticks)                              /* 264f:000c */
{
    char key_code;
    long now;
    long until;
    int key;

    input_pending_clear();
    now = ticks_now();
    until = ticks + now;
    key = ' ';
    for (;;) {
        while (!key_pressed() && ticks_now() < until) {
            if (is_xvga() || !pixel_shift())
                input_poll();
        }
        if (key_poll()) {
            key = key_get();
            if (key == 0x152) {
                sound_toggle();
                continue;
            }
            if (key == 0x153) {
                music_toggle();
                continue;
            }
        }
        break;
    }
    if (ticks_now() >= until)
        key_code = ' ';
    else
        key_code = key;
    key_flush();
    return key_code;
}

void far title_pic(char far *name, unsigned char picture_id, char fade,
                   char unused)                                  /* 264f:00bf */
{
    struct Res far *res;
    short set;
    char file[12];
    unsigned char rgb[0x300];

    mem_fill(rgb, sizeof(rgb), 0);
    res = 0;
    str_copy(file, name);
    str_cat(file, "1");
    switch (picture_id) {
    case 1:
    case 2:
    case 3:
        set = 1;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        set = 5;
        break;
    default:
        set = 0;
    }
    if (set != 0) {
        tile_open_file(file, set, 1, 0, &res);
        screen_draw_begin(0);
        glyph_draw(0, 0, 1, res);
        res_palette_load(res);
        res_free(&res);
    }
    switch (picture_id) {
    case 1:
    case 2:
    case 3:
        set = picture_id + 1;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        set = picture_id + 2;
        break;
    }
    tile_open_file(file, set, 1, 0, &res);
    if (picture_id == 6) {
        res_palette_load(res);
        screen_flush();
        screen_draw_begin(0);
    }
    glyph_draw(0, 0, 1, res);
    switch (picture_id) {
    case 3:
    case 4:
    case 5:
    case 7:
    case 8:
        glyph_draw(0, 0, 2, res);
    }
    if (fade) {
        pal_set(16, 240, rgb, 0, 0);
        pal_commit();
    }
    res_palette_load(res);
    screen_flush();
    res_free(&res);
}

void far ending_pause(int pause_units)                                     /* 264f:02c3 */
{
    screen_flush();
    title_wait(pause_units * 36);
}

void far ending_frame(int base, int over, struct Res far *resource)     /* 264f:02db */
{
    screen_draw_begin(1);
    glyph_draw(1, 1, base, resource);
    if (over > 0)
        glyph_draw(1, 1, over, resource);
}

void far title_done(void)                                        /* 264f:0319 */
{
    if (is_xvga() || !pixel_shift())
        glyph_palette_load(slot_base(0), 0);
    pointer_show();
}

void far title_show(void)                                        /* 264f:0345 */
{
    short index;
    char key;

    mode_change(5);
    set_text_mode(0);
    pal_select(0);
    sfx_play(32);
    set_open_mode('1');
    pointer_hide();
    for (index = 1; index <= 8; index++) {
        title_pic("title", index, 1, 1);
        if ((key = title_wait(800)) == 0x1b) {
            pointer_show();
            return;
        }
        if (key != ' ')
            break;
    }
    title_done();
}

void far ending_show(void)                                       /* 264f:03cb */
{
    struct Res far *res;
    int index;

    mode_change(5);
    screen_draw_begin(0);
    frame_lower_panel();
    pointer_hide();
    set_open_mode('3');
    res = 0;
    tile_open_file("final1", 1, 0, 0, &res);
    sfx_play(39);
    res_palette_load(res);
    pal_select(res);
    ending_frame(1, -1, res);
    ending_pause(1);
    for (index = 2; index <= 16; index++) {
        ending_frame(1, index, res);
        ending_pause(1);
    }
    ending_pause(1);
    res_free(&res);

    set_open_mode('3');
    tile_open_file("final1", 3, 0, 0, &res);
    res_palette_load(res);
    pal_select(res);
    ending_frame(1, -1, res);
    ending_pause(2);
    for (index = 2; index <= 6; index++) {
        ending_frame(1, index, res);
        ending_pause(2);
    }
    ending_frame(1, 5, res);
    ending_pause(1);
    ending_frame(1, 7, res);
    ending_pause(2);
    for (index = 8; index <= 15; index++)
        ending_frame(1, index, res);
    for (index = 0; index < 3; index++) {
        ending_frame(1, 16, res);
        ending_pause(1);
        ending_frame(1, 17, res);
        ending_pause(1);
    }
    ending_frame(1, 16, res);
    ending_pause(3);
    res_free(&res);

    set_open_mode('3');
    tile_open_file("final1", 2, 0, 0, &res);
    res_palette_load(res);
    pal_select(res);
    ending_frame(1, -1, res);
    for (index = 0; index < 6; index++) {
        ending_frame(1, 1, res);
        ending_pause(1);
        ending_frame(1, 2, res);
        ending_pause(1);
        ending_frame(1, 3, res);
        ending_pause(1);
    }
    ending_frame(1, 4, res);
    ending_pause(1);
    ending_frame(1, 1, res);
    ending_frame(1, 3, res);
    ending_pause(1);
    ending_frame(1, 4, res);
    ending_frame(1, 1, res);
    ending_pause(1);
    ending_frame(1, 4, res);
    ending_frame(1, 1, res);
    ending_pause(1);
    ending_frame(1, 2, res);
    ending_frame(1, 5, res);
    ending_pause(1);
    ending_frame(1, 3, res);
    ending_frame(1, 5, res);
    ending_pause(1);
    ending_frame(1, 1, res);
    ending_frame(1, 6, res);
    ending_pause(1);
    ending_frame(1, 2, res);
    ending_frame(1, 6, res);
    ending_pause(1);
    ending_frame(1, 3, res);
    ending_frame(1, 7, res);
    ending_pause(1);
    ending_frame(1, 1, res);
    ending_frame(1, 7, res);
    ending_pause(1);
    ending_frame(1, 2, res);
    ending_frame(1, 8, res);
    ending_pause(1);
    ending_frame(1, 3, res);
    ending_frame(1, 8, res);
    ending_pause(1);
    ending_frame(1, 1, res);
    ending_frame(1, 9, res);
    ending_pause(1);
    ending_frame(1, 2, res);
    ending_frame(1, 9, res);
    ending_pause(1);
    ending_frame(1, 3, res);
    ending_frame(1, 10, res);
    ending_pause(1);
    ending_frame(1, 1, res);
    ending_frame(1, 10, res);
    ending_pause(1);
    ending_frame(1, 2, res);
    ending_frame(1, 11, res);
    ending_pause(1);
    ending_frame(1, 3, res);
    ending_frame(1, 12, res);
    ending_pause(1);
    ending_frame(1, 13, res);
    ending_pause(2);
    ending_frame(1, 14, res);
    ending_pause(2);
    ending_frame(1, 13, res);
    ending_pause(1);
    ending_frame(1, 14, res);
    ending_pause(2);
    res_free(&res);
    pointer_show();
}
