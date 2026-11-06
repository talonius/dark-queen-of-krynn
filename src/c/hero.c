/* DQK.EXE resident module 177e: the party member record.
 * Deriving a member's combat figures from ability scores and readied
 * items (to-hit, damage dice, armour class, movement and load), the
 * ability-score tables behind them, item names and the item list,
 * effects, hit-point loss, the character panel and name line, caster
 * and class levels, readiness to train, freeing and dropping a member,
 * and redrawing the view for the current view mode.
 *
 * EXE 0x009beb..0x00be49, one Turbo C large-model _TEXT segment starting
 * at byte 0xb of paragraph 177e.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Ghidra's index misses view_redraw, which 27 far calls reach, and
 * runs con_hp_adj's body out through its jump table into frame 10d7 and
 * caster_level's into frames 100c and 1bdb.
 * Nothing found calls item_make (1bdb).
 *
 * _DATA is DGROUP 021c..028a: 16 stored literals; -d stores " " as the
 * tail of " No\t " and "%s" as the tail of "%s%s".  _BSS is worn_load and
 * frame_flag at a2aa..a2ac; no code outside this module refers to either.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is, so every
 * routine called ahead of its definition carries a prototype.
 */

#include <stdlib.h>
#include "dqk.h"

void far hero_text(struct Character far *character, char far *text, char row, char pause);
char far is_helpless(struct Character far *character);
char far dex_defense(struct Character far *character);
char far dex_missile(struct Character far *character);
char far str_hit(struct Character far *character);
char far str_damage(struct Character far *character);
int  far str_carry(struct Character far *character);
void far combat_text_clear(void);
void far character_name_show(struct Character far *character, char col, char row, char poss);
char far find_effect(struct Character far *character, unsigned char kind, struct Effect far * far *out);
unsigned char far first_level(struct Character far *character);
unsigned char far class_level(struct Character far *character, int cls);

static int worn_load;                                            /* a2aa */
static char frame_flag;                                          /* a2ac */

void far hero_frame_flag_set(char redraw_flag)                                    /* 177e:000b */
{
    frame_flag = redraw_flag;
}

void far weapon_stats(struct Character far *character)                        /* 177e:0016 */
{
    struct Item far *item;
    unsigned char type;
    char bonus;

    item = character->equipped[0];
    if (item == 0)
        return;
    type = item->rec.type;
    character->thac0_adj = character->thac0_base;
    if (item_table[type].flags & 2)
        character->thac0_adj = character->thac0_adj + dex_missile(character);
    character->mods[0] = item_table[type].mod;
    if (item_table[type].flags & 4) {
        character->thac0_adj = character->thac0_adj + str_hit(character);
        character->mods[0] = character->mods[0] + str_damage(character);
    }
    bonus = item->rec.plus;
    if (item_table[type].flags & 0x80 && character->equipped[12] != 0)
        bonus = character->equipped[12]->rec.plus + bonus;
    if ((item_table[type].flags & 1) > 0 && character->equipped[11] != 0)
        bonus = character->equipped[11]->rec.plus + bonus;
    character->mods[0] = character->mods[0] + bonus;
    if (character->race == 0 || character->race == 1) {
        if (item->rec.type >= 0x15 && item->rec.type <= 0x1b || item->rec.type == 0x13 || item->rec.type == 0x12)
            bonus++;
    }
    if (character->race == 5 && item->rec.type == 0x43)
        bonus += 2;
    character->thac0_adj = character->thac0_adj + bonus;
    character->rolls[0] = item_table[type].rolls;
    character->dice[0] = item_table[type].dice;
}

void far armor_move(struct Character far *character, struct Item far *item)     /* 177e:01ef */
{
    if (item_table[item->rec.type].slot == 2) {
        if (item->rec.weight < 0 || item->rec.weight > 0x18f)
            character->movement = 6;
        else if (item->rec.weight <= 0x96)
            character->movement = character->move_base;
        else
            character->movement = 9;
        if (item->rec.plus != 0 && character->movement <= 9)
            character->movement = character->movement + 3;
    }
}

