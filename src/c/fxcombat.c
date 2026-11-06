/* DQK.EXE overlay 09 module 30cc: effect handlers in combat.
 * The handlers effects.c's fx_install and fx_install_2 store into
 * fx_handler with segment 30cc, and one helper effects.c far-calls,
 * fx_poison_attack, a poison attack: effect 0x37 on the attacker's target,
 * and death if it takes.  The handlers play snakes, charm, silence,
 * nausea, dispelling, confusion and berserk rages, blinking, turning to
 * stone, lost intellect and memorized spells, fear, beheading, stunning,
 * disruption, the level drains, extra damage by the target's flags, and
 * the monsters' breath weapons and gaze.  Every handler takes the member,
 * the effect record and whether the effect is being removed; the one for
 * slot 7 does nothing.
 *
 * EXE 0x0537b8..0x054dcd, one Turbo C large-model _TEXT segment starting at
 * byte 8 of paragraph 30cc, the last module of RTLink section 9 (payload
 * 0x4de90, link segment 2b3a, a root section).  The first 8 bytes of
 * paragraph 30cc are the tail of module 2b3a (effects.c), and zeros follow
 * to the section's initialized end at 0x54dd0.  Built with Turbo C 2.01,
 * tcc -ml -O -Z -d.  Functions are defined in shipped order; bake consumes
 * them in that order.  Sections 10 and 11 hold other bytes at the 2b3a link
 * segment.
 *
 * Every routine after the lone retf at 30cc:0008 is reached as a far
 * pointer from effects.c's installers or as a direct far call from
 * effects.c, so all are public; nothing in the section near-calls them.
 * fx_confusion's roll of 1..100 compiles to a 100-entry jump table, and
 * fx_breath_weapon's switch on six sparse effect kinds to Turbo C's value
 * search through a table in the code segment (30cc:12fd).
 *
 * Of the 33 routines, 15 were compared with DKK's effects.pas and match a
 * handler there: snakesHarass, charmed, silenced, nauseated, dispelEvil,
 * confused, blinking, poisonTouch, berserk, drainOneLevel, drainTwoLevels,
 * fear, damageBonusVsFlag20, disruption and damageEqualsHitPoints (the
 * last three test other flag bits).  DKK's order differs in two places:
 * its level drains come before berserk, and damageEqualsHitPoints before
 * the other two.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are the ones the landed modules give each address, placeholders
 * included; the handlers keep the names effects.c gives them.
 *
 * _DATA is DGROUP 865c..8787, the literals.  The module has no _BSS.
 */

#include "dqk.h"

/* 30cc:0008 */
void far fxcombat_unit_init(void)
{
}

/* 30cc:0009 */
void far fx_snakes(struct Character far *character, struct Effect far *effect, char removed)
{
    unsigned char remaining_attacks;

    remaining_attacks = character->attacks_left[0] + character->attacks_left[1];
    if (effect->strength > remaining_attacks)
        effect->strength -= remaining_attacks;
    else
        fx_remove(character, 3, 0);
    hit_flash(character, 1, "is fighting with snakes");
    combat_text_clear();
    combat_actions_end(character);
}

/* 30cc:0075 */
void far fx_noop(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 30cc:007a */
void far fx_charm(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0) {
        if (removed) {
            character->side = (effect->strength & 0x40) / 0x40;
            if (character->npc == 0xb3) {
                character->npc = 0;
                character->quick_fight = 0;
            }
        } else {
            if (!(effect->strength & 0x20)) {
                effect->strength = effect->strength + (character->side << 6) + 0x20;
                character->side = effect->strength / 0x80;
                character->quick_fight = 1;
                if (!(character->npc & 0x80))
                    character->npc = 0xb3;
                character->combat->target = 0;
                combat_sides_count();
            }
            morale = 100;
        }
    }
}

/* 30cc:012c */
void far fx_silence(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0 && !removed) {
        if (character->combat->can_use_item)
            hero_text(character, msg_format("is %s", fx_names[21]), 10, 1);
        if (character->combat->spell_cast > 0)
            spell_mark(character, character->combat->spell_cast);
        character->combat->spell_cast = 0;
        character->combat->can_use_item = 0;
        character->combat->can_cast = 0;
    }
}

