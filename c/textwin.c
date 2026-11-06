/* DQK.EXE resident module 1a62: text in windows.  The status line, framed
 * on row 0x18 or, in view mode 5, filled on row 0x17; text word-wrapped
 * into a window that waits for a key when it fills; the framed prompts that
 * read a line of text or a number; the delays scaled by the setting at
 * game_state + 0x12; and padding a string on the left.
 *
 * EXE 0x00ca24..0x00d459, one Turbo C large-model _TEXT segment starting at
 * byte 4 of paragraph 1a62.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  A name another module
 * defines is reused from there; the rest are placeholders chosen from use,
 * and a modNNNN_ name says only which segment the callee lives in.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is: text_wrap
 * far-calls text_clear and status_msg far-calls msg_pause, so those carry
 * prototypes.  text_wrap calls wait_continue near ahead of its definition,
 * which only a static function allows.
 *
 * A coordinate from 0x1f40 up is a text cell: 0x1f40 + 4 * n is column or
 * row n.  Some calls pass 1 or 2 above that form; what the low bits select
 * is not established.
 */

#include "dqk.h"

extern char far line_edit(char far *prompt, char far *buf, int max);
                                 /* 1d99:0f77, vector to overlay 3 255d:0001 */
extern int  far num_edit(int lo, int hi, char far *prompt);
                                 /* 1d99:0f81, vector to overlay 3 255d:03ba */

void far text_clear(char x1, char y1, char x2, char y2);
void far msg_pause(void);
static void wait_continue(void);

static unsigned pause_ms[10] = {                                 /* 1cd8 */
    10, 100, 250, 500, 1000, 1500, 2000, 3000, 4000, 5000
};

static char input_ok;                                            /* a62a */
static char input_buf[41];      /* a62b  up to 1b05's a654; needs >= 0x24 */

void far text_fill(int row1, int col1, int row2, int col2, int attr) /* 1a62:0004 */
{
    fill_rect(row1, col1, row2, col2, attr);
}

char far input_result(void)                                      /* 1a62:001f */
{
    return input_ok;
}

void far input_done(void)                                        /* 1a62:0023 */
{
    prompt_window_push(0x1f9e, 1);
    prompt_attr_set(0x8f);
    prompt_beep_set(1);
}

void far status_printf(unsigned char col, unsigned char row, unsigned char foreground,
                       unsigned char background, char far *fmt, ...)     /* 1a62:0046 */
{
    short at_row;
    short top;
    short len;
    short left;
    short right;
    short bottom;
    short junk;
    char buf[82];

    str_format(buf, "%r", fmt, (void far *)(&fmt + 1));
    screen_flush();
    if (background == 0) {
        if (row >= 0x17) {
            background = 8;
            if (foreground == 8)
                foreground = 15;
            else if (foreground == 7)
                foreground = 15;
        } else
            background = 8;
    }
    if (row == 0x18) {
        text_xy(0x1f9e, 0x1f40, &at_row, &top);
        at_row++;
        col++;
        text_xy(at_row, 0, &top, &len);
        text_xy(0x1fa2, col * 4 + 0x1f40, &bottom, &left);
        bottom++;
        len = str_len(buf);
        text_xy(0, (col + len) * 4 + 0x1f40, &junk, &right);
        fill_rect(top - 2, left - 1, top - 1, right + 1, 0);
        text_fill(top - 1, left, top, right, background);
        fill_rect(top - 1, left - 1, bottom, left, 0);
        fill_rect(top - 1, right, bottom, right + 1, 0);
        fill_rect(bottom, left - 1, bottom + 1, right + 1, 0);
    } else if (view_mode == 5 && row == 0x17) {
        at_row = 0x1f99;
        col++;
        text_xy(at_row, col * 4 + 0x1f40, &bottom, &left);
        len = str_len(buf);
        text_fill(bottom - 1, col * 4 + 0x1f40, bottom, (col + len) * 4 + 0x1f40, background);
    } else
        at_row = row * 4 + 0x1f40;
    text_print_at(at_row, col * 4 + 0x1f40, (background << 4) + foreground, "%s", buf);
}

