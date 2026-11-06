/* DQK.EXE resident module 25a7: scrolling and saving a screen window, and
 * loading a font.  win_scroll moves a window's pixels by a row and column
 * offset one pixel row at a time and, when attr is not negative, fills a
 * row strip and a column strip, either of which may be empty.  win_save hands the window, row by row or
 * whole as video_mode selects, to 10d7:0286 or 10d7:0241 with a far pointer
 * into the page segment that is not draw_seg; tiles.c saves rows with
 * 0286, so "save" is a placeholder.  font_load reads a .FON file: 0x480
 * bytes into font_cells, then 0x180 more into a far block allocated on
 * first use.
 *
 * EXE 0x017e76..0x0183f9, one Turbo C large-model _TEXT segment starting at
 * byte 6 of paragraph 25a7.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Module 251e's last routine ends at 0x017e76, and module 25ff starts at
 * 0x0183f9 (25ff:0009) with no stub between.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the
 * rest are placeholders chosen from use.
 *
 * font_cells and font_ext are extern here: this module's _DATA is 9ab4..9abb,
 * and module 22b7's character drawing reads both (0x015c7a, 0x015cb8).
 *
 * Nothing found calls win_save: no far call, far jump, callback push or
 * DGROUP far pointer names 25a7:0284.
 */

#include "dqk.h"

extern void far path_default_ext(char far *path, char far *ext, int force); /* 1c60:025c */

static char col_round[4] = { 0, 1, 3, 7 };                       /* 9ab4, (1 << n) - 1 per pixel_shift() */

void far win_scroll(int row1, int col1, int row2, int col2,
                    int drow, int dcol, int attr)                /* 25a7:0006 */
{
    int src;
    int index;
    int from_col;
    int to_col;
    int width;
    char planar;
    register int from;
    register int destination_row;

    text_xy(row1, col1, &row1, &col1);
    text_xy(row2, col2, &row2, &col2);
    text_offset_xy(row1, col1, drow, dcol, &drow, &dcol);
    drow -= row1;
    dcol -= col1;
    if (dcol > 0) {
        from_col = col1 + dcol;
        to_col = col1;
        width = ((col_round[pixel_shift()] + col2) >> pixel_shift()) -
                (from_col >> pixel_shift());
    } else {
        from_col = col1;
        to_col = col1 - dcol;
        width = ((col_round[pixel_shift()] + col2) >> pixel_shift()) -
                (to_col >> pixel_shift());
    }
    pointer_hide_rect(row1, col1, row2, col2);
    planar = (video_mode >= 5 && video_mode <= 9) || video_mode == 15;
    for (index = 0; index < video_planes(); index++) {
        if (planar) {
            if (index) {
                video_plane_set(0);
                break;
            }
            video_plane_set(0xffef);
        } else
            video_plane_set(index);
        if (drow > 0) {
            for (from = row1 + drow, destination_row = row1; from < row2; from++, destination_row++) {
                video_address_set(from, from_col);
                src = screen_addr;
                video_address_set(destination_row, to_col);
                far_move(src, draw_seg, screen_addr, draw_seg, width);
            }
        } else {
            from = row2 + drow;
            destination_row = row2;
            while (--from >= row1) {
                destination_row--;
                video_address_set(from, from_col);
                src = screen_addr;
                video_address_set(destination_row, to_col);
                far_move(src, draw_seg, screen_addr, draw_seg, width);
            }
        }
    }
    pointer_show();
    if (attr >= 0) {
        if (drow > 0)
            fill_rect(row2 - drow, col1, row2, col2, attr);
        else
            fill_rect(row1, col1, row1 - drow, col2, attr);
        if (dcol > 0)
            fill_rect(row1, col2 - dcol, row2, col2, attr);
        else
            fill_rect(row1, col1, row2, col1 - dcol, attr);
    }
}

void far win_save(int row1, int col1, int row2, int col2)       /* 25a7:0284 */
{
    long base;
    register int width;
    register int height;

    text_xy(row1, col1, &row1, &col1);
    text_xy(row2, col2, &row2, &col2);
    height = row2 - row1;
    width = ((col_round[pixel_shift()] + col2) >> pixel_shift()) -
            (col1 >> pixel_shift());
    if (height <= 0 || width <= 0)
        return;
    if (draw_seg == display_seg)
        base = mem_top_seg;
    else
        base = display_seg;
    base <<= 16;
    pointer_hide_rect(row1, col1, row2, col2);
    switch (video_mode) {
    case 1: case 2: case 3: case 4: case 11: case 12: case 13: case 17:
        for (; row1 < row2; row1++) {
            video_address_set(row1, col1);
            if (width & 1)
                blit_bytes((void far *)(screen_addr + base), 1, width,
                             (screen_cols() >> pixel_shift()) - width);
            else
                blit_words((void far *)(screen_addr + base), 1, width,
                             (screen_cols() >> pixel_shift()) - width);
        }
        break;
    case 5: case 6: case 7: case 9: case 15:   /* win_scroll's 5..9 and 15 hold 8 */
        video_plane_set(0xffef);
        video_address_set(row1, col1);
        blit_bytes((void far *)(screen_addr + base), height, width,
                     (screen_cols() >> pixel_shift()) - width);
        video_plane_set(0);
        break;
    default:
        video_address_set(row1, col1);
        if (width & 1)
            blit_bytes((void far *)(screen_addr + base), height, width,
                         (screen_cols() >> pixel_shift()) - width);
        else
            blit_words((void far *)(screen_addr + base), height, width,
                         (screen_cols() >> pixel_shift()) - width);
        break;
    }
    pointer_show();
}

/* Read 96 twelve-byte base glyphs, then an optional 32 control glyphs.
 * Note: a short base read leaves the partial update in place. An incomplete
 * extension or failed extension allocation still returns success. */
char far font_read_cb(int file_handle)                                    /* 25a7:04b5 */
{
    char buf[0x180];

    if (dos_io(0x3f00, font_cells, file_handle, 0x480) != 0x480)
        return 0;
    if (dos_io(0x3f00, buf, file_handle, 0x180) == 0x180) {
        if (font_ext == 0)
            font_ext = mem_alloc(0x180);
        if (font_ext != 0)
            mem_move(buf, font_ext, 0x180);
    }
    return 1;
}

void far font_load(char disk, char far *name)                    /* 25a7:053e */
{
    char path[80];

    str_copy(path, name);
    path_default_ext(path, "FON", 1);
    file_open(disk, path, 0, font_read_cb);
}
