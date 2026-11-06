/* DQK.EXE resident module 27fa: the far-memory file cache.  One far block
 * holds up to 48 file groups packed end to end, each holding bytes read
 * from a file, keyed by its name; a slot (0..47) names a group.  Closing
 * a slot leaves its group cached, and a later open of the same name
 * reuses it.
 * When room runs out a group no slot names is evicted: the newest one when
 * random_int(3) is nonzero, else the least recently opened one.  Bytes
 * can be inserted into and deleted from a group in place, moving the
 * groups above it.  The error texts call the cache FC.
 *
 * EXE 0x01a3ad..0x01b3c4, one Turbo C large-model _TEXT segment starting at
 * byte d of paragraph 27fa.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call; a variable's is its DGROUP offset.  Callee and variable names are
 * placeholders chosen from use; a modNNNN_ name says only which segment the
 * callee lives in.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is.
 * file_cache_done calls slot_release, load_cb calls grp_read, and
 * slot_used, grp_extend, huge_shift and huge_diff are each called ahead
 * of their definitions, so those carry prototypes.  grp_remove, grp_evict
 * and slot_check are called near ahead of their definitions, which only
 * a static function allows.
 *
 * _BSS follows the statics' names (Turbo C's symbol-table order), not
 * their declaration order; these names reproduce the shipped d6ba..da8d
 * layout, chosen with goldbox.bss_order.  The originals are not
 * recoverable from it.
 */

#include "dqk.h"

extern void far huge_move(void far *src, void far *dst, unsigned n); /* 00d7:08c2 */
extern int  far is_tandy(void);                                  /* 1d99:1553 */

void far slot_release(char far *name);
long far slot_used(int slot);
char far grp_extend(long additional_bytes);
char far grp_read(int file_handle, long byte_count);
void far huge_shift(char far *src, char far *dst, long byte_count);
long far huge_diff(char far *left_pointer, char far *right_pointer);
static void grp_remove(int group_index);
static char grp_evict(char for_open);
static void slot_check(int slot);

static char mru[48];            /* d6ba  groups, most recently opened first */
static char names[48][14];      /* d6ea  each group's file name           */
static char slot_of[48];        /* d98a  each slot's group, -1 when none   */
static int kill_mem;            /* d9ba  evictions to make room            */
static char far *grp_ptrs[49];  /* d9bc  group starts; [n_groups] is free  */
static int n_groups;            /* da80                                    */
static long least_free;         /* da82  the least room not held by a slot */
static char far *far_limit;     /* da86  the end of the block              */
static int peak_grps;           /* da8a  the most groups held at once      */
static int open_kills;          /* da8c  evictions to free a group entry   */

long far file_read(int file_handle, char far *buf, long remaining_bytes)                /* 27fa:000d */
{
    long total;
    short saved_data_segment;
    int got;
    int chunk;

    total = 0;
    while (remaining_bytes > 0) {
        chunk = remaining_bytes > 0x4000 ? 0x4000 : remaining_bytes;
        saved_data_segment = _DS;
        _DS = (unsigned)((unsigned long)buf >> 16);
        got = dos_io(0x3f00, buf, file_handle, chunk);
        _DS = saved_data_segment;
        if (got < 0)
            remaining_bytes = total = got;
        else if (got == 0)
            remaining_bytes = 0;
        else {
            buf = huge_add(buf, (long)got);
            total += got;
            remaining_bytes -= got;
        }
    }
    return total;
}

