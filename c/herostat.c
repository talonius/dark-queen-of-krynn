/* DQK.EXE overlay 11 module 304e: a party member's class-derived values.
 * The spell slots each class grants at its level, and the spells those
 * slots make known; the value at +87 and the saving throws, from per-class
 * tables; the thief skills, from a table by thief level adjusted by race
 * and by +7f; the class bits; and the experience a class level costs.
 *
 * EXE 0x061cfe..0x062c2c, one Turbo C large-model _TEXT segment starting at
 * byte e of paragraph 304e, the last module of RTLink section 11 (payload
 * 0x5cbb0, link segment 2b3a, no parent section).  The first e bytes of
 * paragraph 304e are the tail of module 2f0d, and zeros follow to the
 * section's end at 0x62c30.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Sections 9's and 10's 2b3a link segments hold other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the routine lives in.  Code outside the section
 * reaches six of these routines through vector stubs: 1d99:1445 -> 000f,
 * 1d99:1477 -> 01e7, 1d99:13af -> 07f1, 1d99:1459 -> 0af3, 1d99:144f ->
 * 0c72, 1d99:0f6d -> 0e9c.
 *
 * The module has no _DATA and no _BSS.
 */

#include <stdlib.h>
#include "dqk.h"

void far recalc_thief_skills(struct Character far *character);
void far recalc_saves(struct Character far *character);

/* The translated Pascal unit's empty initialization block (DKK classcal.pas). */
void far herostat_unit_init(void)                                   /* 304e:000e */
{
}

void far recalc_cleric_slots(struct Character far *character)                     /* 304e:000f */
{
    unsigned char index;
    unsigned char lvl;
    short class_row;

    lvl = class_level(character, 0);
    if (lvl > 0) {
        if (character->deity <= 4)
            class_row = 0;
        else
            class_row = 1;
        for (index = 2; index <= 7; index++)
            character->slots[0][index - 1] = 0;
        if (class_row == 0)
            character->slots[0][0] = 1;
        else
            character->slots[0][0] = 2;
        if (lvl > 29)
            lvl = 29;
        for (index = 1; index <= 7; index++)
            if (class_spell_rows[class_row] == 0xff)
                character->slots[0][index - 1] = 0;
            else
                character->slots[0][index - 1] = spell_slots_by_level[class_spell_rows[class_row]][lvl - 1][index - 1];
        if (character->stats[2].cur > 12 && character->slots[0][0] > 0)
            character->slots[0][0]++;
        if (character->stats[2].cur > 13 && character->slots[0][0] > 0)
            character->slots[0][0]++;
        if (character->stats[2].cur > 14 && character->slots[0][1] > 0)
            character->slots[0][1]++;
        if (character->stats[2].cur > 15 && character->slots[0][1] > 0)
            character->slots[0][1]++;
        if (character->stats[2].cur > 16 && character->slots[0][2] > 0)
            character->slots[0][2]++;
        if (character->stats[2].cur > 17 && character->slots[0][3] > 0)
            character->slots[0][3]++;
        if (character->stats[2].cur < 17)
            character->slots[0][5] = 0;
        if (character->stats[2].cur < 18)
            character->slots[0][6] = 0;
    }
}

void far limit_mage_slots(struct Character far *character)                     /* 304e:01a2 */
{
    if (character->stats[1].cur < 12)
        character->slots[2][5] = 0;
    if (character->stats[1].cur < 14)
        character->slots[2][6] = 0;
    if (character->stats[1].cur < 16)
        character->slots[2][7] = 0;
    if (character->stats[1].cur < 18)
        character->slots[2][8] = 0;
}