void far text_repeat(unsigned char col, unsigned char row, unsigned char foreground,
                     unsigned char background, unsigned char repeat_count, unsigned char character_code,
                     char unused)                                /* 1a62:0283 */
{
    register int index;
    register int y;
    short x;

    y = row * 4 + 0x1f40;
    x = col * 4 + 0x1f40;
    if (background == 0)
        background = 8;
    for (index = 0; index < repeat_count; index++)
        text_print_at(y, index * 4 + x, (background << 4) + foreground, "%c", character_code);
}

char far is_break(char character_code)                                        /* 1a62:02f6 */
{
    return *str_find("-.,?!:;", character_code);
}

void far text_wrap(unsigned char x1, unsigned char y1, unsigned char x2,
                   unsigned char y2, unsigned char foreground, unsigned char background,
                   char clear, char far *text)                   /* 1a62:0311 */
{
    register int end;
    register int word_start;
    short last;

    screen_flush();
    if (background == 0)
        background = 8;
    if (x1 > 0x27 || y1 > 0x18 || x2 > 0x27 || y2 > 0x27) /* y2: 0x27, shipped */
        return;
    if (text_col < x1 || text_col > x2 || text_row < y1 || text_row > y2) {
        text_col = x1;
        text_row = y1;
    }
    if (clear) {
        text_clear(x1, y1, x2, y2);
        text_col = x1;
        text_row = y1;
    }
    word_start = 0;
    last = str_length(text) - 1;
    if (last < 0)
        return;
    do {
        for (end = word_start; end < last && is_break(text[end]); end++)
            ;
        for (; end < last && !is_break(text[end]) && text[end] != ' '; end++)
            ;
        if (text[end] != ' ')
            for (; end + 1 < last && is_break(text[end + 1]); end++)
                ;
        if (end - word_start + text_col > x2) {
            if (end - word_start + text_col == x2 + 1 && text[end] == ' ') {
                end--;
                for (; word_start <= end; word_start++, text_col++) {
                    text_repeat(text_col, text_row, foreground, background, 1, text[word_start], 1);
                    if (text_slow)
                        delay(game_state[STATE_SPEED] * 3);
                }
            }
            text_col = x1;
            text_row++;
            for (; word_start <= last && text[word_start] == ' '; word_start++)
                ;
            if (text_row > y2 && word_start < last) {
                text_col = x1;
                text_row = y1;
                box_draw();
                wait_continue();
                text_clear(x1, y1, x2, y2);
                for (; word_start <= end; word_start++, text_col++) {
                    text_repeat(text_col, text_row, foreground, background, 1, text[word_start], 1);
                    if (text_slow)
                        delay(game_state[STATE_SPEED] * 3);
                }
            }
        } else
            for (; word_start <= end; word_start++, text_col++) {
                text_repeat(text_col, text_row, foreground, background, 1, text[word_start], 1);
                if (text_slow)
                    delay(game_state[STATE_SPEED] * 3);
            }
    } while (word_start <= last);
    if (text_col > x2) {
        text_col = x1;
        text_row++;
    }
}

void far status_char(char col, char row, char attr, unsigned char character_code) /* 1a62:05a7 */
{
    status_printf(col, row, attr, attr, "%( %)", character_code);
}