void far file_cache_init(int need_kb, int want_kb)                  /* 27fa:00cc */
{
    char far *cache_base;
    long size;

    n_groups = 0;
    mem_fill(slot_of, sizeof slot_of, -1);
    mem_fill(names, sizeof names, 0);
    size = (long)want_kb << 10;
    if ((cache_base = far_alloc(size)) == 0)
        if ((size = far_fallback) == 0 || (cache_base = far_alloc(size)) == 0)
            error_box("Not enough far memory available!");
    far_limit = huge_add(cache_base, size);
    if (is_tandy()) {
        if (video_mode <= 4) {
            if (((long)far_limit & 0x0f000000L) == 0) {
                far_limit -= 0;             /* sub word [da86],0 (27fa:0194) */
                size -= 0x4000;
            }
        } else if (video_mode == 12) {
            far_limit = (char far *)((unsigned long)mem_top_seg << 16);
            size = huge_diff(far_limit, cache_base);
        }
    }
    grp_ptrs[0] = cache_base;
    if (cache_base == 0)
        size = 0;
    if ((long)need_kb << 10 > size)
        error_box("Out of FAR Memory! %l/%l", size, (long)need_kb << 10);
    least_free = size;
    peak_grps = open_kills = kill_mem = 0;
}

void far file_cache_done(void)                                      /* 27fa:0240 */
{
    slot_release(0);
    far_free(grp_ptrs[0]);
}

char far slot_open(char far *name, register int slot)            /* 27fa:025b */
{
    short index;
    char buf[80];

    path_base_copy(name, buf);
    buf[13] = 0;
    if (slot < 0 || slot >= 48)
        error_box("Invalid group (%d)", slot);
    if (!(slot_of[slot] & 0x80))
        error_box("Group %d in use for '%s'", slot, names[slot_of[slot]]);
    for (index = 0; index < n_groups; index++)
        if (str_same(names[index], buf)) {
            slot_of[slot] = index;
            mem_move(mru, mru + 1, mem_scan(mru, n_groups, index));
            mru[0] = index;
            return 1;
        }
    if (n_groups >= 48 && !grp_evict(1)) {
        error_box("FCSetup: too many file groups");
        return 1;
    }
    str_copy(names[n_groups], buf);
    slot_of[slot] = n_groups;
    mem_move(mru, mru + 1, n_groups);
    mru[0] = n_groups;
    n_groups++;
    grp_ptrs[n_groups] = grp_ptrs[n_groups - 1];
    peak_grps = int_max(peak_grps, n_groups);
    return 0;
}

/* Note: reserves byte_count bytes but copies byte_count + 1, as shipped.
 * An append that fills the cache writes one byte beyond its limit. */
char far grp_append(void far *src, int byte_count)                        /* 27fa:03bc */
{
    char far *append_at;
    long used;

    used = huge_diff(grp_ptrs[n_groups], grp_ptrs[n_groups - 1]);
    if (!grp_extend((long)byte_count))
        return 0;
    append_at = huge_add(grp_ptrs[n_groups - 1], used);
    huge_move(src, append_at, byte_count + 1);
    return 1;
}

void far mem_report(long need)                                   /* 27fa:0448 */
{
    long total;
    char seen[48];
    int index;
    int row;

    set_text_mode(1);
    row = 0;
    total = 0;
    for (index = 0; index < 48; index++)
        seen[index] = 0;
    for (index = 0; index < 48; index++)
        if (!(slot_of[index] & 0x80)) {
            text_print_at(row * 4 + 0x1f40, 0x1f40, 15, "Grp %2d/%2d Size %5l %s",
                          index, slot_of[index], slot_used(index), names[slot_of[index]]);
            row++;
            seen[slot_of[index]] = 1;
            total += slot_used(index);
        }
    for (index = 0; index < n_groups; index++)
        if (!seen[index]) {
            text_print_at(row * 4 + 0x1f40, 0x1f40, 15, "Grp ##/%2d Size %5l %s",
                          index, huge_diff(grp_ptrs[index + 1], grp_ptrs[index]), names[index]);
            row++;
        }
    for (index = 0; index < 48; index++)
        text_print_at(index * 4 + 0x1f40, 0x1fcc, 15, "%2x %2x",
                      slot_of[index] & 0xff, mru[index] & 0xff);
    text_print_at(0x1f9c, 0x1f40, 15, "%d/%d grps %l size %d cnt %d mem",
                  peak_grps, 48, slot_used(-2) - least_free, open_kills, kill_mem);
    error_box("%d Need %l+%l/%l", n_groups, need, total,
              huge_diff(far_limit, grp_ptrs[0]));
}

