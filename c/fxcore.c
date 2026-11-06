/* DQK.EXE resident module 148f: effects on a combatant and the rolls
 * behind them.  Adding, finding and removing effects, the triggers that
 * run an effect's handler, saving throws and attack rolls, the dice,
 * damage that is not a weapon's, recomputing a score an effect or an item
 * changes, raising a fallen member, the clouds on the combat map running
 * out, and the creatures that explode when they fall.
 *
 * EXE 0x006cf4..0x009beb, one Turbo C large-model _TEXT segment starting at
 * byte 4 of paragraph 148f.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * The last retf is byte 0xa of paragraph 177e.  fxcore_unit_init is the lone retf
 * rest.c leaves at 0x006cf4; no call, callback or vector found reaches it.
 *
 * _DATA is DGROUP 00a2..021a: hp_fx and 24 stored literals; -d stores
 * "lost a spell", "is killed", "is Poisoned" and "explodes" once each.
 * No _BSS.
 *
 * The routines follow DKK's FxEngine (dkk-disassembly v11/src/fxengine.pas)
 * routine by routine, in its order, except that fx_resist comes before
 * fx_trigger.  The effect handler is reached through vector stub 1d99:0e7d,
 * so callEffectHandler has no counterpart here, and neither has
 * removeAiEffects; fxcore_unit_init, fx_revive, fx_hurt_immune and the three
 * explosion passes after it have none in DKK.  A Pascal var parameter
 * became a far pointer (fx_trigger_group takes the hero's address), and the two
 * procedures nested in effectRemoved became fx_keep_best and fx_con_bonus.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is, so every
 * routine called ahead of its definition carries a prototype.  fx_hurt_immune
 * is near-called ahead of its definition, so it is static.
 */

#include "dqk.h"

void far fx_trigger_group(unsigned char when, struct Character far * far *character_ref);
void far fx_remove(struct Character far *character, unsigned char kind, struct Effect far *effect);
char far fx_saving_throw(struct Character far *character, int type, char adj);
int  far fx_roll_dice(unsigned char dice_count, unsigned char sides);
int  far fx_roll_damage(unsigned char dice_count, unsigned char sides);
void far fx_add(struct Character far *character, unsigned char kind, int dur,
                      unsigned char strength, char on_remove);
void far fx_invisibility_remove(struct Character far *character);
void far fx_hp_effects_remove(struct Character far *character);
void far fx_stat_recalc(struct Character far *character, short stat);
void far fx_damage(struct Character far *character, unsigned char dmg, int reduce, char saved);
void far fx_add_message(struct Character far *character, unsigned char kind, int dur,
                      unsigned char strength, char on_remove, int save_negates,
                      char saved, char far *msg);
static void fx_hurt_immune(struct Character far *character);

static unsigned char hp_fx[17] = {                               /* 00a2 */
    0x03, 0x03, 0x0b, 0x15, 0x17, 0x1b, 0x1e, 0x1f,
    0x23, 0x34, 0x4d, 0x6a, 0x6b, 0x6f, 0xc4, 0xb2, 0x76   /* v1.0 also 0xc4 */
};

/* The translated Pascal unit's empty initialization block (DKK fxengine.pas). */
void far fxcore_unit_init(void)                                          /* 148f:0004 */
{
}

void far fx_taunt(struct Character far *character)                        /* 148f:0005 */
{
    struct Character far *character_cursor;

    taunting_character = character;
    character_cursor = node_list;
    element_mask = 0;
    while (character_cursor != 0) {
        if (character_cursor->status == 0 && character_cursor->stats[1].cur > 3 && character_cursor->side != character->side
            && !is_helpless(character_cursor) && !fx_saving_throw(character_cursor, 4, 0))
            fx_add_message(character_cursor, 0x8c, 0, 0xff, 0, 0, 0,
                         msg_format("%s%s", wants_text, character->name));
        character_cursor = character_cursor->next;
    }
}

void far fx_disable(struct Character far *character, int how, char far *msg) /* 148f:00d4 */
{
    hero_text(character, msg, 10, 0);
    switch (character->status) {
    case 6:
    case 7:
    case 8:
        break;
    default:
        character->status = how;
        character->enabled = 0;
        character->hp = 0;
        fx_hp_effects_remove(character);
        fx_trigger_group(0xd, &character);
        if (character->enabled == 0 && character->status != 10)
            hero_fall(character);
        text_pause();
        combat_text_clear();
        if (view_mode != 5)
            party_display(cur_node);
    }
}

void far fx_remove(struct Character far *character, unsigned char kind, struct Effect far *effect)
{                                                                /* 148f:017d */
    struct Effect far *prev;
    struct Effect far *node;

    node = effect;
    if (node == 0) {
        node = character->effects;
        while (node != 0 && node->id != kind)
            node = node->next;
    }
    if (node == 0)
        return;
    if (node->on_remove != 0)
        fx_dispatch(kind, character, node, 1);
    if (character->effects == node)
        character->effects = node->next;
    else {
        prev = character->effects;
        while (prev != 0 && prev->next != node)
            prev = prev->next;
        prev->next = node->next;     /* v1.0: no check that prev was found */
    }
    pool_free(node, 9, &effect_pool);
    if (kind == 0xe)
        fx_stat_recalc(character, 5);
    if (kind == 0xc || kind == 0x26)
        fx_stat_recalc(character, 0);
    if (kind == 0x44) {
        fx_stat_recalc(character, 1);
        fx_stat_recalc(character, 2);
    }
}