/* 30cc:01c7 */
void far fx_attack_bonus_vs_type80(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0 && !removed) {
        fx_target = character->combat->target;
        if (fx_target->type_flags & 0x80)
            attack_roll++;
    }
}

/* 30cc:01fb */
void far fx_nausea(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0 && view_mode == 5) {
        if (character->combat->can_use_item)
            hero_text(character, "is coughing", 10, 1);
        character->combat->can_use_item = 0;
        character->combat->can_cast = 0;
        recalc_stats(character);
        character->ac[0] -= 2;
        character->ac[1] -= 2;
        if (character == cur_node)
            hero_panel(character);
    }
}

/* 30cc:028c */
void far fx_dispel_evil_attack(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0 && !removed) {
        fx_target = character->combat->target;
        if ((char)(fx_target->type_flags & 1) & !fx_saving_throw(fx_target, 4, 0)) { /* &, as shipped */
            fx_disable(fx_target, 8, "is dispelled");
            fx_remove(character, 4, 0);
            fx_remove(character, 0x20, 0);
        } else
            hero_text(fx_target, "resists dispel evil", 10, 1);
    }
}

/* 30cc:033f */
void far fx_confusion(struct Character far *character, struct Effect far *effect, char removed)
{
    unsigned char roll;
    char saved;

    if (character->combat != 0 && !removed) {
        roll = fx_roll_dice(1, 100);
        switch (roll) {
        case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
        case 10:
            fx_remove(character, 0x23, 0);
            character->combat->panicked = 1;
            character->quick_fight = 1;
            if (!(character->npc & 0x80))
                character->npc = 0xb3;
            character->combat->target = 0;
            fx_add_message(character, 0x6f, 10, 0, 1, 1, 0, "runs away");
            break;
        case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18:
        case 19: case 20: case 21: case 22: case 23: case 24: case 25: case 26:
        case 27: case 28: case 29: case 30: case 31: case 32: case 33: case 34:
        case 35: case 36: case 37: case 38: case 39: case 40: case 41: case 42:
        case 43: case 44: case 45: case 46: case 47: case 48: case 49: case 50:
        case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58:
        case 59: case 60:
            hit_flash(character, 1, "is confused");
            combat_text_clear();
            fx_end_actions(character, effect, 0);
            break;
        case 61: case 62: case 63: case 64: case 65: case 66: case 67: case 68:
        case 69: case 70: case 71: case 72: case 73: case 74: case 75: case 76:
        case 77: case 78: case 79: case 80:
            fx_add_message(character, 0x4d, 1, character->side, 1, 1, 0, "goes berserk");
            fx_dispatch(0x4d, character, 0, 0);
            break;
        case 81: case 82: case 83: case 84: case 85: case 86: case 87: case 88:
        case 89: case 90: case 91: case 92: case 93: case 94: case 95: case 96:
        case 97: case 98: case 99: case 100:
            hit_flash(character, 1, "is enraged");
            combat_text_clear();
            break;
        }
        saved = fx_saving_throw(character, 4, 0xfe);
        if (saved)
            fx_remove(character, 0x23, 0);
    }
}

/* 30cc:058c */
void far fx_blink(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0 && !removed && character->combat->initiative == 0) {
        refused = 1;
        attack_roll = 0xff;
    }
}

/* 30cc:05b9 */
void far fx_poison_attack(struct Character far *character, char save)
{
    struct Effect far *effect;
    char saved;

    if (character->combat != 0) {
        fx_target = character->combat->target;
        if (find_effect(fx_target, 0x92, &effect)) {
            if (save < 0)
                save = 0;
            else
                save += effect->strength;
        }
        saved = fx_saving_throw(fx_target, 0, save);
        fx_add_message(fx_target, 0x37, 0, 0xff, 0, 1, saved, msg_format("is %s", fx_names[0x37]));
        text_pause();
        if (find_effect(fx_target, 0x37, &effect))
            fx_disable(fx_target, 6, "is killed");
    }
}

/* 30cc:069f */
void far fx_petrify_attack(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0 && !removed) {
        fx_target = character->combat->target;
        if (!fx_saving_throw(fx_target, 1, 0))
            fx_disable(fx_target, 7, "is turned to stone");
    }
}

