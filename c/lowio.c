/* DQK.EXE resident module 251e: the low-level services under the screen
 * and input code.  It allocates the one far work block that library.c's
 * decoders and overlay code borrow, reprograms timer 0 and reads the tick count the
 * runtime's timer handler keeps, reads the keyboard through the BIOS and
 * folds its scan codes into the game's key numbers, loads EGA and VGA
 * palettes, converts text cells to pixel coordinates, keeps the clip
 * rectangle, splits a chained 4-plane buffer into planes, and checks the
 * top of memory.
 *
 * EXE 0x0175e7..0x017e76, one Turbo C large-model _TEXT segment starting at
 * byte 7 of paragraph 251e.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Module 22b7's last routine ends with the retf at 251e:0006, and
 * scrfont.c's module 25a7 starts at 0x017e76 with no stub between.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the
 * rest are placeholders chosen from use.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined: timer_unhook far-calls timer_ticks, which carries a prototype.
 * The port writes are <dos.h>'s outportb, which Turbo C compiles inline.
 *
 * _BSS follows the statics' names (Turbo C's symbol-table order), not their
 * declaration order; these names reproduce the shipped cda6..cdbe layout.
 * ticks and fire_latch are public: the runtime's timer handlers, 10d7:0009
 * and the 10d7:019e timer_rehook installs, count ticks and OR the joystick
 * buttons newly pressed (port 0x201) into fire_latch.  The clip rectangle is
 * public: glyph.c and module 22b7 read it.
 *
 * spare_buffer is extern: it lies at cd98, between module 22b7's cd96 and cd9c,
 * though only this module names it.  txthigh_dots and letter_step are 22b7's.
 */

#include <dos.h>
#define DQK_DEFINES_LOWIO
#include "dqk.h"

long far timer_ticks(void);

long ticks;                                                      /* cda6 */
static long bios_time;                                           /* cdaa */
int clip_top;                                                    /* cdae */
static int clips_top;                                            /* cdb0 */
char fire_latch;                                                 /* cdb2 */
int clip_left;                                                   /* cdb3 */
int clip_right;                                                  /* cdb5 */
static int clips_left;                                           /* cdb7 */
int clip_bottom;                                                 /* cdb9 */
static int clips_right;                                          /* cdbb */
static int clips_bottom;                                         /* cdbd */

/* ALT-letter scan codes 0x10..0x19, 0x1e..0x26 and 0x2c..0x32 in keyboard
 * order; key_get returns the letter + 0xff. */
static char alt_keys[26] = "QWERTYUIOPASDFGHJKLZXCVBNM";         /* 9a7a */

void far *far db_buffer(void)                                 /* 251e:0007 */
{
    return spare_buffer;
}

void far db_init(long size)                                      /* 251e:000f */
{
    if (!spare_buffer)
        spare_buffer = mem_alloc(size);
    if (!spare_buffer) {
        con_printf("Out of memory in DBInit (%ld)\n", size);
        con_getch();
    }
}

/* Timer 0 at mode 3, divisor 0x4000: four times the BIOS rate. */
void far timer_rehook(void)                                      /* 251e:004d */
{
    bios_time = clock_bios(0);
    outportb(0x43, 0x36);                       /* the census's weak `out` markers */
    outportb(0x40, 0);
    outportb(0x40, 0x40);
    ticks = 0;
    timer_install(timer_isr);
}

void far timer_unhook(void)                                      /* 251e:0085 */
{
    outportb(0x43, 0x36);                       /* the census's weak `out` markers */
    outportb(0x40, 0);
    outportb(0x40, 0);
    timer_install(0);
    clock_bios(1, timer_ticks() / 4 + bios_time);
}

long far timer_ticks(void)                                       /* 251e:00c6 */
{
    return ticks;
}

long far sound_ticks(void)                                      /* 251e:00ce */
{
    return ticks;
}

int far mod151e_00d6(void)                                       /* 251e:00d6 */
{
    return 0x1110;
}

/* Empty; sys_init calls it in lowio's place among the subsystem inits. */
void far lowio_init(void)                                      /* 251e:00da */
{
}

/* Empty; sys_shutdown calls it in the mirror place. */
void far lowio_done(void)                                      /* 251e:00db */
{
}

char far key_poll(void)                                          /* 251e:00dc */
{
    return bios_key(1);
}

/* `2e 3b 07` at 251e:0103 is Turbo C's sparse-switch value search: 64 case
 * values at cs:0114 and their targets at cs:0194, scanned with loop. */