void fx_resist(struct Character far *character)                               /* 148f:02ed */
{
    unsigned char level;
    int resist;
    int chance;

    resist = character->spell_resist;
    if (resist != 0 && (effect_id != 0 || ((char)element_mask & 8))) { /* test byte */
        level = caster_level(current_spell, 0);
        chance = (11 - level) * 5;
        chance += resist;
        if (fx_roll_dice(1, 100) <= chance) {
            damage = 0;
            if (effect_id != 0x5b && effect_id != 0x52)
                effect_id = 0;
        }
    }
}

void fx_trigger(struct Character far * far *character_ref, unsigned char kind)   /* 148f:0369 */
{
    unsigned char saved[0x9c];
    struct Character far *character_cursor;
    struct Effect far *effect;
    char found;
    unsigned char step_index;
    unsigned char range;

    found = 0;
    character_cursor = node_list;
    if (find_effect(*character_ref, kind, &effect))
        found = 1;
    else if (kind == 0x15 || kind == 0x2d || kind == 0x2e || kind == 0x31) {
        mem_move5(steps, saved, 0x9c);
        while (character_cursor != 0 && !found) {
            if (find_effect(character_cursor, kind, &effect)) {
                if (view_mode == 5) {
                    if (kind == 0x31)
                        range = 6;
                    else
                        range = 1;
                    plan_steps(combatant_x(character_cursor), combatant_y(character_cursor), range, 0xff,
                                 combatant_size(character_cursor));
                    for (step_index = 1; step_index <= step_count; step_index++)
                        if (combatant_slot(*character_ref) == (steps - 1)[step_index].who)
                            found = 1;
                } else
                    found = 1;
            }
            character_cursor = character_cursor->next;
        }
        mem_move5(saved, steps, 0x9c);
    }
    if (found) {
        if (kind == 0xd4)
            fx_dispatch(kind, cur_node, effect, 0);
        else
            fx_dispatch(kind, *character_ref, effect, 0);
    }
}

