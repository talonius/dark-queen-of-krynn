/* DQK.EXE resident module 20b2: the screen frames.  Each layout clears a
 * text window and draws the frame's pieces with draw_glyph; the main view
 * adds the moons, read from game_state + 0x1fa, 0x1fb and 0x1fc, and the
 * compass letter for dir_names[geo.facing], whose first character alone picks
 * the glyph.  moon_display_update called with 0 redraws the moons; called with any
 * other value it marks a redraw pending, and with 2 it clears one.
 * frame_gen opens, draws and frees the "gen" picture.  frame_select loads
 * the "frame" library while none is loaded and, on an XVGA screen or with
 * the mouse busy, switches its variant.
 *
 * EXE 0x012f2d..0x01342c, one Turbo C large-model _TEXT segment starting at
 * byte 0xd of paragraph 20b2.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Module 2093's last routine ends at 0x012f2d, and 2102's sysutil.c starts
 * at 0x01342c with no stub between.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  A name another module
 * gives an address is reused from there, menu.c's moon_display_update for this
 * module's own routine included; this module's other names are chosen from
 * use.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined: frame_main far-calls moon_display_update, so that one carries a
 * prototype.
 */

#include "dqk.h"

void far moon_display_update(char redraw_mode);

static char moon_pending = 0;                                    /* 58ca */
static int frame_cur = -1;                                       /* 58cb */

static struct Res far *gen_res;                                  /* a892 */

void far frame_rule(int row)                                     /* 20b2:000d */
{
    draw_glyph(row * 4 + 0x1f40, 0x1f40, 1, 7);
}

void far frame_border(void)                                      /* 20b2:0030 */
{
    screen_draw_begin(0);
    text_clear(1, 1, 0x26, 0x16);
    draw_glyph(0x1f40, 0x1f40, 1, 1);
    draw_glyph(0x1f40, 0x1f40, 1, 2);
    draw_glyph(0x1f40, 0x1f40, 1, 3);
    draw_glyph(0x1f40, 0x1f40, 1, 4);
    frame_invalidate();
}

void far frame_character_sheet(void)                                        /* 20b2:00a4 */
{
    frame_border();
    frame_rule(8);
    frame_rule(0x10);
    frame_rule(0x14);
    draw_glyph(0x1f40, 0x1f40, 1, 0xb);
    pal_select(0);
}

void far frame_main(void)                                        /* 20b2:00e4 */
{
    frame_border();
    frame_rule(0x10);
    frame_rule(0xc);
    moon_display_update(2);
    draw_glyph(0x1f40, 0x1f40, 1, 0xc);
    draw_glyph(0x1f40, 0x1f40, 1, game_state[STATE_MOON_PHASE_1] + 0x1d);
    draw_glyph(0x1f40, 0x1f40, 1, game_state[STATE_MOON_PHASE_2] + 0x21);
    draw_glyph(0x1f40, 0x1f40, 1, game_state[STATE_MOON_PHASE_3] + 0x25);
    draw_glyph(0x1f40, 0x1f40, 1, 0x17);
    switch (dir_names[geo.facing][0]) {
    case 'E':
        draw_glyph(0x1f40, 0x1f40, 1, 0x1c);
        break;
    case 'N':
        draw_glyph(0x1f40, 0x1f40, 1, 0x19);
        break;
    case 'S':
        draw_glyph(0x1f40, 0x1f40, 1, 0x1a);
        break;
    case 'W':
        draw_glyph(0x1f40, 0x1f40, 1, 0x1b);
        break;
    }
    hero_frame_flag_set(1);
}

void far moon_display_update(char redraw_mode)                                    /* 20b2:022c */
{
    if (redraw_mode)
        moon_pending = redraw_mode != 2;
    else if (moon_pending) {
        moon_pending = 0;
        screen_draw_begin(1);
        draw_glyph(0x1f40, 0x1f40, 1, 0xc);
        draw_glyph(0x1f40, 0x1f40, 1, game_state[STATE_MOON_PHASE_1] + 0x1d);
        draw_glyph(0x1f40, 0x1f40, 1, game_state[STATE_MOON_PHASE_2] + 0x21);
        draw_glyph(0x1f40, 0x1f40, 1, game_state[STATE_MOON_PHASE_3] + 0x25);
    }
}

void far frame_lower_panel(void)                                        /* 20b2:02d3 */
{
    frame_border();
    frame_rule(0x10);
}

void far frame_lower_panel_palette(void)                                        /* 20b2:02e1 */
{
    frame_border();
    frame_rule(0x10);
    pal_select(0);
}

void far frame_right_panel(void)                                        /* 20b2:02fa */
{
    screen_draw_begin(0);
    text_clear(0x17, 1, 0x26, 0x15);
    draw_glyph(0x1f40, 0x1f40, 1, 1);
    draw_glyph(0x1f40, 0x1f40, 1, 2);
    draw_glyph(0x1f40, 0x1f40, 1, 3);
    draw_glyph(0x1f40, 0x1f40, 1, 5);
    clip_set(0x1f9d, 0x1f40, 0x1fa4, 0x1fe0);
    draw_glyph(0x1f40, 0x1f40, 1, 4);
    clip_reset();
    draw_glyph(0x1f40, 0x1f40, 1, 6);
    frame_invalidate();
}

void far frame_list_panel(void)                                        /* 20b2:03b5 */
{
    frame_border();
    frame_rule(2);
    pal_select(0);
}

void far frame_gen(void)                                         /* 20b2:03ce */
{
    set_open_mode('3');
    tile_open_file("gen", 1, 0, 0, &gen_res);
    screen_draw_begin(0);
    text_clear(1, 1, 0x26, 0xd);
    draw_glyph(0x1f40, 0x1f40, 1, 1);
    draw_glyph(0x1f40, 0x1f40, 1, 2);
    draw_glyph(0x1f40, 0x1f40, 1, 3);
    res_draw(0, 0, gen_res);
    pal_select(0);
    frame_invalidate();
    res_free(&gen_res);
}

void far frame_select(int frame_id)                                     /* 20b2:047c */
{
    register int frame_variant;

    if (frame_id != frame_cur || frame_id == 0) {
        if (frame_cur < 0)
            glyph_library_open('3', "frame", 1);
        switch (frame_id) {
        default:
            frame_variant = 0;
            break;
        case 3:
            frame_variant = 2;
            break;
        case 4:
            frame_variant = 1;
            break;
        case 5:
        case 6:
            frame_variant = 3;
        }
        if (is_xvga() || !pixel_shift())
            glyph_palette_load(slot_base(1), frame_variant + 0x29);
        frame_cur = frame_id;
    }
}
