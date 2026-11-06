/* DQK.EXE resident module 214b: the prompt lines.  A stack of up to five
 * lines (win_top indexes it) keeps each line's row, column and attribute;
 * win_vprint clears the current line from its column to its background and
 * prints the formatted text centred on the full line width.  win_prompt
 * waits for a key, beeping when prompt_beep is set.  win_yes_no beeps, then
 * answers y, or n, Esc or ` as no, and beeps again on any other key; a
 * click ends it, yes inside the line and no outside.  Both draw an icon
 * through pointer_glyph_load in two frames.
 *
 * EXE 0x0138b7..0x013be5, one Turbo C large-model _TEXT segment starting at
 * byte 7 of paragraph 214b.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * sysutil.c's last routine ends at 0x0138b7, and module 217e starts at
 * 0x013be5 with no stub between.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there,
 * textwin.c's mod114b_ names for this module's own routines included; the
 * rest are placeholders chosen from use.
 *
 * The line stack, prompt_icon and prompt_frame are public: code outside
 * this module reads them (0x01e518 prompt_icon and prompt_frame, 0x03700c
 * the line stack).
 */

#define DQK_DEFINES_PROMPT
#include "dqk.h"

int win_top = 0;                                                 /* 91ba */
int prompt_icon = -1;                                            /* 91bc */
int win_row[5] = { 0x1fa0 };                                     /* 91be */
int win_col[5] = { 0 };                                          /* 91c8 */
int win_attr[5] = { 15 };                                        /* 91d2 */
static char prompt_beep = 1;                                     /* 91dc */

int prompt_frame;                                                /* ca1a */

void far prompt_icon_set(int icon, int frame)                    /* 214b:0007 */
{
    prompt_icon = icon;
    prompt_frame = frame;
}

void far prompt_window_push(int row, int col)                          /* 214b:0018 */
{
    if (win_top < 4)
        win_top++;
    win_row[win_top] = row;
    win_col[win_top] = col * 4;
    win_attr[win_top] = 15;
}

void far prompt_attr_set(int attr)                                  /* 214b:0052 */
{
    win_attr[win_top] = attr;
}

void far prompt_beep_set(char enabled)                                   /* 214b:0064 */
{
    prompt_beep = enabled;
}

void far prompt_window_pop(void)                                      /* 214b:006f */
{
    if (win_top > 0)
        win_top--;
}

void far win_vprint(char far *fmt, void far *args)               /* 214b:007b */
{
    int row2;
    int col2;
    char buf[80];

    text_offset_xy(win_row[win_top], 0x1fe0 - win_col[win_top], 0x1f44, 0,
                 &row2, &col2);
    fill_rect(win_row[win_top], win_col[win_top] + 0x1f40, row2, col2,
              (win_attr[win_top] >> 4) & 0xf);
    str_format(buf, "%r", fmt, args);
    buf[0x28 - (win_col[win_top] >> 1)] = 0;
    text_print_at(win_row[win_top], 0x1f90 - str_len(buf) * 2,
                  win_attr[win_top], buf);
}

void far win_printf(char far *fmt, ...)                          /* 214b:015d */
{
    win_vprint(fmt, (void far *)(&fmt + 1));
}

void far win_text_printf(char far *fmt, ...)                     /* 214b:0173 */
{
    set_text_mode(1);
    win_vprint(fmt, (void far *)(&fmt + 1));
}

void far win_prompt(char far *fmt, ...)                          /* 214b:0192 */
{
    set_text_mode(1);
    win_vprint(fmt, (void far *)(&fmt + 1));
    if (prompt_beep)
        beep();
    if (prompt_icon >= 0)
        pointer_glyph_load(prompt_icon, prompt_frame + 1);
    key_wait();
    if (prompt_icon >= 0)
        pointer_glyph_load(prompt_icon, prompt_frame);
}

char far win_yes_no(char far *fmt, ...)                          /* 214b:01ef */
{
    short key;
    int x;
    int y;
    char click;
    char inside;

    set_text_mode(1);
    if (fmt)
        win_vprint(fmt, (void far *)(&fmt + 1));
    beep();
    key_flush();
    for (;;) {
        if (key_poll()) {
            key = key_get();
            key = to_lower(key);
            if (key == 0x1b || key == 0x60 || key == 'n') {
                key = 0;
                break;
            }
            if (key == 'y') {
                key = 1;
                break;
            }
            beep();
        }
        click = mouse_press(7, &x, &y);
        text_xy_to(win_row[win_top], win_col[win_top] + 0x1f40, x, y, &x, &y);
        inside = x >= 0 && x < 4 && y >= 0 && 0xa0 - win_col[win_top] > y;
        if (click) {
            key = inside;
            break;
        }
        if (prompt_icon >= 0)
            pointer_glyph_load(prompt_icon, (inside ? 0 : 1) + prompt_frame);
    }
    if (prompt_icon >= 0)
        pointer_glyph_load(prompt_icon, prompt_frame);
    return key;
}