void far fx_trigger_group(unsigned char when, struct Character far * far *character_ref) /* 148f:0512 */
{
    switch (when) {
    case 1:
        fx_trigger(character_ref, 0x19); fx_trigger(character_ref, 0x45); fx_trigger(character_ref, 0x25);
        break;
    case 2:
        fx_trigger(character_ref, 0x56); fx_trigger(character_ref, 0x65); fx_trigger(character_ref, 0x66);
        fx_trigger(character_ref, 0x79); fx_trigger(character_ref, 0x41); fx_trigger(character_ref, 0x73);
        fx_trigger(character_ref, 0x20); fx_trigger(character_ref, 0x58); fx_trigger(character_ref, 0xaa);
        fx_trigger(character_ref, 0xb0); fx_trigger(character_ref, 0x84); fx_trigger(character_ref, 0x81);
        fx_trigger(character_ref, 0x8b); fx_trigger(character_ref, 0xb8); fx_trigger(character_ref, 0xce);
        fx_trigger(character_ref, 0xd0); fx_trigger(character_ref, 0xd3); fx_trigger(&(*character_ref)->combat->target, 0xd4);
        fx_trigger(character_ref, 0xc7);
        break;
    case 3:
        fx_trigger(character_ref, 0x40); fx_trigger(character_ref, 0x56); fx_trigger(character_ref, 0x65);
        fx_trigger(character_ref, 0x66); fx_trigger(character_ref, 0x79); fx_trigger(character_ref, 0x41);
        fx_trigger(character_ref, 0x20); fx_trigger(character_ref, 0x68); fx_trigger(character_ref, 0x6c);
        fx_trigger(character_ref, 0xaa); fx_trigger(character_ref, 0xb0); fx_trigger(character_ref, 0x84);
        fx_trigger(character_ref, 0x8b); fx_trigger(character_ref, 0xb8); fx_trigger(character_ref, 0xce);
        fx_trigger(character_ref, 0xd0); fx_trigger(character_ref, 0xd3); fx_trigger(&(*character_ref)->combat->target, 0xd4);
        fx_trigger(character_ref, 0xc7);
        break;
    case 4:
        fx_trigger(character_ref, 0x1d); fx_trigger(character_ref, 0x69); fx_trigger(character_ref, 0x78);
        fx_trigger(character_ref, 0x4b); fx_trigger(character_ref, 0x74); fx_trigger(character_ref, 0x95);
        fx_trigger(character_ref, 0x83); fx_trigger(character_ref, 0x93); fx_trigger(character_ref, 0xba);
        fx_trigger(character_ref, 0xc8); fx_trigger(character_ref, 0xc9); fx_trigger(character_ref, 0xd6);
        break;
    case 5:
        fx_trigger(character_ref, 0x1c); fx_trigger(character_ref, 0x29); fx_trigger(character_ref, 0x3c);
        fx_trigger(character_ref, 0x42); fx_trigger(character_ref, 0x43); fx_trigger(character_ref, 0x50);
        fx_trigger(character_ref, 0x51); fx_trigger(character_ref, 0x70); fx_trigger(character_ref, 0x52);
        fx_trigger(character_ref, 0x67); fx_trigger(character_ref, 0x7d); fx_trigger(character_ref, 0xa8);
        fx_trigger(character_ref, 0xa9); fx_trigger(character_ref, 0x9f); fx_trigger(character_ref, 0xa5);
        fx_trigger(character_ref, 0xae); fx_trigger(character_ref, 0xaf); fx_trigger(character_ref, 0xbb);
        fx_trigger(character_ref, 0xc1); fx_trigger(character_ref, 0xc5);
        break;
    case 6:
        fx_resist(*character_ref);
        fx_trigger(character_ref, 0x0a); fx_trigger(character_ref, 0x11); fx_trigger(character_ref, 0x14);
        fx_trigger(character_ref, 0x1c); fx_trigger(character_ref, 0x06); fx_trigger(character_ref, 0x28);
        fx_trigger(character_ref, 0x32); fx_trigger(character_ref, 0x36); fx_trigger(character_ref, 0x3d);
        fx_trigger(character_ref, 0x49); fx_trigger(character_ref, 0x4f); fx_trigger(character_ref, 0x5d);
        fx_trigger(character_ref, 0x62); fx_trigger(character_ref, 0x3f); fx_trigger(character_ref, 0x39);
        fx_trigger(character_ref, 0xa8); fx_trigger(character_ref, 0xa5); fx_trigger(character_ref, 0xad);
        fx_trigger(character_ref, 0xae); fx_trigger(character_ref, 0x86); fx_trigger(character_ref, 0x87);
        fx_trigger(character_ref, 0xc0); fx_trigger(character_ref, 0xcd); fx_trigger(character_ref, 0x77);
        fx_hurt_immune(*character_ref);
        break;
    case 7:
        fx_trigger(character_ref, 0x03); fx_trigger(character_ref, 0x1b); fx_trigger(character_ref, 0x1f);
        fx_trigger(character_ref, 0x34); fx_trigger(character_ref, 0x35); fx_trigger(character_ref, 0x6a);
        fx_trigger(character_ref, 0x33); fx_trigger(character_ref, 0x82);
        break;
    case 8:
        fx_trigger(character_ref, 0x38); fx_trigger(character_ref, 0x59); fx_trigger(character_ref, 0x64);
        fx_trigger(character_ref, 0xab); fx_trigger(character_ref, 0xc3);
        break;
    case 9:
        fx_resist(*character_ref);
        fx_trigger(character_ref, 0x12); fx_trigger(character_ref, 0x1c); fx_trigger(character_ref, 0x28);
        fx_trigger(character_ref, 0x39); fx_trigger(character_ref, 0x3f); fx_trigger(character_ref, 0x4c);
        fx_trigger(character_ref, 0x4f); fx_trigger(character_ref, 0x5c); fx_trigger(character_ref, 0x5f);
        fx_trigger(character_ref, 0x7b); fx_trigger(character_ref, 0x9e); fx_trigger(character_ref, 0xa5);
        fx_trigger(character_ref, 0x5d); fx_trigger(character_ref, 0x8d); fx_trigger(character_ref, 0x8f);
        fx_trigger(character_ref, 0x8a); fx_trigger(character_ref, 0x8e);
        break;
    case 10:
        fx_trigger(character_ref, 0x01); fx_trigger(character_ref, 0x02); fx_trigger(character_ref, 0x1a);
        fx_trigger(character_ref, 0x21); fx_trigger(character_ref, 0x24); fx_trigger(character_ref, 0x31);
        fx_trigger(character_ref, 0x78); fx_trigger(character_ref, 0xa2); fx_trigger(character_ref, 0x83);
        fx_trigger(character_ref, 0x94);
        break;
    case 11:
        fx_trigger(character_ref, 0x04); fx_trigger(character_ref, 0x08); fx_trigger(character_ref, 0x09);
        fx_trigger(character_ref, 0x0d); fx_trigger(character_ref, 0x11); fx_trigger(character_ref, 0x1e);
        fx_trigger(character_ref, 0x21); fx_trigger(character_ref, 0x2d); fx_trigger(character_ref, 0x2e);
        fx_trigger(character_ref, 0x76); fx_trigger(character_ref, 0x47); fx_trigger(character_ref, 0x7a);
        fx_trigger(character_ref, 0xd6);
        break;
    case 12:
        fx_trigger(character_ref, 0x08); fx_trigger(character_ref, 0x09); fx_trigger(character_ref, 0x0a);
        fx_trigger(character_ref, 0x0d); fx_trigger(character_ref, 0x11); fx_trigger(character_ref, 0x14);
        fx_trigger(character_ref, 0x21); fx_trigger(character_ref, 0x24); fx_trigger(character_ref, 0x2d);
        fx_trigger(character_ref, 0x2e); fx_trigger(character_ref, 0x31); fx_trigger(character_ref, 0x32);
        fx_trigger(character_ref, 0x36); fx_trigger(character_ref, 0x3d); fx_trigger(character_ref, 0x5e);
        fx_trigger(character_ref, 0x61); fx_trigger(character_ref, 0x7e); fx_trigger(character_ref, 0x8f);
        fx_trigger(character_ref, 0xcd);
        break;
    case 13:
        fx_trigger(character_ref, 0x9a); fx_trigger(character_ref, 0x99); fx_trigger(character_ref, 0x98);
        fx_trigger(character_ref, 0xa0); fx_trigger(character_ref, 0x9c); fx_trigger(character_ref, 0xb6);
        fx_trigger(character_ref, 0xbc); fx_trigger(character_ref, 0xbd); fx_trigger(character_ref, 0xb3);
        break;
    case 14:
        fx_trigger(character_ref, 0x46); fx_trigger(character_ref, 0x53); fx_trigger(character_ref, 0x54);
        fx_trigger(character_ref, 0x55); fx_trigger(character_ref, 0x5a); fx_trigger(character_ref, 0x5b);
        fx_trigger(character_ref, 0x60); fx_trigger(character_ref, 0x3a); fx_trigger(character_ref, 0x75);
        fx_trigger(character_ref, 0x72); fx_trigger(character_ref, 0x98); fx_trigger(character_ref, 0xb3);
        fx_trigger(character_ref, 0xb4); fx_trigger(character_ref, 0xcb); fx_trigger(character_ref, 0xc6);
        fx_trigger(character_ref, 0xca); fx_trigger(character_ref, 0x58); fx_trigger(character_ref, 0xcc);
        fx_trigger(character_ref, 0xd1);
        break;
    case 15:
        fx_trigger(character_ref, 0x0b); fx_trigger(character_ref, 0x15); fx_trigger(character_ref, 0x1e);
        fx_trigger(character_ref, 0x44); fx_trigger(character_ref, 0x52); fx_trigger(character_ref, 0xa1);
        fx_trigger(character_ref, 0xa7); fx_trigger(character_ref, 0xb1); fx_trigger(character_ref, 0x89);
        break;
    case 16:
        fx_trigger(character_ref, 0x19); fx_trigger(character_ref, 0x45); fx_trigger(character_ref, 0x25);
        fx_trigger(character_ref, 0x2f); fx_trigger(character_ref, 0x30); fx_trigger(character_ref, 0x59);
        fx_trigger(character_ref, 0xcf);
        break;
    case 17:
        fx_trigger(character_ref, 0x01); fx_trigger(character_ref, 0x0b);
        break;
    case 18:
        fx_trigger(character_ref, 0x4a); fx_trigger(character_ref, 0x27); fx_trigger(character_ref, 0x2a);
        fx_trigger(character_ref, 0x63); fx_trigger(character_ref, 0xb2);
        break;
    case 19:
        fx_trigger(character_ref, 0x0b); fx_trigger(character_ref, 0x17); fx_trigger(character_ref, 0xb5);
        break;
    case 20:
        fx_trigger(character_ref, 0x32); fx_trigger(character_ref, 0x36);
        break;
    case 21:
        fx_trigger(character_ref, 0x23);
        break;
    case 24:
        fx_trigger(character_ref, 0x64);
        break;
    }
}