void far item_ac(struct Character far *character, struct Item far *item, char far *armor_modifiers,
               char far *flag)                                   /* 177e:026e */
{
    unsigned char bonus;
    short kind;

    bonus = item_table[item->rec.type].armor_bonus;
    if (!(bonus & 0x80))
        return;
    bonus = bonus & 0x7f;
    kind = item_table[item->rec.type].slot;
    if (kind == 1)
        armor_modifiers[1] = item->rec.plus + bonus;
    else if (bonus == 0) {
        if (kind == 9) {
            if (item->rec.plus > armor_modifiers[3])
                armor_modifiers[3] = item->rec.plus;
        } else if (kind == 7)
            armor_modifiers[4] = item->rec.plus;
        else
            armor_modifiers[2] = armor_modifiers[2] + item->rec.plus;
        character->save_bonus = character->save_bonus + item->rec.save_bonus;
    } else if (item->rec.plus + bonus > armor_modifiers[5]) {
        armor_modifiers[5] = item->rec.plus + bonus;
        if (item->rec.plus > 0 && kind == 2)
            *flag = 1;
    }
}

void far load_move(struct Character far *character)                           /* 177e:038c */
{
    char lim;
    register int load;

    load = character->encumbrance - str_carry(character);
    if (load < 0)
        load = 0;
    if (load < 0 || load > 0x400)
        lim = 3;
    else if (load <= 0x200)
        lim = character->movement;
    else if (load <= 0x300)
        lim = 9;
    else
        lim = 6;
    if (character->movement > lim)
        character->movement = lim;
}

void far item_name(struct Character far *character, struct Item far *item, char col, char row,
               char full, char show)                             /* 177e:03fc */
{
    unsigned char index;
    unsigned char mask;
    struct Effect far *effect;
    char found;
    char plural;
    struct Character far *node;
    char buf[42];

    item->name[0] = 0;
    if (full) {
        if (item->rec.equipped)
            str_copy(item->name, " Yes\t");
        else
            str_copy(item->name, " No\t ");
    }
    found = 0;
    for (node = node_list; node != 0 && found == 0; node = node->next)
        if (find_effect(node, 5, &effect))
            found = 1;
    if (found && (item->rec.plus > 0 || item->rec.save_bonus > 0 || item->rec.cursed != 0))
        str_cat(item->name, "* ");
    if (item->rec.quantity > 0 && item->rec.type != 0x49) {
        str_cat(item->name, byte_str(item->rec.quantity));
        str_cat(item->name, " ");
    }
    mask = 0;
    for (index = 1; index <= 3; index++)
        if (item->rec.name_ids[index - 1] != 0 && !((item->rec.unid_mask >> (3 - index)) & 1))
            mask += 1 << (index - 1);
    plural = 0;
    for (index = 3; index >= 1; index--) {
        if (((mask >> (index - 1)) & 1) > 0) {
            if (item->rec.type == 0x49 && index == 2) {
                str_copy(buf, msg_format("%d", item->rec.quantity));
                str_copy(item->name, msg_format("%s%s", item->name, buf));
            } else
                str_copy(item->name, msg_format("%s%s", item->name, item_words[item->rec.name_ids[index - 1]]));
            if (item->rec.quantity >= 2 && !plural &&
                ((1 << (index - 1)) == mask ||
                 index == 1 && mask > 4 ||
                 index == 1 && item->rec.type == 0x49 ||
                 index == 2 && !pool_lowbit(mask) ||
                 index == 3 && (item->rec.type == 0x37 || item->rec.type == 0x1e || item->rec.type == 5 ||
                            item->rec.type == 7 || item->rec.type == 0xc) && item->rec.name_ids[2] != 0x5d)) {
                str_cat(item->name, "s ");
                plural = 1;
            } else
                str_cat(item->name, " ");
        }
    }
    if (show)
        status_printf(col, row, 7, 0, item->name);
}

void far show_ac(struct Character far *character, char col, char row)         /* 177e:0724 */
{
    char buf[0x52];
    register int armor_class;

    armor_class = character->ac[0] - 0x3c;
    str_copy(buf, byte_str(abs(character->ac[0] - 0x3c)));                  /* Turbo C's inline abs: cwd, xor, sub */
    str_cat(buf, " ");
    if (armor_class > 0)
        str_copy(buf, msg_format("-%s", buf));
    status_printf(col, row, 7, 0, buf);
}

