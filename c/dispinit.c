/* DQK.EXE overlay 04 module 2726: display initialization ("DInit" in its
 * error text).  display_init picks the display mode -- from monitor.dat,
 * from a mode name on the command line, or by probing the adapter and the
 * machine -- and video_set programs it: the BIOS mode, the screen and
 * page geometry the drawing code reads, the page segments (allocated where
 * the adapter has no second page), the palette, and a Hercules card's
 * CRTC.  The small routines ask the machine what it is: the ROM model byte
 * (PCjr, PS/2 Model 30), a Tandy's ROM signature, the memory size and the
 * DOS version.
 *
 * EXE 0x039134..0x039fd4, one Turbo C large-model _TEXT segment starting at
 * byte 4 of paragraph 2726 in RTLink section 4 (payload 0x37910, link
 * segment 25a4), the section's last module; the first 4 bytes of the
 * paragraph are copyprot.c's tail, and 12 zero bytes follow to the end of
 * the section's payload.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Sections 5 to 8 hold other bytes at the 25a4 link segment.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  A name another module
 * gives an address is reused from there; the rest are placeholders chosen
 * from use.  Four routines are reached through vector stubs: is_pcjr
 * (1d99:155d), is_tandy (1d99:1553, from speaker.c and fcache.c), clock_now
 * (1d99:153f) and display_init (1d99:1549), the last two under the names
 * sysutil.c calls the stubs by.  config.c far-calls display_options_set.
 *
 * The census's markers are Turbo C's own: detect_adapter's switch is the
 * sparse-switch value search (cmp ax,cs:[bx] over a sorted table), and the
 * in and out instructions are <dos.h>'s inportb and outportb, which Turbo C
 * compiles inline.  fill_far_words fills words at a far address and is
 * declared without a prototype: every call passes the address as an offset
 * word and a segment word, and case 14 of video_set passes the count 64000,
 * a long, as two words where every other call passes one.
 *
 * _DATA is DGROUP 9268..938c: the default palette, the two EGA palettes,
 * display_opts, the Hercules CRTC values and the literals; -d stores "e",
 * "high", "h", "mono" and "t" on the tails of earlier literals.  ega_pals
 * is public: module 22b7 indexes it by the byte at cb5f.  No _BSS: every
 * variable video_set and display_init write is also referenced by resident
 * code, and is declared extern here.
 */

#include <dos.h>
#define DQK_DEFINES_DISPINIT
#include "dqk.h"

void far herc_init(void);

static int pal_rgb[16] = {                                       /* 9268 */
    0x000, 0x00a, 0x0a0, 0x0aa, 0xa00, 0xa0a, 0xa80, 0xaaa,
    0x555, 0x55f, 0x5f5, 0x5ff, 0xf55, 0xf5f, 0xff5, 0xfff
};
char ega_pals[2][17] = {                                         /* 9288 */
    { 0, 8, 8, 8, 0, 8, 8, 8, 0, 8, 8, 8, 0, 8, 8, 8, 0 },
    { 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0 }
};
static int display_opts = 0;                                     /* 92aa */
static char herc_crtc[16] = {                                    /* 92ac */
    0x35, 0x2d, 0x2e, 0x07, 0x5c, 0x02, 0x58, 0x58, 0x02, 0x03
};

char far is_pcjr(void)                                           /* 2726:0004 */
{
    return peek_byte(0xf000, 0xfffe) == 0xfd;
}

char far is_tandy(void)                                          /* 2726:0020 */
{
    return peek_byte(0xfc00, 0) == 0x21;
}

char far mem_small(void)                                         /* 2726:003b */
{
    return (unsigned)peek_word(_psp, 2) <= 0x6000;
}

char far is_ps2_30(void)                                         /* 2726:0057 */
{
    return peek_byte(0xf000, 0xfffe) == 0xfa;
}

int far dos_version(void)                                        /* 2726:0073 */
{
    unsigned version_word;

    version_word = dos_regs(0x30);
    return (version_word & 0xff) << 8 | (version_word & 0xff00) >> 8;
}

long far clock_now(void)                                         /* 2726:0097 */
{
    return clock_bios(0);
}