void far fx_terrain_apply(struct Character far *character, char may_save, char skip_1d) /* 148f:12ab */
{
    unsigned char cell;
    unsigned char who;
    struct Effect far *effect;
    struct Effect far *fx2;
    char saved;
    struct Character far *node;
    unsigned char kind;
    char harm;

    if (character->enabled == 0)
        return;
    mirror_image_bypass = 1;
    combat_direction_probe(character, 8, &who, &cell, &kind, &harm);
    if (kind != 0 && (kind != 0x1d || skip_1d == 0)) {
        if (!combatant_visible(character, 1) && move_flag)
            combat_view_scroll(combatant_x(character), combatant_y(character), 0xff, 8);
        /* v1.0: element_mask = 8 only for the knives (0x1b), below */
        switch (kind) {
        case 0x1d:
            current_spell = 0x22;
            if (character->level_undead != 0)
                break;
            if (find_effect(character, 0x1f, &effect))
                break;
            if (find_effect(character, 0x8f, &effect))
                break;
            if (character->immunity & 0x18)
                break;
            saved = fx_saving_throw(character, 0, 0);
            if (saved) {
                node = cur_node;
                fx_add_message(character, 0x1e, fx_roll_dice(1, 4) + 1, 0xff, 0, 0, saved,
                             "starts to cough");
                fx2 = 0;
                if (find_effect(character, 0x1e, &fx2)) {
                    fx_dispatch(0x1e, character, effect, 0);                 /* fx, not fx2 */
                    if (character->combat->spell_cast > 0) {
                        hero_text(character, "lost a spell", 12, 1);
                        spell_mark(character, character->combat->spell_cast);
                        combat_actions_end(character);
                    }
                }
                cur_node = node;
            } else {
                node = cur_node;
                fx_add_message(character, 0x1f, fx_roll_dice(1, 4) + 1, 0xff, 0, 0, saved,
                             "chokes and gags from nausea");
                if (find_effect(character, 0x1f, &effect))
                    fx_dispatch(0x1f, character, effect, 0);
                cur_node = node;
            }
            break;
        case 0x1c:
            current_spell = 0x5b;
            if (character->enabled == 0)
                break;
            if (character->immunity & 0x10)
                break;
            switch (character->level_high) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                hero_text(character, "is Poisoned", 10, 0);
                text_pause();
                fx_add(character, 0x37, 0, 0xff, 0);
                fx_disable(character, 6, "is killed");
                break;
            case 5:
                if (!fx_saving_throw(character, 0, -4)) {
                    hero_text(character, "is Poisoned", 10, 0);
                    text_pause();
                    fx_add(character, 0x37, 0, 0xff, 0);
                    fx_disable(character, 6, "is killed");
                }
                break;
            case 6:
                if (!fx_saving_throw(character, 0, 0)) {
                    hero_text(character, "is Poisoned", 10, 0);
                    text_pause();
                    fx_add(character, 0x37, 0, 0xff, 0);
                    fx_disable(character, 6, "is killed");
                }
                break;
            default:
                element_mask = 0x88;
                fx_damage(character, fx_roll_damage(1, 10), 0, 0);
            }
            break;
        case 0x1b:
            current_spell = 0x65;
            element_mask = 8;
            damage = 0xff;
            fx_trigger_group(9, &character);
            if (damage == 0)
                break;
            if (character->enabled == 0)
                break;
            if (may_save == 0 || !fx_saving_throw(character, 4, 0)) {
                hero_text(character, "is cut by knives", 10, 0);
                text_pause();
                fx_damage(character, fx_roll_damage(8, 8), 0, 0);
            }
            break;
        }
    }
    mirror_image_bypass = 0;
    if (harm != 0) {
        element_mask = 0x100;
        fx_damage(character, fx_roll_damage(1, 6), 0, 0);
    }
}