void far show_hp(struct Character far *character, char col, char row, char hilite,
               char max)                                         /* 177e:07b4 */
{
    char color;
    char len;
    char buf[42];

    if (character->hp < character->hp_max)
        color = 0xb;
    else
        color = 7;
    if (hilite)
        color = 0xb;
    str_copy(buf, byte_str(character->hp));
    status_printf(col, row, color, 0, buf);
    if (max) {
        len = str_length(buf);
        status_printf(col + len, row, 7, 0, msg_format("/%d", character->hp_max));
    }
}

void far hero_panel(struct Character far *character)                          /* 177e:085f */
{
    unsigned char row;

    if (!panel_dirty)
        return;
    panel_dirty = 0;
    text_clear(0x17, 1, 0x26, 0x15);
    row = 1;
    hero_text(character, " ", row, 0);
    row++;
    status_printf(0x17, row + 1, 7, 0, "Hitpoints");
    show_hp(character, 0x21, row + 1, 0, 0);
    row += 2;
    status_printf(0x17, row + 1, 7, 0, "AC");
    show_ac(character, 0x1a, row + 1);
    text_row = row + 1;
    if (character->equipped[0] != 0) {
        row += 2;
        item_name(character, character->equipped[0], 0, 0, 0, 0);
        text_wrap(0x17, row + 1, 0x26, row + 3, 7, 0, 1, character->equipped[0]->name);
    }
    row = text_row + 1;
    if (character->enabled == 0)
        status_printf(0x17, row + 1, 0xb, 0, status_names[character->status]);
    else if (is_helpless(character))
        status_printf(0x17, row + 1, 0xb, 0, "(Helpless)");
    else if (character->combat->spell_cast > 0)
        status_printf(0x17, row + 1, 0xb, 0, "(Casting)");
}

char far is_helpless(struct Character far *character)                         /* 177e:0a19 */
{
    char res;
    unsigned char index;
    struct Effect far *effect;

    res = 0;
    for (index = 0; index < 4; index++)
        if (find_effect(character, helpless_kinds[index], &effect))
            res = 1;
    return res;
}

unsigned char far calc_move(struct Character far *character)                  /* 177e:0a5f */
{
    unsigned char ret;
    struct Item far *item;
    struct Effect far *effect;

    character->movement = character->move_base;
    for (item = character->items; item != 0; item = item->next)
        if (item->rec.equipped)
            armor_move(character, item);
    load_move(character);
    if (find_effect(character, 0x27, &effect))
        character->movement = character->movement * 2;
    if (find_effect(character, 0x2a, &effect))
        character->movement = character->movement / 2;
    if (find_effect(character, 0x4a, &effect))
        character->movement = character->movement * 2;
    attack_rate = character->movement;
    if (character->side == 0) {
        if ((char)game_state[STATE_PARTY_MOVE_BONUS] + attack_rate < 1)            /* (char): cbw */
            attack_rate = 1;
        else
            attack_rate = (char)game_state[STATE_PARTY_MOVE_BONUS] + attack_rate;
    }
    attack_rate = attack_rate * 2;
    if (game_state[STATE_UNDERWATER] != 0 && !find_effect(character, 0x96, &effect))
        attack_rate = attack_rate / 3;
    if (find_effect(character, 0xb2, &effect))
        attack_rate = 0;
    ret = attack_rate;
    return ret;
}

