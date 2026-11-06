/* DQK.EXE resident module 269b: glyphs drawn from the tile libraries.
 * A glyph is an 8-byte header and its bits, found by index in a library
 * a slot holds.  The header's low type nibble picks the format: 9 is a
 * list of parts drawn in turn, 10 splits the glyph in two at split_rows,
 * 2, 3 and 7 are packed forms handed to runtime decoders, 0, 1 and 5 are
 * drawn plane by plane by blit_planes, which draws nothing for any other
 * type, and 8 is a palette that only pal_glyph uploads.  glyph_blit returns
 * early for a glyph wholly outside clip_top, clip_bottom, clip_left and
 * clip_right, and cuts the rest to them.  fill_pattern stores a colour's
 * color_map entry as one word when is_xvga() is set or pixel_shift() is 0,
 * and otherwise 16 words from the two dither tables; pointer_glyph_load copies a
 * glyph into glyph_buf and hands it to 12b7:25ff.  glyph_library_init registers
 * the "TILE" converter with the library layer.
 *
 * EXE 0x018db6..0x019d6c, one Turbo C large-model _TEXT segment starting at
 * byte 6 of paragraph 269b.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Module 261b's last retf is at 0x018db5, and module 2796 starts at
 * 0x019d6c (2796:000c, far-called from tiles.c at 0x00c3b1).  Ghidra's
 * index misses glyph_library_init, far-called from sysutil.c at 0x01348b.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the
 * rest are placeholders chosen from use.
 */

#include "dqk.h"

struct Part {                   /* one part of a type 9 glyph */
    unsigned char glyph;        /* +0 */
    unsigned char count;        /* +1 the loop bound, re-read from each part */
    int dy;                     /* +2 */
    int dx;                     /* +4 */
};

struct GlyphBuf {               /* a glyph copied for 12b7:25ff */
    unsigned char dy;           /* d4b4 */
    unsigned char dx;           /* d4b5 */
    unsigned char rows;         /* d4b6 */
    unsigned char width;        /* d4b7 */
    unsigned char bits[256];    /* d4b8 */
    unsigned char planes[4][64]; /* d5b8 */
};

extern void far lib_open(char mode, char far *name, int slot);   /* 193f:00d5 */
extern void far path_default_ext(char far *path, char far *ext, int force); /* 1c60:025c */
extern void far huge_move(void far *src, void far *dst, unsigned n); /* 00d7:08c2 */