char far fx_attack_roll_no_attacker(struct Character far *character, unsigned char need)    /* 148f:16e0 */
{
    char hit;

    hit = 0;
    attack_roll = fx_roll_dice(1, 20);
    if (attack_roll > 1) {
        if (attack_roll > 16)
            crit_hit = 1;
        else
            crit_hit = 0;
        if (attack_roll == 20)
            attack_roll = 100;
        fx_trigger_group(0x10, &character);
        if (attack_roll >= 0) {
            if (attack_roll + need > character->ac[0])
                hit = 1;
            crit_hit = crit_hit && hit;
        }
    }
    return hit;
}

char far fx_attack_roll(struct Character far *character, struct Character far *target, unsigned char need)
{                                                                /* 148f:1771 */
    char hit;
    char result;
    char adj;

    hit = 0;
    result = 0;
    fx_invisibility_remove(character);
    attack_roll = fx_roll_dice(1, 20);
    if (attack_roll > 1) {
        if (attack_roll > 16)
            crit_hit = 1;
        else
            crit_hit = 0;
        if (attack_roll == 20)
            attack_roll = 100;
        fx_trigger_group(10, &character);
        fx_trigger_group(0x10, &target);
        if (character->side == 0) {
            adj = game_state[STATE_PARTY_HIT_BONUS];
            switch (game_state[STATE_DIFFICULTY]) {
            case 1:
                adj += 2;
                break;
            case 2:
                adj++;
                break;
            case 4:
                adj--;
                break;
            case 5:
                adj -= 2;
                break;
            }
        } else
            adj = game_state[STATE_NONPARTY_HIT_BONUS];
        if (attack_roll >= 0 && character->thac0_adj + attack_roll + adj >= need) {
            hit = 1;
            result = 1;
        }
        crit_hit = crit_hit && hit;
    }
    return result;
}

char far fx_saving_throw(struct Character far *character, int type, char adj)    /* 148f:1886 */
{
    char result;
    struct Effect far *effect;
    unsigned short row;             /* inc ax: the +1 is not folded */

    save_succeeded = 1;
    save_bonus = fx_roll_dice(1, 20);
    if (find_effect(character, 0x76, &effect))
        save_bonus = 1;
    switch (save_bonus) {
    case 1:
        save_succeeded = 0;
        break;
    default:
        if (character->level[5] > 0 && character->robe_order > 0) {
            row = character->robe_order + (STATE_MOON_PHASE_1 - 2);
            switch (game_state[row + 1]) {
            case 0:
                adj--;
                break;
            case 2:
                adj++;
                break;
            }
        }
        save_bonus = character->save_bonus + save_bonus + adj;
        current_save_type = type;
        fx_trigger_group(0xc, &character);
        if (character->saves[type] <= save_bonus)
            save_succeeded = 1;
        else
            save_succeeded = 0;
        break;
    case 20:
        save_succeeded = 1;
        break;
    }
    result = save_succeeded;
    return result;
}

int far fx_roll_dice(unsigned char dice_count, unsigned char sides)       /* 148f:1968 */
{
    unsigned char die_index;
    int total;

    total = 0;
    if (dice_count > 0)
        for (die_index = 1; die_index <= dice_count; die_index++)
            total += random_n(sides) + 1;
    return total;
}

int far fx_roll_damage(unsigned char dice_count, unsigned char sides)       /* 148f:199e */
{
    unsigned char result;

    damage_dice_count = dice_count;
    result = fx_roll_dice(dice_count, sides);
    return result;
}

void far fx_add(struct Character far *character, unsigned char kind, int dur,
                      unsigned char strength, char on_remove)    /* 148f:19c2 */
{
    struct Effect far *effect;
    struct Effect far *effect_cursor;

    if (kind == 0)
        return;
    pool_alloc((void far * far *)&effect, 9, &effect_pool);
    if (character->effects == 0)
        character->effects = effect;
    else {
        effect_cursor = character->effects;
        while (effect_cursor->next != 0)
            effect_cursor = effect_cursor->next;
        effect_cursor->next = effect;
    }
    effect->next = 0;
    effect->id = kind;
    effect->duration = dur;
    effect->on_remove = on_remove;
    effect->strength = strength;
}

void far fx_out_of_play(struct Character far *character, int how, char far *msg) /* 148f:1a69 */
{
    unsigned char slot;

    if (character->enabled == 0)
        return;
    slot = combatant_slot(character);
    view_center(character, 3, 0);
    hero_text(character, msg, 10, 1);
    character->enabled = 0;
    character->status = how;
    if (character->status != 3)
        character->hp = 0;
    tile_update(0, 0, slot);
    screen_flush();
    (combatants - 1)[slot].size = 0;
    combat_grid_build();
    if (combat_actions_end(character))
        fx_hp_effects_remove(character);
}

void far fx_invisibility_remove(struct Character far *character)                        /* 148f:1b2a */
{
    struct Effect far *effect;

    while (find_effect(character, 0x19, &effect))
        fx_remove(character, 0x19, effect);
}

void far fx_hp_effects_remove(struct Character far *character)                        /* 148f:1b66 */
{
    unsigned char index;
    struct Effect far *effect;

    for (index = 1; index <= 17; index++)
        fx_remove(character, (hp_fx - 1)[index], 0);
    if (find_effect(character, 0x4d, &effect) && character->npc == 0xb3)
        character->side = 0;
}

char far fx_cure(struct Character far *character, unsigned char kind)    /* 148f:1bc6 */
{
    char result;
    struct Effect far *effect;

    result = 0;
    if (find_effect(character, kind, &effect)) {
        hero_text(character, "is Cured", 10, 1);
        fx_remove(character, kind, effect);
        result = 1;
    }
    return result;
}

