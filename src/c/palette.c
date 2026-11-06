/* DQK.EXE resident module 261b: the VGA palette and its colour cycles.
 * pal_want holds the palette the game asked for and pal_live the one last
 * handed to 00d7:20f6; a bit per entry in pal_stale marks a pal_want entry
 * not yet copied across.  Up to 12 colour cycles rotate a run of pal_live
 * entries each time timer_ticks passes their next tick.
 *
 * pal_set takes a run of RGB triples, stored shifted right by 2, and a list
 * of cycles.  An entry that changes is written to pal_want and marked stale;
 * when anything changed, the running cycles that overlap the run are freed,
 * their entries marked stale, and the list is started in free slots.
 * pal_commit saves the cycle slots pal_set touched into cyc_last, then
 * copies the stale entries to pal_live and hands the span they cover to
 * 00d7:20f6.  input_poll steps every cycle that is due and hands on the
 * entries it rotated; strutil.c and menu.c call it inside their wait loops,
 * so "input_poll" is a placeholder.
 *
 * EXE 0x0185bb..0x018db6, one Turbo C large-model _TEXT segment starting at
 * byte 0xb of paragraph 261b.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Module 25ff's last routine, 25ff:01c7 (far-called from 0x0126ff and five
 * more sites), ends with the retf at 0x0185ba, and module 269b starts at
 * 0x018db6 (269b:0006), far-called from sysutil.c at 0x01348b, with no stub
 * between either.  Ghidra's index misses pal_init, far-called from overlay
 * code at 0x037e44.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the
 * rest are placeholders chosen from use.
 */

#include "dqk.h"

struct Cycle {
    long next;                      /* tick of the next step; 0x7fffffffL when free */
    struct CycSpec spec;
};

static char pal_on;                         /* cdc4, input_poll steps cycles while set */
static unsigned char pal_live[256][3];      /* cdc5 */
static unsigned char pal_stale[256 / 8];    /* d0c5 */
static struct Cycle cyc_last[12];           /* d0e5 */
static unsigned char pal_want[256][3];      /* d145 */
static struct Cycle cyc_slots[12];          /* d445 */
static char cyc_touched[12];                /* d4a5 */

void far pal_init(void)                                          /* 261b:000b */
{
    register int index;

    mem_fill(pal_live, sizeof(pal_live), 0);
    mem_fill(pal_want, sizeof(pal_want), 0);
    mem_fill(pal_stale, sizeof(pal_stale), 0xff);
    mem_fill(cyc_last, sizeof(cyc_last), 0);
    for (index = 0; index < 12; index++)
        cyc_last[index].next = 0x7fffffffL;
    mem_move(cyc_last, cyc_slots, sizeof(cyc_slots));
    mem_fill(cyc_touched, sizeof(cyc_touched), 0);
    pal_on = 1;
}

void far pal_set(int first, int count, unsigned char far *rgb,
                 int nspec, struct CycSpec far *spec)            /* 261b:00b1 */
{
    struct CycSpec far *cycle_specs;
    short color_count;
    unsigned char far *color_cursor;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    char changed;
    register int index;
    register int inner_index;

    cycle_specs = spec;
    changed = 0;
    for (index = inner_index = 0; index < 12 && !changed; index++) {
        if (cyc_slots[index].next == 0x7fffffffL)
            continue;
        if (cyc_slots[index].spec.first >= first + count)
            continue;
        if (cyc_slots[index].spec.first + cyc_slots[index].spec.count <= first)
            continue;
        /* && where || is meant: one matching field counts the slot unchanged, as shipped */
        /* s[j] is not bounded by nspec, as shipped */
        if (cyc_slots[index].spec.flags != cycle_specs[inner_index].flags &&
            cyc_slots[index].spec.rate != cycle_specs[inner_index].rate &&
            cyc_slots[index].spec.first != cycle_specs[inner_index].first &&
            cyc_slots[index].spec.count != cycle_specs[inner_index].count)
            changed = 1;
        inner_index++;
    }
    if (inner_index != nspec)
        changed = 1;
    color_cursor = pal_want[first];
    for (index = count, inner_index = first; --index >= 0; inner_index++) {
        red = *rgb >> 2;
        rgb++;
        green = *rgb >> 2;
        rgb++;
        blue = *rgb >> 2;
        rgb++;
        if (color_cursor[0] != red || color_cursor[1] != green || color_cursor[2] != blue) {
            changed = 1;
            pal_stale[inner_index >> 3] |= 1 << (inner_index & 7);
            *color_cursor = red;
            color_cursor++;
            *color_cursor = green;
            color_cursor++;
            *color_cursor = blue;
            color_cursor++;
        } else
            color_cursor += 3;
    }
    if (!changed)
        return;
    for (index = 0; index < 12; index++) {
        if (cyc_slots[index].next == 0x7fffffffL)
            continue;
        if (cyc_slots[index].spec.first >= first + count)
            continue;
        if (cyc_slots[index].spec.first + cyc_slots[index].spec.count <= first)
            continue;
        inner_index = cyc_slots[index].spec.first;
        color_count = cyc_slots[index].spec.count;
        while (--color_count >= 0) {
            pal_stale[inner_index >> 3] |= 1 << (inner_index & 7);
            inner_index++;
        }
        cyc_slots[index].next = 0x7fffffffL;
        cyc_touched[index] = 1;
    }
    for (index = inner_index = 0; index < nspec; index++, cycle_specs++) {
        while (cyc_slots[inner_index].next != 0x7fffffffL && inner_index < 12) /* reads [12] first, as shipped */
            inner_index++;
        if (inner_index >= 12) {
            error_box("Out of color ranges!");
            return;
        }
        cyc_slots[inner_index].next = timer_ticks() + random_int(8) + cycle_specs->rate - 3;
        mem_move(cycle_specs, &cyc_slots[inner_index].spec, sizeof(struct CycSpec));
        cyc_touched[inner_index] = 1;
    }
}