static char blank_9b02[4] = {0};             /* 9b02, no reference found; type a guess */
static unsigned char dith4[96][4] = {                            /* 9b06, up to dith8 */
    {0x00, 0x00, 0x00, 0x00}, {0x80, 0x00, 0x00, 0x00}, {0x80, 0x00, 0x08, 0x00}, {0x80, 0x04, 0x20, 0x00},
    {0x88, 0x00, 0x22, 0x00}, {0x88, 0x00, 0x88, 0x00}, {0x80, 0x08, 0x80, 0x08}, {0x82, 0x28, 0x82, 0x00},
    {0x4a, 0x00, 0xa4, 0x00}, {0xa8, 0x00, 0x45, 0x00}, {0xaa, 0x00, 0xaa, 0x00}, {0xaa, 0x00, 0x55, 0x00},
    {0x88, 0x22, 0x44, 0x11}, {0x88, 0x22, 0x88, 0x22}, {0x88, 0x88, 0x22, 0x22}, {0x24, 0x90, 0x24, 0x90},
    {0x82, 0x28, 0x82, 0x28}, {0xa2, 0x44, 0x2a, 0x11}, {0x88, 0x22, 0x88, 0x55}, {0x88, 0xaa, 0x88, 0x22},
    {0x8a, 0x22, 0xa8, 0x22}, {0xa2, 0xa0, 0x2a, 0x0a}, {0xa8, 0xa2, 0x8a, 0x2a}, {0x94, 0x52, 0x49, 0x25},
    {0xaa, 0x11, 0xaa, 0x44}, {0xaa, 0x22, 0xaa, 0x88}, {0xaa, 0x15, 0xaa, 0x51}, {0xaa, 0x25, 0xaa, 0x52},
    {0xaa, 0x2a, 0xaa, 0xa2}, {0xaa, 0x4a, 0xaa, 0xa4}, {0xaa, 0x33, 0xcc, 0x55}, {0xaa, 0x55, 0xaa, 0x55},
    {0xaa, 0xaa, 0x55, 0x55}, {0xaa, 0xaa, 0xaa, 0xaa}, {0xaa, 0x55, 0xaa, 0xaa}, {0xda, 0xaa, 0xad, 0xaa},
    {0xba, 0xaa, 0xab, 0xaa}, {0xb5, 0xaa, 0x5b, 0xaa}, {0x75, 0xaa, 0x57, 0xaa}, {0xee, 0xaa, 0xbb, 0xaa},
    {0xaa, 0x77, 0xaa, 0xdd}, {0x5b, 0x6d, 0xb5, 0xd6}, {0xab, 0xae, 0xba, 0xea}, {0xbb, 0xea, 0xbb, 0xae},
    {0xbb, 0xee, 0xaa, 0xee}, {0x55, 0xee, 0xbb, 0xee}, {0x5d, 0xbb, 0xd5, 0xee}, {0xeb, 0xbe, 0xeb, 0xbe},
    {0x6f, 0xbd, 0xf6, 0xdb}, {0xbb, 0xbb, 0xee, 0xee}, {0xbb, 0xee, 0xbb, 0xee}, {0x77, 0xdd, 0xbb, 0xee},
    {0xff, 0x55, 0xff, 0xaa}, {0xff, 0xaa, 0xff, 0xaa}, {0xff, 0x5d, 0xff, 0xba}, {0xff, 0xea, 0xff, 0x5d},
    {0xff, 0xbe, 0xeb, 0xbe}, {0xef, 0xfe, 0xef, 0xfe}, {0xff, 0xee, 0xff, 0xee}, {0xff, 0xbb, 0xff, 0xee},
    {0xff, 0xfb, 0xdf, 0xfe}, {0xff, 0xef, 0xff, 0xfe}, {0xff, 0xff, 0xff, 0xfe}, {0xff, 0xff, 0xff, 0xff},
    {0x55, 0x00, 0x55, 0x00}, {0x00, 0xaa, 0x00, 0xaa}, {0x00, 0x55, 0x00, 0x55}, {0xff, 0x00, 0xff, 0x00},
    {0x55, 0xaa, 0x55, 0xaa}, {0x00, 0xff, 0x00, 0xff}, {0x55, 0x55, 0x55, 0x55}, {0xaa, 0xff, 0xaa, 0xff},
    {0x55, 0xff, 0x55, 0xff}, {0xbb, 0xff, 0xee, 0xff}, {0xaa, 0x00, 0xaa, 0x00}, {0xaa, 0x00, 0x55, 0x00},
    {0x88, 0x22, 0x44, 0x11}, {0x88, 0x22, 0x88, 0x22}, {0x88, 0x88, 0x22, 0x22}, {0x24, 0x90, 0x24, 0x90},
    {0x82, 0x28, 0x82, 0x28}, {0xa2, 0x44, 0x2a, 0x11}, {0x88, 0x22, 0x88, 0x55}, {0x88, 0xaa, 0x88, 0x22},
    {0x8a, 0x22, 0xa8, 0x22}, {0xa2, 0xa0, 0x2a, 0x0a}, {0xa8, 0xa2, 0x8a, 0x2a}, {0x94, 0x52, 0x49, 0x25},
    {0xaa, 0x11, 0xaa, 0x44}, {0xaa, 0x22, 0xaa, 0x88}, {0xaa, 0x15, 0xaa, 0x51}, {0xaa, 0x25, 0xaa, 0x52},
    {0xaa, 0x2a, 0xaa, 0xa2}, {0xaa, 0x4a, 0xaa, 0xa4}, {0xaa, 0x33, 0xcc, 0x55}, {0xaa, 0x55, 0xaa, 0x55},
};
static unsigned char dith8[32][8] = {                            /* 9c86 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0x11, 0x11, 0x40, 0x04, 0x22, 0x22, 0x44, 0x11}, {0x99, 0x99, 0x25, 0x52, 0x33, 0x33, 0x55, 0x55},
    {0xbb, 0xbb, 0xf5, 0x5f, 0xaa, 0xaa, 0x77, 0xdd}, {0x22, 0x22, 0x11, 0x00, 0x11, 0x11, 0x01, 0x10},
    {0x77, 0x77, 0xd5, 0x5d, 0x99, 0x99, 0x77, 0x11}, {0x4c, 0xc4, 0xaa, 0x45, 0x44, 0x44, 0xaa, 0xaa},
    {0xcc, 0xcc, 0xda, 0xad, 0xcc, 0xcc, 0xbb, 0xee}, {0xdd, 0xdd, 0xee, 0xff, 0xee, 0xee, 0xee, 0xff},
    {0xaa, 0xaa, 0x18, 0xc2, 0x88, 0x88, 0x0c, 0xc0}, {0x5a, 0xa5, 0x5f, 0xea, 0x77, 0x77, 0x33, 0xcc},
    {0x88, 0x88, 0x88, 0x22, 0x44, 0x44, 0x20, 0x02}, {0x44, 0x44, 0xaa, 0x55, 0x66, 0x66, 0xee, 0x99},
    {0x45, 0x54, 0xa0, 0x0a, 0x55, 0x55, 0x88, 0x22}, {0xee, 0xee, 0x77, 0xdd, 0xcc, 0xcc, 0xff, 0xff},
    {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}, {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}, {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}, {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}, {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}, {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}, {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
    {0x14, 0x41, 0x14, 0x41, 0x14, 0x41, 0x14, 0x41}, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
};
static unsigned char color_map[16] = {                           /* 9d86 */
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static int pat_ofs;                                              /* d4b2, written only */
static struct GlyphBuf glyph_buf;                                /* d4b4 */
static int split_rows;                                           /* d6b8 */

void far glyph_library_init(void)                                      /* 269b:0006 */
{
    /* tile_loader_identity returns a far pointer; the library reads its dx:ax as a long */
    lib_register(0x454c4954L, (long (far *)())tile_loader_identity);
    split_rows = 8;
    pat_ofs = (video_mode - 1) * 2;
}

/* Empty mirror of glyph_library_init; lib_done drops the registrations. */
void far glyph_library_done(void)                                      /* 269b:002e */
{
}

void far glyph_library_open(char mode, char far *name, int slot)       /* 269b:002f */
{
    char path[80];

    str_copy(path, name);
    path_default_ext(path, "TLB", 1);
    lib_open(mode, path, slot);
}

char far *far glyph_find(char far *lib, int glyph_index, struct Glyph far *hdr) /* 269b:0075 */
{
    char far *glyph_data;

    if (glyph_index >= 0)
        glyph_data = lib_item(lib, glyph_index);
    else
        glyph_data = lib;
    huge_move(glyph_data, hdr, 8);
    return huge_add(glyph_data, 8L);
}

char far *far glyph_words(char far *data_cursor, int far *first_word, int far *second_word)  /* 269b:00d4 */
{
    int words[2];

    huge_move(data_cursor, words, 4);
    *first_word = words[0];
    *second_word = words[1];
    return huge_add(data_cursor, 4L);
}

void far color_map_set(char far *map)                            /* 269b:011c */
{
    mem_move(map, color_map, 16);
}

void far fill_pattern(int color, int far *out)                   /* 269b:0137 */
{
    short pattern_byte;
    short mask;
    register int index;
    register int ofs;

    if (video_mode <= 4) {
        ofs = (video_mode - 1) * 2;
        mask = 1;
    } else {
        ofs = 8;
        mask = 3;
    }
    color &= 0xf;
    if (is_xvga() || !pixel_shift()) {
        *out = color_map[color];
        return;
    }
    for (index = 0; index < 16; index++) {
        if (ofs == 8)
            pattern_byte = dith4[color_map[color] & 0x7f][index & mask]; /* rows past 95 read dith8 */
        else
            pattern_byte = dith8[color_map[color] & 0x1f][(index & mask) + ofs];
        *out = pattern_byte << 8 | pattern_byte;
        out++;
    }
}

void far blit_planes(char far *plane_data, short row, short col, int stride, short rows,
                     short width, short planes, short skip,
                     unsigned char type)                         /* 269b:0204 */
{
    char far *mask_data;
    register int index;

    switch (type) {
    case 0:
        video_address_set(row, col);
        if (!(width & 1)) {
            for (index = 0; index < planes; index++) {
                video_plane_set(index);
                blit_words(plane_data, rows, width, skip);
                plane_data += stride;
            }
        } else {
            for (index = 0; index < planes; index++) {
                video_plane_set(index);
                blit_bytes(plane_data, rows, width, skip);
                plane_data += stride;
            }
        }
        break;
    case 1:
        video_address_set(row, col);
        if (is_xvga()) {
            mask_data = plane_data;
            plane_data += stride * planes;
            for (index = 0; index < planes; index++) {
                video_plane_set(index);
                blit_masked_w(plane_data, mask_data, rows, width, skip);
                plane_data += stride;
                mask_data += stride;
            }
        } else if (!(width & 1)) {
            mask_data = plane_data;
            for (index = 0; index < planes; index++) {
                plane_data += stride;
                video_plane_set(index);
                blit_masked_w(plane_data, mask_data, rows, width, skip);
            }
        } else {
            mask_data = plane_data;
            for (index = 0; index < planes; index++) {
                plane_data += stride;
                video_plane_set(index);
                blit_masked_b(plane_data, mask_data, rows, width, skip);
            }
        }
        break;
    case 5:
        video_address_set(row, col);
        for (index = 0; index < planes; index++) {
            video_plane_set(index);
            keyed_blit(plane_data, rows, width, skip, 0xff);
            plane_data += stride;
        }
        break;
    }
}

void far blit_runs(int row, int col, int rows, int width, int planes, int top,
                   int left, int bottom, int right, char far *run_data) /* 269b:03ba */
{
    register int index;

    video_address_set(row, col);
    for (index = 0; index < planes; index++) {
        video_plane_set(index);
        run_data = runs_draw(top, bottom, rows, left, width - left - right, run_data);
    }
}

void far blit_packed(short row, int col, short rows, short width, short planes,
                     short top, short left, int right, char far *packed_data,
                     char flag)                                  /* 269b:0409 */
{
    short scan_index;
    char line[0x140];
    char buf[8][40];
    register int row_bytes;
    register int visible_width;

    row_bytes = width * planes;
    if (video_mode == 14 && !flag)
        row_bytes >>= 1;
    if (video_planes() != planes && flag)
        row_bytes <<= 2;
    visible_width = width - left - right;
    while (--top >= 0)
        packed_data = packbits_expand(packed_data, line, row_bytes);
    if (video_mode == 15) {
        flag = 1;
        planes = 1;
    }
    if (video_mode == 14 && !flag) {
        while (--rows >= 0) {
            packed_data = packbits_expand(packed_data, line, row_bytes);
            video_address_set(row++, col);
            split_nibbles(line, buf[0], row_bytes);
            blit_words(buf[0] + left, 1, visible_width, 0);
        }
    } else if (planes == 4) {
        while (--rows >= 0) {
            packed_data = packbits_expand(packed_data, line, row_bytes);
            video_address_set(row++, col);
            nibbles_to_planar(line, buf[0], 40, row_bytes);
            for (scan_index = 0; scan_index < planes; scan_index++) {
                video_plane_set(scan_index);
                blit_bytes(buf[scan_index] + left, 1, visible_width, 0);
            }
        }
    } else if (video_planes() == 4 && flag) {
        if (!(visible_width & 1) && !(left & 1) && !(width & 1)) {
            while (--rows >= 0) {
                packed_data = packbits_expand(packed_data, line, row_bytes);
                video_address_set(row++, col);
                video_plane_set(0);
                blit_words(line + left, 1, visible_width, 0);
                video_plane_set(1);
                blit_words(line + (width + left), 1, visible_width, 0);
                video_plane_set(2);
                blit_words(line + (width + width + left), 1, visible_width, 0);
                video_plane_set(3);
                blit_words(line + (width + width + width + left), 1, visible_width, 0);
            }
        } else {
            while (--rows >= 0) {
                packed_data = packbits_expand(packed_data, line, row_bytes);
                video_address_set(row++, col);
                for (scan_index = 0; scan_index < 4; scan_index++) {
                    video_plane_set(scan_index);
                    blit_bytes(line + (width * scan_index + left), 1, visible_width, 0);
                }
            }
        }
    } else if (!(visible_width & 1) && !(left & 1)) {
        video_plane_set(0);
        while (--rows >= 0) {
            packed_data = packbits_expand(packed_data, line, row_bytes);
            video_address_set(row++, col);
            blit_words(line + left, 1, visible_width, 0);
        }
    } else {
        video_plane_set(0);
        while (--rows >= 0) {
            packed_data = packbits_expand(packed_data, line, row_bytes);
            video_address_set(row++, col);
            blit_bytes(line + left, 1, visible_width, 0);
        }
    }
}

void far glyph_blit(char far *glyph_data, register int rows, short width, short row,
                    short col, unsigned char type)               /* 269b:07ca */
{
    char far *run_table;
    short left;
    short bottom;
    short right;
    short scan_index;
    short planes;
    short size;
    short off;
    char flag;
    register int cut;

    if (row >= clip_bottom)
        return;
    if (row + rows <= clip_top)
        return;
    if (col >= clip_right)
        return;
    if ((width << pixel_shift()) + col <= clip_left)
        return;
    planes = (type & 0x80) ? 1 : video_planes();
    flag = type & 0x40;
    type &= 0xf;
    if (type == 10) {
        cut = split_rows;
        switch (planes) {
        case 1:
            size = width * 2 * (rows - cut);
            scan_index = 0x80;
            break;
        case 4:
            scan_index = 0;
            size = (rows - cut) * width * 5;
            break;
        }                               /* other counts leave size and k unset, as shipped */
        if (rows > cut)
            glyph_blit(glyph_data, rows - cut, width, row, col, scan_index + 1);
        glyph_blit(glyph_data + size, cut, width, row + rows - cut, col, scan_index);
        return;
    }
    size = rows * width;
    if (row < clip_top)
        cut = clip_top - row;
    else
        cut = 0;
    if (row + rows > clip_bottom) {
        bottom = rows - (clip_bottom - row);
        rows = clip_bottom - row;
    } else
        bottom = 0;
    if (col < clip_left)
        left = (clip_left - col) >> pixel_shift();
    else
        left = 0;
    if ((width << pixel_shift()) + col > clip_right)
        right = ((width << pixel_shift()) + col - clip_right) >> pixel_shift();
    else
        right = 0;
    row += cut;
    col = int_max(col, clip_left);
    rows -= cut;
    if (type == 3) {
        run_table = glyph_words(glyph_data, &off, &size);
        glyph_data = run_table + off;
        for (scan_index = 0; scan_index < planes; scan_index++) {
            video_plane_set(scan_index);
            video_address_set(row, col);
            rle_draw(cut, rows, left, width - left - right, run_table, glyph_data);
            glyph_data += size;
        }
    } else if (type == 2)
        blit_packed(row, col, rows, width, planes, cut, left, right, glyph_data, flag);
    else if (type == 7)
        blit_runs(row, col, rows, width, planes, cut, left, bottom, right, glyph_data);
    else {
        glyph_data += cut * width + left;
        right += left;
        width -= right;
        if (rows > 0 && width > 0)
            blit_planes(glyph_data, row, col, size, rows, width, planes, right, type);
    }
}

void far glyph_draw_at(int row, int col, char far *lib, int glyph) /* 269b:0a9c */
{
    struct Glyph hdr;
    struct Part part;
    char far *glyph_data;
    register int part_count_or_width;
    register int index;

    text_xy(row, col, &row, &col);
    if ((glyph_data = glyph_find(lib, glyph, &hdr)) == 0)
        return;
    row -= hdr.dy;
    col -= hdr.dx;
    if ((hdr.type & 0xf) == 9) {
        for (part_count_or_width = 1, index = 0; index < part_count_or_width; index++) {
            huge_move(glyph_data, &part, 6);
            glyph_data = huge_add(glyph_data, 6L);
            glyph_draw_at(row + part.dy, col + part.dx, lib, part.glyph);
            part_count_or_width = part.count;
        }
        return;
    }
    part_count_or_width = hdr.width;
    if (!pixel_shift())
        part_count_or_width <<= 3;
    if (text_mode())
        pointer_hide_rect(row, col, row + hdr.rows, (part_count_or_width << pixel_shift()) + col);
    glyph_blit(glyph_data, hdr.rows, part_count_or_width, row, col, hdr.type);
    if (text_mode())
        pointer_show();
}

void far draw_glyph(int row, int col, int library_slot, int glyph)          /* 269b:0bd0 */
{
    glyph_draw_at(row, col, slot_base(library_slot), glyph);
}

void far glyph_bounds(int row, int col, int slot, int glyph, int far *top,
                      int far *left, int far *bottom, int far *right) /* 269b:0bef */
{
    struct Glyph hdr;
    register int right_edge;
    register int bottom_edge;

    if (glyph_find(slot_base(slot), glyph, &hdr) == 0)
        return;
    text_xy(row, col, &row, &col);
    col -= hdr.dx;
    row -= hdr.dy;
    bottom_edge = row + hdr.rows;
    if (!pixel_shift())
        right_edge = (hdr.width << 3) + col;
    else
        right_edge = (hdr.width << pixel_shift()) + col;
    *top = int_max(row, clip_top);
    *left = int_max(col, clip_left);
    *bottom = int_min(bottom_edge, clip_bottom);
    *right = int_min(right_edge, clip_right);
    if (*top >= *bottom || *left >= *right)
        *top = *bottom = *left = *right = 0;
}

void far pointer_glyph_load(int slot, int glyph_index)                           /* 269b:0cf8 */
{
    struct Glyph hdr;
    char far *glyph_data;
    short size;
    unsigned char far *src;
    unsigned char far *dst;
    register int index;
    register int count;

    if ((glyph_data = glyph_find(slot_base(slot), glyph_index, &hdr)) == 0)
        return;
    if (video_mode == 14)
        hdr.width <<= 3;
    count = hdr.width * hdr.rows;
    size = count;
    hdr.width <<= pixel_shift();
    if (video_planes() * size > 0x100) {
        hdr.rows = int_min(hdr.rows, video_planes() > 1 ? 16 : 24);
        hdr.width = int_min(hdr.width, screen_cols() > 400 ? 32 : 16);
        count = 0x100 / video_planes();
    }
    mem_fill(&glyph_buf, sizeof(glyph_buf), 0);
    glyph_buf.dy = hdr.dy;
    glyph_buf.dx = hdr.dx;
    glyph_buf.rows = hdr.rows;
    glyph_buf.width = hdr.width;
    huge_move(glyph_data, glyph_buf.bits, (is_xvga() ? video_planes() : 1) * count);
    glyph_data = huge_add(glyph_data, (long)size * (is_xvga() ? video_planes() : 1));
    for (index = 0; index < int_min(video_planes(), 4); index++) {
        if (index == 0 || !(hdr.type & 0xe0) || (hdr.type & 0xf0) >= 0xd0)
            huge_move(glyph_data, glyph_buf.planes[index], count);
        else
            mem_fill(glyph_buf.planes[index], count, 0);
        glyph_data = huge_add(glyph_data, (long)size);
    }
    if ((hdr.type & 0xf) == 5) {
        src = glyph_buf.bits;
        dst = glyph_buf.planes[0];
        for (index = 0; index < 0x100; index++) {
            if (*src == 0xff) {
                *dst++ = 0;
                src++;
            } else {
                *dst++ = *src;
                *src++ = 0;
            }
        }
    }
    pointer_shape_set(&glyph_buf);
}

void far pal_glyph(int slot, int glyph_index, char fade, char flag)        /* 269b:0f27 */
{
    struct Glyph hdr;
    char far *glyph_data;
    char pal[0x300];

    if ((glyph_data = glyph_find(slot_base(slot), glyph_index, &hdr)) != 0) {
        if ((hdr.type & 0xf) != 8)
            error_box("Invalid TPalette call %d %d", slot, glyph_index);
        else {
            huge_move(glyph_data, pal, 0x300);
            if (fade)
                palette_page_load(pal, flag);
            else
                palette_load(pal);
        }
    }
}