int far key_get(void)                                            /* 251e:00e7 */
{
    register unsigned bios_key_code;

    while (!key_poll())
        ;
    bios_key_code = bios_key(0);
    switch (bios_key_code) {
    case 0x4800: case 0x4838: case 0x487e:
        return 0x108;
    case 0x5000: case 0x5032: case 0x5060:
        return 0x104;
    case 0x4b00: case 0x4b34: case 0x4b7c:
        return 0x106;
    case 0xf400: case 0x4d00: case 0x4d36:
        return 0x102;
    case 0x4700: case 0x4737: case 0x475c:
        return 0x107;
    case 0x4900: case 0x4939:
        return 0x101;
    case 0x4f00: case 0x4f31:
        return 0x105;
    case 0x5100: case 0x5133:
        return 0x103;
    case 0xf300: case 0x4c35:
        return 0x100;
    case 0x5200: case 0x5230:
        return 0x109;
    case 0x3b00: case 0x3c00: case 0x3d00: case 0x3e00: case 0x3f00:
    case 0x4000: case 0x4100: case 0x4200: case 0x4300: case 0x4400:
        return ((bios_key_code >> 8) & 0xff) + 0xd5;
    case 0x5300: case 0x532d: case 0x532e:
        return 0x10a;
    case 0x1000: case 0x1100: case 0x1200: case 0x1300: case 0x1400:
    case 0x1500: case 0x1600: case 0x1700: case 0x1800: case 0x1900:
        return alt_keys[(bios_key_code - 0x1000) >> 8] + 0xff;
    case 0x1e00: case 0x1f00: case 0x2000: case 0x2100: case 0x2200:
    case 0x2300: case 0x2400: case 0x2500: case 0x2600:
        return alt_keys[(bios_key_code - 0x1400) >> 8] + 0xff;
    case 0x2c00: case 0x2d00: case 0x2e00: case 0x2f00: case 0x3000:
    case 0x3100: case 0x3200:
        return alt_keys[(bios_key_code - 0x1900) >> 8] + 0xff;
    }
    return bios_key_code & 0xff;
}

char far shift_held(void)                                        /* 251e:029f */
{
    return (bios_key(2) & 3) ? 1 : 0;
}

void far console_beep(void)                                      /* 251e:02b5 */
{
    con_direct(7);
}

/* One palette entry from a 12-bit 0xRGB: EGA palette register idx (int 10h
 * ax=1000h) in modes 5..9, 12 and 13, DAC register idx (ax=1010h) in modes
 * 14 and 15, and in mode 7 also the DAC register the EGA value names. */
void far pal_entry(int idx, unsigned rgb)                        /* 251e:02c0 */
{
    char ega;

    if (idx < 0)
        return;
    switch (video_mode) {
    case 5: case 6: case 7: case 8: case 9: case 12: case 13:
        if (idx > 15)
            break;
        ega = ((rgb & 0x800) >> 9) + ((rgb & 0x400) >> 5)
            + ((rgb & 0x80) >> 6) + ((rgb & 0x40) >> 2)
            + ((rgb & 8) >> 3) + ((rgb & 4) << 1);
        if (video_mode == 12 || video_mode == 13)
            ega |= (ega & 0x10) >> 1;
        video_bios(0x10, 0, ega, idx);
        if (video_mode != 7)
            break;
        idx = ega;
    case 14: case 15:
        if (idx > 0xff)
            break;
        video_bios(0x10, 0x10, 0, idx, ((rgb & 0xf0) << 6) + ((rgb & 0xf) << 2),
                     (rgb & 0xf00) >> 6);
    }
}

/* Sixteen entries from 8-bit RGB triples: in modes 7, 14 and 15 EGA register
 * i names DAC register i, loaded with each component's top 6 bits; otherwise
 * each component is rounded to 0..15 for pal_entry. */
void far pal_set16(unsigned char far *pal)                       /* 251e:03ca */
{
    short red, green, blue;
    register int index, rgb;

    for (index = 0; index < 16; index++) {
        red = *pal;
        rgb = (red + 8) / 17 << 8;
        pal++;
        green = *pal;
        rgb += (green + 8) / 17 << 4;
        pal++;
        blue = *pal;
        rgb += (blue + 8) / 17;
        pal++;
        if (video_mode == 7 || video_mode == 14 || video_mode == 15) {
            video_bios(0x10, 0, index, index);
            video_bios(0x10, 0x10, 0, index, ((green & 0xfc) << 6) + ((blue & 0xfc) >> 2),
                         (red & 0xfc) >> 2);
        } else
            pal_entry(index, rgb);
    }
}