void far recalc_spells(struct Character far *character)                     /* 304e:01e7 */
{
    short inner_index;
    short class_row;
    unsigned char lvl;
    unsigned char bonus_spell_level;
    unsigned char index;
    unsigned char scan_index;
    struct Item far *item;

    mem_fill(character->slots, 0x24, 0);            /* +167..+18a */
    for (inner_index = 0; inner_index <= 7; inner_index++) {
        lvl = class_level(character, inner_index);
        if (lvl > 29)
            lvl = 29;
        if (lvl > 0)
            switch (inner_index) {
            case 0:
                if (character->deity <= 4)
                    class_row = 0;
                else
                    class_row = 1;
                character->slots[0][0]++;
                for (index = 1; index <= 7; index++)
                    if (class_spell_rows[class_row] != 0xff)
                        character->slots[0][index - 1] += spell_slots_by_level[class_spell_rows[class_row]][lvl - 1][index - 1];
                recalc_cleric_slots(character);
                for (index = 1; index <= 7; index++)
                    if (character->slots[0][index - 1] > 0)
                        for (tick_slot = 1, scan_index = 0xff; tick_slot <= 10 && scan_index != 0;
                             tick_slot++) {
                            scan_index = spells_by_level[0][index - 1][tick_slot - 1];
                            if (scan_index != 0)
                                character->known[(scan_index - 1) / 8] =
                                    character->known[(scan_index - 1) / 8] | bit_mask[(scan_index - 1) % 8];
                        }
                break;
            case 3:
                if (lvl > 8) {
                    for (index = 1; index <= 4; index++)
                        character->slots[0][index - 1] += spell_slots_by_level[class_spell_rows[10]][lvl - 1][index - 1];
                    for (scan_index = 1; scan_index <= 0x7e; scan_index++)
                        if ((spell_info - 1)[scan_index].cast_class == 0
                            && character->slots[0][(spell_info - 1)[scan_index].level - 1] > 0)
                            character->known[(scan_index - 1) / 8] |= bit_mask[(scan_index - 1) % 8];
                }
                break;
            case 1:
                if (lvl > 5 && character->knight_order != 1) {
                    if (character->knight_order == 2)
                        class_row = 4;
                    else
                        class_row = 5;
                    for (index = 1; index <= 7; index++) {
                        if (class_spell_rows[class_row] != 0xff)
                            character->slots[0][index - 1] += spell_slots_by_level[class_spell_rows[class_row]][lvl - 1][index - 1];
                        if (character->slots[0][index - 1] > 0)
                            for (tick_slot = 1, scan_index = 0xff; tick_slot <= 10 && scan_index != 0;
                                 tick_slot++) {
                                scan_index = spells_by_level[0][index - 1][tick_slot - 1];
                                if (scan_index != 0)
                                    character->known[(scan_index - 1) / 8] =
                                        character->known[(scan_index - 1) / 8] | bit_mask[(scan_index - 1) % 8];
                            }
                    }
                }
                break;
            case 4:
                if (lvl > 7) {
                    for (index = 1; index <= 4; index++)
                        character->slots[1][index - 1] += spell_slots_by_level[class_spell_rows[6]][lvl - 1][index - 1];
                    for (index = 5; index <= 8; index++) {
                        bonus_spell_level = index - 4;
                        character->slots[2][bonus_spell_level - 1] += spell_slots_by_level[class_spell_rows[6]][lvl - 1][index - 1];
                    }
                }
                for (scan_index = 1; scan_index <= 0x7e; scan_index++)
                    if ((spell_info - 1)[scan_index].cast_class == 1
                        && character->slots[1][(spell_info - 1)[scan_index].level - 1] > 0
                        || (spell_info - 1)[scan_index].cast_class == 2
                        && character->slots[2][(spell_info - 1)[scan_index].level - 1] > 0) {
                        lvl = (spell_info - 1)[scan_index].level;
                        if (character->slots[1][lvl - 1] > 0)
                            character->known[(scan_index - 1) / 8] =
                                character->known[(scan_index - 1) / 8] | bit_mask[(scan_index - 1) % 8];
                    }
                break;
            case 5:
                if (character->robe_order == 1)
                    class_row = 7;
                else
                    class_row = 8;
                character->slots[2][0]++;
                for (index = 1; index <= 9; index++) {
                    if (class_spell_rows[class_row] != 0xff)
                        character->slots[2][index - 1] += spell_slots_by_level[class_spell_rows[class_row]][lvl - 1][index - 1];
                    limit_mage_slots(character);
                }
            }
    }
    for (item = character->items; item != 0; item = item->next)
        if (item->rec.u.magic.special == 0x81 && item->rec.equipped != 0) {
            character->slots[2][4] += character->slots[2][4];
            break;
        }
}

void far recalc_from_levels(struct Character far *character)                     /* 304e:07f1 */
{
    struct Character far *saved;
    short class_index;
    unsigned char attack_level;
    unsigned char lvl;
    short equipment_slot;

    character->thac0_base = 0;
    for (class_index = 0; class_index <= 6; class_index++) {
        lvl = class_level(character, class_index);
        if (lvl > 21)
            attack_level = 21;
        else
            attack_level = lvl;
        if (attack_scores_by_level[class_index][attack_level] > character->thac0_base)
            character->thac0_base = attack_scores_by_level[class_index][attack_level];
        if (character->level_high < lvl)
            character->level_high = lvl;
        switch (class_index) {
        case 1:
        case 2:
        case 3:
            if (lvl > 6)
                character->attack_rate[0] = 3;
            if (lvl > 12)
                character->attack_rate[0] = 4;
            break;
        case 4:
            if (lvl > 7)
                character->attack_rate[0] = 3;
            if (lvl > 14)
                character->attack_rate[0] = 4;
            break;
        }
    }
    if (character->deity == 6 && (character->race == 3 || character->race == 4) || character->deity == 3)
        character->thac0_base++;
    recalc_spells(character);
    recalc_saves(character);
    if (character->level[6] > 0)
        recalc_thief_skills(character);
    character->class_mask = 0;
    for (class_index = 0; class_index <= 6; class_index++)
        if (character->level[class_index] > 0 || character->old_level[class_index] > 0 && character->old_level[class_index] < character->level_high)
            character->class_mask += class_mask_bits[class_index];
    for (equipment_slot = 0; equipment_slot <= 12; equipment_slot++)
        if (character->equipped[equipment_slot] != 0
            && (item_table[character->equipped[equipment_slot]->rec.type].class_mask & character->class_mask) == 0
            && character->equipped[equipment_slot]->rec.cursed == 0 && character->equipped[equipment_slot]->rec.equipped != 0) {
            saved = cur_node;
            cur_node = character;
            item_equip_toggle(character->equipped[equipment_slot]);
            cur_node = saved;
        }
    if (old_class_abilities_available(character))
        for (class_index = 0; class_index <= 6; class_index++) {
            lvl = character->old_level[class_index];
            switch (class_index) {
            case 2:
            case 3:
                if (lvl >= 13)
                    character->attack_rate[0] = 4;
                else if (lvl >= 7)
                    character->attack_rate[0] = 3;
                break;
            case 4:
                if (lvl >= 15)
                    character->attack_rate[0] = 4;
                else if (lvl >= 8)
                    character->attack_rate[0] = 3;
                break;
            case 6:
                if (lvl > 0)
                    recalc_thief_skills(character);
            }
        }
}