/* 30cc:06f6 */
void far fx_feeblemind(struct Character far *character, struct Effect far *effect, char removed)
{
    unsigned char spell_slot;

    if (character->combat != 0 && !removed) {
        character->stats[1].cur = 3;
        character->stats[2].cur = 3;
        hero_text(character, "loses intellect", 10, 1);
        if (view_mode == 5) {
            character->combat->can_cast = 0;
            if (character->combat->spell_cast > 0) {
                hero_text(character, "lost a spell", 12, 1);
                spell_mark(character, character->combat->spell_cast);
                character->combat->spell_cast = 0;
            }
        }
        for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
            if (character->spells[spell_slot] & 0x80)
                character->spells[spell_slot] = 0;
    }
}

/* 30cc:07c8 */
void far fx_damage_vs_immunity1(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0 && !removed) {
        fx_target = character->combat->target;
        if (fx_target->immunity & 1) {
            damage = fx_roll_damage(6, 6) + str_damage(character);
            attack_roll++;
        }
    }
}

/* 30cc:081f */
void far fx_berserk(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0) {
        if (!removed) {
            character->quick_fight = 1;
            if (!(character->npc & 0x80) || character->npc == 0xb3)
                character->npc = 0xb3;
            else
                character->npc = 0xb2;
            if (view_mode == 5) {
                character->combat->target = 0;
                plan_steps(combatant_x(character), combatant_y(character), 0xff, 0xff, combatant_size(character));
                character->combat->target = (heroes - 1)[(steps - 1)[2].who];
                character->combat->can_cast = 0;
                character->side = character->combat->target->side ^ 1;
                hero_text(character, "goes berserk", 10, 1);
            }
        } else {
            if (character->npc == 0xb3) {
                character->npc = 0;
                character->quick_fight = 0;
            }
            character->side = 0;
        }
    }
}

/* 30cc:093b */
void far fx_drain_one_level(struct Character far *character, struct Effect far *effect, char removed)
{
    if (view_mode == 5)
        fx_drain(character->combat->target, 1);
}

/* 30cc:0960 */
void far fx_drain_two_levels(struct Character far *character, struct Effect far *effect, char removed)
{
    if (view_mode == 5)
        fx_drain(character->combat->target, 2);
}

/* 30cc:0985 */
void far fx_fire_damage_4d6(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *character_cursor;

    if (view_mode == 5) {
        character_cursor = character->combat->target;
        if (character_cursor != 0) {
            element_mask = 9;
            fx_damage(character_cursor, fx_roll_damage(4, 6), 2, fx_saving_throw(character_cursor, 4, 0));
        }
    }
}

/* 30cc:09e8 */
void far fx_berserk_creature(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0) {
        if (!removed) {
            character->quick_fight = 1;
            if (!(character->npc & 0x80) || character->npc == 0xb3)
                character->npc = 0xb3;
            else
                character->npc = 0xb2;
            character->combat->target = 0;
            plan_steps(combatant_x(character), combatant_y(character), 0xff, 0xff, combatant_size(character));
            character->combat->target = (heroes - 1)[(steps - 1)[2].who];
            character->side = character->combat->target->side ^ 1;
        } else {
            if (character->npc == 0xb3)
                character->npc = 0;
            character->side = effect->strength;
        }
    }
}

/* 30cc:0ad9 */
void far fx_fire_damage_2d6(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *character_cursor;

    if (view_mode == 5) {
        character_cursor = character->combat->target;
        if (character_cursor != 0) {
            element_mask = 9;
            fx_damage(character_cursor, fx_roll_damage(2, 6), 2, fx_saving_throw(character_cursor, 4, 0));
        }
    }
}

/* 30cc:0b3c */
void far fx_fear_remove(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0 && removed) {
        if (character->npc == 0xb3) {
            character->npc = 0;
            character->quick_fight = 0;
        }
        character->combat->panicked = 0;
    }
}

/* 30cc:0b74 */
void far fx_vorpal(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *character_cursor;

    if (view_mode == 5 && !removed && crit_hit) {
        character_cursor = character->combat->target;
        if (!(character_cursor->immunity & 0x20))
            fx_disable(character_cursor, 6, "is beheaded");
    }
}

