/* DQK.EXE overlay 04 module 25ca: setting up the slot pools.
 * Eight routines take a slot count and give one pool its capacity, its
 * record size and backing storage for that many records; one takes a
 * count it never reads and points node_pool at item_record_pool instead, and the
 * last takes nothing and copies the "Gem" text into debug_password.  Module 25de
 * calls them all.
 *
 * EXE 0x037b71..0x037cb6, one Turbo C large-model _TEXT segment starting at
 * byte 1 of paragraph 25ca in RTLink section 4 (payload 0x37910, link
 * segment 25a4).  Byte 0 of paragraph 25ca is config.c's final retf, and
 * the last 6 bytes of the range lie in paragraph 25de, whose module starts
 * at 25de:0006.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are
 * defined in shipped order; bake consumes them in that order.  Sections 5
 * to 8 hold other bytes at the 25a4 link segment.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the routine lives in.  mem_alloc and
 * arena_alloc both return storage for the byte count they are passed.
 *
 * _DATA and _BSS are empty: every pool is another module's.
 */

#include "dqk.h"

#define POOL_INIT(pool, n, size, alloc)                                 \
    pool.capacity = n;                                                  \
    pool.stride = size;                                                 \
    pool.base = alloc(n * size)

void far party_pool_init(int record_count)                                  /* 25ca:0001 */
{
    POOL_INIT(party_pool, record_count, sizeof(struct Character), mem_alloc);
}

void far npc_pool_init(int record_count)                                  /* 25ca:0025 */
{
    POOL_INIT(npc_pool, record_count, sizeof(struct Character), arena_alloc);
}

void far item_pool_init(int record_count)                                  /* 25ca:0049 */
{
    POOL_INIT(item_record_pool, record_count, sizeof(struct Item), mem_alloc);                    /* poolfile.c's Item */
}

void far item_pool_select(int unused_count)          /* 25ca:006d */     /* n unread; 25de passes 200 */
{
    node_pool = &item_record_pool;
}

void far effect_pool_init(int record_count)                                  /* 25ca:007c */
{
    POOL_INIT(effect_pool, record_count, sizeof(struct Effect), mem_alloc);
}

void far cloud_pool_init(int record_count)                                  /* 25ca:00a0 */
{
    POOL_INIT(cloud_pool, record_count, sizeof(struct Cloud), arena_alloc);
}

void far combat_pool_init(int record_count)                                  /* 25ca:00c4 */
{
    POOL_INIT(combat_pool, record_count, sizeof(struct Combat), arena_alloc); /* cmbsetup.c's Combat */
}

void far ecl_stack_pool_init(int record_count)                                  /* 25ca:00e8 */
{
    POOL_INIT(ecl_frame_pool, record_count, sizeof(struct EclFrame), mem_alloc);
}

void far drop_pool_init(int record_count)                                  /* 25ca:010c */
{
    POOL_INIT(dropped_item_pool, record_count, sizeof(struct Drop), arena_alloc);
}

void far debug_password_init(void)                                   /* 25ca:0130 */
{
    str_copy(debug_password, gem_text);
}