void far video_set(int mode)                                     /* 2726:00a1 */
{
    int far *pal;
    int index;

    if ((video_mode = mode) == 11 || video_mode == 10 || video_mode == 16)
        poke_byte(0x40, 0x10, saved_equip | 0x30);
    else
        poke_byte(0x40, 0x10, saved_equip & ~0x30 | 0x10);
    pix_shifts = 3;
    letter_step = txthigh_dots = 4;
    switch (video_mode) {
    case 1: case 2: case 3: case 4:
        screen_lines = 200;
        pixel_cols = 320;
        pix_shifts = 2;
        video_len = 0x3f40;
        display_seg = 0xb800;
        if ((mem_top_seg = FP_SEG(far_alloc(0x4000L))) == 0) {
            con_printf("Not enough memory in DInit\n");
            dos_call(0x4c01);
        }
        fill_far_words(0, mem_top_seg, video_len, 0);
        extent_text = 0x4000;
        text_bytes = 0x2000;
        base_offset = 0xc050;
        planes_count = 1;
        if (video_mode == 4) {
            video_bios(0, 4);
            video_bios(0x0b, 0, 0, 0);
            video_bios(0x0b, 0, 1, 1);
            if (video_mode == 4)
                outportb(0x3d8, 0x0e);
        } else if (video_mode == 3)
            video_bios(0, 8);
        else {
            video_bios(0, 6);
            if (video_mode == 1)
                outportb(0x3d8, 0x1a);
        }
        break;
    case 17:
        screen_lines = 200;
        pixel_cols = 640;
        letter_step = 8;
        pix_shifts = 3;
        video_len = 0x3f40;
        display_seg = 0xb800;
        if ((mem_top_seg = FP_SEG(far_alloc(0x4000L))) == 0) {
            con_printf("Not enough memory in DInit\n");
            dos_call(0x4c01);
        }
        fill_far_words(0, mem_top_seg, video_len, 0);
        extent_text = 0x4000;
        text_bytes = 0x2000;
        base_offset = 0xc050;
        planes_count = 1;
        video_bios(0, 6);
        break;
    case 5: case 6: case 7:
        screen_lines = 200;
        pixel_cols = 320;
        video_len = 0x1f40;
        display_seg = 0xa000;
        mem_top_seg = 0xa200;
        extent_text = 0x1f40;
        text_bytes = 0x28;
        base_offset = 0xe098;
        planes_count = 4;
        pal = pal_rgb;
        video_bios(0, 0x0d);
        for (index = 0; index < 16; index++)
            pal_entry(index, pal[index]);
        break;
    case 8: case 9: case 10:
        screen_lines = 300;
        txthigh_dots = 6;
        pixel_cols = 640;
        letter_step = 8;
        video_len = 0x6d60;
        display_seg = mem_top_seg = 0xa000;
        if (video_mode != 10)
            mem_top_seg = 0xa800;
        extent_text = 0x6d60;
        text_bytes = 0x50;
        base_offset = 0x9250;
        planes_count = video_mode == 9 ? 4 : 1;
        video_bios(0, video_mode == 10 ? 0x0f : 0x10);
        if (video_mode != 10) {
            for (index = 0; index < 16; index++)
                pal_entry(index, pal_rgb[index]);
        } else {
            video_bios(0x10, 3, 0, 0);
            video_bios(0x10, 2, 0, 0, 0, (int)ega_pals[0] >> 8, ega_pals[0]);
        }
        break;
    case 11:
        screen_lines = 300;
        txthigh_dots = 6;
        pixel_cols = 640;
        letter_step = 8;
        video_len = 0x7e96;
        display_seg = 0xb000;
        mem_top_seg = 0xb800;
        extent_text = 0x7e96;
        text_bytes = 0x2000;
        base_offset = 0x805a;
        planes_count = 1;
        herc_init();
        break;
    case 12: case 13:
        screen_lines = 200;
        pixel_cols = 320;
        pix_shifts = 1;
        video_len = 0x7f40;
        video_bios(0, 9);
        if (video_mode == 13) {
            display_seg = (peek_byte(0x40, 0x8a) & 6) << 10;
            if ((mem_top_seg = FP_SEG(far_alloc(0x8000L))) == 0) {
                video_restore();
                con_printf("Not enough memory in DInit\n");
                dos_call(0x4c01);
            }
        } else {
            video_bios(5, 0x81, 6, 4);
            poke_word(0xb800, 0, 0x1234);
            for (mem_top_seg = 0; mem_top_seg != 0xc000; mem_top_seg += 0x800)
                if (peek_word(mem_top_seg, 0) == 0x1234) {
                    poke_word(0xb800, 0, 0x4321);
                    if (peek_word(mem_top_seg, 0) == 0x4321)
                        break;
                    poke_word(0xb800, 0, 0x1234);
                }
            if (mem_top_seg == 0xc000) {
                video_restore();
                con_printf("Unable to initialize TANDY\n");
                dos_call(0x4c01);
            }
            if (mem_top_seg == 0xb000) {
                display_seg = mem_top_seg - 0x800;
                fill_far_words(0, display_seg, 0x8000, 0);
                video_bios(5, 0x83, 2, 6);
            } else {
                display_seg = mem_top_seg + 0x800;
                video_bios(5, 0x81, 6, 6);
            }
        }
        fill_far_words(0, mem_top_seg, 0x8000, 0);
        extent_text = 0x7f40;
        text_bytes = 0x2000;
        base_offset = 0x80a0;
        planes_count = 1;
        for (index = 0; index < 16; index++)
            pal_entry(index, pal_rgb[index]);
        break;
    case 14:
        screen_lines = 200;
        pixel_cols = 320;
        pix_shifts = 0;
        video_len = 0xfa00;
        display_seg = 0xa000;
        if ((mem_top_seg = FP_SEG(far_alloc(64000L))) == 0) {
            con_printf("Not enough memory in DInit\n");
            dos_call(0x4c01);
        }
        fill_far_words(0, mem_top_seg, 64000, 0);
        extent_text = 0xfa00;
        text_bytes = base_offset = 0x140;
        planes_count = 1;
        video_bios(0, 0x13);
        for (index = 0; index < 16; index++)
            pal_entry(index, pal_rgb[index]);
        break;
    case 15:
        screen_lines = 200;
        pixel_cols = 320;
        pix_shifts = 2;
        video_len = 0x3e80;
        display_seg = 0xa000;
        mem_top_seg = 0xa400;
        extent_text = 0x3e80;
        text_bytes = base_offset = 0x50;
        planes_count = 4;
        video_bios(0, 0x13);
        reg_write(0x3c4, 4, 6);
        fill_far_words(0, display_seg, video_len, 0);
        fill_far_words(0, mem_top_seg, video_len, 0);
        reg_write(0x3d4, 0x17, 0xc3);
        reg_write(0x3d4, 0x14, 0);
        for (index = 0; index < 16; index++)
            pal_entry(index, pal_rgb[index]);
        break;
    case 16:
        screen_lines = 400;
        txthigh_dots = 8;
        pixel_cols = 640;
        letter_step = 8;
        video_len = 0x7d00;
        display_seg = 0xa0c8;
        if ((mem_top_seg = FP_SEG(far_alloc(0x7d00L))) == 0) {
            con_printf("Not enough memory in DInit\n");
            dos_call(0x4c01);
        }
        fill_far_words(0, mem_top_seg, 0x7d00, 0);
        extent_text = 0x7d00;
        text_bytes = 0x50;
        base_offset = 0x50;
        planes_count = 1;
        video_bios(0, 0x11);
        break;
    }
    draw_seg = display_seg;
    screen_addr = 0;
    draw_char(-1, 0, 0, 0, 0);
    video_plane_set(0);
    visible_on = back_screen = 1;
    clip_reset();
    set_text_mode(1);
}