unsigned char fx_str_code(unsigned char base, unsigned char exceptional_strength)  /* 148f:1c24 */
{
    unsigned char result;

    result = base + 100;
    if (base == 18)
        result = exceptional_strength + 1;
    return result;
}

void fx_str_decode(struct Effect far *effect, unsigned char far *base,
                   unsigned char far *exceptional_strength)                        /* 148f:1c47 */
{
    *exceptional_strength = 0;
    *base = effect->strength & 0x7f;
    if (*base <= 101) {
        *exceptional_strength = *base - 1;
        *base = 18;
    } else
        *base = *base - 100;
}

char far fx_strength_upgrade(struct Character far *character, unsigned char base, unsigned char exceptional_strength,
                      unsigned char far *out)                    /* 148f:1c87 */
{
    char result;

    if (character->stats[0].base < base || (base == 18 && character->str_ex_base < exceptional_strength)) {
        result = 1;
        *out = fx_str_code(base, exceptional_strength);
    } else
        result = 0;
    return result;
}

void fx_keep_best(unsigned char far *best_ex, unsigned char far *cand_ex,
                  unsigned char far *best, unsigned char far *cand) /* 148f:1ccc */
{
    if (*cand > *best || (*cand == 18 && *cand_ex > *best_ex)) {
        *best = *cand;
        *best_ex = *cand_ex;
    }
}

void fx_con_bonus(short cls, unsigned char lvl, unsigned char far *sum,
                  unsigned char far *con)                        /* 148f:1d0e */
{
    if (hit_dice_level_limit[cls] <= lvl)
        lvl = hit_dice_level_limit[cls] - 1;
    if (cls == 4 || cls == 1)
        lvl++;
    if (cls == 2 || cls == 3 || cls == 4 || cls == 1) {
        switch (*con) {
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
            *sum += (*con - 14) * lvl;
            break;
        case 20:
            *sum += lvl * 5;
            break;
        case 21:
        case 22:
        case 23:
            *sum += lvl * 6;
            break;
        case 24:
        case 25:
            *sum += lvl * 7;
            break;
        }
    } else if (*con > 15)
        *sum += lvl * 2;
    else if (*con == 15)
        *sum += lvl;
}

void far fx_stat_recalc(struct Character far *character, short stat)            /* 148f:1dfc */
{
    unsigned char best;
    unsigned char cand;
    unsigned char best_ex;
    unsigned char cand_ex;
    struct Effect far *effect;
    unsigned char old_max;
    unsigned char sum;
    unsigned char lvl;
    short class_index;
    struct Item far *item;
    unsigned char code;
    unsigned char classes;

    sum = 0;
    cand = 0;
    cand_ex = 0;
    item = character->items;
    best = character->stats[stat].base;
    best_ex = character->str_ex_base;
    while (item != 0) {
        if (item->rec.u.magic.special > 0x80 && item->rec.equipped != 0) {
            code = item->rec.u.magic.special & 0x7f;
            if (stat == 0) {
                switch (code) {
                case 3:
                    cand = 18;
                    cand_ex = 100;
                    break;
                case 5:
                    cand = item->rec.u.magic.effect + 18;
                    if (cand == 18)
                        cand_ex = 100;
                    break;
                }
                fx_keep_best(&best_ex, &cand_ex, &best, &cand);
            }
        }
        item = item->next;
    }
    switch (stat) {
    case 0:
        if (find_effect(character, 0x26, &effect)) {
            fx_str_decode(effect, &cand, &cand_ex);
            if (best <= 18 && best_ex < 100) {
                cand = best + cand;
                if (cand > 18) {
                    if (character->level[2] > 0 || character->level[3] > 0 || character->level[4] > 0
                        || character->level[1] > 0) {
                        cand_ex = character->str_ex + (cand - 18) * 10;
                        if (cand_ex > 100)
                            cand_ex = 100;
                    }
                    cand = 18;
                }
            }
            fx_keep_best(&best_ex, &cand_ex, &best, &cand);
        }
        if (find_effect(character, 0x71, &effect)) {
            fx_str_decode(effect, &cand, &cand_ex);
            fx_keep_best(&best_ex, &cand_ex, &best, &cand);
        }
        if (find_effect(character, 0xc, &effect)) {
            fx_str_decode(effect, &cand, &cand_ex);
            fx_keep_best(&best_ex, &cand_ex, &best, &cand);
        }
        character->stats[0].cur = best;
        character->str_ex = best_ex;
        break;
    case 4:
        classes = 0;
        old_max = character->hp_max;
        character->hp_max = character->hp_base;
        for (class_index = 0; class_index <= 6; class_index++) {
            lvl = character->old_level[class_index];
            if (lvl > 0)
                fx_con_bonus(class_index, lvl, &sum, &best);
            lvl = character->level[class_index];
            if (lvl > 0)
                classes++;
            if (hit_dice_level_limit[class_index] < lvl)
                lvl = hit_dice_level_limit[class_index];
            if ((int)character->byte_92 < lvl) {     /* a signed word compare */
                lvl -= character->byte_92;
                fx_con_bonus(class_index, lvl, &sum, &best);
            }
        }
        sum = sum / classes;
        character->hp_max += sum;
        if (character->hp_max > old_max)
            character->hp += character->hp_max - old_max;
        if (character->hp_max < old_max) {
            if (character->hp > old_max - character->hp_max)
                character->hp -= old_max - character->hp_max;
            else
                character->hp = 0;
        }
        character->stats[4].cur = best;
        if (character->stats[4].cur >= 20)
            fx_add(character, 0x3e, 60, 0xff, 1);
        else
            fx_remove(character, 0x3e, 0);
        break;
    case 5:
        if (find_effect(character, 0xe, &effect))
            best += effect->strength;
        character->stats[5].cur = best;
        break;
    case 1:
    case 2:
        if (find_effect(character, 0x44, &effect))
            best = 3;
        character->stats[stat].cur = best;
        break;
    }
}

