/* DQK.EXE resident module 2d1b: formatted output through text_format, to
 * the console and into a string.  con_printf hands each character to the
 * runtime's 00d7:247d, which writes it through DOS (int 21h function 2, a
 * '\n' as CR LF); str_format points str_out at buf,
 * lets str_putc store each character there, ends the string with a NUL and
 * returns its length.  Neither passes text_format a code table.
 *
 * EXE 0x01f5b4..0x01f627, one Turbo C large-model _TEXT segment starting at
 * byte 4 of paragraph 2d1b.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * strscan.c's last routine ends at 0x01f5b4, and module 2d22, text_format's
 * own, starts at 0x01f627 (2d22:0007) with no stub between.  The one static,
 * str_out, is the module's _BSS at DGROUP db5c; no other code names it.
 * Nothing far-calls str_putc (0027): str_format passes it as a callback.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call or callback push.
 */

#include "dqk.h"

static char far *str_out;                                        /* db5c */

void far con_printf(char far *fmt, ...)                          /* 2d1b:0004 */
{
    text_format(fmt, (void far *)(&fmt + 1), con_putc, 0L);
}

void far str_putc(char character_code)                                        /* 2d1b:0027 */
{
    *str_out++ = character_code;
}

int far str_format(char far *buf, char far *fmt, ...)            /* 2d1b:003a */
{
    str_out = buf;
    text_format(fmt, (void far *)(&fmt + 1), str_putc, 0L);
    *str_out = 0;
    return str_out - buf;
}
