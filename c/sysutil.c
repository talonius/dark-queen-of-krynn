/* DQK.EXE resident module 2102: start-up and shutdown, keyboard waits,
 * delays, the random number generator, cursor text output and the error box.
 *
 * EXE 0x01342c..0x0138b7, one Turbo C large-model _TEXT segment starting at
 * byte c of paragraph 2102.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  Callee names are placeholders chosen from call shapes; a modNNNN_
 * name says only which segment the callee lives in.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is: sys_init
 * far-calls random_seed and set_attr_word, sys_shutdown far-calls sys_done,
 * so those three carry prototypes.
 *
 * _BSS follows the statics' names (Turbo C's symbol-table order), not their
 * declaration order; these names reproduce the shipped ca0e..ca18 layout.
 *
 * text_printf hands text_format one handler per escape letter of "vka":
 * %v sets the cursor row, %k the column, %a the attribute.
 */

#include "dqk.h"

void far random_seed(void);
void far set_attr_word(int attribute);
void far sys_done(void);

static long seed;                                                /* ca0e */
static int cur_col;                                              /* ca12 */
static int cur_row;                                              /* ca14 */
static int text_attr;                                            /* ca16 */
static char text_attr_hi;                                        /* ca18 */

void far sys_init(register int argc, char far * far *argv, int need_kb,
                  int want_kb)                                   /* 2102:000c */
{
    display_init(argc, argv);
    sfx_slots_init(argc, argv);
    mouse_pointer_init(argc, argv);
    timer_rehook();
    lowio_init();
    random_seed();
    set_attr_word(15);
    file_cache_init(need_kb, want_kb);
    lib_init();
    glyph_library_init();
}

void far sys_shutdown(void)                                      /* 2102:0073 */
{
    glyph_library_done();
    lib_done();
    file_cache_done();
    lowio_done();
    sys_done();
    timer_unhook();
    mouse_pointer_done();
    sfx_slots_done();
    video_restore();
}

char far key_pressed(void)                                       /* 2102:00a1 */
{
    return kbd_waiting() || inp_pending() || kbd_state_5f();
}

void far key_flush(void)                                         /* 2102:00c4 */
{
    short unused_mouse_x;
    short unused_mouse_y;

    while (kbd_waiting())
        kbd_read();
    while (inp_press(7, &unused_mouse_x, &unused_mouse_y))
        ;
    kbd_state_5f();
}

void far key_wait(void)                                          /* 2102:00fc */
{
    key_flush();
    while (!key_pressed())
        ;
    key_flush();
}

void far wait_or_key(register int wait_units)                             /* 2102:010d */
{
    long start;

    wait_units *= 7;
    start = timer_ticks();
    while (!key_pressed() && (wait_units == 0 || timer_ticks() < start + wait_units))
        ;
}

void far wait_ticks(int wait_units)                                       /* 2102:0158 */
{
    long until;

    until = timer_ticks() + wait_units * 7;
    while (timer_ticks() < until)
        ;
    key_flush();
}

void far beep(void)                                              /* 2102:0193 */
{
    long start;

    start = timer_ticks();
    voice_set(2, 0xa9, 0x7f);
    voice_set(4, 0x712, 0x7f);
    while (timer_ticks() < start + 5)
        ;
    voice_set(2, 0, 0);
    voice_set(4, 0, 0);
    while (timer_ticks() < start + 6)
        ;
}

void far random_seed(void)                                       /* 2102:0226 */
{
    seed = clock_now();
}

void far sys_done(void)                                          /* 2102:0233 */
{
}

int far random_int(register int upper_bound)                               /* 2102:0234 */
{
    long span;

    if (upper_bound <= 1)
        return 0;
    span = upper_bound + 0x3fffffffL;
    span /= upper_bound;
    seed = seed * 0x6d25L + 1;
    return (seed & 0x3fffffffL) / span;
}

void far clear_screen(int attribute)                                     /* 2102:02a4 */
{
    fill_rect(0, 0, screen_rows(), screen_cols(), attribute);
}

void far set_attr(int foreground, int background)                                /* 2102:02c3 */
{
    text_attr = (background << 4) + foreground;
    text_attr_hi = 0;
}

void far set_attr_word(int attribute)                                    /* 2102:02da */
{
    text_attr_hi = (text_attr = attribute) >> 8;
}

void far text_goto(int row, int col)                             /* 2102:02ec */
{
    text_xy(row, col, &cur_row, &cur_col);
}

void far text_code_row(int arg, int value)                       /* 2102:0308 */
{
    text_xy(value, arg, &cur_row, &arg);
}

void far text_code_col(int arg, int value)                       /* 2102:0324 */
{
    text_xy(arg, value, &arg, &cur_col);
}

void far text_code_attr(int arg, int value)                      /* 2102:0340 */
{
    set_attr_word(value);
}

void far text_putc(char character_code)                                       /* 2102:034d */
{
    cur_col += draw_char(cur_row, cur_col, character_code, text_attr, text_attr_hi);
}

void far text_printf(char far *fmt, ...)                         /* 2102:0370 */
{
    text_format(fmt, (void far *)(&fmt + 1), text_putc, "vka",
                text_code_row, text_code_col, text_code_attr);
}

void far text_print_at(int row, int col, int attribute, char far *fmt, ...) /* 2102:03a0 */
{
    set_attr_word(attribute);
    text_goto(row, col);
    text_printf("%r", fmt, (void far *)(&fmt + 1));
}

void far error_box(char far *fmt, ...)                           /* 2102:03cf */
{
    char mode;

    mode = text_mode();
    set_text_mode(1);
    fill_rect(0x1fa0, 0x1f40, 0x1fa4, 0x1fe0, 15);
    set_attr_word(0xf0);
    text_goto(0x1fa0, 0x1f40);
    text_printf("Error: %r", fmt, (void far *)(&fmt + 1));
    console_beep();
    key_flush();
    while (!key_pressed())
        ;
    if (key_poll() && key_get() == 'q') {
        sys_shutdown();
        dos_call(0x4c01);               /* terminate, exit code 1 */
    }
    key_flush();
    set_attr_word(15);
    fill_rect(0x1fa0, 0x1f40, 0x1fa4, 0x1fe0, 0);
    set_text_mode(mode);
}