void far recalc_stats(struct Character far *character)                     /* 177e:0be2, proposed hero_recalc */
{
    unsigned char index;
    short modifier_index;
    struct Item far *item;
    char flag;
    char mods[6];
    short inner_index;
    short scan_index;
    unsigned short item_weight;
    unsigned char kind;

    for (inner_index = 0; inner_index <= 0xc; inner_index++)
        character->equipped[inner_index] = 0;
    character->item_count = 0;
    item = character->items;
    character->hands = 0;
    character->encumbrance = 0;
    worn_load = 0;
    for (; item != 0; item = item->next) {
        character->item_count++;
        item_weight = item->rec.weight;
        if (item->rec.quantity > 0)
            item_weight = item->rec.quantity * item_weight;
        character->encumbrance += item_weight;
        if (item->rec.equipped) {
            worn_load += item_weight;
            kind = item_table[item->rec.type].slot;
            switch (kind) {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
                character->equipped[kind] = item;
                break;
            case 9:
                if (character->equipped[9] != 0) {
                    if (character->equipped[10] == 0)
                        character->equipped[10] = item;
                } else
                    character->equipped[9] = item;
                break;
            }
            if (item->rec.type == 0x1e)
                character->equipped[11] = item;
            if (item->rec.type == 0xc || item->rec.type == 0x4a)
                character->equipped[12] = item;
            character->hands = character->hands + item_table[item->rec.type].hands;
        }
    }
    for (scan_index = 0; scan_index <= 2; scan_index++)
        character->encumbrance = character->encumbrance + character->coins[scan_index];
    for (index = 1; index <= 2; index++) {
        character->rolls[index - 1] = character->unarmed_rolls[index - 1];
        character->dice[index - 1] = character->unarmed_dice[index - 1];
        character->mods[index - 1] = character->unarmed_mods[index - 1];
    }
    mem_zero(mods, 6);
    flag = 0;
    character->save_bonus = 0;
    character->ac[0] = character->ac_base;
    character->movement = character->move_base;
    character->thac0_adj = character->thac0_base;
    mods[0] = dex_defense(character);
    if (character->equipped[0] == 0) {
        character->thac0_adj = character->thac0_adj + str_hit(character);
        character->mods[0] = character->mods[0] + str_damage(character);
    }
    weapon_stats(character);
    for (item = character->items; item != 0; item = item->next)
        if (item->rec.equipped) {
            armor_move(character, item);
            item_ac(character, item, mods, &flag);
        }
    if (flag) {
        if (mods[3] > 0)
            mods[3] = 0;
        if (mods[4] > 0 && (character->equipped[7] == 0 || character->equipped[7]->rec.type != 0x3a))
            mods[4] = 0;
    }
    character->movement = calc_move(character) / 2;
    if (character->ac[0] > mods[5])
        mods[5] = character->ac[0];
    character->ac[0] = 0;
    for (modifier_index = 0; modifier_index <= 5; modifier_index++)
        character->ac[0] = character->ac[0] + mods[modifier_index];
    character->ac[1] = mods[5] + mods[2] + mods[4] + mods[3] - 2;
}

char far dex_defense(struct Character far *character)                         /* 177e:0fbd */
{
    char result;
    unsigned char dex;

    dex = character->stats[3].cur;
    if (dex >= 1 && dex <= 3)
        result = -4;
    else if (dex >= 4 && dex <= 6)
        result = dex - 7;
    else if (dex >= 15 && dex <= 18)
        result = dex - 14;
    else if (dex >= 0x13 && dex <= 0x14)
        result = 4;
    else if (dex >= 0x15 && dex <= 0x17)
        result = 5;
    else if (dex >= 0x18 && dex <= 0x19)
        result = 6;
    else
        result = 0;
    return result;
}

char far dex_missile(struct Character far *character)                         /* 177e:104d */
{
    char result;
    unsigned char dex;

    dex = character->stats[3].cur;
    if (dex < 0 || dex > 0x19)                /* unsigned < 0: emitted, never true */
        result = 0;
    else if (dex <= 2)
        result = -4;
    else if (dex <= 5)
        result = dex - 6;
    else if (dex <= 15)
        result = 0;
    else if (dex <= 0x12)
        result = dex - 15;
    else if (dex <= 0x14)
        result = 3;
    else if (dex <= 0x17)
        result = 4;
    else
        result = 5;
    return result;
}

unsigned char far str_rank(struct Character far *character)                  /* 177e:10c4 */
{
    unsigned char result;
    unsigned char str;
    unsigned char ext;

    str = character->stats[0].cur;
    if (str >= 0 && str <= 0x11)
        result = character->stats[0].cur;
    else if (str == 0x12) {
        ext = character->str_ex;
        if (ext >= 0) {
            if (ext == 0)
                result = 0x12;
            else if (ext <= 0x32)
                result = 0x13;
            else if (ext <= 0x4b)
                result = 0x14;
            else if (ext <= 0x5a)
                result = 0x15;
            else if (ext <= 0x63)
                result = 0x16;
            else if (ext == 0x64)
                result = 0x17;
        }
    } else if (str >= 0x13 && str <= 0x19)
        result = character->stats[0].cur + 5;
    return result;                                 /* unset past str 0x19 or str_ex 0x64, as shipped */
}

