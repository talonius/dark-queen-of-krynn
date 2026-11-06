/* DQK.EXE resident module 2cd9: listing a directory through DOS find first
 * and find next (int 21h 4Eh, 4Fh) into one disk transfer area, dta.
 * file_find_first cuts the name out of a path into pattern, at most eight
 * characters and an extension of up to four with its '.', appends a
 * wildcard extension to both when the name has none, and starts the
 * search; file_find_next returns each next name that wild_match matches
 * against pattern, name_dot adding a '.' to a file name without one (not
 * to a directory or volume label), before the match when the pattern has
 * an extension and after it when not.  wild_match takes '*' and '?' and
 * compares a pattern character upper-cased with the name's.  The entry's
 * attribute, date and time, and size are read back by file_find_attrib,
 * find_stamp and find_size.  name_kind scans a whole path: it gives 0 when
 * any byte is a control character, a space, past '~' or a ',', or when the
 * last component is empty or only dots; 2 when any byte is a wildcard;
 * else 1.
 *
 * EXE 0x01f19a..0x01f505, one Turbo C large-model _TEXT segment starting at
 * byte 10 of paragraph 2cd9.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * pathutil.c's last routine ends at 0x01f19a, and module 2d10 starts at
 * 0x01f505 (2d10:0005) with no stub between.  Ghidra's index misses
 * find_stamp (02f1) and find_size (02f9).  Nothing found calls find_stamp,
 * find_size or name_kind (0301): no far call, far jump, callback push or
 * far-pointer dword in DQK.EXE names them.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  diskfile.c's placeholders file_find_first, file_find_next and
 * file_find_attrib name this module's own routines; the callees reuse
 * pathutil.c's and diskfile.c's names.
 */

#include "dqk.h"

struct Dta {                        /* DOS find first/next record, 43 bytes */
    char reserved[21];
    char attrib;                    /* +15 */
    long stamp;                     /* +16, time in the low word, date high */
    long size;                      /* +1a */
    char name[13];                  /* +1e */
};

static char pending = 0;            /* a258, find first's entry not yet read */

static struct Dta dta;                                           /* db22 */
static char pattern[13];                                         /* db4d */
static char has_ext;                                             /* db5a */

void far name_dot(void);
int far wild_match(char far *name, char far *pat);

int far file_find_first(char far *path, int attr)                   /* 2cd9:000a */
{
    char far *pattern_cursor;
    char far *path_cursor;
    char far *mark;
    char buf[80];
    register int return_code;

    path_cursor = path - 1;
    while (*++path_cursor)
        ;
    while (--path_cursor >= path && *path_cursor != ':' && *path_cursor != '/' && *path_cursor != '\\')
        ;
    path_cursor++;
    pattern_cursor = pattern;
    for (;;) {
        if (*path_cursor == 0) {
            has_ext = 0;
            str_copy(buf, path);
            str_cat(buf, ".*");
            *pattern_cursor = '.';
            pattern_cursor++;
            *pattern_cursor = '*';
            pattern_cursor++;
            path = buf;
            break;
        }
        if (*path_cursor == '.') {
            has_ext = 1;
            mark = pattern_cursor;
            while (*path_cursor && mark + 4 > pattern_cursor) {
                *pattern_cursor = *path_cursor;
                path_cursor++;
                pattern_cursor++;
            }
            break;
        }
        if (pattern + 8 > pattern_cursor) {
            *pattern_cursor = *path_cursor;
            pattern_cursor++;
        }
        path_cursor++;
    }
    *pattern_cursor = 0;
    dos_open(0x1a00, (char far *)&dta);
    return_code = dos_open(0x4e00, path, 0, attr);
    pending = return_code >= 0;
    if (pending || return_code == -18)       /* DOS 12h, no more files */
        return 0;
    return return_code;
}

char far *far file_find_next(void)                                 /* 2cd9:0169 */
{
    do {
        if (!pending) {
            dos_open(0x1a00, (char far *)&dta);
            if (dos_call(0x4f00) < 0)
                return 0;
        }
        pending = 0;
        if (has_ext)
            name_dot();
    } while (!wild_match(dta.name, pattern));
    if (!has_ext)
        name_dot();
    return dta.name;
}

void far name_dot(void)                                          /* 2cd9:01ce */
{
    char far *extension_cursor;

    if (!(dta.attrib & 0x18)) {
        extension_cursor = str_find(dta.name, '.');
        if (!*extension_cursor) {
            *extension_cursor = '.';
            extension_cursor++;
            *extension_cursor = 0;
        }
    }
}

int far wild_match(char far *name, char far *pat)                /* 2cd9:020e */
{
    char far *name_cursor;

    while (*pat) {
        if (*pat == '*') {
            if (pat[1] == 0 || pat[1] == '.' && pat[2] == 0 ||
                pat[1] == '.' && pat[2] == '*' && pat[3] == 0)
                return 1;
            for (name_cursor = name; *name_cursor; name_cursor++)
                if (wild_match(name_cursor, pat + 1))
                    return 1;
            return 0;
        }
        if (*pat == '?') {
            if (*name == 0)
                return 0;
        } else if (*pat != *name && to_upper(*pat) != *name)
            return 0;
        pat++;
        name++;
    }
    return !*name;
}

int far file_find_attrib(void)                                       /* 2cd9:02ec */
{
    return dta.attrib;
}

long far find_stamp(void)                                        /* 2cd9:02f1 */
{
    return dta.stamp;
}

long far find_size(void)                                         /* 2cd9:02f9 */
{
    return dta.size;
}

int far name_kind(char far *name_cursor)                                   /* 2cd9:0301 */
{
    register int len;
    register int kind;

    len = 0;
    kind = 1;
    while (*name_cursor) {
        if (*name_cursor <= ' ' || *name_cursor > '~' || *name_cursor == ',')
            return 0;
        if (*name_cursor == '*' || *name_cursor == '?')
            kind = 2;
        if (*name_cursor == ':' || *name_cursor == '/' || *name_cursor == '\\')
            len = 0;
        else if (*name_cursor != '.')
            len++;
        name_cursor++;
    }
    return len ? kind : 0;
}