void far fx_damage(struct Character far *character, unsigned char dmg, int reduce, char saved)
{                                                                /* 148f:2214 */
    char msg[0x52];

    damage = dmg;
    fx_trigger_group(6, &character);
    if (saved) {
        switch (reduce) {
        case 1:
            damage = 0;
            break;
        case 2:
            damage = damage / 2;
            break;
        }
    } else
        fx_trigger_group(0x14, &character);
    if (damage > 0 && character->enabled != 0) {
        if (damage != 1)
            str_format(msg, "takes %d%s", damage, " points of damage ");
        else
            str_copy(msg, "takes 1 point of damage ");
        switch (element_mask & ~8) {
        case 1:
            str_copy(msg, msg_format("%sfrom Fire", msg));
            break;
        case 2:
            str_copy(msg, msg_format("%sfrom Cold", msg));
            break;
        case 4:
            str_copy(msg, msg_format("%sfrom Electricity", msg));
            break;
        case 0x100:
            str_copy(msg, msg_format("%sfrom Acid", msg));
            break;
        default:
            if ((element_mask & 8) == element_mask)
                str_copy(msg, msg_format("%sfrom Magic", msg));
        }
        panel_dirty = 1;
        if (view_mode != 5)
            panel_dirty = 0;
        hero_panel(character);
        hit_flash(character, 0, msg);
        hero_damage(character, damage);
        if (view_mode == 5) {
            character->combat->can_cast = 0;
            if (character->combat->spell_cast > 0) {
                hero_text(character, "lost a spell", 12, 1);
                spell_mark(character, character->combat->spell_cast);
                combat_actions_end(character);
            }
        }
        if (character->enabled == 0) {
            str_copy(msg, "Goes Down");
            if ((character->immunity & 8) == 0) {
                if (character->status == 5)
                    str_copy(msg, msg_format("%s and is Dying", msg));
                if (character->status == 6 || character->status == 8 || character->status == 7)
                    str_copy(msg, "is killed");
            }
            hero_text(character, msg, text_row + 1, 0);
            if (view_mode == 5) {
                fx_hp_effects_remove(character);
                fx_trigger_group(0xd, &character);
                if (character->enabled == 0 && character->status != 10)
                    hero_fall(character);
                else
                    msg_pause();
            } else
                msg_pause();
        }
        combat_text_clear();
    }
}

void far fx_add_message(struct Character far *character, unsigned char kind, int dur,
                      unsigned char strength, char on_remove, int save_negates,
                      char saved, char far *msg)                 /* 148f:252c */
{
    struct Effect far *effect;

    effect_id = kind;
    fx_trigger_group(9, &character);
    if (effect_id == 0 || (saved != 0 && save_negates == 1))
        hero_text(character, "is Unaffected", 10, 1);
    else {
        if (find_effect(character, effect_id, &effect) && effect->duration != 0)
            fx_remove(character, kind, effect);
        fx_add(character, kind, dur, strength, on_remove);
        if (msg != 0 && *msg != 0) {
            hit_flash(character, 1, msg);
            combat_text_clear();
        }
    }
}

char far fx_heal(struct Character far *character, unsigned char healing_amount, char show) /* 148f:25f7 */
{
    char result;

    result = 0;
    if (character->status == 0 || character->status == 1 || character->status == 4 || character->status == 5) {
        /* v1.0: show == 0 heals with no effect 0xd7 test */
        if ((character->hp < character->hp_max && show != 0) || show == 0) {
            character->hp = character->hp + healing_amount;
            if (character->hp > character->hp_max)
                character->hp = character->hp_max;
            if (character->enabled == 0) {
                if (character->status == 5)
                    character->status = 4;
                if (character->status == 4 && view_mode != 5) {
                    character->status = 0;
                    character->enabled = 1;
                }
            }
            result = 1;
        }
    }
    return result;
}

char far fx_revive(struct Character far *character, unsigned char restored_hp, char far *msg) /* 148f:26c1 */
{
    char result;
    char was;

    if (hero_place(character, combatant_x(character), combatant_y(character), 1)) {
        character->status = 0;
        character->enabled = 1;
        character->hp = restored_hp;
        if (view_mode == 5) {
            was = move_flag;
            move_flag = 1;
            view_center(character, 3, 0);
            move_flag = was;
        }
        hit_flash(character, 1, msg);
        combat_sides_count();
        result = 1;
    } else
        result = 0;
    return result;
}

void fx_cloud_draw(void)                                         /* 148f:2766 */
{
    struct Cloud far *cloud;
    unsigned char index;

    for (cloud = cloud_list; cloud != 0; cloud = cloud->next)
        for (index = 1; index <= 9; index++)
            if (cloud->on[index] != 0)
                cur_map->cells[cloud->y + dir_dy[patch_dir[index]]][cloud->x + dir_dx[patch_dir[index]]]
                    = cloud->kind;
}

