/* DQK.EXE overlay 03 module 1fc4: .glb data files and the mode switch.
 * Cleaning a name of the characters a DOS file name cannot hold, reading
 * one entry of a numbered .glb library into a caller's buffer through
 * file_open, choosing the disk those reads ask for, and switching the
 * mode, which first calls what the mode being left names.
 *
 * EXE 0x031635..0x031886, one Turbo C large-model _TEXT segment starting at
 * byte 5 of paragraph 1fc4 in RTLink section 3 (payload 0x30ed0, link
 * segment 1f4e).  The first 5 bytes of paragraph 1fc4 are the tail of
 * module 1f4e, and the last 6 bytes of the range lie in paragraph 1fe9.
 * Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined in
 * shipped order; bake consumes them in that order.  Section 0's 1f4e link
 * segment holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.
 *
 * _DATA is bad_chars, data_disk, cur_mode and two literals, DGROUP
 * 5e9a..5ec9.  _BSS is acb0..acbd, in the order its names give (Turbo C's
 * symbol-table order), chosen with goldbox.bss_order; the originals are
 * not recoverable from it.
 */

#include "dqk.h"

static char bad_chars[17] = " =+<>\"[].*,?/\\:;|";              /* 5e9a */
static char data_disk = '1';                                     /* 5eab */
static int cur_mode = -1;                                        /* 5eac */

static int data_len;                                             /* acb0 */
static char far *data_path;                                      /* acb2 */
static char far *data_ptr;                                       /* acb6 */
static int data_bank;                                            /* acba */
static int data_entry;                                           /* acbc */

void far str_strip(char far *source_cursor, char removed_char)                          /* 1fc4:0005 */
{
    char far *destination_cursor;

    destination_cursor = source_cursor;
    while (*source_cursor != 0) {
        if (*source_cursor != removed_char)
            *destination_cursor++ = *source_cursor;
        source_cursor++;
    }
    *destination_cursor = 0;
}

void far name_clean(char far *name)                              /* 1fc4:0044 */
{
    unsigned char index;

    for (index = 0; index < 17; index++)
        str_strip(name, bad_chars[index]);
    name[8] = 0;
    str_upr(name);
}

void far set_data_disk(char disk)                                /* 1fc4:0088 */
{
    data_disk = disk;
}

char far data_read(int file_handle, char far *path)                       /* 1fc4:0093 */
{
    int item_index;
    long size;

    if ((item_index = lib_find(file_handle, data_entry)) < 0
        || item_index == 0 && (item_index = lib_find(file_handle, data_bank * 100 + data_entry)) < 0)
        return 0;
    if (item_index == 0) {
        data_len = 0;
        return 1;
    }
    if ((size = lib_find_item(file_handle, item_index)) < 0
        || dos_io(0x3f00, data_ptr, file_handle, (int)size) != size)
        return 0;
    data_len = size;
    return 1;
}

void far data_load(char far *name, long unused, unsigned char entry,
                   int far *len, char far *buf)                  /* 1fc4:0130 */
{
    int last;
    int bank;
    char path[80];
    char tmp[80];

    last = str_len(name) - 1;
    if (name[last] >= '0' && name[last] <= '9') {
        bank = name[last] - '0';
        str_copy(tmp, name);
        tmp[last] = 0;
        str_format(path, "%s.glb", tmp);
        data_entry = entry;
        data_bank = bank;
        data_ptr = buf;
        data_path = path;
        file_open(data_disk, path, 0, data_read);
        *len = data_len;
    } else
        error_box("Loading '%s' as data", name);
}

/* DKK DaxIo.PushState(v): saved the disk number; DQK runs from one disk. */
void far data_disk_push(int unused)                             /* 1fc4:01fa */
{
}

/* DKK DaxIo.PopState: restored it; DQK runs from one disk. */
void far data_disk_pop(void)                                   /* 1fc4:01ff */
{
}

void far mode_change(int mode)                                   /* 1fc4:0200 */
{
    if (mode != cur_mode) {
        switch (cur_mode) {
        case 0:
            pic_release();
            wall_sets_free(0);
            back_free();
            break;
        case 2:
            pic_release();
            break;
        case 3:
            bigpic_release();
            break;
        }
    }
    cur_mode = mode;
    if (mode == 4)
        wall_sets_free(1);
}
