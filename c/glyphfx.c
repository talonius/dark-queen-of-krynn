/* DQK.EXE resident module 2796: glyphs changed in place in their library.
 * glyph_mirror mirrors a glyph left to right ("Mirror" in its error
 * text): on XVGA it hands mem_swap plane blocks 0 and 3, then 1 and 2
 * (and 4 and 7, 5 and 6 when a type 1 glyph carries twice the planes),
 * then every row of every plane to one of four 00d7 routines, chosen by
 * is_xvga and pixel_shift.  glyph_recolor ("ClrTran") runs a glyph through
 * a table clrtran_table fills from a colour map.  A palette glyph (type 8)
 * has its first entry rewritten when rows bit 0 is set and its cycles'
 * first entries when bit 1 is, and is left alone when pixel_shift is set on
 * a screen that is not XVGA; types 0, 1 and 5 have their pixels rewritten,
 * and any other type raises error_box first.  glyph_palette_load ("TNPalette") hands
 * a palette glyph's RGB triples and colour cycles to pal_set.
 *
 * A palette glyph's header reads differently: rows holds flags (bit 0:
 * dy and dx give the first entry and the count; bit 1: width cycles follow
 * the RGB triples), and without bit 0 the palette starts at entry 0.
 *
 * EXE 0x019d6c..0x01a3ad, one Turbo C large-model _TEXT segment starting at
 * byte 0xc of paragraph 2796.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Module 269b's last retf is at 0x019d6b, and module 27fa starts at
 * 0x01a3ad (27fa:000d) with no stub between.  Nothing found far-calls or
 * pushes clrtran_table; its one caller is the push cs / near call at
 * 2796:032e.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the
 * rest are placeholders chosen from use.
 */

#include "dqk.h"

extern void far huge_move(void far *src, void far *dst, unsigned n); /* 00d7:08c2 */

void far glyph_mirror(char far *lib, int glyph_index)                      /* 2796:000c */
{
    struct Glyph hdr;
    char far *glyph_data;
    short planes;
    short scan_index;
    register int pixel_width;
    register int size;

    if ((glyph_data = glyph_find(lib, glyph_index, &hdr)) == 0)
        return;
    pixel_width = hdr.width;
    if (!pixel_shift())
        pixel_width <<= 3;
    planes = video_planes();
    if ((hdr.type & 0xf) == 1) {
        if (is_xvga())
            planes += planes;
        else
            planes++;
    } else if ((hdr.type & 0xf) && (hdr.type & 0xf) != 5)
        error_box("Invalid type (%x) to Mirror", hdr.type);
    if (is_xvga()) {
        size = hdr.width * hdr.rows;
        mem_swap(glyph_data, glyph_data + size * 3, size);
        mem_swap(glyph_data + size, glyph_data + size * 2, size);
        if (video_planes() < planes) {
            mem_swap(glyph_data + size * 4, glyph_data + size * 7, size);
            mem_swap(glyph_data + size * 5, glyph_data + size * 6, size);
        }
    }
    while (--planes >= 0) {
        scan_index = hdr.rows;
        while (--scan_index >= 0) {
            if (is_xvga())
                row_mirror8(glyph_data, pixel_width);
            else
                switch (pixel_shift()) {
                case 3:
                    row_mirror1(glyph_data, pixel_width);
                    break;
                case 2:
                    row_mirror2(glyph_data, pixel_width);
                    break;
                case 1:
                    row_mirror4(glyph_data, pixel_width);
                    break;
                case 0:
                    row_mirror8(glyph_data, pixel_width);
                    break;
                }
            glyph_data = huge_add(glyph_data, (long)pixel_width);
        }
    }
}

void far clrtran_table(unsigned char far *map, unsigned char far *tab) /* 2796:01ef */
{
    register int index;

    switch (is_xvga() ? 0 : pixel_shift()) {
    case 3:
        mem_move(map, tab, 16);
        break;
    case 2:
        for (index = 0; index < 0x100; index++)
            tab[index] = map[index & 3] | map[(index & 0xc) >> 2] << 2 |
                     map[(index & 0x30) >> 4] << 4 | map[(index & 0xc0) >> 6] << 6;
        break;
    case 1:
        for (index = 0; index < 0x100; index++)
            tab[index] = map[index & 0xf] | map[(index & 0xf0) >> 4] << 4;
        break;
    case 0:
        mem_move(map, tab, 0x100);
        break;
    }
}

void far glyph_recolor(char far *lib, int glyph_index, unsigned char far *map) /* 2796:02f4 */
{
    struct Glyph hdr;
    struct CycSpec cyc;
    char far *glyph_data;
    unsigned char tab[0x100];
    register int index;

    if ((glyph_data = glyph_find(lib, glyph_index, &hdr)) == 0)
        return;
    clrtran_table(map, tab);
    if ((hdr.type & 0xf) == 8) {
        if (pixel_shift() && !is_xvga())
            return;
        if (hdr.rows & 1) {
            hdr.dy = tab[hdr.dy];
            huge_move(&hdr, huge_add(glyph_data, -8L), 8);
            glyph_data = huge_add(glyph_data, hdr.dx * 3L);
        } else
            glyph_data = huge_add(glyph_data, 0x300L);
        if (!(hdr.rows & 2))
            return;
        for (index = 0; index < hdr.width; index++) {
            huge_move(glyph_data, &cyc, 4);
            cyc.first = tab[cyc.first];
            huge_move(&cyc, glyph_data, 4);
            glyph_data = huge_add(glyph_data, 4L);
        }
        return;
    }
    index = hdr.width * hdr.rows;
    if (!pixel_shift())
        index <<= 3;
    if (is_xvga())
        index <<= 2;
    if ((hdr.type & 0xf) == 1)
        glyph_data = huge_add(glyph_data, (long)index);
    else if ((hdr.type & 0xf) && (hdr.type & 0xf) != 5)
        error_box("Invalid type (%x) to ClrTran", hdr.type);
    if (video_planes() == 4 && !is_xvga())
        plane_recolor(glyph_data, index, index, tab);
    else
        row_recolor(glyph_data, index, tab);
}

void far glyph_palette_load(char far *lib, int glyph_index)                      /* 2796:0536 */
{
    struct Glyph hdr;
    struct CycSpec cyc[16];
    char far *glyph_data;
    unsigned char rgb[0x300];
    register int first;
    register int count;

    if ((glyph_data = glyph_find(lib, glyph_index, &hdr)) == 0)
        return;
    if ((hdr.type & 0xf) != 8) {
        error_box("Invalid TNPalette call %lx %d", lib, glyph_index);
        return;
    }
    if (hdr.rows & 1) {
        first = hdr.dy;
        count = hdr.dx;
    } else {
        first = 0;
        count = is_xvga() ? 0x100 : pixel_shift() ? 0x10 : 0x100;
    }
    if (!(hdr.rows & 2))
        hdr.width = 0;
    huge_move(glyph_data, rgb, count * 3);
    glyph_data = huge_add(glyph_data, (long)(count * 3));
    huge_move(glyph_data, cyc, hdr.width * 4); /* width is not bounded by 16, as shipped */
    pal_set(first, count, rgb, hdr.width, cyc);
}
