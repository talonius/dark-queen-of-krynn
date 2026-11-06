/* DQK.EXE resident module 217e: opening files by disk letter, with the
 * insert-disk prompts, and walking a directory.  A disk letter names one of
 * the game disks ('S' the save disk); disk_path makes a file's path from
 * it, prefixing "A:\" or "B:\" after a failure and, with disk_subdirs set,
 * the "SAVE" or "DISKx" directory.  archive_open tries up to three times,
 * each failure moving that disk on to the next drive in disk_drive, which
 * holds five words though a letter indexes it up to 25.  file_open repeats
 * archive_open behind the "Insert ... and Press a Key" prompt ("'%s' not
 * found" for disk 0) until it succeeds or the player picks C (give up) or
 * Q (quit); ? shows the name.
 * disk_msg shows the prompts, through disk_handler when disk_setup set one.
 * dir_first and dir_next list a disk's directories and the files whose
 * extension matches dir_ext.
 *
 * EXE 0x013be5..0x014290, one Turbo C large-model _TEXT segment starting at
 * byte 5 of paragraph 217e.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * prompt.c's last routine ends at 0x013be5, and module 21e9 starts at
 * 0x014290 with no stub between.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there,
 * sound.c's archive_open and file_open for this module's own routines
 * included; the rest are placeholders chosen from use.
 */

#include "dqk.h"

static char disk_subdirs = 0;                                    /* 91e0 */
static char far *disk_arg = 0;          /* 91e1, no read of it found */
static char save_disk = 'S';                                     /* 91e5 */
static int disk_drive[5] = { 0 };       /* 91e6, indexed up to 25 */

static char msg_mode;                                            /* ca1c */
static char far *dir_ext;                                        /* ca1d */
static char dir_dirs;                                            /* ca21 */
static char dir_files;                                           /* ca22 */
static void (far *disk_handler)(int kind, int disk, char far *text); /* ca23 */

void far disk_msg(char kind, char disk, char far *name)          /* 217e:0005 */
{
    char buf[100];

    switch (kind) {
    case 0:
        if (disk_handler)
            disk_handler(kind, disk, "");
        else
            win_text_printf("");
        set_text_mode(msg_mode);
        break;
    case 1:
        if (disk == 'S')
            disk = save_disk;
        if (disk)
            str_format(buf, "Insert %s and Press a Key",
                       disk != ' ' ? (disk != 'S' ? "Disk %c" : "SAVE Disk")
                                   : "the Disk");
        else
            str_format(buf, "'%s' not found", name);
        break;
    case 2:
        str_format(buf, "Disk %s error", disk ? "write" : "read");
        break;
    }
    if (kind) {
        msg_mode = text_mode();
        set_text_mode(1);
        if (disk_handler)
            disk_handler(kind, disk, buf);
        else
            win_text_printf(buf, disk);
    }
}

void far disk_path(char far *out, char disk, int drive, char far *name) /* 217e:0132 */
{
    if (disk == 0) {
        str_copy(out, name);
        return;
    }
    *out = 0;
    if (drive) {
        *out = drive + '@';
        str_copy(out + 1, ":\\");
    }
    if (disk_subdirs) {
        if (disk == 'S')
            str_cat(out, "SAVE");
        else if (disk > ' ') {
            str_cat(out, "DISKA");
            out[str_len(out) - 1] = disk;
        }
    }
    path_append_name(out, name);
}

