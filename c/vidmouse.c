/* DQK.EXE resident module 22b7: the display layer under the game's screens --
 * the video mode's pages and planes, filling a rectangle, drawing a font glyph,
 * the pixel position of a text cell, and the mouse pointer: its driver set-up,
 * button events, the pointer shape and saving and redrawing the pixels under it.
 *
 * EXE 0x014f7d..0x0175e7, one Turbo C large-model _TEXT segment starting at
 * byte 0x0d of paragraph 22b7: module 21e9's last routine ends with the retf at
 * 22b7:000c.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined
 * in shipped order; bake consumes them in that order.  All 188 MZ relocation
 * sites inside the range have frame 12b7.  Module 251e starts at 0x0175e7
 * (251e:0007): the ten far calls to that routine store 151e:0007, none 12b7.
 *
 * _DATA is DGROUP 938e..9a79: the edge masks, the CGA colour map, the built-in
 * 96-glyph font (12 bytes a glyph), the default pointer shape and three small
 * mask tables.  Overlay 4 module 2726's literals end at 938c; module 251e's
 * _DATA starts at 9a7a.  ega_pals (9288) lies in 2726's _DATA and is extern.
 * _BSS is ca28..cda4: diskfile.c's statics end at ca26 (ca27 is the pad), and
 * module 251e's start at cda6 (cda5 is the pad).  text_bytes (cb6c),
 * base_offset (cb80), extent_text (cb83) and spare_buffer (cd98) lie inside the
 * run though nothing in this module reads them: overlay 4 module 2726 writes
 * the first three and the runtime (10d7) reads them, and module 251e uses
 * spare_buffer.  Turbo C lays _BSS out in its symbol-table order, so the
 * names of the 59 variables there were chosen with goldbox.bss_order until
 * the layout matched; they are placeholders the layout constrains, not
 * recovered names.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the rest
 * are placeholders chosen from use.
 *
 * Nothing found calls page_copy, page_swap, screen_page_present, draw_redirect or
 * mouse_up_which: no far call or far jump, near call, callback push, RTLink
 * vector or pointer word names them.
 */

#include <dos.h>
#define DQK_DEFINES_VIDMOUSE
#include "dqk.h"

struct Pointer {                /* a mouse pointer: hot spot, size, and its masks per plane */
    char row;                   /* hot spot */
    char col;
    char rows;
    char cols;
    unsigned char mask[4][0x40];
    unsigned char image[4][0x40];
};

extern void far huge_move();                                     /* 00d7:08c2, unprototyped */

static char pat[2];                                             /* ca28 */
static int btn_col[2];                                          /* ca2a */
static int btn_at[2];                                           /* ca2e */
char dos_mode;                                                  /* ca32 */
unsigned draw_seg;                                              /* ca33 */
static unsigned prev_seg;                                       /* ca35 */
static int left_up[2];                                          /* ca37 */
static int fore_mask;                                           /* ca3b */
static int right_up[2];                                         /* ca3d */
int video_mode;                                                 /* ca41 */
unsigned video_len;                                             /* ca43 */
static char ptr_under[4][0x40];                                 /* ca45 */
unsigned display_seg;                                           /* cb45 */
unsigned mem_top_seg;                                           /* cb47 */
static char btn1_press;                                         /* cb49 */
static int bkgnd_bits;                                          /* cb4a */
char visible_on;                                                /* cb4c */
static int btn1_clicked[2];                                     /* cb4d */
static char fore_bytes[4];                                      /* cb51 */
int column_mouse;                                               /* cb55 */
int hide_count;                                                 /* cb57 */
static int mouse_click[2];                                      /* cb59 */
static int map_planes;                                          /* cb5d */
char back_screen;                                               /* cb5f */
static int seen_color;                                          /* cb60 */
static char left_fetched;                                       /* cb62 */
int pix_shifts;                                                 /* cb63 */
int pixel_cols;                                                 /* cb65 */
static char rbtn_press;                                         /* cb67 */
static char plback_pat[4];                                      /* cb68 */
int text_bytes;                                                 /* cb6c */
int shape_boot;                                                 /* cb6e */
int show_count;                                                 /* cb70 */
static char lifted_lbtn;                                        /* cb72 */
static int lastfx_seen;                                         /* cb73 */
static int left_arrow;                                          /* cb75 */
int mousey_now;                                                 /* cb77 */
static char right_taken;                                        /* cb79 */
int letter_step;                                                /* cb7a */
static int arrow_bytes;                                         /* cb7c */
static int button_last;                                         /* cb7e */
int base_offset;                                                /* cb80 */
char saved_equip;                                               /* cb82 */
int extent_text;                                                /* cb83 */
unsigned screen_addr;                                            /* cb85 */
static int kept_offset;                                         /* cb87 */
int buttons_now;                                                /* cb89 */
static int sprite_xpos;                                         /* cb8b */
static char rbtn_released;                                      /* cb8d */
static int pointer_state;                                       /* cb8e */
static struct Pointer pointer_now;                              /* cb90 */
static int sprite_trow;                                         /* cd94 */
static int sprite_lskip;                                        /* cd96 */
void far *spare_buffer;                                         /* cd98 */
int screen_lines;                                               /* cd9c */
int planes_count;                                               /* cd9e */
int txthigh_dots;                                               /* cda0 */
static int shape_height;                                        /* cda2 */
static char mouse_present;                                      /* cda4 */