void far palette_load(unsigned char far *pal)                    /* 251e:049d */
{
    unsigned char dac[0x300];
    unsigned char far *dac_cursor;
    register int index;

    if (video_mode == 14 || video_mode == 15) {
        dac_cursor = dac;
        for (index = 0; index < 0x100; index++) {
            *dac_cursor++ = *pal++ >> 2;
            *dac_cursor++ = *pal++ >> 2;
            *dac_cursor++ = *pal++ >> 2;
        }
        dac_load(0, 0x56, 1, dac);
        dac_load(0x56, 0x56, 1, dac + 0x102);
        dac_load(0xac, 0x54, 1, dac + 0x204);
    } else
        pal_set16(pal);
}

void far palette_page_load(unsigned char far *pal, char flag)         /* 251e:056e */
{
    if (flag)
        screen_page_show();
    palette_load(pal);
    if (!flag)
        screen_page_show();
}

/* A coordinate above 6000 is 8000 plus a count of txthigh_dots / 2 (a row) or
 * letter_step / 2 (a column) units; a smaller one passes through. */
void far text_xy(int row, int col, int far *out_row, int far *out_col) /* 251e:0595 */
{
    if (row > 6000)
        *out_row = (txthigh_dots >> 1) * (row - 8000);
    else
        *out_row = row;
    if (col > 6000)
        *out_col = (letter_step >> 1) * (col - 8000);
    else
        *out_col = col;
}

void far text_xy_to(int row, int col, int x, int y,
                    int far *out_row, int far *out_col)          /* 251e:05e6 */
{
    short half_cell_height, half_cell_width;

    half_cell_height = txthigh_dots >> 1;
    half_cell_width = letter_step >> 1;
    text_xy(row, col, &row, &col);
    x -= row;
    if (x < 0)
        x -= half_cell_height - 1;
    *out_row = x / half_cell_height;
    y -= col;
    if (y < 0)
        y -= half_cell_width - 1;
    *out_col = y / half_cell_width;
}

void far text_offset_xy(int row, int col, int row2, int col2,
                      int far *out_row, int far *out_col)        /* 251e:0653 */
{
    text_xy(row, col, &row, &col);
    if (row2 > 6000)
        *out_row = (txthigh_dots >> 1) * (row2 - 8000) + row;
    else
        *out_row = row + row2;
    if (col2 > 6000)
        *out_col = (letter_step >> 1) * (col2 - 8000) + col;
    else
        *out_col = col + col2;
}

void far clip_set(int row1, int col1, int row2, int col2)    /* 251e:06cb */
{
    text_xy(row1, col1, &clip_top, &clip_left);
    text_xy(row2, col2, &clip_bottom, &clip_right);
    if (clip_top < 0)
        clip_top = 0;
    if (clip_left < 0)
        clip_left = 0;
    if (screen_rows() < clip_bottom)
        clip_bottom = screen_rows();
    if (screen_cols() < clip_right)
        clip_right = screen_cols();
    if (clip_top >= clip_bottom || clip_left >= clip_right)
        clip_top = clip_left = clip_bottom = clip_right = 0;
}

void far clip_reset(void)                                      /* 251e:075c */
{
    clip_top = clip_left = 0;
    clip_bottom = screen_rows();
    clip_right = screen_cols();
}

void far clip_narrow(int row1, int col1, int row2, int col2)     /* 251e:0775 */
{
    text_xy(row1, col1, &row1, &col1);
    text_xy(row2, col2, &row2, &col2);
    clip_top = int_max(clip_top, row1);
    clip_left = int_max(clip_left, col1);
    clip_bottom = int_min(clip_bottom, row2);
    clip_right = int_min(clip_right, col2);
    if (clip_top >= clip_bottom || clip_left >= clip_right)
        clip_top = clip_left = clip_bottom = clip_right = 0;
}

void far clip_save(void)                                         /* 251e:080a */
{
    clips_top = clip_top;
    clips_left = clip_left;
    clips_bottom = clip_bottom;
    clips_right = clip_right;
}

void far clip_restore(void)                                      /* 251e:0823 */
{
    clip_top = clips_top;
    clip_left = clips_left;
    clip_bottom = clips_bottom;
    clip_right = clips_right;
}

/* Every fourth byte from src + plane, n of them per plane. */
void far plane_split(char far *src, int byte_count, char far *dst)        /* 251e:083c */
{
    char far *plane_cursor;
    register int plane, scan_index;

    for (plane = 0; plane < 4; plane++) {
        plane_cursor = src + plane;
        scan_index = byte_count;
        while (--scan_index >= 0) {
            *dst++ = *plane_cursor;
            plane_cursor += 4;
        }
    }
}

int far memory_above_320k(void)                                       /* 251e:087a */
{
    return (unsigned)peek_word(_psp, 2) >= 0x5000 ? 1 : 0;
}
