/* DQK.EXE resident module 2c60: finding and naming files.  path_open opens
 * a file by name: first as given, then under each directory of the
 * environment's path (read through env_get) and of an optional
 * ';'-separated list, and returns the handle or the first error.  A name
 * starting with '.', '/' or '\\' is tried only as given.  With try_exec
 * set, a name without an extension is tried with the extension exe and
 * then com instead.  file_size returns an open file's length
 * and leaves its position where it was; file_attr returns a file's DOS
 * attributes (int 21h ax=4300h hands them back in cx) or the negative
 * error; path_default_ext appends an extension to a name that has none,
 * replaces one only when force is set, and sets path_sep to '/' when the
 * scan back from the end meets a '/' first.
 *
 * EXE 0x01ea00..0x01ecec, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 2c60.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Module 2c52's last routine ends at 0x01ea00, and module 2c8e starts at
 * 0x01ecec (2c8e:000c) with no stub between.  All 14 MZ relocation sites
 * inside it have frame 1c60.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the
 * rest are placeholders chosen from use.  Nothing found calls path_open
 * (0000) or file_attr (0233): no far call, far jump, callback push or
 * far-pointer dword anywhere in DQK.EXE names either.
 *
 * path_sep (a248) is public: module 2c8e reads it, and its own _DATA
 * starts at a256.  try_exec (db20) is read here and written by no direct
 * operand found in the image; db21 is padding, and db22 is read only by
 * module 2cd9 (0x01f2ab, 0x01f302); module 2c8e has no _BSS.
 */

#define DQK_DEFINES_PATHFIND
#include "dqk.h"

int far path_default_ext();     /* unprototyped: a char force would load with mov al */

char path_sep = '\\';                                            /* a248 */

static char try_exec;                                            /* db20 */

int far path_open(char far *name, char mode, char far *dirs,
                  char far *found)                               /* 2c60:0000 */
{
    char path[80];
    char list[256];
    char far *list_cursor;
    char far *path_cursor;
    register int file_handle;
    register int err;

    list[0] = 0;
    if (*name == '.' || *name == '/' || *name == '\\') {
        list[1] = 0;
    } else {
        env_get("path", list + 1);
        for (list_cursor = list; *++list_cursor; )
            if (*list_cursor == ';')
                *list_cursor = 0;
        if (dirs) {
            if (list_cursor[-1]) {
                *list_cursor = 0;
                list_cursor++;
            }
            str_copy(list_cursor, dirs);
            for (; *++list_cursor; )
                if (*list_cursor == ';')
                    *list_cursor = 0;
        }
        list_cursor[1] = 0;
    }
    list_cursor = list;
    err = 0;
    do {
        path_cursor = path;
        if (*list_cursor) {
            while ((*path_cursor = *list_cursor++) != 0)
                path_cursor++;
            if (path_cursor[-1] != '/' && path_cursor[-1] != '\\') {
                *path_cursor = path_sep;
                path_cursor++;
            }
        } else
            list_cursor++;
        str_copy(path_cursor, name);
        if (try_exec && path_default_ext(path, "exe", 0)) {
            if ((file_handle = dos_open(mode | 0x3d00, path)) >= 0)
                break;
            path_default_ext(path, "com", 1);
        }
        if ((file_handle = dos_open(mode | 0x3d00, path)) >= 0)
            break;
        if (err == 0)
            err = file_handle;
        else
            file_handle = err;
    } while (*list_cursor);
    if (file_handle >= 0 && found)
        str_copy(found, path);
    return file_handle;
}

long far file_size(int file_handle)                                    /* 2c60:01de */
{
    long pos;
    long end;

    pos = file_seek(file_handle, 0L, 1);
    end = file_seek(file_handle, 0L, 2);
    file_seek(file_handle, pos, 0);
    return end;
}

int far file_attr(char far *path)                                /* 2c60:0233 */
{
    register int return_code;
    register int attr;

    return_code = dos_open(0x4300, path);
    attr = _CX;
    if (return_code < 0)
        return return_code;
    return attr;
}

int far path_default_ext(char far *path, char far *ext, char force) /* 2c60:025c */
{
    char far *path_cursor;
    char far *dot;

    path_cursor = dot = path + str_len(path);
    while (--path_cursor > path) {
        if (*path_cursor == '.') {
            if (!force)
                return 0;
            dot = path_cursor;
            break;
        } else if (*path_cursor == '/') {
            path_sep = '/';
            break;
        } else if (*path_cursor == '\\')
            break;
    }
    *dot = '.';
    dot++;
    str_copy(dot, ext);
    return 1;
}
