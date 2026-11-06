/* DQK.EXE resident module 204c: pictures and icons.  Loading a picture
 * library into pic_res and freeing it; loading, clearing and drawing an
 * icon through the resource icon_res.  icon_load fills entries idx and
 * idx + 0x26; icon_clear clears idx, then for idx < 0x26 idx + 0x26, and
 * idx + 0x4c and idx + 0x72 when icon_head[idx] is set; icon_draw draws
 * entry bank * 0x26 + idx, and + 0x4c when icon_head[idx] is set.  A name
 * starting "CH" or "CB" is cut with substr(name, 1, len - 1), and
 * icon_head records which it was.
 * The one reference to tile_start_slot found in the image is pic_load's store.
 *
 * EXE 0x0128c6..0x012d35, one Turbo C large-model _TEXT segment starting at
 * byte 6 of paragraph 204c.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * The lone retf at 0x0128c6 (charpic_unit_init) is claimed here: module 2023's
 * last routine ends at 0x0128c5, and no call, stored pointer or relocation
 * found in the image reaches the byte.  The lone retf at 0x012d35 is
 * module 2093's: it is far-called as 1093:0005.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use.
 *
 * glyph_at's fourth argument is pushed as a byte here (mov al,1), and
 * extra_row's result is tested as a word (or ax,ax), so both are declared
 * to fit these calls.  extra_row sets only al; this is the one of its 12
 * shipped callers that tests the word.
 */

#include "dqk.h"

extern int  far extra_row(void);                                 /* 0bdb:21dd */

static int tile_start_slot = 0x7f;                                      /* 3ab4 */

static unsigned char icon_head[0x26];                            /* a848 */

/* The translated Pascal unit's empty initialization block (DKK maptiles.pas). */
void far charpic_unit_init(void)                                      /* 204c:0006 */
{
}

void far pic_load(char far *name, unsigned char set, unsigned char start_slot)
{                                                                /* 204c:0007 */
    set_open_mode('2');
    if (start_slot)
        tile_start_slot = start_slot;
    tile_open_file(name, set, 0, 0, &pic_res);
    res_palette_load(pic_res);
}

void far icon_clear(unsigned char idx)                           /* 204c:004d */
{
    res_copy(0, 0, 0, icon_res, idx);
    if (idx < 0x26) {
        res_copy(0, 0, 0, icon_res, idx + 0x26);
        if (icon_head[idx]) {
            res_copy(0, 0, 0, icon_res, idx + 0x4c);
            res_copy(0, 0, 0, icon_res, idx + 0x72);
        }
    }
}

void far icon_load(char far *name, unsigned char set, unsigned char idx)
{                                                                /* 204c:00df */
    struct Res far *resource;
    char buf[80];

    if (name[0] == 'C' && (name[1] == 'H' || name[1] == 'B')) {
        icon_head[idx % 0x26] = name[1] == 'H';
        name = substr(name, 1, str_len(name) - 1);
    } else
        icon_head[idx] = 0;
    resource = 0;
    set_open_mode(str_same(name, "CPIC") || str_same(name, "COMSPR") ? '2' : '3');
    str_copy(buf, name);
    str_cat(buf, "1");
    if (!str_same(name, "CBODY") || !extra_row()) {
        tile_open_file(buf, set, 3, name[0] == 'C' && name[1] == 'H' ? 0 : 15, &resource);
        res_copy(resource, 0, 0, icon_res, idx);
        res_copy(resource, name[0] == 'C' && name[1] == 'H' ? 0 : 1, 0, icon_res, idx + 0x26);
    } else {
        tile_open_file(name, 0xff, 1, 0, &resource);
        res_copy(resource, set, 1, icon_res, idx);
        if (name[0] == 'C' && name[1] == 'H')
            res_copy(resource, set, 1, icon_res, idx + 0x26);
        else
            res_copy(resource, set + 0x80, 1, icon_res, idx + 0x26);
    }
    res_free(&resource);
}

void far icon_draw(register int col, register int row, unsigned char kind,
                   unsigned char bank, unsigned char idx)        /* 204c:030f */
{
    col = (col + col + col) * 4 + 0x1f44; /* add,add,shl,shl; col * 12 is a mul */
    if (row < -900)
        row += 1000;
    row = (row + row + row) * 4 + 0x1f44;
    if (kind > 3) {
        res_copy(icon_res, bank * 0x26 + idx, 0, icon_res, 0x98);
        res_glyph_mirror(icon_res, 0x98);
        glyph_at(col, row, 0x98, 1, icon_res);
        if (icon_head[idx]) {
            res_copy(icon_res, bank * 0x26 + idx + 0x4c, 0, icon_res, 0x98);
            res_glyph_mirror(icon_res, 0x98);
            glyph_at(col, row, 0x98, 1, icon_res);
        }
    } else {
        glyph_at(col, row, bank * 0x26 + idx, 1, icon_res);
        if (icon_head[idx])
            glyph_at(col, row, bank * 0x26 + idx + 0x4c, 1, icon_res);
    }
}

void far pic_free(void)                                          /* 204c:0468 */
{
    res_free(&pic_res);
}
