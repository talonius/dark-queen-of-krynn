/* DQK.EXE resident module 2a3d: editing a resource library in its slot.
 * Starting an empty library, inserting, deleting and resizing one item in
 * place -- the header's count and size and every later index entry move
 * with it, and the file cache moves the bytes -- and reading an item from
 * an open file into the slot and converting it through the converter
 * registered for its signature.
 *
 * EXE 0x01c7de..0x01ce6e, one Turbo C large-model _TEXT segment starting at
 * byte e of paragraph 2a3d.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  Callee names are placeholders chosen from use; a modNNNN_ name says
 * only which segment the callee lives in.  This module's own functions keep
 * the mod1a3d_ names tiles.c gave them; lib_delete (020d) has no caller
 * found.
 *
 * sig_tags, sig_count and sig_convert are library.c's (module 293f), which
 * registers the converters.  This module has no _BSS of its own.
 *
 * 0x42494c48L is the library tag "HLIB" as four bytes.
 */

#include "dqk.h"

extern void far huge_move(void far *src, void far *dst, unsigned n); /* 00d7:08c2 */

void far lib_create(char far *name, int slot, long tag)        /* 2a3d:000e */
{
    struct LibHdr hdr;
    long end;

    if (!slot_open(name, slot)) {
        hdr.tag = 0x42494c48L;
        hdr.size = end = 0x14L;         /* the header and one entry */
        hdr.count = 0;
        hdr.method = 0;
        hdr.itype = 0;
        hdr.sig = tag;
        grp_append(&hdr, 16);
        grp_append(&end, 4);
    }
}

void far lib_insert(int slot, int item_index, long size)                /* 2a3d:0085 */
{
    struct LibHdr hdr;
    char far *library_data;
    long off;
    long pos;
    short last;
    int index;

    library_data = slot_base(slot);
    if (item_index < 0 || lib_items(library_data) < item_index)
        error_box("LBInsert invalid item (%d)", item_index);
    if (size < 0)
        error_box("LBInsert invalid size (%l)", size);
    huge_move(library_data, &hdr, 16);
    last = ++hdr.count;
    hdr.size += size + 4;
    huge_move(&hdr, library_data, 16);
    slot_insert_delete(slot, (item_index + 1) * sizeof(long) + sizeof hdr, 4L);
    library_data = huge_add(slot_base(slot), 16L);
    for (index = 0; index <= last; index++) {
        if (index != item_index + 1) {
            huge_move(library_data, &off, 4);
            off += 4;
        }
        if (index == item_index)
            pos = off;
        else if (index > item_index)
            off += size;
        huge_move(&off, library_data, 4);
        library_data = huge_add(library_data, 4L);
    }
    slot_insert_delete(slot, pos, size);
}

void far lib_delete(int slot, int item_index)                             /* 2a3d:020d */
{
    struct LibHdr hdr;
    char far *library_data;
    long size;
    long off;
    long pos;
    short last;
    int index;

    library_data = slot_base(slot);
    if (item_index < 0 || lib_items(library_data) <= item_index)
        error_box("LBDelete invalid item (%d)", item_index);
    size = lib_item_size(library_data, item_index);
    huge_move(library_data, &hdr, 16);
    last = hdr.count--;
    hdr.size -= size + 4;
    huge_move(&hdr, library_data, 16);
    library_data = huge_add(library_data, 16L);
    for (index = 0; index <= last; index++) {
        huge_move(library_data, &off, 4);
        off -= 4;
        if (index == item_index)
            pos = off;
        else if (index > item_index)
            off -= size;
        huge_move(&off, library_data, 4);
        library_data = huge_add(library_data, 4L);
    }
    slot_insert_delete(slot, item_index * sizeof(long) + sizeof hdr, -4L);
    slot_insert_delete(slot, pos, -size);
}

void far lib_resize(int slot, int item_index, long size)                /* 2a3d:0382 */
{
    struct LibHdr hdr;
    char far *library_data;
    long old;
    long off;
    long pos;
    long delta;
    short last;
    int index;

    library_data = slot_base(slot);
    if (item_index < 0 || lib_items(library_data) <= item_index)
        error_box("LBResize invalid item (%d)", item_index);
    old = lib_item_size(library_data, item_index);
    if ((delta = size - old) != 0) {
        huge_move(library_data, &hdr, 16);
        last = hdr.count;
        hdr.size += delta;
        huge_move(&hdr, library_data, 16);
        library_data = huge_add(library_data, 16L);
        for (index = 0; index <= last; index++) {
            if (index > item_index) {
                huge_move(library_data, &off, 4);
                if (index == item_index + 1) {
                    pos = off;
                    if (delta < 0)
                        pos += delta;
                }
                off += delta;
                huge_move(&off, library_data, 4);
            }
            library_data = huge_add(library_data, 4L);
        }
        slot_insert_delete(slot, pos, delta);
    }
}

char far lib_read_convert(int file_handle, long len, int slot, int item_index, long tag) /* 2a3d:04f5 */
{
    char far *directory_entry;
    long got;
    long off;
    register int index;

    /* shipped: 0382 takes the new size, so this is right only for an empty
     * item; both calls found, in tiles.c, insert item n empty first. */
    lib_resize(slot, item_index, len - lib_item_size(slot_base(slot), item_index));
    if (file_read(file_handle, lib_item(slot_base(slot), item_index), len) != len)
        return 0;
    for (index = 0; sig_tags[index] != tag && index < sig_count; index++)
        ;
    if (index < sig_count) {
        if ((got = (*sig_convert[index])(slot, huge_diff(lib_item(slot_base(slot), item_index),
                                                     slot_base(slot)), len)) < 0)
            return 0;
        if (got != len) {
            directory_entry = huge_add(slot_base(slot), (item_index + 1) * sizeof(long) + 16L);
            for (index = item_index + 1; index <= lib_items(slot_base(slot)); index++) {
                huge_move(directory_entry, &off, 4);
                off += got - len;
                huge_move(&off, directory_entry, 4);
                directory_entry = huge_add(directory_entry, 4L);
            }
        }
    }
    return 1;
}