static unsigned lmask_tab[16] = {
    0xffff, 0xff7f, 0xff3f, 0xff1f, 0xff0f, 0xff07, 0xff03, 0xff01,
    0xff00, 0x7f00, 0x3f00, 0x1f00, 0x0f00, 0x0700, 0x0300, 0x0100
};
static unsigned rmask_tab[16] = {
    0x0000, 0x0080, 0x00c0, 0x00e0, 0x00f0, 0x00f8, 0x00fc, 0x00fe,
    0x00ff, 0x80ff, 0xc0ff, 0xe0ff, 0xf0ff, 0xf8ff, 0xfcff, 0xfeff
};
static char cga_map[16] = { 0, 3, 2, 2, 3, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 3 };
unsigned char font_cells[0x480] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* space */
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00, 0x00, /* ! */
    0x6c, 0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* " */
    0x00, 0x36, 0x36, 0x7f, 0x36, 0x36, 0x7f, 0x36, 0x36, 0x00, 0x00, 0x00, /* # */
    0x38, 0x10, 0x7c, 0x7c, 0x10, 0x38, 0x38, 0x6c, 0x6c, 0xc6, 0x00, 0x00, /* $ */
    0x00, 0xc6, 0x06, 0x0c, 0x18, 0x18, 0x30, 0x60, 0x66, 0x00, 0x00, 0x00, /* % */
    0x00, 0x38, 0x6c, 0x6c, 0x38, 0x6c, 0x6f, 0xc6, 0xc6, 0x7f, 0x00, 0x00, /* & */
    0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' */
    0x0e, 0x18, 0x18, 0x30, 0x30, 0x30, 0x30, 0x18, 0x18, 0x0e, 0x00, 0x00, /* ( */
    0x70, 0x18, 0x18, 0x0c, 0x0c, 0x0c, 0x0c, 0x18, 0x18, 0x70, 0x00, 0x00, /* ) */
    0x00, 0x00, 0x00, 0x66, 0x3c, 0x18, 0xff, 0x18, 0x3c, 0x66, 0x00, 0x00, /* asterisk */
    0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x00, 0x00, /* + */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x30, 0x00, /* , */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, /* - */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, /* . */
    0x00, 0x06, 0x06, 0x0c, 0x18, 0x18, 0x30, 0x60, 0x60, 0x00, 0x00, 0x00, /* slash */
    0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, /* 0 */
    0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, /* 1 */
    0x3c, 0x66, 0x06, 0x06, 0x0c, 0x0c, 0x18, 0x30, 0x30, 0x7e, 0x00, 0x00, /* 2 */
    0x7e, 0x06, 0x06, 0x0c, 0x18, 0x0c, 0x06, 0x06, 0x66, 0x3c, 0x00, 0x00, /* 3 */
    0x66, 0x66, 0x66, 0x66, 0x7e, 0x06, 0x06, 0x06, 0x06, 0x06, 0x00, 0x00, /* 4 */
    0x7e, 0x60, 0x60, 0x60, 0x7c, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00, 0x00, /* 5 */
    0x3c, 0x66, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, /* 6 */
    0x7e, 0x06, 0x0c, 0x0c, 0x18, 0x18, 0x18, 0x30, 0x30, 0x30, 0x00, 0x00, /* 7 */
    0x3c, 0x66, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, /* 8 */
    0x3c, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00, 0x00, /* 9 */
    0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, /* : */
    0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x30, 0x00, 0x00, /* ; */
    0x00, 0x06, 0x0c, 0x18, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x00, 0x00, 0x00, /* < */
    0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, /* = */
    0x00, 0x60, 0x30, 0x18, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, /* > */
    0x3c, 0x66, 0x06, 0x06, 0x0c, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00, 0x00, /* ? */
    0x00, 0x7c, 0xc6, 0x92, 0xaa, 0x82, 0xaa, 0x92, 0x8c, 0x70, 0x00, 0x00, /* @ */
    0x3c, 0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, /* A */
    0x7c, 0x66, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x7c, 0x00, 0x00, /* B */
    0x3c, 0x66, 0x66, 0x60, 0x60, 0x60, 0x60, 0x66, 0x66, 0x3c, 0x00, 0x00, /* C */
    0x7c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7c, 0x00, 0x00, /* D */
    0x7e, 0x60, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00, 0x00, /* E */
    0x7e, 0x60, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, /* F */
    0x3c, 0x66, 0x60, 0x60, 0x6e, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, /* G */
    0x66, 0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, /* H */
    0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00, 0x00, /* I */
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, /* J */
    0x66, 0x66, 0x6c, 0x6c, 0x78, 0x6c, 0x6c, 0x66, 0x66, 0x66, 0x00, 0x00, /* K */
    0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00, 0x00, /* L */
    0x63, 0x77, 0x77, 0x7f, 0x6b, 0x6b, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, /* M */
    0x66, 0x66, 0x76, 0x76, 0x7e, 0x6e, 0x6e, 0x66, 0x66, 0x66, 0x00, 0x00, /* N */
    0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, /* O */
    0x7c, 0x66, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, /* P */
    0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7c, 0x36, 0x00, 0x00, /* Q */
    0x7c, 0x66, 0x66, 0x66, 0x7c, 0x6c, 0x6c, 0x66, 0x66, 0x66, 0x00, 0x00, /* R */
    0x3c, 0x66, 0x60, 0x70, 0x1c, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00, 0x00, /* S */
    0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, /* T */
    0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, /* U */
    0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00, 0x00, /* V */
    0x63, 0x63, 0x63, 0x63, 0x6b, 0x6b, 0x7f, 0x77, 0x77, 0x63, 0x00, 0x00, /* W */
    0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x3c, 0x66, 0x66, 0x66, 0x00, 0x00, /* X */
    0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, /* Y */
    0x7e, 0x06, 0x06, 0x0c, 0x18, 0x18, 0x30, 0x60, 0x60, 0x7e, 0x00, 0x00, /* Z */
    0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00, 0x00, /* [ */
    0x00, 0x60, 0x60, 0x30, 0x18, 0x18, 0x0c, 0x06, 0x06, 0x00, 0x00, 0x00, /* backslash */
    0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00, 0x00, /* ] */
    0x18, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ^ */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, /* _ */
    0x18, 0x18, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ` */
    0x00, 0x00, 0x00, 0x3c, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3e, 0x00, 0x00, /* a */
    0x60, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7c, 0x00, 0x00, /* b */
    0x00, 0x00, 0x00, 0x3c, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x00, 0x00, /* c */
    0x06, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00, 0x00, /* d */
    0x00, 0x00, 0x00, 0x3c, 0x66, 0x66, 0x7e, 0x60, 0x60, 0x3e, 0x00, 0x00, /* e */
    0x1e, 0x30, 0x30, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, /* f */
    0x00, 0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x7c, 0x00, /* g */
    0x60, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, /* h */
    0x18, 0x00, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, /* i */
    0x18, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x70, 0x00, /* j */
    0x60, 0x66, 0x66, 0x6c, 0x78, 0x6c, 0x6c, 0x66, 0x66, 0x66, 0x00, 0x00, /* k */
    0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, /* l */
    0x00, 0x00, 0x00, 0x76, 0x7f, 0x6b, 0x6b, 0x63, 0x63, 0x63, 0x00, 0x00, /* m */
    0x00, 0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, /* n */
    0x00, 0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, /* o */
    0x00, 0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x00, /* p */
    0x00, 0x00, 0x00, 0x3e, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x06, 0x00, /* q */
    0x00, 0x00, 0x00, 0x7c, 0x66, 0x60, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, /* r */
    0x00, 0x00, 0x00, 0x3e, 0x60, 0x60, 0x3c, 0x06, 0x06, 0x7c, 0x00, 0x00, /* s */
    0x18, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x0e, 0x00, 0x00, /* t */
    0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00, 0x00, /* u */
    0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00, 0x00, /* v */
    0x00, 0x00, 0x00, 0x63, 0x63, 0x63, 0x6b, 0x6b, 0x7f, 0x36, 0x00, 0x00, /* w */
    0x00, 0x00, 0x00, 0xc6, 0x6c, 0x38, 0x38, 0x38, 0x6c, 0xc6, 0x00, 0x00, /* x */
    0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x7c, 0x00, /* y */
    0x00, 0x00, 0x00, 0x7e, 0x0c, 0x0c, 0x18, 0x30, 0x30, 0x7e, 0x00, 0x00, /* z */
    0x1c, 0x30, 0x30, 0x30, 0x60, 0x60, 0x30, 0x30, 0x30, 0x1c, 0x00, 0x00, /* { */
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, /* | */
    0x38, 0x0c, 0x0c, 0x0c, 0x06, 0x06, 0x0c, 0x0c, 0x0c, 0x38, 0x00, 0x00, /* } */
    0x00, 0x00, 0x00, 0x33, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ~ */
    0x00, 0x18, 0x34, 0x34, 0x62, 0x62, 0xc1, 0xc1, 0xff, 0x00, 0x00, 0x00  /* DEL */
};
unsigned char far *font_ext = 0;
static struct Pointer pointer = {
    0, 0, 16, 16,
    { {
        0x3f, 0xff, 0x1f, 0xff, 0x0f, 0xff, 0x07, 0xff,
        0x03, 0xff, 0x01, 0xff, 0x00, 0xff, 0x00, 0x7f,
        0x00, 0x3f, 0x00, 0x3f, 0x01, 0xff, 0x10, 0xff,
        0x30, 0xff, 0xf8, 0x7f, 0xf8, 0x7f, 0xfc, 0x7f
    } },
    { {
        0x00, 0x00, 0x40, 0x00, 0x60, 0x00, 0x70, 0x00,
        0x78, 0x00, 0x7c, 0x00, 0x7e, 0x00, 0x7f, 0x00,
        0x7f, 0x80, 0x7c, 0x00, 0x6c, 0x00, 0x46, 0x00,
        0x06, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00
    } }
};
static char pix_round[4] = { 0, 1, 3, 7 };
static unsigned char lmask8[8] = { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };
static unsigned char rmask8[8] = { 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 };

void far video_restore(void)                                      /* 22b7:000d */
{
    if (video_mode == 11) {
        outportb(0x3b8, 1);       /* out dx,al at 0x014f89 and 0x014f8f: the census's weak marker */
        outportb(0x3bf, 0);
    } else if (video_mode == 12) {
        video_bios(5, 0x83, 6, 6);
    }
    poke_byte(0x40, 0x10, saved_equip);
    video_bios(0, dos_mode);
    video_bios(1, 0, 0, 0, shape_boot);
}

void far fill_rect(int row1, int col1, int row2, int col2, register int attr) /* 22b7:0078 */
{
    short lmask;
    short rmask;
    short fill;
    short ones;
    short pat[16];
    short plane;
    short step;
    char patterned;
    char cga4;
    register int interior_columns;

    text_xy(row1, col1, &row1, &col1);
    text_xy(row2, col2, &row2, &col2);
    row1 = int_max(row1, clip_top);
    col1 = int_max(col1, clip_left);
    row2 = int_min(row2, clip_bottom);
    col2 = int_min(col2, clip_right);
    if (row1 >= row2 || col1 >= col2)
        return;
    patterned = cga4 = 0;
    if (visible_on)
        pointer_hide_rect(row1, col1, row2, col2);
    if (video_mode == 12 || video_mode == 13) {
        lmask = lmask_tab[(col1 & 3) * 4];
        rmask = rmask_tab[(col2 & 3) * 4];
        interior_columns = (col2 >> 2) - (col1 >> 2) - 1;
        col1 &= 0xfffc;
        col2 &= 0xfffc;
        step = 4;
    } else if (video_mode <= 4) {
        lmask = lmask_tab[(col1 & 7) * 2];
        rmask = rmask_tab[(col2 & 7) * 2];
        interior_columns = (col2 >> 3) - (col1 >> 3) - 1;
        col1 &= 0xfff8;
        col2 &= 0xfff8;
        step = 8;
        if (!(attr & 0x100) && attr > 1) {
            patterned = 1;
            fill_pattern(attr, pat);
        } else
            attr = (attr & 0x100) | cga_map[attr & 0xf];
    } else if (video_mode == 14) {
        if ((attr & 0xff) < 0x10) {
            fill_pattern(attr & 0xf, &lmask);
            attr = (attr & 0x100) | lmask;
        }
        lmask = lmask_tab[(col1 & 1) * 8];
        rmask = rmask_tab[(col2 & 1) * 8];
        interior_columns = (col2 >> 1) - (col1 >> 1) - 1;
        col1 &= 0xfffe;
        col2 &= 0xfffe;
        step = 2;
    } else if (video_mode == 15) {
        if ((attr & 0xff) < 0x10) {
            fill_pattern(attr & 0xf, &lmask);
            attr = (attr & 0x100) | lmask;
        }
        if (attr & 0x100)
            for (; col1 < col2; col1++) {
                video_address_set(row1, col1);
                video_plane_set(col1 & 3);
                mask_words(attr & 0xff, 0xffff, row2 - row1, 2);
            }
        if (col1 & 3) {
            video_address_set(row1, col1);
            for (; (col1 & 3) && col1 < col2; col1++) {
                video_plane_set(col1 & 3);
                fill_bytes(row2 - row1, 1, attr);
            }
        }
        if (col2 - col1 >= 4) {
            video_address_set(row1, col1);
            video_plane_set(0xffee);
            fill_bytes(row2 - row1, (col2 - col1) >> 2, attr);
            col1 = col2 & 0xfffc;
        }
        if (col2 > col1) {
            video_address_set(row1, col1);
            for (; col2 > col1; col1++) {
                video_plane_set(col1 & 3);
                fill_bytes(row2 - row1, 1, attr);
            }
        }
        goto done;
    } else {
        if (video_mode >= 5 && video_mode <= 9)
            cga4 = 1;
        lmask = lmask_tab[col1 & 0xf];
        rmask = rmask_tab[col2 & 0xf];
        interior_columns = (col2 >> 4) - (col1 >> 4) - 1;
        col1 &= 0xfff0;
        col2 &= 0xfff0;
        step = 0x10;
        if (video_planes() == 1) {
            if (!(attr & 0x100) && attr > 1) {
                patterned = 1;
                fill_pattern(attr, pat);
            } else if (attr && !(attr & 1))
                attr |= 1;
        }
    }
    ones = 0xffff;
    row2 -= row1;
    if (interior_columns < 0) {
        interior_columns = 0;
        lmask &= rmask;
        rmask = 0;
    }
    if (lmask == -1) {
        step = 0;
        interior_columns++;
    }
    if (lmask == 0xff00 && rmask == 0xff) {
        step = 0;
        interior_columns++;
        col1 += 1 << pixel_shift();
        rmask = 0;
    }
    for (plane = 0; plane < video_planes(); plane++) {
        if (video_mode <= 4) {
            switch (attr & 3) {
            case 0: fill = 0; break;
            case 1: fill = 0x5555; break;
            case 2: fill = 0xaaaa; break;
            case 3: fill = 0xffff; break;
            }
        } else if (video_mode == 12 || video_mode == 13) {
            fill = ((attr & 0xf) << 4) | (attr & 0xf);
            fill |= fill << 8;
        } else if (video_mode == 14 || video_mode == 15) {
            fill = ((attr & 0xff) << 8) | (attr & 0xff);
        } else if (video_planes() == 1 && attr == 3) {
            fill = 0x5555;
        } else
            fill = (attr >> plane) & 1 ? 0xffff : 0;
        video_plane_set(plane);
        video_address_set(row1, col1);
        if (step) {
            if (patterned)
                pattern_column(row2, lmask, pat, row1);
            else
                mask_words(fill & lmask, attr & 0x100 ? ones : ~lmask, row2, 2);
        }
        if (interior_columns > 0) {
            video_address_set(row1, col1 + step);
            if (patterned)
                pattern_rows(row2, interior_columns + interior_columns, pat, row1);
            else if (attr & 0x100)
                mask_words(fill, ones, row2, interior_columns + interior_columns);
            else if (cga4) {
                if (plane == 0) {
                    video_plane_set(-attr - 1);
                    fill_words(row2, interior_columns + interior_columns, 0);
                    video_plane_set(0);
                }
            } else
                fill_words(row2, interior_columns + interior_columns, fill);
        }
        if (rmask) {
            video_address_set(row1, col2);
            if (patterned)
                pattern_column(row2, rmask, pat, row1);
            else
                mask_words(fill & rmask, attr & 0x100 ? ones : ~rmask, row2, 2);
        }
    }
done:
    if (visible_on)
        pointer_show();
}

void far page_copy(void)                                      /* 22b7:06dc */
{
    char buf[0x100];
    register int plane;

    pointer_hide_rect(-1, -1, -1, -1);
    pointer_draw_over_buffer(buf);
    switch (video_mode) {
    case 1: case 2: case 3: case 4:
        dissolve_2bpp(mem_top_seg, display_seg);
        break;
    case 8: case 11:
        dissolve_2bank(mem_top_seg, display_seg);
        break;
    case 12: case 13:
        dissolve_nibbles(mem_top_seg, display_seg);
        break;
    case 14:
        dissolve_bytes(mem_top_seg, display_seg);
        break;
    case 15:
        dissolve_planes(mem_top_seg, display_seg);
        break;
    case 5: case 6: case 7:
        dissolve_bits(mem_top_seg, display_seg);
        break;
    case 9:
        for (plane = 0; plane < video_planes(); plane++) {
            video_plane_set(plane);
            huge_move(0, mem_top_seg, 0, display_seg, video_len);
        }
        break;
    case 10:
        reg_write(0x3c4, 2, back_screen ? 3 : 0xc);
        reg_write(0x3ce, 4, back_screen ? 3 : 1);
    case 16: case 17:
        huge_move(0, mem_top_seg, 0, display_seg, video_len);
        set_text_mode(visible_on);
        break;
    }
    pointer_restore_from_buffer(0L);
    pointer_show();
}

void far screen_page_save(void)                                      /* 22b7:0849 */
{
    pointer_hide_rect(-1, -1, -1, -1);
    switch (video_mode) {
    case 1: case 2: case 3: case 4: case 11: case 12: case 13: case 14: case 16: case 17:
        huge_move(0, display_seg, 0, mem_top_seg, video_len);
        break;
    case 5: case 6: case 7: case 8: case 9: case 15:
        video_plane_set(0xffef);
        far_move(0, display_seg, 0, mem_top_seg, video_len);
        video_plane_set(0);
        break;
    case 10:
        reg_write(0x3c4, 2, back_screen ? 0xc : 3);
        reg_write(0x3ce, 4, back_screen ? 1 : 3);
        huge_move(0, display_seg, 0, mem_top_seg, 0x6d60);
        set_text_mode(visible_on);
        break;
    }
    pointer_restore_from_buffer(0L);
    pointer_show();
}

void far page_swap(void)                                      /* 22b7:094a */
{
    char save[0x400];
    char buf[0x100];
    register unsigned pos;
    register unsigned chunk_size;

    if (video_mode <= 4 || video_mode == 13 || video_mode == 14 || video_mode == 17) {
        pointer_hide_rect(-1, -1, -1, -1);
        pointer_draw_over_buffer(buf);
        pos = 0;
        do {
            chunk_size = uint_min(video_len - pos, 0x400);
            huge_move(pos, mem_top_seg, (char far *)save, chunk_size);
            huge_move(pos, display_seg, pos, mem_top_seg, chunk_size);
            huge_move((char far *)save, pos, display_seg, chunk_size);
            pos += 0x400;
        } while (pos < video_len && pos);
        pointer_restore_from_buffer(buf);
        pointer_show();
    } else
        screen_page_show();
}

void far screen_page_show(void)                                      /* 22b7:0a0b */
{
    char buf[0x100];
    register unsigned seg;

    pointer_hide_rect(-1, -1, -1, -1);
    pointer_draw_over_buffer(buf);
    switch (video_mode) {
    case 1: case 2: case 3: case 4: case 17:
        cga_copy(mem_top_seg, display_seg);
        pointer_restore_from_buffer(0L);
        pointer_show();
        return;
    case 13: case 14: case 16:
        huge_move(0, mem_top_seg, 0, display_seg, video_len);
        pointer_restore_from_buffer(0L);
        pointer_show();
        return;
    case 5: case 6: case 7: case 8: case 9:
        video_bios(5, display_seg == 0xa000 ? 1 : 0);
        while (!(inportb(0x3da) & 8))         /* in al,dx: wait for vertical retrace */
            ;
        break;
    case 15:
        reg_write(0x3c4, 4, 6);
        reg_write(0x3d4, 0x17, 0xc3);
        reg_write(0x3d4, 0x14, 0);
        reg_write(0x3d4, 0xc, display_seg == 0xa000 ? 0x40 : 0);
        while (!(inportb(0x3da) & 8))
            ;
        break;
    case 10:
        video_bios(0x10, 2, 0, 0, 0, (int)ega_pals[back_screen] >> 8, ega_pals[back_screen]);
        break;
    case 11:
        outportb(0x3b8, display_seg == 0xb000 ? 0x8a : 0x0a);
        break;
    case 12:
        video_bios(5, 0x83, back_screen ? 4 : mem_top_seg == 0xb000 ? 2 : 6, 6);
        break;
    }
    seg = display_seg;
    display_seg = mem_top_seg;
    mem_top_seg = seg;
    back_screen = 1 - back_screen;
    set_text_mode(visible_on);
    pointer_restore_from_buffer(buf);
    pointer_show();
}

void far screen_page_present(void)                                      /* 22b7:0bde */
{
    switch (video_mode) {
    case 1: case 2: case 3: case 4: case 13: case 14: case 16: case 17:
        screen_page_show();
        break;
    case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12: case 15:
        screen_page_show();
        screen_page_save();
        break;
    }
}

void far set_text_mode(char mode)                                /* 22b7:0c21 */
{
    if (mode)
        draw_seg = display_seg;
    else
        draw_seg = mem_top_seg;
    visible_on = mode ? 1 : 0;
    if (video_mode == 10) {
        reg_write(0x3c4, 2, back_screen != visible_on ? 0xc : 3);
        reg_write(0x3ce, 4, back_screen != visible_on ? 3 : 1);
    }
}

char far text_mode(void)                                         /* 22b7:0c92 */
{
    return visible_on;
}

int far draw_char(int row, int col, char character_code, char attr, char flags)  /* 22b7:0c96 */
{
    short index;
    char rows[8];
    char plane[0x40];
    unsigned char far *glyph_data;
    register int foreground, background;

    text_xy(row, col, &row, &col);
    if (row < 0) {
        seen_color = lastfx_seen = 0x100;
        return 0;
    }
    if (row < 0 || col < 0 || screen_rows() < row + txthigh_dots + txthigh_dots
        || screen_cols() < col + letter_step + letter_step)
        return 0;
    character_code &= 0x7f;
    if (character_code < 0x20 && font_ext == 0)
        character_code = 0;
    else
        character_code -= 0x20;
    if (character_code < 0)
        glyph_data = font_ext + (character_code + 0x20) * 12;
    else
        glyph_data = font_cells + character_code * 12;

    if (video_mode >= 8 && video_mode <= 11 || video_mode == 16) {
        if (attr != seen_color || flags != lastfx_seen) {
            seen_color = attr;
            lastfx_seen = flags;
            foreground = attr & 0xf;
            background = (attr >> 4) & 0xf;
            if (foreground != 15 && background != 0
                && (foreground == 0 || background == 15 || (attr & 0xf) < ((attr >> 4) & 0xf)))
                bkgnd_bits = -1;
            else
                bkgnd_bits = 0;
            fore_mask = -1;
            if ((flags & 1) || foreground == 9) {
                pat[0] = 0x55;
                pat[1] = 0xaa;
            } else {
                pat[0] = pat[1] = 0xff;
            }
        }
        if (visible_on)
            pointer_hide_rect(row, col, row + 0xc, col + 0x10);
        if (video_mode == 9) {
            fill_rect(row, col, row + 0xc, ((flags & 2) ? 8 : 0x10) + col, 0);
            video_plane_set(0);
        }
        video_address_set(row, col);
        if ((flags & 2) == 2)
            column_1bpp(glyph_data, 0xc, fore_mask, bkgnd_bits, pat[0], pat[1]);
        else
            column_2bpp(glyph_data, 0xc, fore_mask, bkgnd_bits, pat[0], pat[1]);
        if (visible_on)
            pointer_show();
        if ((flags & 2) != 2)
            return letter_step << 1;
        return letter_step;
    }
    rows[0] = *glyph_data;
    glyph_data++;
    rows[1] = *glyph_data;
    glyph_data++;
    glyph_data++;
    rows[2] = *glyph_data;
    glyph_data++;
    rows[3] = *glyph_data;
    glyph_data++;
    glyph_data++;
    rows[4] = *glyph_data;
    glyph_data++;
    glyph_data++;
    rows[5] = *glyph_data;
    glyph_data++;
    rows[6] = *glyph_data;
    glyph_data++;
    rows[7] = *glyph_data;
    if (video_mode >= 5 && video_mode <= 7) {
        if (attr != seen_color || flags != lastfx_seen) {
            seen_color = attr;
            lastfx_seen = flags;
            if (flags) {
                pat[0] = 0x55;
                pat[1] = 0xaa;
            } else {
                pat[0] = pat[1] = 0xff;
            }
            for (index = 0; index < 4; index++) {
                switch ((attr >> index) & 0x11) {   /* 2e 3b 07 at 0x015f25: Turbo C's sparse-switch value search */
                case 0:
                    fore_bytes[index] = plback_pat[index] = 0;
                    break;
                case 1:
                    fore_bytes[index] = 0xff;
                    plback_pat[index] = 0;
                    break;
                case 0x10:
                    fore_bytes[index] = plback_pat[index] = 0xff;
                    break;
                case 0x11:
                    fore_bytes[index] = 0;
                    plback_pat[index] = 0xff;
                    break;
                }
            }
        }
        if (visible_on)
            pointer_hide_rect(row, col, row + 8, col + 8);
        video_address_set(row, col);
        for (index = 0; index < 4; index++) {
            video_plane_set(index);
            column_1bpp(rows, 8, fore_bytes[index], plback_pat[index], pat[0], pat[1]);
        }
    } else if (video_mode >= 12 && video_mode <= 15) {
        if (attr != seen_color || flags != lastfx_seen) {
            seen_color = attr;
            lastfx_seen = flags;
            foreground = attr & 0xf;
            background = (attr >> 4) & 0xf;
            if (video_mode == 14 || video_mode == 15) {
                fill_pattern(foreground, &fore_mask);
                fill_pattern(background, &bkgnd_bits);
                fore_mask ^= bkgnd_bits;
            } else {
                fore_mask = foreground ^ background;
                fore_mask = fore_mask << 4 | fore_mask;
                bkgnd_bits = background << 4 | background;
            }
        }
        if (video_mode == 14) {
            expand_1to8(rows, plane, 8);
            index = 0x40;
        } else if (video_mode == 15) {
            index = 0;
        } else {
            expand_1to4(rows, plane, 8);
            index = 0x20;
        }
        while (--index >= 0)
            plane[index] = plane[index] & fore_mask ^ bkgnd_bits;
        if (visible_on)
            pointer_hide_rect(row, col, row + 8, col + 8);
        video_address_set(row, col);
        if (video_mode == 15)
            xvga_char(fore_mask ^ bkgnd_bits, bkgnd_bits, rows);
        else
            blit_words(plane, 8, video_mode == 14 ? 8 : 4, 0);
    } else if (video_mode == 17 && (flags & 2) == 2) {
        if (attr != seen_color || flags != lastfx_seen) {
            seen_color = attr;
            lastfx_seen = flags;
            foreground = attr & 0xf;
            background = (attr >> 4) & 0xf;
            if (foreground != 15 && background != 0
                && (foreground == 0 || background == 15 || (attr & 0xf) < ((attr >> 4) & 0xf)))
                bkgnd_bits = -1;
            else
                bkgnd_bits = 0;
            fore_mask = -1;
            if ((flags & 1) || foreground == 9) {
                pat[0] = 0x55;
                pat[1] = 0xaa;
            } else {
                pat[0] = pat[1] = 0xff;
            }
        }
        if (visible_on)
            pointer_hide_rect(row, col, row + 8, col + 8);
        video_address_set(row, col);
        column_1bpp(rows, 8, fore_mask, bkgnd_bits, pat[0], pat[1]);
        if (visible_on)
            pointer_show();
        return letter_step;
    } else {
        if (attr != seen_color || flags != lastfx_seen) {
            seen_color = attr;
            lastfx_seen = flags;
            attr = cga_map[attr & 0xf] | cga_map[(attr >> 4) & 0xf] << 4;
            if (((attr & 0x30) >> 4) == (attr & 3))
                attr = 3;
            if (((attr & 0x10) >> 4) == (attr & 1)) {
                fore_mask = 0;
                bkgnd_bits = (attr & 1) ? 0x5555 : 0;
            } else {
                fore_mask = 0x5555;
                bkgnd_bits = (attr & 1) ? 0 : 0x5555;
            }
            if (((attr & 0x20) >> 4) == (attr & 2)) {
                bkgnd_bits |= (attr & 2) ? 0xaaaa : 0;
            } else {
                fore_mask |= 0xaaaa;
                bkgnd_bits |= (attr & 2) ? 0 : 0xaaaa;
            }
            if (flags)
                pat[0] = 0;
            else
                pat[0] = 0xff;
            pat[1] = 0xff;
        }
        if (visible_on)
            pointer_hide_rect(row, col, row + 8, (video_mode == 17 ? 0x10 : 8) + col);
        video_address_set(row, col);
        column_2bpp(rows, 8, fore_mask, bkgnd_bits, pat[0], pat[1]);
    }
    if (visible_on)
        pointer_show();
    return letter_step << 1;
}

int far screen_rows(void)                                        /* 22b7:14ac */
{
    return screen_lines;
}

int far screen_cols(void)                                        /* 22b7:14b0 */
{
    return pixel_cols;
}

int far pixel_shift(void)                                        /* 22b7:14b4 */
{
    return pix_shifts;
}

int far video_planes(void)                                       /* 22b7:14b8 */
{
    return planes_count;
}

char far is_xvga(void)                                           /* 22b7:14bc */
{
    return video_mode == 15;
}

void far video_plane_set(int plane)                                 /* 22b7:14cb */
{
    if (video_mode >= 5 && video_mode <= 9 || video_mode == 15) {
        map_planes = plane;
        if (plane >= 0) {
            reg_write(0x3c4, 2, 1 << plane);
            reg_write(0x3ce, 4, plane);
            reg_write(0x3ce, 1, 0);
        } else if (plane < -0x10) {
            reg_write(0x3c4, 2, 0xf);
            if (plane >= -0x11) {
                reg_write(0x3ce, 5, video_mode == 15 ? 0x41 : 1);
                return;
            }
        } else {
            reg_write(0x3c4, 2, 0xf);
            reg_write(0x3ce, 0, -plane - 1);
            reg_write(0x3ce, 1, 0xf);
        }
        reg_write(0x3ce, 5, video_mode == 15 ? 0x40 : 0);
    }
}

void far draw_redirect(char far *buf)                             /* 22b7:15ca */
{
    if (buf) {
        kept_offset = screen_addr;
        prev_seg = draw_seg;
        screen_addr = (int)buf;
        draw_seg = _DS;
    } else {
        screen_addr = kept_offset;
        draw_seg = prev_seg;
    }
}

void far video_address_set(int row, int col)                          /* 22b7:15fc */
{
    short aligned_row;

    if (col < 0 && row <= 0)
        row = col = 0;
    switch (video_mode) {
    case 1: case 2: case 3: case 4:
        screen_addr = (row << 5) + (row << 3) + (col >> 2);
        if (row & 1)
            screen_addr += 0x1fd8;
        break;
    case 5: case 6: case 7:
        screen_addr = (row << 5) + (row << 3) + (col >> 3);
        break;
    case 8: case 9: case 10:
        screen_addr = (row << 6) + (row << 4) + (col >> 3) + 0x7d0;
        break;
    case 11:
        screen_addr = (row >> 2) * 0x5a + (col >> 3) + ((row & 3) << 13) + 0x221;
        break;
    case 12: case 13:
        screen_addr = ((aligned_row = row & 0xfffc) << 5) + (aligned_row << 3) + (col >> 1) + ((row & 3) << 13);
        break;
    case 14:
        screen_addr = (row << 8) + (row << 6) + col;
        break;
    case 15:
        screen_addr = (row << 6) + (row << 4) + (col >> 2);
        break;
    case 16:
        screen_addr = (row << 6) + (row << 4) + (col >> 3);
        break;
    case 17:
        screen_addr = (row << 5) + (row << 3) + (col >> 3);
        if (row & 1)
            screen_addr += 0x1fd8;
        break;
    }
}

void far mouse_event(events)                                    /* 22b7:178c */
char events;                    /* old-style: callers push mov ax,imm, and the body tests a byte */
{
    if (events & 2) {
        btn_at[0] = mousey_now;
        btn_col[0] = column_mouse;
        btn1_press = 1;
        lifted_lbtn = 0;
    }
    if (events & 4) {
        left_up[0] = mousey_now;
        left_up[1] = column_mouse;
        lifted_lbtn = 1;
        if (left_fetched) {
            btn1_clicked[0] = mousey_now;
            btn1_clicked[1] = column_mouse;
            left_fetched = 0;
        }
    }
    if (events & 8) {
        btn_at[1] = mousey_now;
        btn_col[1] = column_mouse;
        rbtn_press = 1;
        rbtn_released = 0;
    }
    if (events & 0x10) {
        right_up[0] = mousey_now;
        right_up[1] = column_mouse;
        rbtn_released = 1;
        if (right_taken) {
            mouse_click[0] = mousey_now;
            mouse_click[1] = column_mouse;
            right_taken = 0;
        }
    }
}

void far mouse_poll(void)                                      /* 22b7:1827 */
{
    short mouse_function;
    short y;
    short x;

    if (pointer_state == 1) {
        mouse_function = 3;
        mouse_call_regs(&mouse_function, &buttons_now, &x, &y);
        mousey_now = y >> 3;
        column_mouse = x >> 3;
    }
}

void far pointer_follow(void)                                      /* 22b7:1871 */
{
    if (pointer_now.row + sprite_trow != mousey_now || pointer_now.col + left_arrow != column_mouse) {
        pointer_hide();
        pointer_show();
    }
    sfx_tick(sound_ticks());
}

void far pointer_scale(void)                                      /* 22b7:18a6 */
{
    struct Pointer save;
    long far *src;
    long far *dst;
    register int index;

    for (index = 1; index < 4; index++)
        mem_move(pointer.image[0], pointer.image[index], 0x40);
    mem_move(&pointer, &save, sizeof(struct Pointer));
    switch (video_mode) {
    case 17:
        pointer.cols <<= 1;
        pointer.col <<= 1;
    case 1: case 2: case 3: case 4:
        expand_1to2(save.mask, pointer.mask, 0x20);
        expand_1to2(save.image, pointer.image, 0x20);
        break;
    case 8: case 10: case 11: case 16:
        pointer.rows += pointer.rows >> 1;
        pointer.row += pointer.row >> 1;
        pointer.cols <<= 1;
        pointer.col <<= 1;
        expand_1to2(pointer.mask, save.mask, 0x20);
        expand_1to2(pointer.image, save.image, 0x20);
        src = (long far *)save.mask;
        dst = (long far *)pointer.mask;
        for (index = 0; index < 0x10; index++) {
            *dst++ = *src;
            if (index & 1)
                *dst++ = *src;
            src++;
        }
        src = (long far *)save.image;
        dst = (long far *)pointer.image;
        for (index = 0; index < 0x10; index++) {
            *dst++ = *src;
            if (index & 1)
                *dst++ = *src;
            src++;
        }
        break;
    case 12: case 13:
        expand_1to4(save.mask, pointer.mask, 0x20);
        expand_1to4(save.image, pointer.image, 0x20);
        break;
    case 14: case 15:
        expand_1to8(save.mask, pointer.mask, 0x20);
        expand_1to8(save.image, pointer.image, 0x20);
        src = (long far *)pointer.image;
        for (index = 0; index < 0x40; index++)
            *src++ &= 0x0f0f0f0fL;
        if (video_mode == 15) {
            mem_move(&pointer, &save, sizeof(struct Pointer));
            plane_split(save.mask, 0x40, pointer.mask);
            plane_split(save.image, 0x40, pointer.image);
        }
    }
}

void far mouse_setup(void)                                      /* 22b7:1b39 */
{
    if (mouse_present) {
        mouse_call(0);
        mouse_call(7, 0, 0, (screen_cols() << 3) - 1);
        mouse_call(8, 0, 0, (screen_rows() << 3) - 1);
        mouse_call(0xf, 0, 3, 4);
        mouse_call(4, 0, column_mouse << 3, mousey_now << 3);
        mouse_handler_set(mouse_handler, 0x7f);
        if (video_mode == 10)
            set_text_mode(visible_on);
    }
}

void far mouse_pointer_init(int argc, char far * far *argv)            /* 22b7:1be3 */
{
    mouse_present = peek_word(0, 0xcc) && peek_word(0, 0xce) && mouse_call(0);
    show_count = 0;
    hide_count = 0;
    button_last = 0;
    pointer_scale();
    pointer_shape_set(0L);
    text_xy(0x1f44, 0x1f44, &mousey_now, &column_mouse);
    if (mouse_present) {
        mouse_setup();
        pointer_state = 1;
    } else
        pointer_state = 0;
    rbtn_released = lifted_lbtn = 1;
}

void far mouse_pointer_done(void)                                      /* 22b7:1c7b */
{
    if (mouse_present)
        mouse_call(0);
}

int far pointer_mode_set(int mode)                                   /* 22b7:1c8c */
{
    if (mode != pointer_state) {
        switch (mode) {
        case 0:
            if (pointer_state == 1)
                mouse_call(0);
            break;
        case 1:
            if (!mouse_present)
                return pointer_state;
            mouse_setup();
            break;
        default:
            return pointer_state;
        }
        pointer_state = mode;
    }
    return pointer_state;
}

int far pix_rem(int x)                                      /* 22b7:1ccd */
{
    return (pix_round[pixel_shift()] & x) << (3 - pixel_shift());
}

void far pointer_draw(short row, register int col, int page)             /* 22b7:1cec */
{
    short off;
    short old_plane;
    short old_ofs;
    short skip;
    short lskip;
    short rskip;
    short extra;
    char edge;
    char left_mask;
    char right_mask;
    char old_page;
    register int index;

    row -= pointer_now.row;
    col -= pointer_now.col;
    sprite_trow = row;
    left_arrow = col;
    shape_height = pointer_now.rows;
    if (screen_rows() <= row)
        goto hide;
    if (row + shape_height <= 0)
        goto hide;
    if (screen_cols() <= col)
        goto hide;
    if (pointer_now.cols + col <= 0)
        goto hide;
    if (row < 0)
        skip = -row;
    else
        skip = 0;
    if (screen_rows() < row + shape_height)
        shape_height = screen_rows() - row;
    row += skip;
    shape_height -= skip;
    if (video_mode == 15) {
        if (screen_cols() < pointer_now.cols + col)
            arrow_bytes = screen_cols() - col;
        else
            arrow_bytes = pointer_now.cols;
        if (col < 0)
            sprite_lskip = -col;
        else
            sprite_lskip = 0;
        if (shape_height <= 0)
            goto hide;
        if (arrow_bytes - sprite_lskip <= 0)
            goto hide;
        old_plane = map_planes;
        old_ofs = screen_addr;
        old_page = visible_on;
        set_text_mode(page);
        for (index = 0; index < video_planes(); index++) {
            video_address_set(row, col + sprite_lskip + index);
            video_plane_set((col + sprite_lskip + index) & 3);
            grab_bytes(ptr_under[index], shape_height, (arrow_bytes - sprite_lskip + 3) >> 2);
            if (sprite_lskip) {
                extra = (arrow_bytes - sprite_lskip + ((sprite_lskip + index) & 3)) >> 2;
                off = (pointer_now.cols >> 2) * skip + (((sprite_lskip + index) & 3) << 6)
                      + ((sprite_lskip + 3 - ((sprite_lskip + index) & 3)) >> 2);
            } else {
                extra = (arrow_bytes + 3 - index) >> 2;
                off = (pointer_now.cols >> 2) * skip + ((index & 3) << 6);
            }
            if (extra) {
                rskip = (pointer_now.cols >> 2) - extra;
                /* the (long) casts load each address into dx:ax before off is added (0x016e51) */
                blit_shifted((char far *)(long)pointer_now.image + off,
                             (char far *)(long)pointer_now.mask + off,
                             shape_height, extra, rskip, 0xff, 0xff, 0);
            }
        }
        set_text_mode(old_page);
        video_plane_set(old_plane);
        screen_addr = old_ofs;
        return;
    }
    arrow_bytes = pointer_now.cols >> pixel_shift();
    extra = 0;
    if (col < 0) {
        sprite_lskip = lskip = -col >> pixel_shift();
        left_mask = lmask8[pix_rem(-col)];
        col += lskip << pixel_shift();
        rskip = 0;
        right_mask = 0xff;
    } else {
        sprite_lskip = lskip = 0;
        left_mask = 0xff;
        if (screen_cols() < pointer_now.cols + col) {
            index = pointer_now.cols + col - screen_cols();
            rskip = index >> pixel_shift();
            right_mask = rmask8[pix_rem(index)];
            if (pix_rem(col))
                extra--;
        } else {
            rskip = 0;
            right_mask = 0xff;
        }
    }
    if (edge = pix_rem(col))
        extra++;
    sprite_xpos = col;
    if (col < 0) {
        sprite_lskip++;
        if (row == 0) {
            skip++;
            row++;
        }
    }
    off = skip * arrow_bytes + lskip;
    arrow_bytes -= rskip;
    rskip += lskip;
    if (shape_height <= 0)
        goto hide;
    if (arrow_bytes - sprite_lskip <= 0)
        goto hide;
    old_plane = map_planes;
    old_ofs = screen_addr;
    old_page = visible_on;
    set_text_mode(page);
    video_address_set(row, col);
    for (index = 0; index < video_planes(); index++) {
        video_plane_set(index);
        if (col < 0)
            video_address_set(row, 0);
        grab_bytes(ptr_under[index], shape_height, arrow_bytes - sprite_lskip + (edge ? extra : 0));
        if (col < 0)
            video_address_set(row, col);
        blit_shifted(pointer_now.image[index] + off, (char far *)(long)pointer_now.mask + off,
                     shape_height, arrow_bytes - lskip, rskip, left_mask, right_mask, edge);
    }
    set_text_mode(old_page);
    video_plane_set(old_plane);
    screen_addr = old_ofs;
    arrow_bytes += edge ? extra : 0;
    return;
hide:
    shape_height = 0;
}

void far pointer_show(void)                                      /* 22b7:2122 */
{
    if (hide_count > 0) {
        if (--hide_count <= 0)
            pointer_follow();
    } else {
        if (show_count == 0)
            pointer_draw(mousey_now, column_mouse, 1);
        show_count++;
    }
}

void far pointer_restore(int page, char far *buf)                  /* 22b7:2154 */
{
    short old_plane;
    short old_ofs;
    char old_page;
    register int index;
    register int row;

    if (shape_height > 0) {
        old_plane = map_planes;
        old_ofs = screen_addr;
        old_page = visible_on;
        set_text_mode(page);
        if (video_mode == 15) {
            for (index = 0; index < video_planes(); index++) {
                video_address_set(int_max(0, sprite_trow), left_arrow + sprite_lskip + index);
                video_plane_set((left_arrow + sprite_lskip + index) & 3);
                put_bytes(buf + (index << 6), shape_height, (arrow_bytes - sprite_lskip + 3) >> 2);
            }
        } else {
            row = sprite_trow;
            if (row <= 0) {
                if (sprite_xpos < 0)
                    row = 1;
                else
                    row = 0;
            }
            video_address_set(row, int_max(0, left_arrow));
            for (index = 0; index < video_planes(); index++) {
                video_plane_set(index);
                put_bytes(buf + (index << 6), shape_height, arrow_bytes - sprite_lskip);
            }
        }
        set_text_mode(old_page);
        video_plane_set(old_plane);
        screen_addr = old_ofs;
    }
}

void far pointer_erase(void)                                      /* 22b7:2271 */
{
    pointer_restore(1, ptr_under);
    shape_height = 0;
}

void far pointer_hide_rect(int row1, int col1, int row2, int col2)    /* 22b7:2288 */
{
    register int col;

    if (show_count != 1) {
        pointer_hide();
        return;
    }
    hide_count++;
    if (sprite_trow < row2 && pointer_now.rows + sprite_trow > row1) {
        col = -1 << pixel_shift() & left_arrow;
        if (col < col2 && pointer_now.cols + col + (1 << pixel_shift()) > col1)
            while (hide_count > 0) {
                pointer_hide();
                hide_count--;
            }
    }
}

void far pointer_draw_over_buffer(char far *buf)                             /* 22b7:22f6 */
{
    if (show_count > 0) {
        mem_move(ptr_under, buf, 0x100);
        pointer_draw(pointer_now.row + sprite_trow, pointer_now.col + left_arrow, 0);
    }
}

void far pointer_restore_from_buffer(char far *buf)                             /* 22b7:2333 */
{
    if (show_count > 0)
        pointer_restore(0, buf ? buf : ptr_under);
}

int far mouse_press(char mask, int far *x, int far *y)           /* 22b7:235f */
{
    mouse_poll();
    if (btn1_press && (mask & 1)) {
        if (lifted_lbtn) {
            btn1_clicked[0] = left_up[0];
            btn1_clicked[1] = left_up[1];
            left_fetched = 0;
        } else
            left_fetched = 1;
        *x = btn_at[0];
        *y = btn_col[0];
        btn1_press = 0;
        button_last = 0;
        return 1;
    }
    if (rbtn_press && (mask & 2)) {
        if (rbtn_released) {
            mouse_click[0] = right_up[0];
            mouse_click[1] = right_up[1];
            right_taken = 0;
        } else
            right_taken = 1;
        *x = btn_at[1];
        *y = btn_col[1];
        rbtn_press = 0;
        button_last = 1;
        return 2;
    }
    *x = mousey_now;
    *y = column_mouse;
    return 0;
}

char far mouse_clicked(int far *x, int far *y)                   /* 22b7:2418 */
{
    mouse_poll();
    if (!button_last && !left_fetched) {
        *x = btn1_clicked[0];
        *y = btn1_clicked[1];
        left_fetched = 1;
        return 1;
    }
    if (button_last && !right_taken) {
        *x = mouse_click[0];
        *y = mouse_click[1];
        right_taken = 1;
        return 1;
    }
    *x = mousey_now;
    *y = column_mouse;
    return 0;
}

void far mouse_pos(int far *x, int far *y)                       /* 22b7:2485 */
{
    mouse_poll();
    *x = mousey_now;
    *y = column_mouse;
}

void far mouse_move_by(int drow, int dcol)                        /* 22b7:24a0 */
{
    mousey_now = int_max(0, int_min(mousey_now + drow, screen_rows() - 1));
    column_mouse = int_max(0, int_min(column_mouse + dcol, screen_cols() - 1));
    if (mouse_present && pointer_state == 1)
        mouse_call(4, 0, column_mouse << 3, mousey_now << 3);
}

void far mouse_move_to(int row, int col)                          /* 22b7:2519 */
{
    text_xy(row, col, &row, &col);
    mouse_move_by(row - mousey_now, col - column_mouse);
}

void far mouse_fake(int button, char both)                     /* 22b7:254b */
{
    if (button == 2) {
        if (!rbtn_released)
            mouse_event(0x10);
        else
            mouse_event(8);
        if (both) {
            if (!rbtn_released)
                mouse_event(0x10);
            else
                mouse_event(0x18);
        }
    } else {
        if (!lifted_lbtn)
            mouse_event(4);
        else
            mouse_event(2);
        if (both) {
            if (!lifted_lbtn)
                mouse_event(4);
            else
                mouse_event(6);
        }
    }
}

char far mouse_up_which(void)                                      /* 22b7:25d0 */
{
    if (!lifted_lbtn)
        return 1;
    if (!rbtn_released)
        return 2;
    return 0;
}

int far mouse_pending(void)                                      /* 22b7:25e9 */
{
    return btn1_press || rbtn_press;
}

void far pointer_shape_set(struct Pointer far *shape)                     /* 22b7:25ff */
{
    struct Pointer far *src;
    int far *current_words;
    int far *new_words;
    register int remaining_words;

    if (!shape)
        src = &pointer;
    else
        src = shape;
    current_words = (int far *)&pointer_now;
    new_words = (int far *)src;
    remaining_words = 0x102;
    while (*current_words++ == *new_words++ && --remaining_words > 0)
        ;
    if (remaining_words) {
        mem_move(src, &pointer_now, sizeof(struct Pointer));
        pointer_hide();
        pointer_show();
    }
}