char far str_hit(struct Character far *character)                             /* 177e:1168 */
{
    char result;
    unsigned char strength_rank;

    result = 0;
    strength_rank = str_rank(character);
    if (strength_rank > 0) {
        if (strength_rank <= 3)
            result = -3;
        else if (strength_rank <= 5)
            result = -2;
        else if (strength_rank <= 7)
            result = -1;
        else if (strength_rank > 0x10) {
            if (strength_rank <= 0x13)
                result = 1;
            else if (strength_rank <= 0x16)
                result = 2;
            else if (strength_rank <= 0x19)
                result = 3;
            else if (strength_rank <= 0x1b)
                result = 4;
            else if (strength_rank <= 0x1e)
                result = strength_rank - 0x17;
        }
    }
    return result;
}

char far str_damage(struct Character far *character)                          /* 177e:11f9 */
{
    char result;
    unsigned char strength_rank;

    result = 0;
    strength_rank = str_rank(character);
    if (strength_rank > 0) {
        if (strength_rank <= 2)
            result = -2;
        else if (strength_rank <= 5)
            result = -1;
        else if (strength_rank > 0xf) {
            if (strength_rank == 0x10)
                result = 1;
            else if (strength_rank <= 0x13)
                result = strength_rank - 0x10;
            else if (strength_rank <= 0x1d)
                result = strength_rank - 0x11;
            else if (strength_rank == 0x1e)
                result = 0xe;
        }
    }
    return result;
}

int far str_carry(struct Character far *character)                            /* 177e:1273 */
{
    unsigned char strength_rank;
    register int result;

    strength_rank = str_rank(character);
    if (strength_rank > 0) {
        if (strength_rank <= 3)
            result = -350;
        else if (strength_rank <= 5)
            result = -250;
        else if (strength_rank <= 7)
            result = -150;
        else if (strength_rank <= 0xb)
            result = 0;
        else if (strength_rank <= 0xd)
            result = 100;
        else if (strength_rank <= 0xf)
            result = 200;
        else if (strength_rank <= 0x10)
            result = 350;
        else if (strength_rank <= 0x15)
            result = (strength_rank - 0x11) * 250 + 500;
        else if (strength_rank <= 0x1a)
            result = (strength_rank - 0x16) * 1000 + 2000;
        else if (strength_rank <= 0x1b)
            result = 7500;
        else if (strength_rank <= 0x1e)
            result = (strength_rank - 0x1c) * 3000 + 9000;
        else
            result = 0;
    }
    return result;                                 /* unset for s == 0, as shipped */
}

void far spell_mark(struct Character far *character, unsigned char spell)     /* 177e:1346 */
{
    unsigned char spell_slot;

    for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
        if (character->spells[spell_slot] == spell) {
            character->spells[spell_slot] = spell | 0x80;
            break;
        }
}

char far has_ready_8c(struct Character far *character)                        /* 177e:1389 */
{
    char found;
    struct Item far *item;

    found = 0;
    for (item = character->items; item != 0; item = item->next)
        if (item->rec.u.magic.effect == 0x8c && item->rec.equipped)
            found = 1;
    return found;
}

void far item_remove(struct Character far *character, struct Item far *item)    /* 177e:13d2 */
{
    struct Item far *extra_item;

    if (item->rec.type == 0x49)
        while (item->more != 0) {
            extra_item = item->more;
            item->more = extra_item->more;
            rec_free((void far * far *)&extra_item);
        }
    extra_item = character->items;
    if (extra_item == item)
        character->items = item->next;
    else
        while (extra_item != 0 && extra_item->next != item)
            extra_item = extra_item->next;
    extra_item->next = item->next;                       /* p is 0 when it is not listed, as shipped */
    item->next = 0;
    rec_free((void far * far *)&item);
}

void far item_append(struct Character far *character, struct Item far *src)   /* 177e:14aa */
{
    struct Item far *item;
    struct Item far *new_item;

    pool_alloc((void far * far *)&new_item, 0x3d, &item_record_pool);
    mem_move5(src, new_item, 0x3d);
    new_item->next = 0;
    item = character->items;
    if (item == 0)
        character->items = new_item;
    else {
        while (item->next != 0)
            item = item->next;
        item->next = new_item;
    }
}

void far bottom_msg(char far *msg)                            /* 177e:153c, proposed bottom_msg */
{
    box_draw();
    status_printf(0, 0x18, 0xf, 8, "%s", msg);
    msg_pause();
    box_draw();
}