void far fx_clouds_tick(char now)                                  /* 148f:2817 */
{
    struct Cloud far *prev;
    struct Cloud far *cloud;
    struct Cloud far *next;
    char removed;
    unsigned char index;
    char x;
    char y;
    unsigned char record_index;
    char taken;
    char due;
    char acid;
    char blades;

    acid = 0;
    blades = 0;
    prev = 0;
    cloud = cloud_list;
    removed = 0;
    while (cloud != 0) {
        next = cloud->next;
        if (now)
            due = cloud->strength == 0xff;
        else
            due = cloud->strength <= 1;
        if (due) {
            if (cloud->kind == 0x46)
                acid = 1;
            if (cloud->kind == 0x1b)
                blades = 1;
            for (index = 1; index <= 9; index++) {
                if (cloud->on[index] != 0) {
                    x = cloud->x + dir_dx[patch_dir[index]];
                    y = cloud->y + dir_dy[patch_dir[index]];
                    taken = 0;
                    for (record_index = 1; record_index <= rec_count; record_index++)
                        if ((recs - 1)[record_index].hero != 0 && (recs - 1)[record_index].x == x
                            && (recs - 1)[record_index].y == y)
                            taken = 1;
                    if (taken)
                        cur_map->cells[y][x] = 0x1f;
                    else
                        cur_map->cells[y][x] = cloud->tile[index];
                }
            }
            removed = 1;
            if (prev == 0)
                cloud_list = next;
            else
                prev->next = next;
            pool_free(cloud, 0x21, &cloud_pool);
        } else {
            prev = cloud;
            if (!now)
                cloud->strength--;
        }
        cloud = next;
    }
    if (removed) {
        fx_cloud_draw();
        if (blades)
            bottom_msg("The blade barrier is gone");
        if (acid)
            bottom_msg("The acid drains");
        else
            bottom_msg("The air clears a little...");
    }
}

static void fx_hurt_immune(struct Character far *character)                   /* 148f:2a6d */
{
    char immune;

    immune = 0;
    if (((char)element_mask & 0x80) && (character->immunity & 0x10))
        immune = 1;
    if (((char)element_mask & 0x40) && (character->immunity & 8))
        immune = 1;
    if (immune) {
        damage = 0;
        effect_id = 0;
    }
}

void far fx_death_explosions(void)                                      /* 148f:2ab8 */
{
    unsigned char explosion_index;
    unsigned char step_index;
    struct Character far *character;
    int ids[8];
    unsigned char last;

    death_explosion_active = 1;
    element_mask = 0;
    for (explosion_index = 1; explosion_index <= death_explosion_count; explosion_index++) {
        character = death_explosion_queue[explosion_index];
        if (character != 0) {
            hero_text(character, "explodes.", 10, 1);
            plan_steps(combatant_x(character), combatant_y(character), 1, 0xff, combatant_size(character));
            if (step_count > 1) {
                last = step_count - 1;
                for (step_index = 2; step_index <= step_count; step_index++)
                    ids[step_index - 2] = (steps - 1)[step_index].who;
                for (step_index = 1; step_index <= last; step_index++)
                    if ((heroes - 1)[ids[step_index - 1]]->status != 10)
                        fx_damage((heroes - 1)[ids[step_index - 1]], fx_roll_damage(1, 6), 0, 0);
            }
        }
        character->enabled = 0;                  /* also for a null entry */
        character->status = 6;
        hero_fall(character);
    }
    if (death_explosion_count > 0) {
        death_explosion_count = 0;
        mem_fill(death_explosion_queue, 0x54, 0);
    }
    turn_checks();
    death_explosion_active = 0;
}

void far fx_ice_explosions(void)                                      /* 148f:2c4f */
{
    unsigned char explosion_index;
    struct Character far *character;
    unsigned char target_index;

    ice_explosion_active = 1;
    element_mask = 0;
    for (explosion_index = 1; explosion_index <= ice_explosion_count; explosion_index++) {
        character = ice_explosion_queue[explosion_index];
        if (character != 0) {
            view_cx = combatant_x(character);
            view_cy = combatant_y(character);
            hero_text(character, "explodes", 10, 1);
            plan_steps(view_cx, view_cy, (spell_info - 1)[0x57].targeting & 7, 0xff, 1);
            for (target_index = 1; target_index <= step_count; target_index++)
                targets[target_index] = (heroes - 1)[(steps - 1)[target_index].who];
            target_count = step_count;
            current_spell = 0x57;
            icon_scatter(2, 0x1a, 0xc);
            (*ice_storm_handler)();
        }
        character->enabled = 0;                  /* also for a null entry */
        character->status = 6;
        hero_fall(character);
    }
    if (ice_explosion_count > 0) {
        ice_explosion_count = 0;
        mem_fill(ice_explosion_queue, 0x54, 0);
    }
    turn_checks();
    ice_explosion_active = 0;
}

void far fx_meteor_explosions(void)                                      /* 148f:2d9b */
{
    unsigned char explosion_index;
    struct Character far *character;
    unsigned char target_index;
    struct Character far *node;

    meteor_explosion_active = 1;
    element_mask = 0;
    for (explosion_index = 1; explosion_index <= meteor_explosion_count; explosion_index++) {
        character = meteor_explosion_queue[explosion_index];
        if (character != 0) {
            node = cur_node;
            cur_node = character;
            view_cx = combatant_x(character);
            view_cy = combatant_y(character);
            hero_text(character, "explodes", 10, 1);
            plan_steps(view_cx, view_cy, (spell_info - 1)[0x7c].targeting & 7, 0xff, 1);
            for (target_index = 1; target_index <= step_count; target_index++)
                targets[target_index] = (heroes - 1)[(steps - 1)[target_index].who];
            target_count = step_count;
            current_spell = 0x7c;
            spell_run(0x7c);
            cur_node = node;
        }
        character->enabled = 0;                  /* also for a null entry */
        character->status = 6;
        hero_fall(character);
    }
    if (meteor_explosion_count > 0) {
        meteor_explosion_count = 0;
        mem_fill(meteor_explosion_queue, 0x54, 0);
    }
    turn_checks();
    meteor_explosion_active = 0;
}