char far mem_room(long need)                                     /* 27fa:05f8 */
{
    for (;;) {
        if (huge_diff(far_limit, grp_ptrs[n_groups]) >= need)
            return 1;
        if (!grp_evict(0))
            break;
    }
    error_box("Out of FAR memory!");
    mem_report(need);
    return 0;
}

char far grp_extend(long additional_bytes)                                      /* 27fa:0652 */
{
    if (mem_room(additional_bytes)) {
        grp_ptrs[n_groups] = huge_add(grp_ptrs[n_groups], additional_bytes);
        if (slot_used(-1) < least_free)
            least_free = slot_used(-1);
        return 1;
    }
    return 0;
}

void far grp_pop(void)                                      /* 27fa:06c3 */
{
    int index;

    n_groups--;
    for (index = 0; index < 48; index++)
        if (slot_of[index] == n_groups)
            slot_of[index] = -1;
    mem_move(mru + 1, mru, n_groups);
}

void far slot_close(int slot)                                  /* 27fa:06fa */
{
    slot_of[slot] = -1;
}

void far slot_release(char far *name)                            /* 27fa:0707 */
{
    char buf[80];
    int index;

    if (name == 0) {
        n_groups = 0;
        mem_fill(slot_of, sizeof slot_of, -1);
    } else {
        path_base_copy(name, buf);
        buf[13] = 0;
        for (index = 0; index < n_groups; index++)
            if (str_same(names[index], buf))
                grp_remove(index--);
    }
}

char far load_cb(int file_handle)                                         /* 27fa:0780 */
{
    return grp_read(file_handle, -1L);
}

char far grp_read(register int file_handle, long byte_count)                   /* 27fa:0796 */
{
    long pos;

    if (byte_count < 0) {
        if ((pos = file_seek(file_handle, 0L, 1)) < 0 || (byte_count = file_seek(file_handle, 0L, 2)) < 0 ||
            file_seek(file_handle, pos, 0) < 0)
            return 0;
        byte_count -= pos;
    }
    pos = huge_diff(grp_ptrs[n_groups], grp_ptrs[n_groups - 1]);
    if (!grp_extend(byte_count + (byte_count & 1 ? 1 : 0)))
        return 0;
    return file_read(file_handle, huge_add(grp_ptrs[n_groups - 1], pos), byte_count) == byte_count;
}

void far slot_load(char mode, char far *name, int slot)          /* 27fa:08cd */
{
    if (!slot_open(name, slot))
        file_open(mode, name, 0, load_cb);
}

void far *far slot_base(int slot)                                /* 27fa:08fd */
{
    slot_check(slot);
    return grp_ptrs[slot_of[slot]];
}

long far slot_used(register int slot)                            /* 27fa:0920 */
{
    long used_bytes;

    if (slot < 0) {
        used_bytes = huge_diff(far_limit, grp_ptrs[0]);
        if (slot != -2)
            for (slot = 0; slot < n_groups; slot++)
                if (mem_scan(slot_of, 48, slot) < 48)
                    used_bytes -= huge_diff(grp_ptrs[slot + 1], grp_ptrs[slot]);
        return used_bytes;
    }
    if (slot >= 48 || slot_of[slot] & 0x80)
        return 0L;
    return huge_diff(grp_ptrs[slot_of[slot] + 1], grp_ptrs[slot_of[slot]]);
}

char far slot_reserve(register int slot, long additional_bytes)                 /* 27fa:09ef */
{
    if (slot_used(slot) < -additional_bytes)
        additional_bytes = -slot_used(slot);
    if (additional_bytes != 0) {
        if (additional_bytes > 0 && !mem_room(additional_bytes))
            return 0;
        slot = slot_of[slot] + 1;
        if (slot < n_groups)
            huge_shift(grp_ptrs[slot], huge_add(grp_ptrs[slot], additional_bytes),
                       huge_diff(grp_ptrs[n_groups], grp_ptrs[slot]));
        for (; slot <= n_groups; slot++)
            grp_ptrs[slot] = huge_add(grp_ptrs[slot], additional_bytes);
    }
    if (slot_used(-1) < least_free)
        least_free = slot_used(-1);
    return 1;
}