void far hero_text(struct Character far *character, char far *text, char row, char pause) /* 177e:156e */
{
    char top;
    char none;

    if (character == 0)
        none = 1;
    else
        none = 0;
    if (view_mode == 5) {
        text_clear(0x17, row, 0x26, 0x15);
        if (!none)
            character_name_show(character, 0x17, row, 0);
        text_wrap(0x17, row + 1, 0x26, 0x15, 7, 0, 1, text);
    } else {
        if (resting)
            top = 0x12;
        else
            top = 0x11;
        text_clear(1, top, 0x26, 0x16);
        if (!none)
            character_name_show(character, 1, top + 1, 0);
        text_wrap(1, top + 2, 0x26, 0x16, 7, 0, 1, text);
    }
    if (pause) {
        msg_pause();
        combat_text_clear();
    }
}

void far combat_text_clear(void)                                      /* 177e:1665 */
{
    if (view_mode == 5)
        text_clear(0x17, 0xa, 0x26, 0x15);
    else
        text_clear(1, 0x12, 0x26, 0x16);
}

void far character_name_show(struct Character far *character, char col, char row, char poss) /* 177e:1697, proposed hero_name */
{
    char color;

    if (character->enabled == 0)
        color = 0;
    else if (character->side == 1 && view_mode == 5)
        color = 2;
    else if (character->level_up != 0)
        color = 0xd;
    else
        color = 7;
    status_printf(col, row, color, 0, "%s", character->name);
    if (poss)
        status_printf(col + str_length(character->name), row, color, 0, "'s");
}

char far find_effect(struct Character far *character, unsigned char kind, struct Effect far * far *out) /* 177e:1734 */
{
    char ret;
    char found;

    found = 0;
    *out = character->effects;
    while (*out != 0 && found == 0) {
        if ((*out)->id == kind)
            found = 1;
        else
            *out = (*out)->next;
    }
    ret = found;
    return ret;
}

void far hero_damage(struct Character far *character, unsigned short dmg)     /* 177e:17a0 */
{
    unsigned short over;
    unsigned char hp;

    over = 0;
    hp = 0;
    if (character->hp >= dmg)
        hp = character->hp - dmg;
    else
        over = dmg - character->hp;
    if (over > 9 || hp == 0 && character->status == 1)
        character->status = 6;
    else if (over > 0) {
        character->status = 5;
        if (view_mode == 5)
            character->combat->dying_turns = over;
    } else if (hp == 0)
        character->status = 4;
    if (character->status != 0 && character->status != 1) {
        character->enabled = 0;
        character->hp = 0;
        if (view_mode == 5) {
            side_count[character->side]--;
            character->combat->initiative = 0;
        }
    } else
        character->hp = hp;
}

void far heal_msg(struct Character far *character)                            /* 177e:187a */
{
    char buf[42];

    if (character->hp == character->hp_max)
        str_copy(buf, "is fully healed");
    else
        str_copy(buf, "is partially healed");
    hero_text(character, buf, 10, 1);
    if (view_mode != 5)
        party_rows(character, character == cur_node);
}

unsigned char far caster_level(unsigned char spell, char cap)    /* 177e:1900 */
{
    unsigned short moon_state_index;
    signed char ret;
    signed char casting_level;
    signed char alternate_level;

    if (class_level(cur_node, 0) == 0 && class_level(cur_node, 5) == 0 &&
        class_level(cur_node, 4) < 8 && class_level(cur_node, 1) < 9)
        casting_level = 6;
    else
        switch ((spell_info - 1)[spell].cast_class) {     /* r unset past kind 4, as shipped */
        case 0: case 3:
            casting_level = class_level(cur_node, 0);
            alternate_level = class_level(cur_node, 1);
            if (casting_level < alternate_level)
                casting_level = alternate_level;
            break;
        case 1:
            casting_level = class_level(cur_node, 4) - 7;
            if (casting_level < 0)
                casting_level = 0;
            break;
        case 2:
            casting_level = class_level(cur_node, 5);
            alternate_level = class_level(cur_node, 4) - 8;
            if (cur_node->level[5] > 0 && cur_node->robe_order > 0) {
                moon_state_index = cur_node->robe_order + (STATE_MOON_PHASE_1 - 2);  /* unsigned: at + 1 is inc ax */
                switch (game_state[moon_state_index + 1]) {
                case 0:
                    casting_level--;
                    break;
                case 2:
                    if (cur_node->level[5] > 5)
                        casting_level++;
                    break;
                }
            }
            if (casting_level < alternate_level)
                casting_level = alternate_level;
            break;
        case 4:
            casting_level = 0xc;
            break;
        }
    if (item_cast_active && (spell_info - 1)[spell].cast_class != 4)
        casting_level = 6;
    if (cap && casting_level > 0x1f)
        casting_level = 0x1f;
    ret = casting_level;
    return ret;
}