/* 30cc:0bc0 */
void far fx_entangle_attack(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *character_cursor;

    if (view_mode == 5 && !removed) {
        character_cursor = character->combat->target;
        if (!fx_saving_throw(character_cursor, 1, 0))
            fx_add_message(character_cursor, 0x82, 3, 0, 0, 0, 0, game_text[5]);
    }
}

/* 30cc:0c28 */
void far fx_fixed_dice_attack(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *character_cursor;
    unsigned char side;
    unsigned char bonus;

    if (view_mode == 5) {
        if (character->equipped[0] != 0)
            bonus = character->equipped[0]->rec.plus;   /* left unset with no weapon, as shipped */
        character_cursor = character->combat->target;
        side = get_distance(character, character_cursor);
        character->thac0_adj = character->thac0_base;
        if (side < 2) {
            damage = fx_roll_damage(1, 6) + str_damage(character) + bonus + 2;
            character->thac0_adj = character->thac0_adj + str_hit(character);
        } else {
            damage = fx_roll_damage(1, 4) + bonus + 1;
            character->thac0_adj = character->thac0_adj + dex_missile(character);
        }
    }
}

/* 30cc:0d07 */
void far fx_damage_vs_immunity4(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *character_cursor;

    if (view_mode == 5) {
        character_cursor = character->combat->target;
        if (character_cursor->immunity & 4)
            damage += 3;
    }
}

/* 30cc:0d36 */
void far fx_disruption_attack(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *character_cursor;
    struct Effect far *node;

    if (view_mode == 5 && !removed) {
        character_cursor = character->combat->target;
        if (character_cursor->type_flags & 0x20)
            damage *= 2;
        else if (find_effect(character_cursor, 0xc6, &node) && fx_roll_dice(1, 2) == 1 || character_cursor->level_undead != 0)
            fx_disable(character_cursor, 8, game_text[0]);
    }
}

/* 30cc:0dc6 */
void far fx_movement_stop(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->combat != 0) {
        character->combat->movement = 0;
        if (side_count[3])
            move_rate = 0;
    }
}

/* 30cc:0ded */
void far fx_hp_damage_vs_immunity80(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *character_cursor;

    if (view_mode == 5) {
        character_cursor = character->combat->target;
        if (character_cursor->immunity & 0x80)
            damage = character->hp;
    }
}

/* 30cc:0e24 */
void far fx_attack_vs_immunity80(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *character_cursor;
    struct Effect far *node;

    if (view_mode == 5) {
        if (character == cur_node) {
            character_cursor = character->combat->target;
            if (character_cursor->immunity & 0x80) {
                if (find_effect(character_cursor, 0x54, &node))
                    damage = damage + 6;
                else
                    damage = fx_roll_damage(3, 12) + str_damage(character) + 6;
            }
        } else if (cur_node->immunity & 0x80)
            attack_roll += 3;
    }
}

/* 30cc:0ebf */
void far fx_poison_steam_breath(struct Character far *character, struct Effect far *effect, char removed)
{
    char x;
    char y;
    char ok;
    char saved;
    struct Character far *character_cursor;
    unsigned char target_index;
    char dir;

    mirror_image_bypass = 1;
    if (!combat_round)
        effect->strength = 3;
    if (effect->strength > 0) {
        x = combatant_x(character);
        y = combatant_y(character);
        (*hit_proc)(0x82, 1, &ok);
        if (ok) {
            spell_path_targets(x, y, view_cx, view_cy, 0xff, 3);
            if (target_count > 0) {
                element_mask = 0x20;
                if (effect->id == 0xcc)
                    hero_text(character, "breathes steam", 10, 1);
                else
                    hero_text(character, "breathes poison", 10, 1);
                dir = combatant_direction(character, targets[1]);
                hero_panel(character);
                combatant_facing_set(character, dir, 1, 0);
                icon_facings(0x13);
                icon_fly(combatant_x(character), combatant_y(character),
                             combatant_x(targets[1]), combatant_y(targets[1]), 1, 0x1e);
                for (target_index = 1; target_index <= target_count; target_index++)
                    if (targets[target_index] != 0) {
                        character_cursor = targets[target_index];
                        saved = fx_saving_throw(character_cursor, 3, 0);
                        fx_damage(character_cursor, character->hp_max, 2, saved);
                    }
                effect->strength--;
                combat_actions_end(character);
            }
        }
    }
    mirror_image_bypass = 0;
}

