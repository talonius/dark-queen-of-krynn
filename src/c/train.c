/* DQK.EXE overlay 05 module 2aa7: training a party member to the next level.
 *
 * EXE 0x03f4f1..0x03fd66, one Turbo C large-model _TEXT segment starting at
 * byte 1 of paragraph 2aa7, the last module of RTLink section 5 (payload
 * 0x3a4c0, link segment 25a4).  Byte 0 of paragraph 2aa7 is the final retf
 * of module 2946.  Zeros follow to the section's end at 0x3fd70.  Built
 * with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped
 * order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, and a mod1d99_ callee is an RTLink vector
 * stub whose target is named beside it.
 *
 * _DATA is the eight literals, DGROUP 74fa..75a9.  The module has no _BSS.
 *
 * train_hero raises cur_node one level in every class whose next level it
 * has the experience for; unless extra_row is set, it first cuts the
 * experience to one short of the level after that.  The hall at game_state +
 * 0x30 only gates and lists: unless free_train is set, at least one of those
 * classes must be one the hall teaches, and only the classes it teaches are
 * shown as "will become"; the others still level up.  free_train waives the
 * experience and hall checks and raises the experience to the cost.
 * level_xp_threshold gives a class level's experience cost.
 */

#include "dqk.h"

extern void far status_msg(char far *fmt, char foreground, char background);     /* 0a62:0904 */
extern char far extra_row(void);                                 /* 0bdb:21dd */

char far is_race6(struct Character far *hero)                         /* 2aa7:0001 */
{
    char result;

    result = hero->race == 6;
    return result;
}

