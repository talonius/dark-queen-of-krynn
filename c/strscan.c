/* DQK.EXE resident module 2d10: scanning a string against a character set,
 * and a case-blind prefix test.  str_break steps over the characters of s
 * that are not in set and str_skip over those that are, each returning where
 * it stopped (the NUL when it runs off the end); set membership is
 * str_find's, whose result points at set's NUL when the character is not
 * there.  str_match says whether the string at begins with s, comparing
 * each pair lowered by to_lower.
 *
 * EXE 0x01f505..0x01f5b4, one Turbo C large-model _TEXT segment starting at
 * byte 5 of paragraph 2d10.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * dirfind.c's last routine ends at 0x01f505, and module 2d1b starts at
 * 0x01f5b4 (2d1b:0004) with no stub between.  The module names no DGROUP
 * address, so it has no _DATA and no _BSS.  Nothing found calls str_break
 * (0005) or str_skip (0039): no far call, far jump, callback push or
 * far-pointer dword in DQK.EXE names them.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  str_match returns a char, set in al alone; strutil.c declares it
 * int, and both shipped callers (0x010454, 0x036151) test all of ax.
 */

#include "dqk.h"

char far *far str_break(char far *text_cursor, char far *set)              /* 2d10:0005 */
{
    while (*text_cursor && !*str_find(set, *text_cursor))
        text_cursor++;
    return text_cursor;
}

char far *far str_skip(char far *text_cursor, char far *set)               /* 2d10:0039 */
{
    while (*text_cursor && *str_find(set, *text_cursor))
        text_cursor++;
    return text_cursor;
}

char far str_match(char far *prefix_cursor, char far *text_cursor)                    /* 2d10:006d */
{
    for (; *prefix_cursor; prefix_cursor++, text_cursor++)
        if (!*text_cursor || to_lower(*prefix_cursor) != to_lower(*text_cursor))
            return 0;
    return 1;
}