void far view_redraw(void)                                       /* 177e:1ad1 */
{
    redraw_wanted = 1;
    switch (view_mode) {
    case 0:
        frame_border();
        break;
    case 1: case 10:
        if (shop_temple_frame)
            frame_main();
        pic_frame(3, 3, 1, 0);
        party_display(cur_node);
        party_info_display();
        break;
    case 2:
        frame_main();
        pic_open("PIC", 0x80, 0, &pic);
        pic_frame(3, 3, 1, 0);
        party_display(cur_node);
        party_info_display();
        break;
    case 4:
        frame_main();
        view_refresh();
        party_display(cur_node);
        party_info_display();
        noncombat_redraw_pending = 0;
        break;
    case 3:
        if (view_kind != 9 && game_state[STATE_WILDERNESS] == 1)
            view_refresh();
        break;
    case 6:
        frame_main();
        pic_open("PIC", 0x81, 0, &pic);
        pic_frame(3, 3, 1, 0);
        party_display(cur_node);
        break;
    }
}

struct Item far *far item_make(unsigned char type, char first_name_id, char second_name_id, char third_name_id,
                               char plus, char plus2, char ready, char hide,
                               char x34, int weight, char count, int x2e,
                               char x36, char x37, char x38)     /* 177e:1bdb */
{
    struct Item far *ret;
    struct Item far *item;

    pool_alloc((void far * far *)&item, 0x3d, &item_record_pool);
    mem_fill(item, 0x3d, 0);
    item->rec.type = type;
    item->rec.name_ids[0] = first_name_id;
    item->rec.name_ids[1] = second_name_id;
    item->rec.name_ids[2] = third_name_id;
    item->rec.plus = plus;
    item->rec.save_bonus = plus2;
    item->rec.equipped = ready;
    item->rec.unid_mask = hide;
    item->rec.cursed = x34;
    item->rec.weight = weight;
    item->rec.quantity = count;
    item->rec.value = x2e;
    item->rec.u.magic.charges = x36;
    item->rec.u.magic.effect = x37;
    item->rec.u.magic.special = x38;
    ret = item;
    return ret;
}

void far hero_free(struct Character far * far *character_ref)                    /* 177e:1c87 */
{
    struct Item far *extra_item;
    struct Item far *released_item;
    struct Effect far *released_effect;
    struct Effect far *effect_cursor;
    char was_not_pc;

    was_not_pc = 0;
    if ((*character_ref)->combat != 0) {
        was_not_pc = (*character_ref)->combat->not_pc;
        pool_free((*character_ref)->combat, 0x18, &combat_pool);
    }
    cur_item = (*character_ref)->items;
    while (cur_item != 0) {
        if (cur_item->rec.type == 0x49) {
            extra_item = cur_item->more;
            for (tick_slot = 1; cur_item->rec.quantity >= tick_slot; tick_slot++)
                if (extra_item != 0) {
                    released_item = extra_item;
                    extra_item = extra_item->more;
                    pool_free(released_item, 0x3d, &item_record_pool);
                }
        }
        released_item = cur_item;
        cur_item = cur_item->next;
        pool_free(released_item, 0x3d, &item_record_pool);
    }
    (*character_ref)->items = 0;
    effect_cursor = (*character_ref)->effects;
    while (effect_cursor != 0) {
        released_effect = effect_cursor;
        effect_cursor = effect_cursor->next;
        pool_free(released_effect, 9, &effect_pool);
    }
    if (was_not_pc == 1 || (*character_ref)->side == 1)
        pool_free(*character_ref, 0x19e, &npc_pool);
    else
        pool_free(*character_ref, 0x19e, &party_pool);
    *character_ref = 0;
}

