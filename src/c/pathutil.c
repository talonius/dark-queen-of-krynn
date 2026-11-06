/* DQK.EXE resident module 2c8e: taking paths apart and putting them
 * together.  cur_dir writes the current drive letter, ':', a separator and
 * the current directory (int 21h 19h, 47h); change_dir changes drive and
 * directory (0Eh, 3Bh), returning -1 for a drive letter at or past the
 * drive count 0Eh reports; full_path makes an absolute path from one
 * relative to the current directory, dropping "./", climbing "../" by
 * hand and adding a '.' to a name with no extension; path_dir, path_name
 * and path_ext cut out the directory, the name without its extension and
 * the extension, and path_base_copy the name with it; path_base finds where
 * the name starts, path_has_dir says whether a '/', '\\' or ':' occurs,
 * and path_append_name and path_in_dir join a directory and the name's last
 * component.  A separator added is pathfind.c's path_sep, cur_dir and
 * full_path turn each '\\' DOS returns into it, change_dir sets it to '/'
 * when the path holds one, and path_base when the last separator is one.
 *
 * EXE 0x01ecec..0x01f19a, one Turbo C large-model _TEXT segment starting at
 * byte 12 of paragraph 2c8e.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * pathfind.c's last routine ends at 0x01ecec, and module 2cd9 starts at
 * 0x01f19a (2cd9:000a) with no stub between.  All 30 MZ relocation sites
 * inside it have frame 1c8e.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there,
 * sound.c's path_name and path_ext and the placeholders path_base_copy and
 * path_append_name for this module's own routines included; the rest are
 * placeholders chosen from use.  Nothing found calls cur_dir (000c),
 * change_dir (006e), full_path (0100), path_dir (0252) or path_in_dir
 * (0358): no far call, far jump, callback push or far-pointer dword in
 * DQK.EXE names any of them.
 *
 * The one _DATA byte is the empty extension full_path hands to
 * path_default_ext (a256); a257 is padding.  The module has no _BSS.
 */

#include "dqk.h"

extern void far path_default_ext(char far *path, char far *ext, int force); /* 1c60:025c */

char far *far path_base(char far *path);
char far path_has_dir(char far *path);
void far path_append_name(char far *dir, char far *name);

void far cur_dir(char far *buf)                                  /* 2c8e:000c */
{
    buf[0] = dos_call(0x1900) + 'A';
    buf[1] = ':';
    buf[2] = path_sep;
    buf[3] = 0;
    dos_call(0x4700, buf[0] - '@', buf + 3);
    while (*++buf)
        if (*buf == '\\')
            *buf = path_sep;
}

int far change_dir(char far *path)                               /* 2c8e:006e */
{
    char drive;

    if (*str_find(path, '/'))
        path_sep = '/';
    if (*path && path[1] == ':') {
        drive = to_upper(*path) - 'A';
        if ((char)dos_call(0x0e00, to_upper(*path) - 'A') <= drive) /* byte cmp */
            return -1;
        path += 2;
    }
    if (*path)
        return dos_open(0x3b00, path);
    return 0;
}

void far full_path(char far *path, char far *out)               /* 2c8e:0100 */
{
    if (*path && path[1] == ':') {
        *out = to_upper(*path);
        out++;
        path += 2;
    } else {
        *out = dos_call(0x1900) + 'A';
        out++;
    }
    *out = ':';
    out++;
    if (*path != '/' && *path != '\\') {
        out[0] = path_sep;
        out[1] = 0;
        dos_call(0x4700, out[-2] - '@', out + 1);
        for (; *out; out++)
            if (*out == '\\')
                *out = path_sep;
        if (out[-1] == '/' || out[-1] == '\\')
            out--;
        while (*path == '.') {
            if (path[1] == '.' && (path[2] == '/' || path[2] == '\\')) {
                do {
                    out--;
                    if (*out == ':') {
                        out++;
                        break;
                    }
                } while (*out != '/' && *out != '\\');
                path += 3;
            } else if (path[1] == '/' || path[1] == '\\')
                path += 2;
            else
                break;
        }
        *out = path_sep;
        out++;
    }
    str_copy(out, path);
    path_default_ext(out, "", 0);
}

void far path_dir(char far *path, char far *out)                 /* 2c8e:0252 */
{
    char far *end;

    str_copy(out, path);
    end = path_base(out);
    if (out + 2 <= end && out[1] == ':')
        out += 2;
    if (out + 1 < end)
        end--;
    *end = 0;
}

void far path_name(char far *path, char far *out)                /* 2c8e:02b4 */
{
    str_copy(out, path_base(path));
    *str_find(out, '.') = 0;
}

void far path_ext(char far *path, char far *out)                 /* 2c8e:02ed */
{
    char far *extension_cursor;

    extension_cursor = str_find(path_base(path), '.');
    if (*extension_cursor)
        extension_cursor++;
    str_copy(out, extension_cursor);
}

void far path_base_copy(char far *path, char far *out)             /* 2c8e:0337 */
{
    str_copy(out, path_base(path));
}

void far path_in_dir(char far *path, char far *dir, char force)  /* 2c8e:0358 */
{
    char buf[80];

    if (force || !path_has_dir(path)) {
        str_copy(buf, dir);
        path_append_name(buf, path);
        str_copy(path, buf);
    }
}

void far path_append_name(char far *dir, char far *name)             /* 2c8e:03b2 */
{
    char far *end;

    end = dir + str_len(dir);
    if (*dir && (dir[1] != ':' || dir[2]) && end[-1] != '/' && end[-1] != '\\') {
        *end = path_sep;
        end++;
    }
    str_copy(end, path_base(name));
}

char far *far path_base(char far *path)                          /* 2c8e:042e */
{
    char far *path_cursor;

    path_cursor = path + str_len(path);
    while (--path_cursor >= path && *path_cursor != ':' && *path_cursor != '/' && *path_cursor != '\\')
        ;
    if (*path_cursor == '/')              /* reads path[-1] when none is found, as shipped */
        path_sep = '/';
    return path_cursor + 1;
}

char far path_has_dir(char far *path)                            /* 2c8e:048c */
{
    while (*path) {
        if (*path == '/' || *path == '\\' || *path == ':')
            return 1;
        path++;
    }
    return 0;
}