int far detect_adapter(void)                                     /* 2726:085e */
{
    if (display_code() == 8)
        return 7;
    if ((ega_query() & 0xff) == 1) {
        switch (ega_query() & 0xf00) {
        case 0: case 0x100: case 0x600: case 0x700:
            return 5;
        default:
            return 6;
        }
    }
    if ((ega_query() & 0xff) == 2)
        return 10;
    return 0;
}

void far display_options_set(int opts)                                  /* 2726:08c6 */
{
    display_opts = opts;
}

void far display_init(int argc, char far * far *argv)            /* 2726:08d1 */
{
    char far *arg;
    int file_handle;

    crit_install();
    dos_mode = video_bios(0x0f);
    saved_equip = peek_byte(0x40, 0x10);
    shape_boot = peek_word(0x40, 0x60);
    if (is_tandy())
        outportb(0x62, inportb(0x62) | 8);
    video_mode = 0;
    if ((file_handle = dos_open(0x3d00, "monitor.dat")) >= 0) {
        dos_io(0x3f00, &video_mode, file_handle, 2);
        dos_call(0x3e00, 0, file_handle);
    }
    if (video_mode < 0 || video_mode > 17)
        video_mode = 0;
    if (argc >= 2) {
        arg = argv[argc - 1];
        if (str_same(arg, "cga") || str_same(arg, "c"))
            video_mode = 4;
        else if (str_same(arg, "b&w"))
            video_mode = 2;
        else if (str_same(arg, "rgb"))
            video_mode = 4;
        else if (str_same(arg, "composite") || str_same(arg, "tv"))
            video_mode = 1;
        else if (str_same(arg, "cgahigh"))
            video_mode = 17;
        else if (str_same(arg, "tandylow"))
            video_mode = 4;
        else if (str_same(arg, "t160"))
            video_mode = 3;
        else if (str_same(arg, "ega") || str_same(arg, "e")) {
            if ((video_mode = detect_adapter()) == 0)
                video_mode = 5;
        } else if (str_same(arg, "ega16"))
            video_mode = 5;
        else if (str_same(arg, "ega64"))
            video_mode = 6;
        else if (str_same(arg, "vga"))
            video_mode = 7;
        else if (str_same(arg, "xvga"))
            video_mode = 15;
        else if (str_same(arg, "high"))
            video_mode = 8;
        else if (str_same(arg, "high4"))
            video_mode = 9;
        else if (str_same(arg, "egamono") || str_same(arg, "mono"))
            video_mode = 10;
        else if (str_same(arg, "herc") || str_same(arg, "h")
                 || str_same(arg, "hercules"))
            video_mode = 11;
        else if (str_same(arg, "tandy") || str_same(arg, "t"))
            video_mode = 12;
        else if (str_same(arg, "pcjr"))
            video_mode = 13;
        else if (str_same(arg, "m25") || str_same(arg, "m30")
                 || str_same(arg, "mcga") || str_same(arg, "m"))
            video_mode = 14;
        else if (str_same(arg, "m30mono"))
            video_mode = 16;
        else if (str_same(arg, "auto"))
            video_mode = 0;
        else
            arg = 0;
    }
    if (video_mode == 0) {
        if ((saved_equip & 0x30) == 0x30) {
            if (is_ps2_30())
                video_mode = 16;
            else if ((ega_query() & 0xff) == 2)
                video_mode = 10;
            else
                video_mode = 11;
        } else {
            if ((video_mode = detect_adapter()) == 0 || video_mode == 10) {
                video_mode = 0;
                if (is_pcjr())
                    video_mode = 4;
                else if (is_tandy()) {
                    if (memory_above_320k() && (!mem_small() || dos_version() < 0x300))
                        video_mode = 12;
                    else
                        video_mode = 4;
                } else if (is_ps2_30())
                    video_mode = 14;
                else
                    video_mode = 4;
            }
            if ((display_opts & 5) == 5 && video_mode == 7)
                video_mode = 15;
            if (display_opts & 1 && video_mode == 7)
                video_mode = 14;
            if (display_opts & 2) {
                if (video_mode == 4 || video_mode == 12)
                    video_mode = 17;
                if (video_mode == 5 || video_mode == 6 || video_mode == 7)
                    video_mode = 8;
                if (video_mode == 14)
                    video_mode = 16;
            }
        }
    }
    video_set(video_mode);
}

void far herc_init(void)                                         /* 2726:0e37 */
{
    int index;

    video_bios(0, 7);
    outportb(0x3bf, 3);
    outportb(0x3b8, 2);
    for (index = 0; index < 16; index++)
        reg_write(0x3b4, index, herc_crtc[index]);
    fill_far_words(0, 0xb000, 0x7fff, 0);
    fill_far_words(0x8000, 0xb000, 0x7fff, 0);
    outportb(0x3b8, 0x0a);
}
