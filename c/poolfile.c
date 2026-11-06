/* DQK.EXE overlay 03 module 251a: saving and loading the pool.
 * Emptying the pool, its coins zeroed and its items freed; writing the
 * pooled coins and items to a file and reading them back, as file
 * callbacks; and the pool's record count, which pool_load sets and section 8
 * lowers as items leave.
 * An item whose type is 'I' carries count more items on its sub chain,
 * and each is written as its own record; pool_save pads the records out
 * to 50 from item_templates.
 *
 * EXE 0x036b92..0x036fc1, one Turbo C large-model _TEXT segment starting at
 * byte 2 of paragraph 251a in RTLink section 3 (payload 0x30ed0, link
 * segment 1f4e).  The first 2 bytes of paragraph 251a are the tail of
 * module 24da, and the last byte of the range, pool_load's retf, is byte 0
 * of paragraph 255d, whose module starts at 255d:0001.  Built with Turbo C
 * 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped order; bake
 * consumes them in that order.  Section 0's 1f4e link segment holds other
 * bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use.  pool_clear is far-called from
 * 0x36722 (section 3) and 0x4b778, 0x4b84f, 0x4c21a (section 8);
 * pool_set_records and pool_records are reached through the vector stubs
 * 1d99:13cd and 1d99:13c3 and far-called from section 8.  Module 2177
 * passes pool_load (0x36745) and pool_save (0x36784) as file callbacks.
 *
 * _DATA is empty.  _BSS is records at acd4.
 */

#include "dqk.h"

struct Hdr {                    /* what precedes the item records         */
    int tag;                    /* +0  written as -1                      */
    unsigned count;             /* +2  items on pool_items                */
};

static int records;                                              /* acd4 */

void far pool_clear(void)                                        /* 251a:0002 */
{
    struct Item far *sub;
    struct Item far *item;

    mem_zero(pool, sizeof pool);
    for (cur_item = pool_items; cur_item != 0; ) {
        if (cur_item->rec.type == 'I') {
            sub = cur_item->more;
            for (tick_slot = 1; tick_slot <= cur_item->rec.quantity; tick_slot++)
                if (sub != 0) {
                    item = sub;
                    sub = sub->more;
                    pool_free(item, sizeof(struct Item), &item_record_pool);
                }
        }
        item = cur_item;
        cur_item = cur_item->next;
        pool_free(item, sizeof(struct Item), &item_record_pool);
    }
    pool_items = 0;
}

void far pool_set_records(int record_count)                                 /* 251a:00d0 */
{
    records = record_count;
}

int far pool_records(void)                                       /* 251a:00db */
{
    return records;
}

char far pool_save(int file_handle)                                       /* 251a:00df */
{
    struct Item far *item;
    struct Item far *sub;
    struct Hdr hdr;
    register int record_count;

    if (dos_io(0x4000, pool, file_handle, sizeof pool) != sizeof pool)
        return 0;
    hdr.count = 0;
    for (item = pool_items; item != 0; item = item->next)
        hdr.count++;
    hdr.tag = -1;
    record_count = 0;
    if (dos_io(0x4000, &hdr, file_handle, sizeof hdr) != sizeof hdr)
        return 0;
    for (item = pool_items; item != 0; item = item->next) {
        if (dos_io(0x4000, &item->rec, file_handle, sizeof(struct ItemRec))
            != sizeof(struct ItemRec))
            return 0;
        record_count++;
        if (item->rec.type == 'I') {
            sub = item;
            for (tick_slot = 1; tick_slot <= item->rec.quantity; tick_slot++) {
                sub = sub->more;
                if (dos_io(0x4000, &sub->rec, file_handle, sizeof(struct ItemRec))
                    != sizeof(struct ItemRec))
                    return 0;
                record_count++;
            }
        }
    }
    if (record_count >= 50 || dos_io(0x4000, item_templates, file_handle, (50 - record_count) * sizeof(struct ItemRec))
        == (50 - record_count) * sizeof(struct ItemRec)) {
        pool_clear();
        return 1;
    }
    return 0;
}

char far pool_load(int file_handle)                                       /* 251a:0239 */
{
    struct Item far *item;
    struct Item far *sub;
    struct Item rec;
    struct Hdr hdr;
    char err;
    register int record_count;

    pool_clear();
    record_count = 0;
    if (dos_io(0x3f00, pool, file_handle, sizeof pool) != sizeof pool)
        return 0;
    treasure_has_items = 0;
    if (dos_io(0x3f00, &hdr, file_handle, sizeof hdr) != sizeof hdr)
        return 0;
    for (err = 0; hdr.count > 0 && !err; hdr.count--) {
        mem_fill(&rec, sizeof rec, 0);
        if (dos_io(0x3f00, &rec.rec, file_handle, sizeof(struct ItemRec))
            != sizeof(struct ItemRec))
            err = 1;
        if (!err) {
            if (pool_items != 0) {
                pool_alloc(&item->next, sizeof(struct Item), &item_record_pool);
                item = item->next;
            } else {
                pool_alloc(&pool_items, sizeof(struct Item), &item_record_pool);
                item = pool_items;
            }
            mem_move(&rec, item, sizeof rec);
            treasure_has_items = 1;
            record_count++;
            if (item->rec.type == 'I') {
                sub = item;
                for (tick_slot = 1; tick_slot <= item->rec.quantity && !err;
                     tick_slot++) {
                    mem_fill(&rec, sizeof rec, 0);
                    if (dos_io(0x3f00, &rec.rec, file_handle, sizeof(struct ItemRec))
                        != sizeof(struct ItemRec))
                        err = 1;
                    if (!err) {
                        pool_alloc(&sub->more, sizeof(struct Item), &item_record_pool);
                        sub = sub->more;
                        mem_move(&rec, sub, sizeof rec);
                    } else
                        item->rec.quantity = tick_slot - 1;
                }
                record_count += item->rec.quantity;
            }
        }
    }
    if (err)
        pool_clear();
    else
        pool_set_records(record_count);
    return !err;
}