void far recalc_thief_skills(struct Character far *character)                     /* 304e:0af3 */
{
    unsigned char stat_index;
    unsigned char skill_bonus;
    unsigned char lvl;
    unsigned char save;

    skill_bonus = 0;
    lvl = class_level(character, 6);
    if (lvl > 18)
        lvl = 18;
    save = lvl;
    for (stat_index = 1; stat_index <= 8; stat_index++) {
        if ((thief_race_adjust[character->race][stat_index - 1] < 0)
            & (abs(thief_race_adjust[character->race][stat_index - 1]) + skill_bonus > thief_skill_base[lvl - 1][stat_index - 1]))
            character->thief[stat_index - 1] = 0;
        else {
            character->thief[stat_index - 1] = thief_skill_base[lvl - 1][stat_index - 1] + thief_race_adjust[character->race][stat_index - 1] + skill_bonus;
            if (stat_index < 6)
                character->thief[stat_index - 1] = thief_dex_adjust[character->stats[3].cur - 9][stat_index - 1] + character->thief[stat_index - 1];
        }
        lvl = save;
    }
}

void far recalc_saves(struct Character far *character)                     /* 304e:0c72 */
{
    short save_type;
    short inner_index;
    unsigned char item_special;
    struct Item far *item;
    char flag;
    unsigned char save_level;
    unsigned char lvl;

    flag = 0;
    for (item = character->items; item != 0 && flag == 0; item = item->next)
        if (item->rec.u.magic.special > 0x80 && item->rec.equipped != 0) {
            item_special = item->rec.u.magic.special & 0x7f;
            flag = item_special == 6;
        }
    for (save_type = 0; save_type <= 4; save_type++) {
        character->saves[save_type] = 20;
        for (inner_index = 0; inner_index <= 6; inner_index++) {
            lvl = class_level(character, inner_index);
            if (lvl > 0) {
                if (lvl > 21)
                    save_level = 21;
                else
                    save_level = lvl;
                if (saving_throws[inner_index][save_level][save_type] < character->saves[save_type])
                    character->saves[save_type] = saving_throws[inner_index][save_level][save_type];
                if (save_type == 0
                    && (character->race == 3 || character->race == 5 || character->race == 4 || flag != 0))
                    switch (character->stats[4].cur) {
                    case 4:
                    case 5:
                    case 6:
                        character->saves[save_type]++;
                        break;
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                        character->saves[save_type] += 2;
                        break;
                    case 11:
                    case 12:
                    case 13:
                        character->saves[save_type] += 3;
                        break;
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                        character->saves[save_type] += 4;
                        break;
                    case 18:
                        character->saves[save_type] += 5;
                    }
                if (save_type == 0)
                    switch (character->stats[4].cur) {
                    case 19:
                    case 20:
                        character->saves[save_type]++;
                        break;
                    case 21:
                    case 22:
                        character->saves[save_type] += 2;
                        break;
                    case 23:
                    case 24:
                        character->saves[save_type] += 3;
                        break;
                    case 25:
                        character->saves[save_type] += 4;
                    }
            }
        }
    }
}

long far level_xp_threshold(struct Character far *character, unsigned char level, short cls)
                                                                 /* 304e:0e9c */
{
    long xp_threshold;

    xp_threshold = 0x7fffffffL;
    cls = class_progression_row(character, cls);
    if (level < 41)
        if (level < 18)
            xp_threshold = xp_by_level[cls][level];
        else
            xp_threshold = (level - 17) * high_level_xp_step[cls] + xp_by_level[cls][17];
    return xp_threshold;
}