void far pal_commit(void)                                      /* 261b:0455 */
{
    short hi;
    register int index;
    register int lo;

    lo = 0xff;
    hi = 0;
    pal_on = 0;
    for (index = 0; index < 12; index++) {
        if (cyc_touched[index]) {
            mem_move(&cyc_slots[index], &cyc_last[index], sizeof(struct Cycle));
            cyc_touched[index] = 0;
        }
    }
    index = 0;
    while (index < 0x100) {
        if (pal_stale[index >> 3]) {
            if (pal_stale[index >> 3] == 0xff) {
                if (lo > index)
                    lo = index;
                if (index + 7 > hi)
                    hi = index + 7;
                mem_move(pal_want[index], pal_live[index], 8 * 3);
                pal_stale[index >> 3] = 0;
            } else {
                do {
                    if (pal_stale[index >> 3] & (1 << (index & 7))) {
                        mem_move(pal_want[index], pal_live[index], 3);
                        if (lo > index)
                            lo = index;
                        if (hi < index)
                            hi = index;
                    }
                    index++;
                } while (index & 7);
                pal_stale[(index - 8) >> 3] = 0;
                continue;
            }
        }
        index += 8;
    }
    if (lo <= hi)
        dac_load(lo, hi - lo + 1, 1, pal_live[lo]);
    pal_on = 1;
}

void far input_poll(void)                                        /* 261b:05be */
{
    struct Cycle far *cycle;
    long now;
    short scan_index;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    char wait;
    register int first_color;
    register int color_span;

    if (!pal_on)
        return;
    wait = 1;
    now = timer_ticks();
    cycle = cyc_slots;
    for (scan_index = 0; scan_index < 12; scan_index++, cycle++) {
        if (cycle->next > now)
            continue;
        first_color = cycle->spec.first;
        color_span = cycle->spec.count - 1;
        while (cycle->next <= now) {
            if (cycle->spec.flags & 1) {
                red = pal_live[first_color][0];
                green = pal_live[first_color][1];
                blue = pal_live[first_color][2];
                mem_move(pal_live[first_color + 1], pal_live[first_color], color_span * 3);
                pal_live[first_color + color_span][0] = red;
                pal_live[first_color + color_span][1] = green;
                pal_live[first_color + color_span][2] = blue;
            } else {
                red = pal_live[first_color + color_span][0];
                green = pal_live[first_color + color_span][1];
                blue = pal_live[first_color + color_span][2];
                mem_move(pal_live[first_color], pal_live[first_color + 1], color_span * 3);
                pal_live[first_color][0] = red;
                pal_live[first_color][1] = green;
                pal_live[first_color][2] = blue;
            }
            cycle->next += cycle->spec.rate;
        }
        dac_load(first_color, color_span + 1, wait, pal_live[first_color]);
        wait = 0;
    }
}
