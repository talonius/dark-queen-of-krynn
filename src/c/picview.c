/* DQK.EXE overlay 03 module 24da: the picture window and the big picture.
 * Opening a picture from the PIC tile libraries or by name, drawing it and
 * its animation frames at a cell position, and closing it; opening,
 * drawing and closing a bigpic library picture, which big_redraw reopens
 * from big_last when it has been closed.
 *
 * EXE 0x03679c..0x036b92, one Turbo C large-model _TEXT segment starting at
 * byte 0xc of paragraph 24da in RTLink section 3 (payload 0x30ed0, link
 * segment 1f4e).  The first 0xc bytes of paragraph 24da are the tail of
 * module 2177, and the last 2 bytes of the range lie in paragraph 251a,
 * whose module starts at 251a:0002 (far-called from 0x36722).  Built
 * with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped
 * order; bake consumes them in that order.  Section 0's 1f4e link segment
 * holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use.  Besides direct far calls,
 * pic_frame is reached through the vector stub 1d99:0f45 and pic_open
 * through 1d99:0f4f; no vector and no far call from sections 3 to 8
 * reaches picture_noop.  pic_open calls pic_release ahead of its
 * definition, so that one is prototyped.  menu.c reads pic's anim byte
 * (c2ef) as anim_speed.
 *
 * _DATA is five literals stored as four, DGROUP 6074..608c: -d stores ""
 * on the terminator of "PIC" (6077).  _BSS is big_last at acd2.
 */

#include "dqk.h"

void far pic_release(void);

static char big_last;                                            /* acd2 */

void far pic_frame(int col, int row, char show, unsigned char frame_index) /* 24da:000c */
{
    if (pic.res != 0) {
        if (show) {
            screen_draw_begin(1);
            if (frame_index > 0)
                glyph_draw(col, row, 1, pic.res);
            glyph_draw(col, row, frame_index + 1, pic.res);
        }
        res_palette_load(pic.res);
        screen_flush();
    }
}

void far pic_open(char far *name, unsigned char id, char unused,
                  struct Pic far *picture)                             /* 24da:0078 */
{
    char disk[2];
    char pal[0x300];

    mem_fill(pal, 0x300, 0);
    mode_change(str_same(name, "PIC") ? 2 : 0);
    if (str_same(name, pic_name) && id == view_kind || id == 0xff)
        return;
    pic_release();
    str_copy(pic_name, name);
    view_kind = id;
    int_to_str(pic_disk, disk);
    set_open_mode('2');
    if (str_same(name, "PIC"))
        tile_open_file(msg_format("PIC%c1",
                                  id < 0x80 ? 'A' : id < 0x97 ? 'B' : 'C'),
                       id, 0, 0, &picture->res);
    else
        tile_open_file(msg_format("%s%s", name, disk), id, 0, 0, &picture->res);
    if (str_same(name, "PIC"))
        pal_set(0x20, 0xe0, pal, 0, 0);
    else
        res_palette_load(picture->res);
    pal_commit();
    picture->frames = res_item_count(picture->res);
    if (str_same(name, "PIC")) {
        picture->frames--;
        picture->anim = 1;
    } else
        picture->anim = 0;
    anim_last = picture->frames - 1;
    anim_first = 0;
    if (str_same(name, "PIC")) {
        if (id == 0x0e)
            anim_first = 5;
        else if (id == 0x46)
            anim_first = 1;
        else if (id == 0x87)
            anim_first = 5;
        else if (id == 0xa1)
            anim_first = 3;
    }
    byte_b20b = 0;
    anim_frame = 0;
    moon_display_update(0);
    box_draw();
}

void far pic_release(void)                                   /* 24da:02b0 */
{
    res_free(&pic.res);
    mem_zero(&pic, 6);
    str_copy(pic_name, "");
    view_kind = -1;
}

void far pic_show(unsigned char frame_index)                               /* 24da:02e5 */
{
    if (pic.res != 0) {
        glyph_draw(3, 3, frame_index, pic.res);
        pal_select(pic.res);
        screen_flush();
    }
}

void far big_open(unsigned char id)                              /* 24da:0321 */
{
    char pal[0x300];

    mem_fill(pal, 0x300, 0);
    mode_change(3);
    if (view_flag != id) {
        set_open_mode('2');
        tile_open_file(msg_format("bigpic%d", pic_disk), id, 0, 0, &big_res);
        pal_set(0x20, 0xe0, pal, 0, 0);
        pal_commit();
        big_last = view_flag = id;
    }
}

void far big_redraw(void)                                        /* 24da:03b0 */
{
    if (view_flag == -1)
        big_open(big_last);
    frame_lower_panel();
    glyph_draw(1, 1, 1, big_res);
    res_palette_load(big_res);
}

void far bigpic_release(void)                                   /* 24da:03eb */
{
    res_free(&big_res);
    view_flag = -1;
}

void far picture_noop(int unused)                             /* 24da:03fd */
{
}