void far train_hero(void)                                        /* 2aa7:0022 */
{
    unsigned char blocked;
    long cap;
    long next;
    unsigned char train;
    unsigned char has;
    unsigned char count;
    short class_index;
    unsigned char row;
    unsigned char was5;
    unsigned char lvl;
    unsigned char pick;
    int choice;
    char more;
    long best_xp;
    short best;
    unsigned char hall;
    unsigned char shown;
    unsigned char was4;

    if (cur_node->status != 0) {
        status_msg("we only train conscious people", 0xb, 0);
        return;
    }
    train = 0;
    has = 0;
    cap = 0;
    hall = game_state[STATE_TRAINING_CLASSES];
    cap = 0;                            /* stored twice, 2aa7:0051 and 0066 */
    for (class_index = 0; class_index <= 6; class_index++) {
        if (cur_node->level[class_index] > 0) {
            has += class_bit[class_index];
            blocked = 0;
            lvl = cur_node->level[class_index];
            if (class_index == 1 && cur_node->knight_order == 3)
                blocked = lvl >= 0x28;
            else if (class_index == 1 && (cur_node->knight_order == 2 || cur_node->knight_order == 4))
                blocked = lvl >= 0x12;
            else
                blocked = max_level[cur_node->race][class_index] <= lvl;
            if (!blocked) {
                if (level_xp_threshold(cur_node, lvl + 1, class_index) > cur_node->xp && !free_train)
                    continue;
                if (level_xp_threshold(cur_node, lvl + 1, class_index) > 0) {
                    if (free_train && level_xp_threshold(cur_node, lvl + 1, class_index) > 0
                        && level_xp_threshold(cur_node, lvl + 1, class_index))
                        cur_node->xp = level_xp_threshold(cur_node, lvl + 1, class_index);
                    train += class_bit[class_index];
                    next = level_xp_threshold(cur_node, lvl + 2, class_index);
                    if (next > 0 && cur_node->xp >= next && next > cap)
                        cap = next - 1;
                }
            }
        }
    }
    if (!extra_row()) {
        best_xp = 0;
        next = 0;
        for (class_index = 0; class_index <= 6; class_index++) {
            if ((char)(class_bit[class_index] & train)) {     /* (char): and/or, not test */
                /* lvl is still the last level the loop above read */
                if (level_xp_threshold(cur_node, lvl + 1, class_index) > best_xp) {
                    best_xp = level_xp_threshold(cur_node, lvl + 1, class_index);
                    best = class_index;
                }
            }
        }
        if (best_xp > 0) {
            train = class_bit[best] | train;
            next = level_xp_threshold(cur_node, lvl + 2, best);
            if (next > 0 && cur_node->xp >= next && next > cap)
                cap = next - 1;
        }
    }
    if (cap > 0 && !extra_row() && cur_node->xp > cap)
        cur_node->xp = cap;
    if (!free_train) {
        if (!(char)(has & hall) && !extra_row()) {    /* (char): and/or */
            status_msg("we don't train that class here", 0xb, 0);
            return;
        }
        if (!(train & hall)) {
            training_done = 1;
            cur_node->level_up = 0;
            if (extra_row())
                return;
            status_msg("Not Enough Experience", 0xb, 0);
            return;
        }
    }
    if (!free_train)
        shown = train & hall;
    else
        shown = train;
    if (!extra_row()) {
        text_clear(1, 1, 0x26, 0x16);
        row = 4;
        character_name_show(cur_node, 4, row, 0);
        status_printf(str_length(cur_node->name) + 4, row, 7, 0, " will become:");
        for (class_index = 0; class_index <= 6; class_index++) {
            if (cur_node->level[class_index] > 0 && (class_bit[class_index] & shown)) {
                row++;
                if (row == 5)
                    status_printf(6, row, 7, 0,
                                  msg_format("    a level %s %s",
                                             byte_str(cur_node->level[class_index] + 1),
                                             class_names[class_index]));
                else
                    status_printf(6, row, 7, 0,
                                  msg_format("and a level %s %s",
                                             byte_str(cur_node->level[class_index] + 1),
                                             class_names[class_index]));
            }
        }
        if (!confirm("Do you wish to train? ", 1))
            return;
        bottom_msg("Congratulations...");
    }
    count = 0;
    was5 = cur_node->level[5];
    was4 = cur_node->level[4];
    cur_node->class_mask = 0;
    for (class_index = 0; class_index <= 6; class_index++) {
        if (cur_node->level[class_index] > 0) {
            count++;
            if (class_bit[class_index] & train)
                cur_node->level[class_index]++;
        }
    }
    recalc_from_levels(cur_node);
    if (!extra_row()) {
        if (cur_node->level[5] > was5
            || cur_node->level[4] > 8 && cur_node->level[4] > was4) {
            choice = -1;
            do {
                pick = spell_menu(4, 4, &choice, &more);
            } while (pick <= 0 && more != 0);
            if (pick > 0)
                cur_node->known[(pick - 1) / 8] =
                    cur_node->known[(pick - 1) / 8] | bit_mask[(pick - 1) % 8];
        }
    }
    if (extra_row()) {
        if (cur_node->robe_order == 1) {
            switch (cur_node->level[5]) {
            case 3:
                cur_node->known[3] = cur_node->known[3] | bit_mask[4];
                cur_node->known[4] = cur_node->known[4] | bit_mask[0];
                cur_node->known[4] = cur_node->known[4] | bit_mask[1];
                break;
            case 5:
                cur_node->known[5] = cur_node->known[5] | bit_mask[5];
                cur_node->known[5] = cur_node->known[5] | bit_mask[6];
                cur_node->known[6] = cur_node->known[6] | bit_mask[2];
                break;
            case 7:
                cur_node->known[10] = cur_node->known[10] | bit_mask[0];
                cur_node->known[10] = cur_node->known[10] | bit_mask[1];
                cur_node->known[10] = cur_node->known[10] | bit_mask[4];
                cur_node->known[10] = cur_node->known[10] | bit_mask[6];
                break;
            }
        } else {
            switch (cur_node->level[5]) {
            case 3:
                cur_node->known[3] = cur_node->known[3] | bit_mask[5];
                cur_node->known[3] = cur_node->known[3] | bit_mask[6];
                cur_node->known[3] = cur_node->known[3] | bit_mask[7];
                cur_node->known[4] = cur_node->known[4] | bit_mask[1];
                break;
            case 5:
                cur_node->known[5] = cur_node->known[5] | bit_mask[4];
                cur_node->known[5] = cur_node->known[5] | bit_mask[6];
                cur_node->known[5] = cur_node->known[5] | bit_mask[7];
                cur_node->known[6] = cur_node->known[6] | bit_mask[1];
                break;
            case 7:
                cur_node->known[10] = cur_node->known[10] | bit_mask[2];
                cur_node->known[10] = cur_node->known[10] | bit_mask[4];
                cur_node->known[10] = cur_node->known[10] | bit_mask[3];
                cur_node->known[10] = cur_node->known[10] | bit_mask[6];
                break;
            }
        }
    }
    hp_adjust(cur_node, train, count, 1);
    cur_node->level_up = can_level_up(cur_node);
}