/* 30cc:10ab */
void far fx_lightning_breath(struct Character far *character, struct Effect far *effect, char removed)
{
    char x;
    char y;
    char dir;

    if (!combat_round || effect->strength > 3)
        effect->strength = 3;
    if (effect->strength > 0) {
        element_mask = 0x24;
        x = combatant_x(character);
        y = combatant_y(character);
        (*hit_proc)(0x33, 1, &turn_done);
        if (turn_done) {
            hero_text(character, "breathes lightning", 10, 1);
            dir = combatant_direction(character, targets[1]);
            hero_panel(character);
            combatant_facing_set(character, dir, 1, 0);
            sfx_play(9);
            icon_facings(0x14);
            icon_fly(x, y, view_cx, view_cy, 4, 0x32);
            bolt_run(9, character->hp_max, 0);
            effect->strength--;
            combat_actions_end(character);
        }
    }
}

/* 30cc:11d6 */
void far fx_spell_gaze(struct Character far *character, struct Effect far *effect, char removed)
{
    char ok;
    char dir;

    (*hit_proc)(10, 1, &ok);
    if (ok) {
        mirror_image_bypass = 1;
        element_mask = 0x10;
        hero_text(character, "gazes", 10, 1);
        dir = combatant_direction(character, targets[1]);
        combatant_facing_set(character, dir, 1, 0);
        icon_facings(0x13);
        icon_fly(combatant_x(character), combatant_y(character), view_cx, view_cy, 4, 0x32);
        current_spell = 10;
        spell_run(10);
        mirror_image_bypass = 0;
    }
}

/* 30cc:12a1 */
void far fx_breath_weapon(struct Character far *character, struct Effect far *effect, char removed)
{
    char x;
    char y;
    unsigned char target_index;
    struct Character far *character_cursor;
    char saved;
    char lo;
    char hi;
    char name[21];
    char dir;
    int dmg;

    mirror_image_bypass = 1;
    if (!combat_round || effect->strength > 3)
        effect->strength = 3;
    if (effect->strength > 0) {
        dmg = character->hp_max;
        switch (effect->id) {
        case 0x53: case 0x54:
            element_mask = 0x21;
            lo = 3;
            hi = 9;
            str_copy(name, "Fire");
            if (effect->id == 0x53)
                dmg = fx_roll_damage(24, 10) + 12;
            break;
        case 0x55: case 0xca:
            if (effect->id == 0xca && random_n(2) == 1) {
                lo = 4;
                hi = 5;
                str_copy(name, "paralysis");
            } else {
                element_mask = 0x22;
                lo = 3;
                hi = 7;
                str_copy(name, "cold");
            }
            break;
        case 0x60:
            element_mask = 0x20;
            lo = 1;
            hi = 6;
            str_copy(name, "acid");
            break;
        case 0xb4:
            element_mask = 0x60;
            lo = 1;
            hi = 8;
            str_copy(name, "death");
            break;
        }
        x = combatant_x(character);
        y = combatant_y(character);
        (*hit_proc)(0x17, 1, &turn_done);
        if (turn_done) {
            spell_path_targets(x, y, view_cx, view_cy, lo, hi);
            for (target_index = 1; target_index <= target_count; target_index++)
                if (enemy_team(character) != targets[target_index]->side)
                    turn_done = 0;
            if (target_count > 0 && turn_done) {
                hero_text(character, msg_format("breathes %s", name), 10, 1);
                dir = combatant_direction(character, targets[1]);
                hero_panel(character);
                combatant_facing_set(character, dir, 1, 0);
                icon_facings(0x13);
                icon_fly(combatant_x(character), combatant_y(character),
                             combatant_x(targets[1]), combatant_y(targets[1]), 1, 0x1e);
                for (target_index = 1; target_index <= target_count; target_index++)
                    if (targets[target_index] != 0) {
                        character_cursor = targets[target_index];
                        saved = fx_saving_throw(character_cursor, 3, 0);
                        if (effect->id == 0xca)
                            fx_add_message(targets[target_index], 0x34, 10, 0xc, 0, 1, saved, "paralyzed");
                        else
                            fx_damage(character_cursor, dmg, 2, saved);
                    }
                effect->strength--;
                combat_actions_end(character);
            }
        }
    }
    mirror_image_bypass = 0;
}