void far hero_drop(char keep, char icon)                         /* 177e:1e31 */
{
    struct Character far *character_cursor;

    if (node_list == 0)
        return;
    character_cursor = node_list;
    if (character_cursor != cur_node) {
        while (character_cursor->next != cur_node && character_cursor->next != 0)
            character_cursor = character_cursor->next;
        if (character_cursor->next != 0) {
            if (icon)
                icon_clear(cur_node->order_no);
            character_cursor->next = cur_node->next;
            hero_free(&cur_node);
            cur_node = character_cursor;
            if (!keep)
                game_state[STATE_PARTY_COUNT]--;
        }
    } else {                                  /* -O cross-jumps the first arm's tail into this arm */
        if (icon)
            icon_clear(cur_node->order_no);
        node_list = cur_node->next;
        character_cursor = cur_node->next;
        hero_free(&cur_node);
        cur_node = character_cursor;
        if (!keep)
            game_state[STATE_PARTY_COUNT]--;
    }
    if (cur_node == 0)
        cur_node = node_list;
}

int far class_progression_row(struct Character far *character, int what)               /* 177e:1f39 */
{
    short result;

    switch (what) {
    case 0:
        if (character->deity <= 4)
            result = 0;
        else
            result = 1;
        break;
    case 1:
        switch (character->knight_order) {
        case 1:
            result = 3;
            break;
        case 2:
            result = 4;
            break;
        case 3:
            result = 5;
            break;
        }
        break;
    case 2:
        result = 2;
        break;
    case 4:
        result = 6;
        break;
    case 5:
        if (character->robe_order == 1)
            result = 7;
        else
            result = 8;
        break;
    case 6:
        result = 9;
        break;
    case 3:
        result = 0xa;
        break;
    }
    return result;                                 /* unset for other values, as shipped */
}

char far can_level_up(struct Character far *character)                      /* 177e:1fe5, proposed train_ready */
{
    char capped;
    char ready;
    short class_index;
    unsigned char lvl;
    long need;

    capped = 0;
    ready = 0;
    for (class_index = 0; class_index <= 6 && ready == 0; class_index++)
        if (character->level[class_index] > 0 && character->level[class_index] < 0x28) {
            lvl = character->level[class_index];
            need = level_xp_threshold(character, lvl + 1, class_index);
            capped = 0;
            if ((class_index != 1 || character->knight_order != 3 || lvl >= 0x28) && max_level[character->race][class_index] <= lvl)
                capped = 1;
            if (character->xp >= need && need > 0 && !capped)
                ready = 1;
        }
    return ready;
}

/* `2e ff a7 f2 20` is Turbo C's dense switch, jmp cs:[bx + table]. */
char far con_hp_adj(struct Character far *character, short arg)               /* 177e:20ce */
{
    char hp_adjustment;
    char constitution_adjustment;

    switch (character->stats[4].cur) {
    case 3:
        constitution_adjustment = -2;
        break;
    case 4: case 5: case 6:
        constitution_adjustment = -1;
        break;
    case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14:
        constitution_adjustment = 0;
        break;
    case 15:
        constitution_adjustment = 1;
        break;
    case 16:
        constitution_adjustment = 2;
        break;
    case 17: case 18: case 19:
        if (arg == 2 || arg == 4 || arg == 3 || arg == 1)
            constitution_adjustment = character->stats[4].cur - 14;
        else
            constitution_adjustment = 2;
        break;
    }
    hp_adjustment = constitution_adjustment;                                    /* unset outside con 3..19, as shipped */
    return hp_adjustment;
}

char far chance(unsigned char pct)                               /* 177e:2164 */
{
    char result;

    result = (unsigned)random_n(100) < pct;
    return result;
}

char far wields_43(struct Character far *character)                           /* 177e:218e */
{
    char result;

    if (character->equipped[0] != 0 && character->equipped[0]->rec.type == 0x43)
        result = 1;
    else
        result = 0;
    return result;
}

char far old_class_abilities_available(struct Character far *character)                        /* 177e:21bd */
{
    char result;

    result = first_level(character) > character->byte_92;
    return result;
}

unsigned char far first_level(struct Character far *character)                /* 177e:21eb */
{
    unsigned char result;
    short class_index;

    if (character->race != 6)
        result = 0;
    else {
        for (class_index = 0; class_index < 6 && character->level[class_index] == 0; class_index++)
            ;
        result = character->level[class_index];
    }
    return result;
}

unsigned char far class_level(struct Character far *character, int cls)       /* 177e:2234 */
{
    unsigned char result;
    unsigned char lvl;
    unsigned char zero;

    lvl = character->level[cls];
    zero = 0;
    if (lvl > zero)
        result = lvl;
    else
        result = zero;
    return result;
}