char far *far input_text(char far *prompt, unsigned char foreground, unsigned char background,
                         unsigned char max)                      /* 1a62:05c9 */
{
    short at_row;
    short top;
    short len;
    short left;
    short right;
    short bottom;
    short junk;
    short col;

    if (background == 0)
        background = 8;
    if (foreground == background)
        foreground = 0;
    max = int_min(0x23, max);
    input_buf[0] = input_buf[1] = 0;
    text_xy(0x1f9e, 0x1f40, &at_row, &col);
    at_row++;
    col = 0;
    col++;
    text_xy(at_row, 0, &top, &len);
    text_xy(0x1fa2, col * 4 + 0x1f40, &bottom, &left);
    bottom++;
    len = 0x26;
    text_xy(0, (col + len) * 4 + 0x1f40, &junk, &right);
    fill_rect(top - 2, left - 1, top - 1, right + 1, 0);
    text_fill(top - 1, left, top, right, background);
    fill_rect(top - 1, left - 1, bottom, left, 0);
    fill_rect(top - 1, right, bottom, right + 1, 0);
    fill_rect(bottom, left - 1, bottom + 1, right + 1, 0);
    prompt_window_push(at_row, 1);
    prompt_attr_set((background << 4) + foreground);
    if (line_edit(prompt, input_buf, max))
        input_ok = 1;
    else
        input_ok = 0;
    prompt_window_pop();
    str_upr(input_buf);
    return input_buf;
}

int far input_number(char far *prompt, unsigned char foreground, unsigned char background) /* 1a62:0781 */
{
    short at_row;
    short top;
    short len;
    short left;
    short right;
    short bottom;
    short junk;
    short col;
    register int entered_number;

    if (background == 0)
        background = 8;
    if (foreground == background)
        foreground = 0;
    text_xy(0x1f9e, 0x1f40, &at_row, &col);
    at_row++;
    col = 0;
    col++;
    text_xy(at_row, 0, &top, &len);
    text_xy(0x1fa2, col * 4 + 0x1f40, &bottom, &left);
    bottom++;
    len = 0x26;
    text_xy(0, (col + len) * 4 + 0x1f40, &junk, &right);
    fill_rect(top - 2, left - 1, top - 1, right + 1, 0);
    text_fill(top - 1, left, top, right, background);
    fill_rect(top - 1, left - 1, bottom, left, 0);
    fill_rect(top - 1, right, bottom, right + 1, 0);
    fill_rect(bottom, left - 1, bottom + 1, right + 1, 0);
    prompt_window_push(at_row, 1);
    prompt_attr_set((background << 4) + foreground);
    while ((entered_number = num_edit(0, 0x7fff, prompt)) < 0)
        ;
    prompt_window_pop();
    return entered_number;
}

void far status_msg(char far *fmt, short foreground, short background, ...)      /* 1a62:0904 */
{
    box_draw();
    status_printf(0, 0x18, foreground, background, "%r", fmt, (void far *)(&background + 1));
    screen_flush();
    msg_pause();
    box_draw();
}

void far continue_prompt(char unused_arg_a, char unused_arg_b, char far *items)        /* 1a62:093f */
{
    press_return_clear();
}

void far text_pause(void)                                        /* 1a62:0949 */
{
    delay(game_state[STATE_SPEED] * 100);
}

void far msg_pause(void)                                         /* 1a62:0960 */
{
    delay(pause_ms[game_state[STATE_SPEED]]);
}

long far ticks_now(void)                                         /* 1a62:0979 */
{
    return timer_ticks() * 100 / 72;
}

void far clear_black(void)                                       /* 1a62:0997 */
{
    clear_screen(0);
}

void far text_clear(char x1, char y1, char x2, char y2)          /* 1a62:09a1 */
{
    text_fill(y1 * 4 + 0x1f40, x1 * 4 + 0x1f40, y2 * 4 + 0x1f44, x2 * 4 + 0x1f44, 8);
}

static void wait_continue(void)                                  /* 1a62:09e0 */
{
    input_pending_clear();
    continue_prompt(0, 0xb, continue_items);
}

void far str_pad_left(char far *text, unsigned char width)          /* 1a62:09fb */
{
    while (str_length(text) < width)
        str_copy(text, msg_format(" %s", text));
}
