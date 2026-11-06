/* DQK.EXE overlay 03 module 255d: editing a line of text on the prompt line.
 * line_edit prints an optional prompt on the current prompt line, then
 * reads keys into a buffer up to max characters with a cursor that blinks
 * on the timer tick: Enter or a line feed accepts a non-empty line, Esc or
 * ` abandons it, Backspace (or key 0x106) removes the last character, and
 * any other key from ' ' to '~' is appended; everything else beeps, and so
 * does a mouse click.  Text already in the buffer is offered as a default
 * when the byte after its NUL is 1: the first edit key replaces it.
 * num_edit reads a number between lo and hi through line_edit, as wide as
 * hi's digits and one wider when lo is negative, and asks again until one
 * is in range.
 *
 * EXE 0x036fc1..0x03742f, one Turbo C large-model _TEXT segment starting at
 * byte 1 of paragraph 255d in RTLink section 3 (payload 0x30ed0, link
 * segment 1f4e): byte 0 is poolfile.c's last retf, and the byte after the
 * range is the 00 that ends section 3's payload.  Built with Turbo C 2.01,
 * tcc -ml -O -Z -d.  Functions are defined in shipped order; bake consumes
 * them in that order.  Section 0's 1f4e link segment holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  A name another module
 * gives an address is reused from there; the rest are placeholders chosen
 * from use.  Both routines are reached through the vector stubs 1d99:0f77
 * and 1d99:0f81, from textwin.c.
 *
 * The census marker, 2e 3b 07 at 255d:0224, is Turbo C's sparse-switch
 * value search for line_edit's switch on the key.
 *
 * _DATA is five literals stored as four at 91ac..91b9, " " on the tail of
 * "%s ".
 * _BSS is empty: the line stack is prompt.c's.
 */

#include <stdlib.h>
#include "dqk.h"

char far line_edit(char far *prompt, char far *buf, int max, int col) /* 255d:0001 */
{
    long now;
    int row2;
    int mx;
    int col2;
    short key;
    short step;
    char far *end;
    char foreground;
    char background;
    char going;
    char ok;
    char fresh;
    register int cursor_offset;
    register int x;

    pointer_hide();
    fresh = *buf && buf[str_len(buf) + 1] == 1;  /* a default, replaced by the first edit key */
    end = buf;
    cursor_offset = 0;
    set_text_mode(1);
    foreground = win_attr[win_top] & 0xf;
    background = (win_attr[win_top] >> 4) & 0xf;
    step = (win_attr[win_top] & 0x200) ? 2 : 4;
    text_offset_xy(win_row[win_top], 0x1fe0 - win_col[win_top], 0x1f44, 0,
                 &row2, &col2);
    if (prompt) {
        fill_rect(win_row[win_top], win_col[win_top] + 0x1f40, row2, col2, background);
        text_print_at(win_row[win_top], win_col[win_top] + 0x1f40,
                      win_attr[win_top], "%s", prompt);
    }
    x = win_col[win_top] + step + 0x1f40;
    if (prompt)
        x += str_len(prompt) * step;
    else
        x += col;
    if (fresh) {
        text_print_at(win_row[win_top], x, win_attr[win_top], "%s ", buf);
        cursor_offset = -1;
    } else
        *end = 0;
    going = 1;
    while (going) {
        if (((now = timer_ticks()) & 0x1f) == 0)
            fill_rect(win_row[win_top], cursor_offset * step + x, row2, cursor_offset * step + x + step,
                      (now & 0x20) ? foreground : background);
        if (mouse_press(7, &mx, &col2))                          /* the mouse y reuses col2's slot, as shipped */
            beep();
        if (!key_poll())
            continue;
        key = key_get();
        switch (key) {
        case 0x1b:
        case 0x60:
            pointer_show();
            going = ok = 0;
            break;
        case '\n':
        case '\r':
            if (fresh) {
                cursor_offset = str_len(buf);
                fresh = 0;
            }
            if (cursor_offset == 0) {
                beep();
                break;
            }
            pointer_show();
            going = 0;
            ok = 1;
            break;
        case 8:
        case 0x106:
            if (fresh) {
                text_print_at(win_row[win_top], x - step, win_attr[win_top],
                              "%* ", str_len(buf) + 1);
                *end = 0;
                cursor_offset = 0;
                fresh = 0;
            }
            if (cursor_offset > 0) {
                text_print_at(win_row[win_top], cursor_offset * step + x, win_attr[win_top], " ");
                cursor_offset--;
                end--;
                *end = 0;
            }
            break;
        default:
            if (key < 0x20 || key > 0x7e || cursor_offset >= max) {
                beep();
                break;
            }
            if (fresh) {
                text_print_at(win_row[win_top], x - step, win_attr[win_top],
                              "%* ", str_len(buf) + 1);
                cursor_offset = 0;
                fresh = 0;
            }
            *end = key;
            end++;
            *end = 0;
            cursor_offset++;
        }
        text_print_at(win_row[win_top], x, win_attr[win_top], "%s ", buf);
    }
    return ok;
}

int far num_edit(int lo, int hi, char far *fmt, int col)         /* 255d:03ba */
{
    char text[80];
    char digits[8];
    register int numeric_value;
    register int width;

    width = lo < 0 ? 2 : 1;
    for (numeric_value = -abs(hi); numeric_value <= -10; numeric_value /= 10)
        width++;
    if (fmt)
        str_format(text, "%r", fmt, (void far *)&col);
    for (;;) {
        digits[0] = 0;
        if (!line_edit(fmt ? text : (char far *)0, digits, width, col))
            return lo - 1;
        numeric_value = str_to_num(digits);
        if (numeric_value >= lo && numeric_value <= hi)
            return numeric_value;
        beep();
    }
}