char far archive_open(char disk, char far *name, int flags,
                      char (far *filename)())   /* (fd, path) */      /* 217e:01e3 */
{
    short tries;
    short drive;
    char path[79];
    char quiet;
    register int drive_index;
    register int file_handle;

    if (disk & 0x80) {
        quiet = 1;
        disk &= 0x7f;
    } else if (disk >= 'a' && disk <= 'z') {
        quiet = 1;
        disk = to_upper(disk);
    } else
        quiet = 0;
    if (disk == 'S')
        drive_index = 4;
    else if (disk >= 'A' && disk <= 'Z')
        drive_index = disk - 'A';
    else if (disk >= '0' && disk <= '9')
        drive_index = disk - '0';
    else
        drive_index = 0;
    if (!memory_above_320k())
        sfx_reset();
    tries = 3;
    while (--tries >= 0) {
        drive = disk ? disk_drive[drive_index] : 0;
        disk_path(path, disk, drive, name);
        if (flags == 4 ||
            (file_handle = dos_open((char)(flags == 3 ? 0 : flags) | 0x3d00, path)) >= 0) {
            if (flags == 1) {
                dos_call(0x3e00, 0, file_handle);
                file_handle = dos_open(0x3c00, path, 0, 0);
            } else if (flags == 4)
                file_handle = dos_open(0x3c00, path, 0, 0);
            if (file_handle >= 0 && (filename == 0 || filename(file_handle, path))) {
                dos_call(0x3e00, 0, file_handle);
                if (flags == 3)
                    dos_open(0x4100, path);
                return 1;
            }
            if (file_handle >= 0)
                dos_call(0x3e00, 0, file_handle);
            disk_msg(2, flags != 0, 0L);
            beep();
            key_wait();
            disk_msg(0, 0, 0L);
        }
        if (disk == 0 || quiet)
            break;
        if (++disk_drive[drive_index] > 2)
            disk_drive[drive_index] = 0;
        else if (disk_drive[drive_index] == 2 && !(int_call(0x11) & 0xc0)) {
            disk_drive[drive_index] = 0;          /* int 0x11 reports one floppy */
            tries--;
        }
    }
    return 0;
}

void far file_open(char disk, char far *name, int flags,
                   char (far *filename)())   /* (fd, path) */         /* 217e:03f2 */
{
    register int key;

    while (!archive_open(disk, name, flags, filename)) {
        disk_msg(1, disk, name);
        beep();
        key_flush();
        while (!key_pressed())
            sfx_update();
        key = 0;
        if (key_poll())
            key = key_get();
        else
            key_flush();
        disk_msg(0, 0, 0L);
        if (key == 'Q') {
            sys_shutdown();
            dos_call(0x4c01);
        } else if (key == 'C')
            break;
        else if (key == '?')
            error_box("On '%s'", name);
    }
}

void far disk_setup(void (far *handler)(),                       /* (kind, disk, text) */
                    char subdirs, char far *arg, char save)      /* 217e:04a6 */
{
    disk_handler = handler;
    disk_subdirs = subdirs;
    disk_arg = arg;
    save_disk = save;
}

char far dir_first(char disk, char far *name, char far *ext,
                   char files, char dirs)                        /* 217e:04cd */
{
    char path[80];
    register int drive_index;
    register int tries;

    dir_ext = ext;
    dir_dirs = dirs;
    dir_files = files;
    if (disk == 'S')
        drive_index = 4;
    else if (disk >= 'A' && disk <= 'Z')
        drive_index = disk - 'A';
    else if (disk >= '0' && disk <= '9')
        drive_index = disk - '0';
    else
        drive_index = 0;
    tries = 3;
    while (--tries >= 0) {
        if (disk)
            disk_path(path, disk, disk_drive[drive_index], "*.*");
        else {
            str_copy(path, name);
            path_append_name(path, "*.*");
        }
        if (file_find_first(path, dirs ? 0x10 : 0) >= 0)
            return 1;
        if (disk == 0)
            break;
        if (++disk_drive[drive_index] > 2)
            disk_drive[drive_index] = 0;
        else if (disk_drive[drive_index] == 2 && !(int_call(0x11) & 0xc0)) {
            disk_drive[drive_index] = 0;
            tries--;
        }
    }
    return 0;
}

char far * far dir_next(char far *is_dir)                        /* 217e:05ef */
{
    char far *entry;
    char ext[80];
    register int dir;

    while ((entry = file_find_next()) != 0) {
        dir = file_find_attrib() & 0x10;
        if (dir && !dir_dirs)
            continue;
        if (!dir && !dir_files)
            continue;
        if (dir) {
            if (entry[0] == '.' &&
                (entry[1] == 0 || entry[1] == '.' && entry[2] == 0))
                continue;
        } else if (dir_ext) {
            path_ext(entry, ext);
            if (!str_same(dir_ext, ext))
                continue;
        }
        if (dir)
            *is_dir = 1;
        else
            *is_dir = 0;
        return entry;
    }
    return 0;
}