void far slot_insert_delete(register int slot, long pos, long delta)   /* 27fa:0b20 */
{
    char far *edit_at;
    long used;

    used = slot_used(slot);
    if (pos < 0 || pos > used) {
        error_box("FCInsDel: Invalid offset %l", pos);
        return;
    }
    if (delta < 0) {
        if (pos - delta > used) {
            error_box("FCInsDel: bad length %l", delta);
            return;
        }
    } else if (!mem_room(delta))
        return;
    if (delta < 0) {
        edit_at = huge_add(slot_base(slot), pos);
        huge_shift(huge_add(edit_at, -delta), edit_at, used - pos + delta);
    }
    slot_reserve(slot, delta);
    if (delta > 0) {
        edit_at = huge_add(slot_base(slot), pos);
        huge_shift(edit_at, huge_add(edit_at, delta), used - pos);
    }
}

void far huge_shift(char far *src, char far *dst, long byte_count)        /* 27fa:0c95 */
{
    long pointer_distance;
    unsigned chunk;

    pointer_distance = huge_diff(src, dst);
    if (pointer_distance < 0) {
        src = huge_add(src, byte_count);
        dst = huge_add(dst, byte_count);
    }
    while (byte_count > 0) {
        chunk = byte_count > 0xffdcL ? 0xffdc : byte_count;
        if (pointer_distance < 0) {
            src = huge_add(src, -(long)chunk);
            dst = huge_add(dst, -(long)chunk);
        }
        huge_move(src, dst, chunk);
        byte_count -= chunk;
        if (pointer_distance >= 0) {
            src = huge_add(src, (long)chunk);
            dst = huge_add(dst, (long)chunk);
        }
    }
}

static void grp_remove(int group_index)                                    /* 27fa:0ddd */
{
    long size;
    int index;

    for (index = 0; index < 48; index++) {
        if (slot_of[index] == group_index)
            slot_of[index] = -1;
        else if (slot_of[index] > group_index)
            slot_of[index]--;
        if (index < n_groups) {
            if (mru[index] == group_index)
                mem_move(&mru[index + 1], &mru[index], n_groups - index - 1);
            if (mru[index] > group_index)
                mru[index]--;
        }
    }
    n_groups--;
    for (index = group_index; index < n_groups; index++) {
        size = huge_diff(grp_ptrs[index + 2], grp_ptrs[index + 1]);
        huge_shift(grp_ptrs[index + 1], grp_ptrs[index], size);
        grp_ptrs[index + 1] = huge_add(grp_ptrs[index], size);
        str_copy(names[index], names[index + 1]);
    }
    names[n_groups][0] = 0;
}

static char grp_evict(char for_open)                             /* 27fa:0f14 */
{
    int index;

    if (for_open)
        open_kills++;
    else
        kill_mem++;
    if (random_int(3)) {
        for (index = n_groups - 1; index >= 0; index--)
            if (mem_scan(slot_of, 48, index) >= 48) {
                grp_remove(index);
                return 1;
            }
    } else {
        for (index = n_groups - 1; index >= 0; index--)
            if (mem_scan(slot_of, 48, mru[index]) >= 48) {
                grp_remove(mru[index]);
                return 1;
            }
    }
    return 0;
}

static void slot_check(register int slot)                        /* 27fa:0f9f */
{
    if (slot < 0 || slot >= 48)
        error_box("Invalid group number (%d)", slot);
    if (slot_of[slot] & 0x80)
        error_box("Refrencing a non-loaded group (%d)", slot); /* sic */
}

long far huge_diff(char far *left_pointer, char far *right_pointer)                     /* 27fa:0fd5 */
{
    return ((((long)left_pointer & ~0xffffL) - ((long)right_pointer & ~0xffffL)) >> 12) +
           ((long)left_pointer & 0xffffL) - ((long)right_pointer & 0xffffL);
}
