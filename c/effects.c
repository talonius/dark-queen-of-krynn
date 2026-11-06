/* DQK.EXE overlay 09 module 2b3a: the effect handlers.
 * Every effect a party member or monster carries has an id; fx_install and
 * fx_install_2 fill fx_handler with one far handler per id, and fx_dispatch
 * calls the handler for an id with the member, the effect record and
 * whether it is being removed -- or, while item_handler_active is set, the item
 * handler in slot 0x7f.  The handlers adjust the current roll (save_bonus,
 * attack_roll), damage and element mask, add and remove effects, drain
 * levels, and play the monsters' special attacks: gazes, breath, rays,
 * acid, engulfing.  fx_paralyze, fx_cancel, fx_rearm, fx_cancel_if,
 * fx_avoid, fx_weapon, fx_need_plus, fx_forget_spells, fx_puddle and
 * fx_improve_rolls are helpers the handlers near-call.  The counterpart is
 * DKK's effects.pas (installHandlers and its handlers): of 59 handler
 * bodies compared, 23 match DKK's handler at the same id and 36 one at
 * another id.
 *
 * EXE 0x04de90..0x0537b8, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 2b3a in RTLink section 9 (payload 0x4de90, link
 * segment 2b3a, a root section).  The last 8 bytes lie in paragraph 30cc;
 * the uncalled retf after them, 30cc:0008, is left to module 30cc.  Built
 * with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped
 * order; bake consumes them in that order.  Sections 10 and 11 hold other
 * bytes at the 2b3a link segment.
 *
 * One file: the installers store a handler already defined here as
 * mov [m+2],cs, the form Turbo C gives only a function defined earlier in
 * the same file, and those stores and the near calls tie 2b3a:0000 to
 * 2b3a:549e; fx_dispatch is reached as 2b3a:58e5.  The 38 stores with
 * segment 30cc put 31 of module 30cc's handlers in the table, and the 39th
 * relocated store is the vector stub 1d99:10a3.
 *
 * The seg:off beside each extern is the far pointer as stored in the
 * shipped call, or the DGROUP offset of an extern variable.  Callee and
 * variable names are placeholders chosen from use; a handler's name follows
 * DQK's own effect name for its slot (fx_names, DGROUP 6524) where it has
 * one, except fx_animal_invisibility (slot 69, "Invisible to Animals") and fx_rotting
 * (slot 172, a second "Diseased").  fx_install is reached through vector
 * stub 1d99:13eb (from newgame.c), fx_drain through 1d99:1341 and
 * fx_dispatch through 1d99:0e7d; module 30cc far-calls fx_end_actions,
 * fx_drain and fx_dispatch.
 *
 * _DATA is DGROUP 8304..865b, the literals.  _BSS is fx_handler at
 * beec..c27f; its slot 0, bee8..beeb, is other modules' flags.
 */

#include "dqk.h"

void far fx_dispatch(unsigned char id, struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 2b3a:58e5 */
void far fx_install_2(void);                                     /* 2b3a:549e */

static void (far *fx_handler[229])(struct Character far *h, struct Effect far *fx, char removed);
                                /* by effect id from 1: (fx_handler - 1)[id] */

/* 2b3a:0000 */
void far fx_paralyze(struct Character far *character, int unused, char save)
{
    char saved;
    long node;

    fx_target = character->combat->target;
    saved = fx_saving_throw(fx_target, 0, save);
    if (find_effect(character, 0x58, &node))
        fx_add_message(fx_target, 0x34, 10, 0xc, 0, 1, saved, "paralyzed");
    else
        fx_add_message(fx_target, 0x34, 10, 0xc, 0, 1, saved, "paralyzed by poison");
}

/* 2b3a:00a1 */
void far fx_cancel(char id)
{
    if (id == 0 || effect_id == id) {
        damage = 0;
        effect_id = 0;
    }
}

/* 2b3a:00bf */
char far fx_rearm(struct Character far *character, char id, char strength, int minutes)
{
    char ok;

    if (fx_lock)
        ok = 0;
    else {
        ok = 1;
        fx_add(character, id, minutes, strength, 1);
    }
    return ok;
}

/* 2b3a:00f7 */
void far fx_cancel_if(int mask)
{
    if ((char)element_mask & (char)mask)
        fx_cancel(0);
}

/* 2b3a:010d */
void far fx_end_actions(struct Character far *character, struct Effect far *effect, char removed)
{
    combat_actions_end(character);
}

/* 2b3a:011f */
void far fx_improve_rolls(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        save_bonus++;
        attack_roll++;
    }
}

/* 2b3a:0132 */
void far fx_bless(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        morale += 5;
        attack_roll++;
    }
}

/* 2b3a:0146 */
void far fx_curse(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (morale < 5)
            morale = 0;
        else
            morale -= 5;
        attack_roll--;
    }
}

/* 2b3a:0168 */
void far fx_dispel_evil(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && (cur_node->type_flags & 1))
        attack_roll -= 7;
}

/* 2b3a:0184 */
void far fx_elec_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        fx_cancel_if(4);
}

/* 2b3a:0198 */
void far fx_prot_evil(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (cur_node->alignment == 2 || cur_node->alignment == 5 || cur_node->alignment == 8) {
            save_bonus += 2;
            attack_roll -= 2;
        }
    }
}

/* 2b3a:01c6 */
void far fx_prot_good(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (cur_node->alignment == 0 || cur_node->alignment == 3 || cur_node->alignment == 6) {
            save_bonus += 2;
            attack_roll -= 2;
        }
    }
}

/* 2b3a:01f4 */
void far fx_resist_cold(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && (element_mask & 2)) {
        damage = damage / 2;
        save_bonus += 3;
    }
}

/* 2b3a:0218 */
void far fx_enlarged(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 2b3a:021d */
void far fx_barkskin(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (!(element_mask & 8))
            save_bonus++;
        attack_roll--;
    }
}

/* 2b3a:0238 */
void far fx_friendly(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 2b3a:023d */
void far fx_slow_poison(struct Character far *character, struct Effect far *effect, char removed)
{
    if (fx_rearm(character, 0x1a, effect->strength, 10) && character->hp > 1) {
        element_mask = 0;
        fx_damage(character, 1, 0, 0);
        if (view_mode != 5)
            party_rows(character, character == cur_node);
    }
}

/* 2b3a:02b4 */
void far fx_read_magic(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 2b3a:02b9 */
void far fx_shielded(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (character->ac[0] < 0x39)
            character->ac[0] = 0x39;
        save_bonus++;
        if (current_spell == 0xf)
            damage = 0;
    }
}

/* 2b3a:02e6 */
void far fx_half_elf(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (fx_roll_dice(1, 100) <= 30) {
            fx_cancel(0xb);
            fx_cancel(0x35);
        }
    }
}

/* 2b3a:0313 */
void far fx_resist_fire(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && (element_mask & 1)) {
        damage = damage / 2;
        save_bonus += 3;
    }
}

/* 2b3a:0337 */
void far fx_slow_poison_end(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;

    if (!removed) {
        if (find_effect(character, 0x37, &node))
            fx_disable(character, 6, "dies from poison");
        fx_lock = 1;
        fx_remove(character, 0xf, 0L);
        fx_lock = 0;
    }
}

/* 2b3a:0397 */
void far fx_spirit_hammer(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *node;
    char found;
    struct Character far *saved;

    node = character->items;
    found = 0;
    while (node != 0 && !found)
        if (node->rec.type == 6 && node->rec.name_ids[2] == 0x79)
            found = 1;
        else
            node = node->next;
    if (removed && node != 0)
        item_remove(character, node);
    if (!removed && !found && character->item_count < 0x10) {
        pool_alloc(&node, 0x3d, &item_record_pool);
        mem_zero(node, 0x3d);
        node->rec.type = 6;
        node->rec.name_ids[1] = 6;
        node->rec.name_ids[2] = 0x79;
        node->rec.plus = 1;
        node->rec.u.magic.effect = 0x17;
        node->rec.u.magic.special = 0x80;
        item_append(character, node);
        pool_free(node, 0x3d, &item_record_pool);
        node = character->items;
        found = 0;
        while (node != 0 && !found)
            if (node->rec.name_ids[2] == 0x79 && node->rec.type == 6)
                found = 1;
            else
                node = node->next;
        saved = cur_node;
        cur_node = character;
        if (node != 0 && character->equipped[0] == 0) {
            item_equip_toggle(node);
            node->rec.equipped = 1;
        }
        cur_node = saved;
        hero_text(character, "Gains an item", 10, 1);
    }
    recalc_stats(character);
}

/* 2b3a:054a */
void far fx_invisible(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;

    if (!removed) {
        if (!find_effect(cur_node, 0x18, &node)) {
            refused = 1;
            attack_roll -= 4;
        }
    }
}

/* 2b3a:0580 */
void far fx_mirror_image(struct Character far *character, struct Effect far *effect, char removed)
{
    unsigned char image_count;

    if (!removed && view_mode == 5 && fx_roll_dice(1, effect->strength / 0x20 + 1) > 1
        && (!mirror_image_bypass || (current_spell > 0 && (spell_info - 1)[current_spell].hostile == 0))) {
        fx_cancel(0);
        hero_text(character, "lost an image", 10, 1);
        image_count = effect->strength >> 5;
        image_count--;
        if (image_count == 0)
            fx_remove(character, 0x1c, 0L);
        else {
            image_count = image_count << 5;
            effect->strength = effect->strength & 0x1f | image_count;
        }
    }
}

/* 2b3a:0654 */
void far fx_weakened(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        damage = damage - damage / 4;
}

/* 2b3a:0672 */
void far fx_blind(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        attack_roll -= 4;
        character->ac[0] -= 4;
        character->ac[1] -= 4;
        save_bonus -= 4;
    }
}

/* 2b3a:0696 */
void far fx_diseased(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        fx_dispatch(0x2b, character, effect, removed);
        fx_dispatch(0x2c, character, effect, removed);
    }
}

/* 2b3a:06d3 */
void far fx_accursed(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        attack_roll -= 4;
        save_bonus -= 4;
    }
}

/* 2b3a:06e8 */
void far fx_hastened(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (!(effect->strength & 0x20)) {
            effect->strength = effect->strength + 0x20;
            hero_text(character, "ages", 10, 1);
            character->age++;
            year_rolled = 1;
        }
        attack_rate *= 2;
        move_rate *= 2;
    }
}

/* 2b3a:073b */
void far fx_block_lower_spell_levels(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && current_spell != 0 && (spell_info - 1)[current_spell].level < 8)
        fx_cancel(0);
}

/* 2b3a:076e */
void far fx_avoid(struct Character far *character, unsigned char chance)
{
    if (cur_node->equipped[0] != 0 && get_distance(cur_node, character) > 1
        && fx_roll_dice(1, 100) <= chance) {
        hero_text(character, "Avoids it", 10, 1);
        damage = 0;
        attack_roll = -1;
        hits_left[0]--;
    }
}

/* 2b3a:07d4 */
struct Item far *far fx_weapon(struct Character far *character)
{
    struct Item far *ret;
    struct Item far *weapon;
    char ok;

    weapon = 0;
    if (character->equipped[0] != 0) {
        ok = ranged_ammo_find(character, &weapon);
        if (!ok || weapon == 0)
            weapon = character->equipped[0];
    }
    ret = weapon;
    return ret;
}

/* 2b3a:0832 */
void far fx_missile_prot(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && fx_weapon(cur_node) != 0 && fx_weapon(cur_node)->rec.plus == 0)
        fx_avoid(character, 100);
}

/* 2b3a:0877 */
void far fx_slowed(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;

    if (!removed && !find_effect(character, 0x96, &node)) {
        attack_rate /= 2;
        move_rate /= 2;
    }
}

/* 2b3a:08ba */
void far fx_periodic_strength_drain(struct Character far *character, struct Effect far *effect, char removed)
{
    if (fx_rearm(character, 0x2b, effect->strength, 0x3c)) {
        if (character->stats[0].cur > 3) {
            hero_text(character, "is weakened", 10, 1);
            character->stats[0].cur--;
        } else
            fx_add(character, 0x1f, 0, 0xff, 0);
    }
}

/* 2b3a:0920 */
void far fx_periodic_hp_drain(struct Character far *character, struct Effect far *effect, char removed)
{
    if (fx_rearm(character, 0x2c, effect->strength, 10)) {
        if (character->hp > 1) {
            element_mask = 0;
            fx_damage(character, 1, 0, 0);
            if (view_mode != 5)
                party_rows(character, character == cur_node);
        } else
            fx_add(character, 0x1f, 0, 0xff, 0);
    }
}

/* 2b3a:09b2 */
void far fx_attack_penalty_type_4(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && (cur_node->type_flags & 4))
        attack_roll -= 4;
}

/* 2b3a:09ce */
void far fx_noop_slot_48(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 2b3a:09d3 */
void far fx_prayer(struct Character far *character, struct Effect far *effect, char removed)
{
    short side;

    if (!removed) {
        side = (effect->strength & 0x80) / 0x80;
        if (character->side == side)
            fx_improve_rolls(character, effect, removed);
        else {
            attack_roll--;
            save_bonus--;
        }
    }
}

/* 2b3a:0a22 */
void far fx_fire_shield_hot(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (element_mask & 2) {
            save_bonus += 2;
            if (save_succeeded)
                damage = 0;
            else
                damage = damage / 2;
        } else if (((element_mask & 1) != 0) & !save_succeeded)
            damage *= 2;
    }
}

/* 2b3a:0a7e */
void far fx_fire_shield_cold(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (element_mask & 1) {
            save_bonus += 2;
            if (save_succeeded)
                damage = 0;
            else
                damage = damage / 2;
        } else if ((element_mask & 2) && !save_succeeded)
            damage *= 2;
    }
}

/* 2b3a:0aca */
void far fx_poisoned(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 2b3a:0acf */
void far fx_invis_ring(struct Character far *character, struct Effect far *effect, char removed)
{
    if (removed) {
        if (view_mode == 5)
            fx_remove(character, 0x19, 0L);
    } else
        fx_add(character, 0x19, 1, 0xc, 0);
}

/* 2b3a:0b11 */
void far fx_major_globe(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && current_spell > 0 && (int)(spell_info - 1)[current_spell].level <= 4 && (element_mask & 8))
        fx_cancel(0);
}

/* 2b3a:0b50 */
void far fx_stone_gaze(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;

    if (!removed) {
        (*hit_proc)(0x82, 1, &turn_done);
        if (enemy_team(character) != targets[1]->side)
            turn_done = 0;
        if (turn_done) {
            fx_target = targets[1];
            hero_text(character, "gazes...", 10, 0);
            icon_facings(0x14);
            icon_fly(combatant_x(character), combatant_y(character),
                         combatant_x(fx_target), combatant_y(fx_target), 4, 0x2d);
            if (find_effect(character, 0x3b, &node) && find_effect(fx_target, 0x48, &node)) {
                hero_text(fx_target, "reflects it!", 0xc, 0);
                icon_fly(combatant_x(fx_target), combatant_y(fx_target),
                             combatant_x(character), combatant_y(character), 4, 0x2d);
                fx_target = character;
            }
            if (!fx_saving_throw(fx_target, 1, 0) && !find_effect(fx_target, 0x7e, &node)
                && !find_effect(fx_target, 0x4f, &node))
                fx_disable(fx_target, 7, "is turned to stone");
        }
    }
}

/* 2b3a:0d30 */
void far fx_noop_slot_59(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 2b3a:0d35 */
void far fx_need_plus(unsigned char required_plus)
{
    unsigned char level;
    struct Item far *weapon;

    weapon = fx_weapon(cur_node);
    if (weapon != 0) {
        if (weapon->rec.plus < required_plus)
            damage = 0;
    } else if (cur_node->race == 7) {
        level = (required_plus - 1) * 2 + 5;
        if (cur_node->level_high < level)
            damage = 0;
    } else
        damage = 0;
}

/* 2b3a:0d8c */
void far fx_need_plus_3(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_need_plus(3);
}

/* 2b3a:0d99 */
void far fx_fire_ring(struct Character far *character, struct Effect far *effect, char removed)
{
    unsigned char index;

    if (!removed && ((char)element_mask & 1) && damage > 0) {
        for (index = 1; index <= damage_dice_count; index++) {
            damage -= 2;
            if (damage_dice_count > damage)
                damage = damage_dice_count;
        }
        save_bonus += 4;
        if (!(element_mask & 8))
            fx_cancel(0);
    }
}

/* 2b3a:0df6 */
void far fx_periodic_regeneration(struct Character far *character, struct Effect far *effect, char removed)
{
    if (fx_rearm(character, 0x3e, effect->strength, 0x3c) && fx_heal(character, 1, 1))
        heal_msg(character);
}

/* 2b3a:0e3e */
void far fx_minor_globe(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && current_spell > 0 && (int)(spell_info - 1)[current_spell].level <= 3 && (element_mask & 8))
        fx_cancel(0);
}

/* 2b3a:0e7d */
void far fx_poison_attack_normal(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        fx_poison_attack(character, 0);
}

/* 2b3a:0e98 */
void far fx_halve_low_class_weapon_damage(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;

    weapon = fx_weapon(cur_node);
    if (weapon != 0 && (char)(item_table[weapon->rec.type].wpn_class <= 1))
        damage = damage / 2;
}

/* 2b3a:0eeb */
void far fx_halve_high_bit_weapon_damage(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;

    weapon = fx_weapon(cur_node);
    if (weapon != 0 && (item_table[weapon->rec.type].wpn_class & 0x80))
        damage = damage / 2;
}

/* 2b3a:0f33 */
void far fx_animal_invisibility(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;

    if (!removed && (character->type_flags & 0x45) && !find_effect(character, 0x18, &node))
        attack_roll -= 4;
}

/* 2b3a:0f6b */
void far fx_confusing_gaze(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;
    char ok;

    if (!removed) {
        (*hit_proc)(0x44, 1, &ok);
        fx_target = targets[1];
        if (ok && get_distance(character, fx_target) < 2) {
            element_mask = 0x10;
            hero_text(character, "gazes...", 10, 0);
            icon_facings(0x14);
            icon_fly(combatant_x(character), combatant_y(character),
                         combatant_x(fx_target), combatant_y(fx_target), 4, 0x2d);
            if (!fx_saving_throw(fx_target, 4, 0) && !find_effect(fx_target, 0x7e, &node))
                fx_add_message(fx_target, 0x23, fx_roll_dice(1, 10) + 2, character->side, 1, 1, 0,
                             "stares blankly");
        }
    }
}

/* 2b3a:10a6 */
void far fx_highlighted(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        character->ac[0] -= 2;
        character->ac[1] -= 2;
    }
}

/* 2b3a:10c0 */
void far fx_noop_slot_72(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 2b3a:10c5 */
void far fx_breath_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && (element_mask & 0x20) > 0)
        fx_cancel(0);
}

/* 2b3a:10e0 */
void far fx_magic_boots(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        attack_rate *= 2;
}

/* 2b3a:10f3 */
void far fx_disease_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        fx_cancel(0x22);
}

/* 2b3a:1106 */
void far fx_magic_stone(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        save_bonus++;
}

/* 2b3a:1115 */
void far fx_healed_by_fire(struct Character far *character, struct Effect far *effect, char removed)
{
    unsigned short hp;

    if (((char)element_mask & 1) && ((char)element_mask & 8)) {
        hero_text(character, "Is Healed.", 10, 1);
        hp = character->hp + damage;
        if (character->hp_max < hp)
            character->hp = character->hp_max;
        else
            character->hp = hp;
    } else if ((char)element_mask & 4) {
        fx_add(character, 0x2a, 3, 0xff, 0);
        hero_text(character, msg_format("is %s", fx_names[42]), 10, 1);
    }
    fx_cancel(0);
}

/* 2b3a:11d1 */
void far fx_halve_weapon_below_plus_3(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;

    weapon = fx_weapon(cur_node);
    if (weapon != 0 && (char)((char)weapon->rec.plus < 3))
        damage = damage / 2;
}

/* 2b3a:1214 */
void far fx_halve_weapon_below_plus_4(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;

    weapon = fx_weapon(cur_node);
    if (weapon != 0 && (char)((char)weapon->rec.plus < 4))
        damage = damage / 2;
}

/* 2b3a:1257 */
void far fx_slain_by_type(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;

    weapon = fx_weapon(cur_node);
    if (weapon != 0 && weapon->rec.type == 0x4a)
        fx_disable(character, 6, "is slain");
}

/* 2b3a:129a */
void far fx_poison_attack_penalized(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        fx_poison_attack(character, 0xfe);
}

/* 2b3a:12b5 */
void far fx_adjacent_paralysis(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && get_distance(character, character->combat->target) < 2)
        fx_paralyze(character, 100, 0);
}

/* 2b3a:12f3 */
void far fx_magic_cloak(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        if (!combat_round && !attack_roll)
            effect->strength = effect->strength & 0xf;
        else if (!(effect->strength & 0x10)) {
            attack_roll = -1;
            effect->strength = effect->strength | 0x10;
        }
    }
}

/* 2b3a:1334 */
void far fx_fear_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        fx_cancel(0x6f);
}

/* 2b3a:1347 */
void far fx_fire_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_cancel_if(1);
}

/* 2b3a:1355 */
void far fx_constitution_save_bonus(struct Character far *character, struct Effect far *effect, char removed)
{
    char bonus;

    if (!removed && (current_save_type == 4 || current_save_type == 2 || current_save_type == 0)) {
        switch (character->stats[4].cur) {
        case 4: case 5: case 6:
            bonus = 1;
            break;
        case 7: case 8: case 9: case 10:
            bonus = 2;
            break;
        case 11: case 12: case 13:
            bonus = 3;
            break;
        case 14: case 15: case 16: case 17:
            bonus = 4;
            break;
        case 18: case 19: case 20:
            bonus = 5;
            break;
        }
        save_bonus = save_bonus + bonus;
    }
}

/* 2b3a:13df */
void far fx_resist_charm_sleep(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && fx_roll_dice(1, 100) <= 90) {
        fx_cancel(0xb);
        fx_cancel(0x35);
    }
}

/* 2b3a:140c */
void far fx_constitution_magic_saves(struct Character far *character, struct Effect far *effect, char removed)
{
    char bonus;

    if (!removed && (current_save_type == 4 || current_save_type == 2)) {
        switch (character->stats[4].cur) {
        case 4: case 5: case 6:
            bonus = 1;
            break;
        case 7: case 8: case 9: case 10:
            bonus = 2;
            break;
        case 11: case 12: case 13:
            bonus = 3;
            break;
        case 14: case 15: case 16: case 17:
            bonus = 4;
            break;
        case 18: case 19: case 20:
            bonus = 5;
            break;
        }
        save_bonus = save_bonus + bonus;
    }
}

/* 2b3a:148c */
void far fx_cold_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed && ((char)element_mask & 2))
        fx_cancel(0);
}

/* 2b3a:14a6 */
void far fx_random_secondary_attacks(struct Character far *character, struct Effect far *effect, char removed)
{
    if (fx_roll_dice(1, 4) == 1)
        character->attacks_left[1] = 8;
    else
        character->attacks_left[1] = 4;
}

/* 2b3a:14d1 */
void far fx_terrify(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;
    struct Character far *nearby_character;

    mirror_image_bypass = 1;
    node = 0;
    for (nearby_character = node_list; nearby_character != 0; nearby_character = nearby_character->next)
        if (nearby_character->side != character->side && nearby_character->status == 0 && nearby_character->level_high < 7
            && !find_effect(nearby_character, 0x78, &node))  /* v1.0: no 0x5c test */
            fx_add_message(nearby_character, 0x78, 0, 0, 1, 1, 0, "is terrified");
    mirror_image_bypass = 0;
}

/* 2b3a:158d */
void far fx_forget_spells(struct Character far *character)
{
    int count[4][9];
    unsigned char spell;
    unsigned char spell_slot;
    unsigned short kind;
    unsigned char level;
    char keep;

    mem_zero(count, 0x48);
    if (character->level[5] > 0)
        switch (game_state[(STATE_MOON_PHASE_1 - 1) + character->robe_order]) {
        case 1:
            keep = 1;
            break;
        case 2:
            keep = 2;
            break;
        default:
            keep = 0;
        }
    for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
        if (character->spells[spell_slot] != 0 && !(character->spells[spell_slot] & 0x80)) {
            spell = character->spells[spell_slot] & 0x7f;
            kind = (spell_info - 1)[spell].cast_class;
            level = (spell_info - 1)[spell].level;
            if (count[kind][level - 1] >= (character->slots[kind] - 1)[level]) {
                if ((character->slots[kind] - 1)[level] == 0 || !keep || kind != 2)
                    character->spells[spell_slot] = 0;
                else
                    keep--;
            } else
                count[kind][level - 1]++;
        }
    for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
        if (character->spells[spell_slot] & 0x80) {
            spell = character->spells[spell_slot] & 0x7f;
            kind = (spell_info - 1)[spell].cast_class;
            level = (spell_info - 1)[spell].level;
            if (count[kind][level - 1] >= (character->slots[kind] - 1)[level])
                character->spells[spell_slot] = 0;
            else
                count[kind][level - 1]++;
        }
}

/* 2b3a:17dd */
void far fx_drain(struct Character far *character, unsigned char count)
{
    unsigned char cand;
    unsigned char best_level;
    short idx;
    short best_idx;
    long upper;
    long lower;
    unsigned char pass;
    unsigned char active;
    char any;

    if (character != 0) {
        hero_text(character, "gets drained", 10, 0);
        text_pause();
        for (pass = 1; pass <= count; pass++) {
            active = 0;
            best_level = 0;
            for (idx = 0; idx <= 6; idx++) {
                cand = character->level[idx];
                if (cand > 0 || character->level_max[idx] > 0)
                    active++;
                if (cand > best_level) {
                    best_level = cand;
                    best_idx = idx;
                } else if (cand == best_level && best_level != 0
                           && level_xp_threshold(character, cand, best_idx) > level_xp_threshold(character, best_level, idx)) {
                    best_level = cand;
                    best_idx = idx;
                }
            }
            if (character->level_max[best_idx] <= character->level[best_idx])
                character->level_max[best_idx] = character->level[best_idx];
            if (character->old_xp <= character->xp)
                character->old_xp = character->xp;
            if (character->old_hp_max <= character->hp_max)
                character->old_hp_max = character->hp_max;
            switch (best_level) {
            case 1:
                lower = 0;
                upper = 0;
                break;
            case 2:
                lower = 0;
                upper = level_xp_threshold(character, best_level, best_idx);
                break;
            default:
                lower = level_xp_threshold(character, best_level - 1, best_idx);
                upper = level_xp_threshold(character, best_level, best_idx);
            }
            character->xp = lower + (upper - lower) / 2;
            if (character->level[best_idx] > 0)
                character->level[best_idx]--;
            if (character->level[best_idx] >= character->byte_92)
                hp_adjust(character, class_bit[best_idx], active, 0);
            recalc_from_levels(character);
            fx_forget_spells(character);
        }
        any = 0;
        for (idx = 0; idx <= 6; idx++)
            if (character->level[idx] > 0)
                any = 1;
        if (!any || character->hp_max <= 0)
            /* v1.0 spells it "drainning"; v1.1 corrects both */
            fx_disable(character, 8, "dies from drainning");
        else if (character->hp <= 0)
            fx_disable(character, 4, "collapses from drainning");
    }
}

/* 2b3a:1a95 */
void far fx_need_plus_1(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_need_plus(1);
}

/* 2b3a:1aa2 */
void far fx_level_damage_vs_type_8(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        fx_target = character->combat->target;
        if (get_distance(character, fx_target) <= 1 && (fx_target->type_flags & 8))
            damage += class_level(character, 4);
    }
}

/* 2b3a:1af9 */
void far fx_stunned(struct Character far *character, struct Effect far *effect, char removed)
{
    combat_actions_end(character);
}

/* 2b3a:1b0b */
void far fx_restore_cure_allowance(struct Character far *character, struct Effect far *effect, char removed)
{
    if (removed)
        character->cure = class_level(character, 3) / 5 + 1;
}

/* 2b3a:1b39 */
void far fx_adjacent_double_zap(struct Character far *character, struct Effect far *effect, char removed)
{
    short saved_damage;
    short saved_spell;
    short saved_mask;

    if (!removed && get_distance(cur_node, character) < 2) {
        saved_damage = damage;
        damage *= 2;
        if (damage > 0) {
            saved_mask = element_mask;
            element_mask = 8;
            saved_spell = current_spell;
            current_spell = 0x55;
            hero_text(cur_node, "gets zapped", 10, 1);
            fx_damage(cur_node, damage, 0, 0);
            damage = saved_damage;
            element_mask = saved_mask;
            current_spell = saved_spell;
        }
    }
}

/* 2b3a:1bd5 */
void far fx_giant_str(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        hero_text(character, msg_format("%s%s", game_text[92], fx_names[113]), 10, 1);
}

/* 2b3a:1c12 */
void far fx_paralyze_gaze(struct Character far *character, struct Effect far *effect, char removed)
{
    char saved;
    long node;

    if (!removed) {
        (*hit_proc)(10, 1, &turn_done);
        if (enemy_team(character) != targets[1]->side)
            turn_done = 0;
        if (turn_done) {
            element_mask = 0x10;
            fx_target = targets[1];
            hero_text(character, "gazes...", 10, 0);
            icon_facings(0x14);
            icon_fly(combatant_x(character), combatant_y(character),
                         combatant_x(fx_target), combatant_y(fx_target), 4, 0x2d);
            if (find_effect(character, 0x3b, &node) && find_effect(fx_target, 0x48, &node)) {
                hero_text(fx_target, "reflects it!", 0xc, 0);
                icon_fly(combatant_x(fx_target), combatant_y(fx_target),
                             combatant_x(character), combatant_y(character), 4, 0x2d);
                fx_target = character;
            }
            saved = fx_saving_throw(fx_target, 1, 3) || find_effect(fx_target, 0x7e, &node);
            fx_add_message(fx_target, 0x34, 10, 0xc, 0, 1, saved, "paralyzed");
        }
    }
}

/* 2b3a:1df6 */
void far fx_vary_primary_attack_damage(struct Character far *character, struct Effect far *effect, char removed)
{
    char text[0x28];
    char bonus;

    str_copy(text, "");
    bonus = str_damage(character);
    switch (attacks_used[0]) {
    case 1:
        damage = fx_roll_dice(1, 10) + bonus + 4;
        str_copy(text, msg_format("%s%s", item_words[20], item_words[114]));
        break;
    case 2:
        damage = fx_roll_dice(2, 4) + bonus + 4;
        str_copy(text, msg_format("%s%s", item_words[16], item_words[114]));
        break;
    case 3:
        damage = fx_roll_dice(1, 8) + bonus + 4;
        str_copy(text, msg_format("%s%s", item_words[18], item_words[114]));
        break;
    case 4:
        damage = fx_roll_dice(1, 8) + bonus + 4;
        str_copy(text, msg_format("%s%s", item_words[13], item_words[114]));
        break;
    case 5:
        damage = fx_roll_dice(1, 4) + bonus + 4;
        str_copy(text, msg_format("%s%s", item_words[4], item_words[114]));
        break;
    case 6:
        damage = fx_roll_dice(1, 6) + bonus + 4;
        str_copy(text, msg_format("%s%s", item_words[14], item_words[114]));
        break;
    }
    text_wrap(0x17, 7, 0x26, 9, 7, 0, 1, text);
}

/* 2b3a:2025 */
void far fx_armor_class_minus_four(struct Character far *character, struct Effect far *effect, char removed)
{
    character->ac[0] -= 4;
    character->ac[1] -= 4;
}

/* 2b3a:2039 */
void far fx_druid_fire(struct Character far *character, struct Effect far *effect, char removed)
{
    unsigned short left;

    left = 6;
    if ((char)element_mask & 1) {
        left = effect->strength * 12;
        if ((int)left > damage) {
            left -= damage;
            damage = 0;
            effect->strength = left / 12;
            if (effect->strength == 0)
                fx_remove(character, 0x77, 0L);
        } else {
            damage -= left;
            fx_remove(character, 0x77, 0L);
        }
    }
}

/* 2b3a:20ab */
void far fx_reduce_damage_and_attack(struct Character far *character, struct Effect far *effect, char removed)
{
    if (damage > 2)
        damage -= 2;
    else
        damage = 0;
    attack_roll -= 2;
}

/* 2b3a:20c9 */
void far fx_iron_skin(struct Character far *character, struct Effect far *effect, char removed)
{
    character->ac[0] += 4;
    character->ac[1] += 4;
}

/* 2b3a:20dd */
void far fx_mind_blank(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_cancel(0xb);
    fx_cancel(0x23);
    fx_cancel(0x34);
    fx_cancel(0x44);
    fx_cancel(0x6f);
}

/* 2b3a:210a */
void far fx_need_plus_2(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_need_plus(2);
}

/* 2b3a:2117 */
void far fx_save_bonus_mask_16(struct Character far *character, struct Effect far *effect, char removed)
{
    if ((char)element_mask & 0x10)
        save_bonus = 0x14;
}

/* 2b3a:2128 */
void far fx_entangled(struct Character far *character, struct Effect far *effect, char removed)
{
    combat_actions_end(character);
}

/* 2b3a:213a */
void far fx_paralyze_unconditional(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_paralyze(character, 100, 0);
}

/* 2b3a:2152 */
void far fx_damage_adjacent_characters(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *nearby_character;

    view_cx = combatant_x(character);
    view_cy = combatant_y(character);
    icon_scatter(1, 0x19, 0xc);
    plan_steps(view_cx, view_cy, 1, 0xff, combatant_size(character));
    for (tick_slot = 2; (unsigned char)tick_slot <= step_count; tick_slot++) {
        nearby_character = (heroes - 1)[(steps - 1)[tick_slot].who];
        element_mask = 1;
        fx_damage(nearby_character, fx_roll_damage(1, 6), 0, 0);
    }
}

/* 2b3a:2214 */
void far fx_double_damage_mask_2(struct Character far *character, struct Effect far *effect, char removed)
{
    if ((char)element_mask & 2)
        damage *= 2;
}

/* 2b3a:2228 */
void far fx_heal_from_mask_1_damage(struct Character far *character, struct Effect far *effect, char removed)
{
    if ((char)element_mask & 1) {
        if ((char)element_mask & 8) {
            hero_text(character, game_text[4], 10, 1);
            character->hp += damage;
            if (character->hp > character->hp_max)
                character->hp = character->hp_max;
        }
        fx_cancel(0);
    }
}

/* 2b3a:227f */
void far fx_switch_combat_side(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed) {
        character->quick_fight = 1;
        if (character->npc <= 0x7f || character->npc == 0xb3)
            character->npc = 0xb3;
        else
            character->npc = 0xb2;
        if (view_mode == 5) {
            character->combat->target = 0;
            plan_steps(combatant_x(character), combatant_y(character), 0xff, 0xff, combatant_size(character));
            character->combat->target = (heroes - 1)[(steps - 1)[2].who];
            character->combat->can_cast = 0;
            character->side = character->combat->target->side ^ 1;
            hero_text(character, game_text[3], 10, 1);
        }
    } else {
        if (character->npc == 0xb3) {
            character->npc = 0;
            character->quick_fight = 0;
        }
        character->side = 0;
    }
}

/* 2b3a:2391 */
void far fx_adjacent_save_aura(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *nearby_character;
    long node;

    hero_text(character, game_text[2], 10, 0);
    plan_steps(combatant_x(character), combatant_y(character), 1, 0xff, combatant_size(character));
    for (tick_slot = 2; (unsigned char)tick_slot <= step_count; tick_slot++) {
        nearby_character = (heroes - 1)[(steps - 1)[tick_slot].who];
        if (nearby_character->side != character->side && !fx_saving_throw(nearby_character, 4, 0) && !find_effect(nearby_character, 0x76, &node)) {
            hit_flash(nearby_character, 1, game_text[1]);
            fx_add(character, 0x94, 0, 0xff, 0);
        }
    }
}

/* 2b3a:24a1 */
void far fx_confusion_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_cancel(0x23);
}

/* 2b3a:24ae */
void far fx_poison_attack_unconditional(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_poison_attack(character, 0xfe);
}

/* 2b3a:24c3 */
void far fx_noop_slot_140(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 2b3a:24c8 */
void far fx_charm_sleep_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_cancel(0xb);
    fx_cancel(0x35);
}

/* 2b3a:24dd */
void far fx_hold_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_cancel(0x34);
}

/* 2b3a:24ea */
void far fx_poison_hold_immune(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_cancel(0x37);
    fx_cancel(0x34);
    if (current_save_type == 0)
        save_bonus = 100;
}

/* 2b3a:250b */
void far fx_attack_penalty_two(struct Character far *character, struct Effect far *effect, char removed)
{
    attack_roll -= 2;
}

/* 2b3a:2515 */
void far fx_cap_nonranged_weapon_damage(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;
    long node;

    if (!find_effect(character, 0x96, &node)) {
        weapon = fx_weapon(cur_node);
        if (weapon != 0) {
            if (has_ranged_weapon(character))
                damage = 0;
            else if (item_table[weapon->rec.type].wpn_class != 0)
                damage = int_min(damage, character->mods[0] + 1);
        }
    }
}

/* 2b3a:259d */
void far fx_breath_bolt(struct Character far *character, struct Effect far *effect, char removed)
{
    char turn_finished;
    char did;
    struct Character far *target;
    unsigned char target_index;

    target = 0;
    did = 0;
    if (!combat_round)
        effect->strength = 3;
    if (fx_roll_dice(1, 100) <= 75) {
        if (effect->strength != 0 && fx_roll_dice(1, 100) <= 25) {
            target = 0;
            plan_steps(combatant_x(character), combatant_y(character), 1, 0xff, 1);
            for (target_index = 2; target_index <= step_count; target_index++)
                if ((heroes - 1)[(steps - 1)[target_index].who]->side != character->side)
                    target = (heroes - 1)[(steps - 1)[target_index].who];
            if (target != 0) {
                hero_text(character, "Breathes!", 10, 1);
                did = 1;
                if (fx_saving_throw(target, 3, 0))
                    fx_damage(target, 10, 0, 1);
                else {
                    fx_damage(target, 0x14, 0, 0);
                    fx_add_message(target, 0x21, 0, 0, 0, 0, 0, "is blinded");
                    turn_finished = combat_actions_end(character);
                }
                effect->strength--;
            }
        }
        if (!did) {
            turn_finished = 0;
            target_index = 0;
            do {
                (*hit_proc)(0x82, 1, &turn_done);
                if (targets[1] != 0 && turn_done) {
                    target_index++;
                    hero_text(character, "Fires An Energy Bolt!", 10, 1);
                    icon_facings(0x13);
                    icon_fly(combatant_x(character), combatant_y(character),
                                 combatant_x(targets[1]), combatant_y(targets[1]), 1, 0x1e);
                    fx_damage(targets[1], fx_roll_damage(1, 8) + 2, 0, 0);
                    if (targets[1]->enabled != 0 && target_index == 1) {
                        fx_damage(targets[1], fx_roll_damage(1, 8) + 2, 0, 0);
                        target_index++;
                    }
                    turn_finished = 1;
                } else
                    target_index = 2;
            } while (target_index < 2);
            if (turn_finished)
                turn_finished = combat_actions_end(character);
        }
    }
}

/* 2b3a:287d */
void far fx_go_mad(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->status != 0) {
        character->status = 0;
        character->enabled = 1;
        if (fx_revive(character, 0x14, "goes mad!")) {
            fx_add(character, 0xa0, 0, 0xff, 0);
            fx_add(character, 0xa1, 0, 0xff, 0);
            fx_add(character, 0xa2, 0, 0xff, 0);
            fx_add(character, 0xa3, 6, 0xff, 1);
            fx_remove(character, 0x98, effect);
            mem_fill(character->spells, 0x89, 0);
        }
    }
}

/* 2b3a:2949 */
void far fx_disarm(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;
    struct Drop far *drop_record;

    weapon = fx_weapon(cur_node);
    if (fx_roll_dice(1, 20) >= cur_node->stats[3].cur - 3 && current_spell == 0 && weapon != 0
        && weapon->rec.cursed == 0 && !death_explosion_active && !ice_explosion_active && !meteor_explosion_active
        && get_distance(cur_node, character) == 1) {
        hero_text(cur_node, "loses his weapon", 10, 1);
        drop_record = drops;
        if (weapon->rec.type != 6 || weapon->rec.name_ids[2] != 0x79) {
            item_equip_toggle(weapon);
            if (drop_record == 0) {
                pool_alloc(&drops, 0x45, &dropped_item_pool);
                drops->item = *weapon;
                drops->owner = cur_node;
                drops->next = 0;
            } else {
                while (drop_record->next != 0)
                    drop_record = drop_record->next;
                pool_alloc(&drop_record->next, 0x45, &dropped_item_pool);
                drop_record = drop_record->next;
                drop_record->item = *weapon;
                drop_record->owner = cur_node;
                drop_record->next = 0;
            }
            item_remove(cur_node, weapon);
            cur_node->equipped[0] = 0;
            hero_panel(cur_node);
        }
    }
}

/* 2b3a:2b1e */
void far fx_queue_death_explosion(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!character->enabled) {
        character->status = 10;
        death_explosion_count++;
        if (death_explosion_count < 1)
            death_explosion_count = 1;
        if (death_explosion_count <= 0x14)
            death_explosion_queue[death_explosion_count] = character;
        else
            death_explosion_count = 0x14;
        fx_remove(character, 0x9a, 0L);
    }
}

/* 2b3a:2b7e */
void far fx_puddle(char x, char y, struct Character far *character)
{
    short cx;
    short cy;
    struct Cloud far *cloud;
    unsigned char tile;
    unsigned char occupant_slot;
    int old[9];

    pool_alloc(&cloud, 0x21, &cloud_pool);
    mem_zero(cloud, 0x21);
    mem_zero(old, 0x12);
    cloud->x = x;
    cloud->y = y;
    cloud->caster = character;
    cloud->next = 0;
    cloud->kind = 0x46;
    cloud->strength = fx_roll_dice(1, 6);
    cx = cloud->x;
    cy = cloud->y;
    if (cx >= 0 && cx <= 0x31 && cy >= 0 && cy <= 0x18) {
        cell_get(cx, cy, &occupant_slot, &tile);
        if (occupant_slot == old[0])
            occupant_slot = 0;
        old[0] = occupant_slot;
        tile = combat_terrain_under(cx, cy);
        cloud->on[1] = tile > 0 && (tile_info - 1)[tile].cost < 0xff;
        cloud->tile[1] = tile;
        if (cloud->on[1])
            cur_map->cells[cy][cx] = cloud->kind;
    } else
        cloud->on[1] = 0;
    cloud->next = cloud_list;
    cloud_list = cloud;
    cur_map->cells[cloud->y][cloud->x] = 0x46;
    combat_view_scroll(combatant_x(character), combatant_y(character), 0xff, 8);
}

/* 2b3a:2d38 */
void far fx_acid_pools(struct Character far *character, struct Effect far *effect, char removed)
{
    int dx;
    int dy;
    unsigned char x;
    unsigned char y;
    unsigned char tile;

    x = combatant_x(character);
    y = combatant_y(character);
    if (character->hp <= 0 || !character->enabled) {
        for (dx = -1; dx <= 1; dx++)
            for (dy = -1; dy <= 1; dy++)
                if (random_n(2) == 1 || dx == 0 && dy == 0)
                    if (x + dx >= 0 && x + dx <= 0x31 && y + dy >= 0 && y + dy <= 0x18) {
                        tile = cur_map->cells[y + dy][x + dx];
                        if ((tile_info - 1)[tile].cost == 1)
                            fx_puddle(x + dx, y + dy, character);
                    }
        hero_text(character, "dissolves into acid!", 10, 0);
    }
}

/* 2b3a:2e53 */
void far fx_cancel_hold(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_cancel(0x34);
}

/* 2b3a:2e60 */
void far fx_new_form(struct Character far *character, struct Effect far *effect, char removed)
{
    if (character->status != 0) {
        character->status = 0;
        character->enabled = 1;
        if (fx_revive(character, 0x14, "rises in a new form")) {
            fx_add(character, 0xa5, 0, 0xff, 0);
            fx_add(character, 0xd2, 0, 0xff, 0);
            fx_add(character, 0xa6, 3, 0xff, 0);
            fx_add(character, 0xa7, 0, 0xff, 0);
            fx_remove(character, 0xa0, 0L);
            fx_remove(character, 0x97, 0L);
            fx_remove(character, 0xa1, 0L);
            fx_remove(character, 0xa2, 0L);
            fx_remove(character, 0xa3, 0L);
        }
    }
}

/* 2b3a:2f61 */
void far fx_immolate(struct Character far *character, struct Effect far *effect, char removed)
{
    int last;
    unsigned char step_index;
    int who[8];

    plan_steps(combatant_x(character), combatant_y(character), 1, 0xff, combatant_size(character));
    hero_text(character, "immolates", 10, 1);
    last = step_count - 1;
    for (step_index = 2; step_index <= step_count; step_index++)
        who[step_index - 2] = (steps - 1)[step_index].who;
    for (step_index = 1; step_index <= last; step_index++)
        if ((heroes - 1)[who[step_index - 1]]->enabled) {
            element_mask = 1;
            if (enemy_team(character) == (heroes - 1)[who[step_index - 1]]->side)
                fx_damage((heroes - 1)[who[step_index - 1]], fx_roll_damage(1, 6), 0, 0);
        }
}

/* 2b3a:30a9 */
void far fx_attack_and_armor_plus_two(struct Character far *character, struct Effect far *effect, char removed)
{
    attack_roll += 2;
    character->ac[0] += 2;
    character->ac[1] += 2;
}

/* 2b3a:30c2 */
void far fx_install_death_effects(struct Character far *character, struct Effect far *effect, char removed)
{
    if (removed) {
        fx_add(character, 0xa5, 0, 0xff, 0);
        fx_add(character, 0xa6, 3, 0xff, 1);
        fx_add(character, 0xd5, 0, 0xff, 0);
        fx_remove(character, 0x97, 0L);
        fx_remove(character, 0xa0, 0L);
        fx_remove(character, 0xa1, 0L);
        fx_remove(character, 0xa2, 0L);
    }
}

/* 2b3a:316c */
void far fx_cancel_effect_and_damage(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_cancel(0);
    damage = 0;
}

/* 2b3a:317f */
void far fx_explode(struct Character far *character, struct Effect far *effect, char removed)
{
    int step_index;
    int last;
    int who[8];
    long node;
    char has;

    has = find_effect(character, 0xbf, &node);
    if (removed) {
        plan_steps(combatant_x(character), combatant_y(character), 1, 0xff, combatant_size(character));
        fx_disable(character, 8, "explodes!");
        last = step_count - 1;
        for (step_index = 2; step_index <= step_count; step_index++)
            who[step_index - 2] = (steps - 1)[step_index].who;
        for (step_index = 1; step_index <= last; step_index++)
            if ((heroes - 1)[who[step_index - 1]] != 0 && (heroes - 1)[who[step_index - 1]]->enabled) {
                if (has)
                    fx_damage((heroes - 1)[who[step_index - 1]], fx_roll_damage(0x10, 8), 0, 0);
                else
                    fx_damage((heroes - 1)[who[step_index - 1]], fx_roll_damage(3, 6), 0, 0);
                if (!fx_saving_throw((heroes - 1)[who[step_index - 1]], 4, 0)
                    && !find_effect((heroes - 1)[who[step_index - 1]], 0x98, &node)
                    && !find_effect((heroes - 1)[who[step_index - 1]], 0xa0, &node)
                    && !find_effect((heroes - 1)[who[step_index - 1]], 0xa6, &node)) {
                    hero_text((heroes - 1)[who[step_index - 1]], "is stunned", 10, 1);
                    if (!find_effect((heroes - 1)[who[step_index - 1]], 0x35, &node))
                        fx_add((heroes - 1)[who[step_index - 1]], 0x35, 0, 0xff, 0);
                }
            }
        turn_checks();
    }
}

/* 2b3a:3441 */
void far fx_zap(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *target;
    long node;

    target = 0;
    plan_steps(combatant_x(character), combatant_y(character), 1, 0xff, combatant_size(character));
    turn_done = 0;
    tick_slot = 2;
    do {
        if ((unsigned char)tick_slot <= step_count
            && (heroes - 1)[(steps - 1)[tick_slot].who] != 0
            && (heroes - 1)[(steps - 1)[tick_slot].who]->side != character->side) {
            target = (heroes - 1)[(steps - 1)[tick_slot].who];
            turn_done = 1;
        }
        tick_slot++;
    } while (!turn_done && (unsigned char)tick_slot <= step_count);
    if (target != 0) {
        hero_text(target, "gets zapped!", 10, 1);
        if (find_effect(character, 0xbf, &node))
            fx_damage(target, fx_roll_damage(8, 8), 0, 0);
        else
            fx_damage(target, fx_roll_damage(2, 6), 0, 0);
        combat_actions_end(character);
    }
}

/* 2b3a:35ca */
void far fx_add_damage_dice_count(struct Character far *character, struct Effect far *effect, char removed)
{
    if ((element_mask & 1) && (element_mask & 8))
        damage += damage_dice_count;
}

/* 2b3a:35e8 */
void far fx_halve_magic_weapon_damage(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;

    weapon = fx_weapon(cur_node);
    if (weapon != 0 && (char)weapon->rec.plus > 0)
        damage = damage / 2;
}

/* 2b3a:3620 */
void far fx_disease_touch(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;

    fx_target = character->combat->target;
    /* v1.0 tests for and adds 0xac/0x31 where v1.1 uses 0xd7, and packs the rot's
     * count in strength's high nibble (<< 4, + 0xf) where v1.1 uses the top three bits */
    if (!find_effect(fx_target, 0x32, &node) && !find_effect(fx_target, 0xac, &node)) {
        fx_add(fx_target, 0x31, 0x3840, 0xff, 1);
        fx_add(fx_target, 0xac, 0xa8c0, (fx_roll_dice(1, 6) << 4) + 0xf, 1);
        hero_text(fx_target, "is Diseased", 10, 1);
    }
}

/* 2b3a:36db */
void far fx_fear_aura(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *nearby_character;
    char bonus;
    char saved;

    for (nearby_character = node_list; nearby_character != 0; nearby_character = nearby_character->next) {
        fx_remove(nearby_character, 0xab, 0L);
        if (nearby_character->enabled && enemy_team(character) == nearby_character->side) {
            bonus = 0;
            if (side_count[nearby_character->side] >= 6)
                bonus++;
            if (nearby_character->race == 6)
                bonus += 2;
            saved = fx_saving_throw(nearby_character, 0, bonus);
            fx_add_message(nearby_character, 0x34, fx_roll_dice(1, 4), 0xc, 0, 1, saved, "is paralyzed with fear");
        }
    }
}

/* 2b3a:37aa */
void far fx_rotting(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!fx_lock) {
        hero_text(character, "Rots...", 10, 1);
        if (character->stats[5].cur > 2)        /* v1.0: > 2, else 0; v1.1: > 3 */
            character->stats[5].cur -= 2;
        else
            character->stats[5].cur = 0;
        if (character->stats[5].cur < 3)
            character->stats[5].cur = 3;
        if (effect->strength / 0x10 > 1) {      /* v1.0: steps of 0x10 */
            effect->strength -= 0x10;
            fx_rearm(character, 0xac, effect->strength, 0xa8c0);
        } else
            fx_disable(character, 8, "Dies rots away");
    }
}

/* 2b3a:3842 */
void far fx_heal_eight_on_mask_4(struct Character far *character, struct Effect far *effect, char removed)
{
    if ((char)element_mask & 4) {
        fx_cancel(0);
        fx_roll_dice(1, 8);             /* the roll is dropped: 2b3a:385a */
        character->hp += 8;
    }
}

/* 2b3a:386c */
void far fx_save_against_mask_1_or_2(struct Character far *character, struct Effect far *effect, char removed)
{
    if (((char)element_mask & 1) || ((char)element_mask & 2)) {
        if (fx_saving_throw(character, 4, 0) && (spell_info - 1)[current_spell].save_effect != 0)
            damage = 0;
        else
            damage = damage / 2;
    }
}

/* 2b3a:38c4 */
void far fx_halve_damage(struct Character far *character, struct Effect far *effect, char removed)
{
    damage = damage / 2;
}

/* 2b3a:38d5 */
void far fx_engulf(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;
    struct Character far *target;

    target = character->combat->target;
    if (hits_left[0] == 2 && target->enabled && !find_effect(target, 0xb2, &node)
        && !find_effect(target, 0xb1, &node) && !find_effect(target, 0x96, &node)) {
        target = character->combat->target;
        hero_text(character, msg_format("engulfs %s", target->name), 0xc, 1);
        fx_add(target, 0xb2, 0, combatant_slot(target), 0);
        fx_dispatch(0xb2, target, 0L, 0);
        fx_add(target, 0xb1, 0, fx_roll_dice(2, 4), 0);
        fx_add(character, 0xb3, 0, combatant_slot(target), 1);
    }
}

/* 2b3a:3a1f */
void far fx_suffocate(struct Character far *character, struct Effect far *effect, char removed)
{
    if (effect->strength == 0)
        fx_disable(character, 6, "Suffocates");
    else
        effect->strength--;
}

/* 2b3a:3a4d */
void far fx_attack_linked_character(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_target = (heroes - 1)[effect->strength];
    if (fx_target == 0)
        fx_remove(character, 0xb3, effect);
    else if (removed || !character->enabled || !fx_target->enabled) {
        fx_remove(fx_target, 0xb2, 0L);
        fx_target->combat->movement = calc_move(fx_target);
        fx_remove(fx_target, 0xb1, 0L);
        if (!removed || !character->enabled) {
            effect->on_remove = 0;
            fx_remove(character, 0xb3, effect);
        }
    } else {
        character->attacks_left[0] = 2;
        character->attacks_left[1] = 0;
        character->rolls[0] = 2;
        character->dice[0] = 8;
        combat_attack(character, fx_target, 1, 0L, &turn_done);
        turn_done = combat_actions_end(character);
        if (!fx_target->enabled) {
            fx_remove(character, 0xb3, 0L);
            fx_remove(fx_target, 0xb2, 0L);
            fx_remove(fx_target, 0xb1, 0L);
        }
    }
}

/* 2b3a:3baa */
void far fx_regenerate_with_enemies(struct Character far *character, struct Effect far *effect, char removed)
{
    if (side_count[1] > 0 && fx_heal(character, 1, 1))
        heal_msg(character);
}

/* 2b3a:3bda */
void far fx_burst(struct Character far *character, struct Effect far *effect, char removed)
{
    int last;
    unsigned char step_index;
    int who[8];
    long node;
    char saved;

    if (!find_effect(character, 0xc2, &node)) {
        hero_text(character, "explodes.", 10, 1);
        plan_steps(combatant_x(character), combatant_y(character), 2, 0xff, combatant_size(character));
        if (step_count > 1) {
            last = step_count - 1;
            for (step_index = 2; step_index <= step_count; step_index++)
                who[step_index - 2] = (steps - 1)[step_index].who;
            for (step_index = 1; step_index <= last; step_index++) {
                saved = fx_saving_throw((heroes - 1)[who[step_index - 1]], 2, 0);
                if (!find_effect((heroes - 1)[who[step_index - 1]], 0xb7, &node))
                    fx_damage((heroes - 1)[who[step_index - 1]], fx_roll_damage(6, 6), 2, saved);
            }
        }
        character->enabled = 0;
        character->status = 6;
        hero_fall(character);
    }
    turn_checks();
}

/* 2b3a:3d76 */
void far fx_disease_target_and_die(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;

    fx_target = character->combat->target;
    if (!find_effect(fx_target, 0x4c, &node)) {
        fx_add(fx_target, 0xb9, 0x5a0, 0xff, 1);
        hero_text(fx_target, "is Diseased", 10, 1);
    }
    fx_add(character, 0xc2, 0, 0xff, 1);
    fx_disable(character, 6, "");
}

/* 2b3a:3e15 */
void far fx_die_on_disease_removal(struct Character far *character, struct Effect far *effect, char removed)
{
    if (removed)
        fx_disable(character, 6, "dies from disease");
}

/* 2b3a:3e36 */
void far fx_armor_acid(struct Character far *character, struct Effect far *effect, char removed)
{
    int floor;
    struct Item far *armor_item;
    char hit;

    fx_target = character->combat->target;
    armor_item = fx_target->items;
    hit = 0;
    for (; armor_item != 0; armor_item = armor_item->next)
        if (armor_item->rec.equipped && item_table[armor_item->rec.type].slot == 2) {
            hit = 1;
            if (armor_item->rec.type == 0x23 || armor_item->rec.type == 0x24 || armor_item->rec.type == 0x26)
                floor = -1;
            else
                floor = 0;
            if ((char)armor_item->rec.plus > floor) {
                armor_item->rec.plus--;
                hero_text(character, msg_format("is Disolving %s's armor!", fx_target->name), 10, 1);
            } else {
                hero_text(character, msg_format("Dissolves %s's armor!", fx_target->name), 10, 1);
                item_remove(fx_target, armor_item);
            }
        }
    if (hit)
        damage = 0;
}

/* 2b3a:3f58 */
void far fx_disarm_hit(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;
    struct Drop far *drop_record;

    if (get_distance(character, cur_node) <= 1 && damage != 0 && damage % 3 == 0) {
        weapon = fx_weapon(cur_node);
        if (fx_roll_dice(1, 20) >= cur_node->stats[3].cur - 3 && current_spell == 0 && weapon != 0
            && weapon->rec.cursed == 0 && !death_explosion_active && !ice_explosion_active && !meteor_explosion_active) {
            hero_text(cur_node, "loses his weapon", 10, 1);
            drop_record = drops;
            if (weapon->rec.type != 6 || weapon->rec.name_ids[2] != 0x79) {
                item_equip_toggle(weapon);
                if (drop_record == 0) {
                    pool_alloc(&drops, 0x45, &dropped_item_pool);
                    drops->item = *weapon;
                    drops->owner = cur_node;
                    drops->next = 0;
                } else {
                    while (drop_record->next != 0)
                        drop_record = drop_record->next;
                    pool_alloc(&drop_record->next, 0x45, &dropped_item_pool);
                    drop_record = drop_record->next;
                    drop_record->item = *weapon;
                    drop_record->owner = cur_node;
                    drop_record->next = 0;
                }
                item_remove(cur_node, weapon);
                cur_node->equipped[0] = 0;
                hero_panel(cur_node);
            }
        }
    }
}

/* 2b3a:4147 */
void far fx_queue_ice_explosion(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!character->enabled) {
        character->status = 10;
        ice_explosion_count++;
        if (ice_explosion_count < 1)
            ice_explosion_count = 1;
        if (ice_explosion_count <= 0x14)
            ice_explosion_queue[ice_explosion_count] = character;
        else
            ice_explosion_count = 0x14;
        fx_remove(character, 0xbc, 0L);
    }
}

/* 2b3a:41a7 */
void far fx_queue_meteor_explosion(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!character->enabled) {
        character->status = 10;
        meteor_explosion_count++;
        if (meteor_explosion_count < 1)
            meteor_explosion_count = 1;
        if (meteor_explosion_count <= 0x14)
            meteor_explosion_queue[meteor_explosion_count] = character;
        else
            meteor_explosion_count = 0x14;
        fx_remove(character, 0xbd, 0L);
    }
}

/* 2b3a:4207 */
void far fx_increase_mask_2_damage(struct Character far *character, struct Effect far *effect, char removed)
{
    if ((char)element_mask & 2)
        damage = damage + damage / 5;
}

/* 2b3a:4223 */
void far fx_need_plus_any(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;

    fx_need_plus(0xff);
    weapon = fx_weapon(cur_node);
    if (weapon == 0 && !(element_mask & 8))
        damage = 0;
}

/* 2b3a:425b */
void far fx_gorgon_breath(struct Character far *character, struct Effect far *effect, char removed)
{
    char x;
    char y;
    unsigned char target_index;
    struct Character far *target;
    char saved;
    char size;
    char reach;
    char dir;

    if (effect->strength != 0) {
        effect->strength--;
        mirror_image_bypass = 1;
        element_mask = 0;
        size = 2;
        reach = 6;
        x = combatant_x(character);
        y = combatant_y(character);
        (*hit_proc)(0x17, 1, &turn_done);
        if (turn_done) {
            spell_path_targets(x, y, view_cx, view_cy, size, reach);
            for (target_index = 1; target_index <= target_count; target_index++)
                if (enemy_team(character) != targets[target_index]->side)
                    turn_done = 0;
            if (target_count > 0 && turn_done) {
                hero_text(character, "breathes", 10, 1);
                dir = combatant_direction(character, targets[1]);
                hero_panel(character);
                combatant_facing_set(character, dir, 1, 0);
                icon_facings(0x13);
                icon_fly(combatant_x(character), combatant_y(character),
                             combatant_x(targets[1]), combatant_y(targets[1]), 1, 0x1e);
                for (target_index = 1; target_index <= target_count; target_index++)
                    if (targets[target_index] != 0) {
                        target = targets[target_index];
                        saved = fx_saving_throw(target, 1, 0);
                        if (!saved)
                            fx_disable(target, 7, "is petrified by the gorgon's breath");
                    }
                combat_actions_end(character);
            }
        }
        mirror_image_bypass = 0;
    }
}

/* 2b3a:445c */
void far fx_green_fire(struct Character far *character, struct Effect far *effect, char removed)
{
    unsigned char minutes;

    /* v1.0 rearms first and burns only when the rearm succeeds; it has no fx_lock
     * test, no final hp test, and never turns status 4/5 into 6 */
    if (character->hp > 0 && character->hp < character->hp_max) {
        if (view_mode == 5)
            minutes = 1;
        else
            minutes = 10;
        if (fx_rearm(character, 0xc4, 0xff, minutes) && !no_study) {
            element_mask = 1;
            fx_damage(character, fx_roll_damage(1, 8), 0, 0);
            if (damage != 0 && character->status != 3) {
                if (character->hp == 0) {
                    if (view_mode == 5)
                        hero_text(character, "is engulfed by green fire!", 10, 1);
                    else
                        fx_disable(character, 6, "is engulfed by green fire!");
                } else
                    hero_text(character, "is turning to green fire!", 10, 1);
            }
        }
    }
}

/* 2b3a:4596 */
void far fx_cancel_adjacent_plain_weapon(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *weapon;

    if (get_distance(character, cur_node) <= 1) {
        weapon = fx_weapon(cur_node);
        if (weapon == 0 || weapon->rec.plus == 0)
            damage = 0;
    }
}

/* 2b3a:45e2 */
void far fx_disintegrate(struct Character far *character, struct Effect far *effect, char removed)
{
    char x;
    char y;
    unsigned char target_index;
    struct Character far *target;
    char saved;
    char size;
    char reach;
    char dir;

    if ((unsigned char)combat_round % 6 == 0 || effect->strength > 1)
        effect->strength = 1;
    if (effect->strength > 0) {
        mirror_image_bypass = 1;
        element_mask = 0;
        size = 1;
        reach = 0xd;
        x = combatant_x(character);
        y = combatant_y(character);
        (*hit_proc)(0x17, 1, &turn_done);
        if (turn_done) {
            spell_path_targets(x, y, view_cx, view_cy, size, reach);
            for (target_index = 1; target_index <= target_count; target_index++)
                if (enemy_team(character) != targets[target_index]->side)
                    turn_done = 0;
            if (target_count > 0 && turn_done) {
                hero_text(character, "flashes ray", 10, 1);
                dir = combatant_direction(character, targets[1]);
                hero_panel(character);
                combatant_facing_set(character, dir, 1, 0);
                icon_facings(0x13);
                icon_fly(combatant_x(character), combatant_y(character),
                             combatant_x(targets[1]), combatant_y(targets[1]), 1, 0x1e);
                for (target_index = 1; target_index <= target_count; target_index++)
                    if (targets[target_index] != 0) {
                        target = targets[target_index];
                        saved = fx_saving_throw(target, 3, 0);
                        fx_damage(target, fx_roll_damage(4, 4), 0, saved);
                        if (target->hp == 0)
                            fx_disable(target, 8, "is disintegrated");
                    }
                effect->strength--;
                combat_actions_end(character);
            }
        }
        mirror_image_bypass = 0;
    }
}

/* 2b3a:4830 */
void far fx_paralyze_after_target_save(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!fx_saving_throw(character->combat->target, 4, -4))
        fx_paralyze(character, fx_roll_dice(3, 6), 0);
}

/* 2b3a:4873 */
void far fx_green_fire_touch(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;

    if (!fx_saving_throw(character->combat->target, 4, 0)) {
        if (!find_effect(character->combat->target, 0xc4, &node))
            fx_add(character->combat->target, 0xc4, 1, 0, 1);
        if (damage != 0) {
            if (character->combat->target->hp <= 0)
                hero_text(character->combat->target, "is engulfed in green fire!", 10, 1);
            else
                hero_text(character->combat->target, "is turning to green fire!", 10, 1);
        }
    }
}

/* 2b3a:4949 */
void far fx_grant_gorgon_breath(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;
    struct Character far *nearby_character;

    for (nearby_character = node_list; nearby_character != 0; nearby_character = nearby_character->next)
        if (find_effect(nearby_character, 0xc3, &node) && !find_effect(character, 0xcb, &node))
            fx_add(character, 0xcb, 0xff, 4, 0);
}

/* 2b3a:49c2 */
void far fx_resist_damage_and_save_bonus(struct Character far *character, struct Effect far *effect, char removed)
{
    if (element_mask & 1) {
        damage = damage / 5 * 4;
        save_bonus += 4;
    }
}

/* 2b3a:49e4 */
void far fx_apply_venom_to_target(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Character far *target;

    target = character->combat->target;
    if (!fx_saving_throw(target, 0, 0)) {
        hero_text(target, "is crippled by venom.", 10, 1);
        fx_remove(target, 0xcf, 0L);
        fx_add(target, 0xcf, fx_roll_dice(1, 4), 0xff, 0);
    }
}

/* 2b3a:4a68 */
void far fx_venom_attack_penalty(struct Character far *character, struct Effect far *effect, char removed)
{
    if (!removed)
        attack_roll--;
    else
        fx_remove(character, 0xcf, 0L);
}

/* 2b3a:4a8d */
void far fx_constrict(struct Character far *character, struct Effect far *effect, char removed)
{
    long node;
    struct Character far *target;

    if (fx_roll_dice(1, 10) <= 4) {
        target = character->combat->target;
        if (hits_left[0] == 2 && target->enabled && !find_effect(target, 0xb2, &node)
            && !find_effect(target, 0xb1, &node) && !find_effect(target, 0x96, &node)) {
            target = character->combat->target;
            hero_text(character, msg_format("constricts %s", target->name), 0xc, 1);
            fx_add(target, 0xb2, 0, combatant_slot(target), 0);
            fx_dispatch(0xb2, target, 0L, 0);
            fx_add(target, 0xb1, 0, fx_roll_dice(2, 4), 0);
            fx_add(character, 0xb3, 0, combatant_slot(target), 1);
        }
    }
}

/* 2b3a:4bec */
void far fx_blinding_ray(struct Character far *character, struct Effect far *effect, char removed)
{
    char x;
    char y;
    unsigned char target_index;
    struct Character far *target;
    char saved;
    char size;
    char reach;
    char dir;

    mirror_image_bypass = 1;
    element_mask = 0;
    size = 2;
    reach = 3;
    x = combatant_x(character);
    y = combatant_y(character);
    (*hit_proc)(0x17, 1, &turn_done);
    if (turn_done) {
        spell_path_targets(x, y, view_cx, view_cy, size, reach);
        for (target_index = 1; target_index <= target_count; target_index++)
            if (enemy_team(character) != targets[target_index]->side)
                turn_done = 0;
        if (target_count > 0 && turn_done) {
            hero_text(character, "gazes", 10, 1);
            dir = combatant_direction(character, targets[1]);
            hero_panel(character);
            combatant_facing_set(character, dir, 1, 0);
            icon_facings(0x13);
            icon_fly(combatant_x(character), combatant_y(character),
                         combatant_x(targets[1]), combatant_y(targets[1]), 1, 0x1e);
            for (target_index = 1; target_index <= target_count; target_index++)
                if (targets[target_index] != 0) {
                    target = targets[target_index];
                    saved = fx_saving_throw(target, 0, 0);
                    fx_add_message(target, 0x34, fx_roll_dice(2, 4), 0xff, 0, 1, saved,
                                 "is stunned by the Eye of the Deep's Blinding Ray");
                }
            combat_actions_end(character);
        }
    }
    mirror_image_bypass = 0;
}

/* 2b3a:4def */
void far fx_poison_bite(struct Character far *character, struct Effect far *effect, char removed)
{
    fx_target = character->combat->target;
    if (!fx_saving_throw(fx_target, 0, 0)) {
        damage = damage + fx_roll_dice(1, 6);
        fx_add_message(fx_target, 0x34, fx_roll_dice(3, 6), 0xc, 0, 1, 0, "paralyzed by poison");
    }
}

/* 2b3a:4e63 */
void far fx_anemone_acid(struct Character far *character, struct Effect far *effect, char removed)
{
    int saved;

    fx_target = cur_node;
    if (get_distance(fx_target->combat->target, fx_target) <= 1 && damage <= 0xf) {
        hero_text(fx_target, "is squirted by anemone's acid!", 10, 1);
        saved = element_mask;
        element_mask = 0x100;
        fx_damage(fx_target, fx_roll_damage(3, 4), 0, 0);
        element_mask = saved;
    }
}

/* v1.0 has no fx_rot_touch (v1.1 2b3a:4ee7, effect 0xd7) */

/* 2b3a:4f2f */
void far fx_none(struct Character far *character, struct Effect far *effect, char removed)
{
}

/* 2b3a:4f34 */
void far fx_item(struct Character far *character, struct Effect far *effect, char removed)
{
    struct Item far *item;

    item_handler_active = 0;
    item = (struct Item far *)effect;
    if (removed)
        fx_remove(character, item->rec.u.magic.effect, 0L);
    else if (item->rec.u.magic.effect == 0x96)
        fx_add(character, item->rec.u.magic.effect, 0, 0xff, 0);
    else
        fx_add(character, item->rec.u.magic.effect, 0, 0xff, 1);
}

/* 2b3a:4fb2 */
void far fx_install(void)
{
    (fx_handler - 1)[0] = fx_none;      /* bee8..beeb, other modules' flags */
    (fx_handler - 1)[1] = fx_bless;
    (fx_handler - 1)[2] = fx_curse;
    (fx_handler - 1)[3] = fx_snakes;
    (fx_handler - 1)[4] = fx_dispel_evil;
    (fx_handler - 1)[5] = fx_none;
    (fx_handler - 1)[6] = fx_elec_immune;
    (fx_handler - 1)[7] = fx_noop;
    (fx_handler - 1)[8] = fx_prot_evil;
    (fx_handler - 1)[9] = fx_prot_good;
    (fx_handler - 1)[10] = fx_resist_cold;
    (fx_handler - 1)[11] = fx_charm;
    (fx_handler - 1)[12] = fx_enlarged;
    (fx_handler - 1)[13] = fx_barkskin;
    (fx_handler - 1)[14] = fx_friendly;
    (fx_handler - 1)[15] = fx_slow_poison;
    (fx_handler - 1)[16] = fx_read_magic;
    (fx_handler - 1)[17] = fx_shielded;
    (fx_handler - 1)[18] = fx_half_elf;
    (fx_handler - 1)[19] = fx_none;
    (fx_handler - 1)[20] = fx_resist_fire;
    (fx_handler - 1)[21] = fx_silence;
    (fx_handler - 1)[22] = fx_slow_poison_end;
    (fx_handler - 1)[23] = fx_spirit_hammer;
    (fx_handler - 1)[24] = fx_none;
    (fx_handler - 1)[25] = fx_invisible;
    (fx_handler - 1)[26] = fx_attack_bonus_vs_type80;
    (fx_handler - 1)[27] = fx_end_actions;
    (fx_handler - 1)[28] = fx_mirror_image;
    (fx_handler - 1)[29] = fx_weakened;
    (fx_handler - 1)[30] = fx_nausea;
    (fx_handler - 1)[31] = fx_end_actions;
    (fx_handler - 1)[32] = fx_dispel_evil_attack;
    (fx_handler - 1)[33] = fx_blind;
    (fx_handler - 1)[34] = fx_diseased;
    (fx_handler - 1)[35] = fx_confusion;
    (fx_handler - 1)[36] = fx_accursed;
    (fx_handler - 1)[37] = fx_blink;
    (fx_handler - 1)[38] = fx_enlarged;
    (fx_handler - 1)[39] = fx_hastened;
    (fx_handler - 1)[40] = fx_block_lower_spell_levels;
    (fx_handler - 1)[41] = fx_missile_prot;
    (fx_handler - 1)[42] = fx_slowed;
    (fx_handler - 1)[43] = fx_periodic_strength_drain;
    (fx_handler - 1)[44] = fx_periodic_hp_drain;
    (fx_handler - 1)[45] = fx_prot_evil;
    (fx_handler - 1)[46] = fx_prot_good;
    (fx_handler - 1)[47] = fx_attack_penalty_type_4;
    (fx_handler - 1)[48] = fx_noop_slot_48;
    (fx_handler - 1)[49] = fx_prayer;
    (fx_handler - 1)[50] = fx_fire_shield_hot;
    (fx_handler - 1)[51] = fx_end_actions;
    (fx_handler - 1)[52] = fx_end_actions;
    (fx_handler - 1)[53] = fx_end_actions;
    (fx_handler - 1)[54] = fx_fire_shield_cold;
    (fx_handler - 1)[55] = fx_poisoned;
    (fx_handler - 1)[56] = fx_invis_ring;
    (fx_handler - 1)[57] = fx_major_globe;
    (fx_handler - 1)[58] = fx_stone_gaze;
    (fx_handler - 1)[59] = fx_noop_slot_59;
    (fx_handler - 1)[60] = fx_need_plus_3;
    (fx_handler - 1)[61] = fx_fire_ring;
    (fx_handler - 1)[62] = fx_periodic_regeneration;
    (fx_handler - 1)[63] = fx_minor_globe;
    (fx_handler - 1)[64] = fx_poison_attack_normal;
    (fx_handler - 1)[65] = fx_petrify_attack;
    (fx_handler - 1)[66] = fx_halve_low_class_weapon_damage;
    (fx_handler - 1)[67] = fx_halve_high_bit_weapon_damage;
    (fx_handler - 1)[68] = fx_feeblemind;
    (fx_handler - 1)[69] = fx_animal_invisibility;
    (fx_handler - 1)[70] = fx_confusing_gaze;
    (fx_handler - 1)[71] = fx_highlighted;
    (fx_handler - 1)[72] = fx_noop_slot_72;
    (fx_handler - 1)[73] = fx_breath_immune;
    (fx_handler - 1)[74] = fx_magic_boots;
    (fx_handler - 1)[75] = fx_damage_vs_immunity1;
    (fx_handler - 1)[76] = fx_disease_immune;
    (fx_handler - 1)[77] = fx_berserk;
    (fx_handler - 1)[78] = fx_magic_stone;
    (fx_handler - 1)[79] = fx_healed_by_fire;
    (fx_handler - 1)[80] = fx_halve_weapon_below_plus_3;
    (fx_handler - 1)[81] = fx_halve_weapon_below_plus_4;
    (fx_handler - 1)[82] = fx_slain_by_type;
    (fx_handler - 1)[86] = fx_poison_attack_penalized;
    (fx_handler - 1)[87] = fx_none;
    (fx_handler - 1)[88] = fx_adjacent_paralysis;
    (fx_handler - 1)[89] = fx_magic_cloak;
    (fx_handler - 1)[92] = fx_fear_immune;
    (fx_handler - 1)[93] = fx_fire_immune;
    (fx_handler - 1)[94] = fx_constitution_save_bonus;
    (fx_handler - 1)[95] = fx_resist_charm_sleep;
    (fx_handler - 1)[97] = fx_constitution_magic_saves;
    (fx_handler - 1)[98] = fx_cold_immune;
    (fx_handler - 1)[99] = fx_random_secondary_attacks;
    (fx_handler - 1)[100] = fx_terrify;
    (fx_handler - 1)[101] = fx_drain_one_level;
    (fx_handler - 1)[102] = fx_drain_two_levels;
    (fx_handler - 1)[103] = fx_need_plus_1;
    (fx_handler - 1)[104] = fx_fire_damage_4d6;
    (fx_handler - 1)[105] = fx_level_damage_vs_type_8;
    (fx_handler - 1)[106] = fx_stunned;
    (fx_handler - 1)[107] = fx_berserk_creature;
    (fx_handler - 1)[108] = fx_fire_damage_2d6;
    (fx_handler - 1)[109] = fx_none;
    (fx_handler - 1)[110] = fx_restore_cure_allowance;
    (fx_handler - 1)[111] = fx_fear_remove;
    (fx_handler - 1)[112] = fx_adjacent_double_zap;
    (fx_handler - 1)[113] = fx_giant_str;
    (fx_handler - 1)[114] = fx_paralyze_gaze;
    (fx_handler - 1)[115] = fx_vorpal;
    (fx_handler - 1)[116] = fx_vary_primary_attack_damage;
    (fx_handler - 1)[117] = eye_rays;
    (fx_handler - 1)[118] = fx_armor_class_minus_four;
    (fx_handler - 1)[119] = fx_druid_fire;
    (fx_handler - 1)[120] = fx_reduce_damage_and_attack;
    (fx_handler - 1)[121] = fx_fire_damage_2d6;
    (fx_handler - 1)[122] = fx_iron_skin;
    (fx_handler - 1)[123] = fx_mind_blank;
    (fx_handler - 1)[125] = fx_need_plus_2;
    (fx_handler - 1)[126] = fx_save_bonus_mask_16;
    (fx_handler - 1)[127] = fx_item;
    fx_install_2();
}

/* 2b3a:549e */
void far fx_install_2(void)
{
    (fx_handler - 1)[129] = fx_entangle_attack;
    (fx_handler - 1)[130] = fx_entangled;
    (fx_handler - 1)[131] = fx_fixed_dice_attack;
    (fx_handler - 1)[132] = fx_paralyze_unconditional;
    (fx_handler - 1)[133] = fx_damage_adjacent_characters;
    (fx_handler - 1)[134] = fx_double_damage_mask_2;
    (fx_handler - 1)[135] = fx_heal_from_mask_1_damage;
    (fx_handler - 1)[136] = fx_switch_combat_side;
    (fx_handler - 1)[137] = fx_adjacent_save_aura;
    (fx_handler - 1)[138] = fx_confusion_immune;
    (fx_handler - 1)[139] = fx_poison_attack_unconditional;
    (fx_handler - 1)[140] = fx_noop_slot_140;
    (fx_handler - 1)[141] = fx_charm_sleep_immune;
    (fx_handler - 1)[142] = fx_hold_immune;
    (fx_handler - 1)[143] = fx_poison_hold_immune;
    (fx_handler - 1)[145] = fx_damage_vs_immunity4;
    (fx_handler - 1)[146] = fx_none;
    (fx_handler - 1)[147] = fx_disruption_attack;
    (fx_handler - 1)[148] = fx_attack_penalty_two;
    (fx_handler - 1)[149] = fx_cap_nonranged_weapon_damage;
    (fx_handler - 1)[150] = fx_none;
    (fx_handler - 1)[151] = fx_breath_bolt;
    (fx_handler - 1)[152] = fx_go_mad;
    (fx_handler - 1)[153] = fx_disarm;
    (fx_handler - 1)[154] = fx_queue_death_explosion;
    (fx_handler - 1)[155] = fx_paralyze_unconditional;
    (fx_handler - 1)[156] = fx_acid_pools;
    (fx_handler - 1)[158] = fx_cancel_hold;
    (fx_handler - 1)[159] = fx_none;
    (fx_handler - 1)[160] = fx_new_form;
    (fx_handler - 1)[161] = fx_immolate;
    (fx_handler - 1)[162] = fx_attack_and_armor_plus_two;
    (fx_handler - 1)[163] = fx_install_death_effects;
    (fx_handler - 1)[164] = fx_none;
    (fx_handler - 1)[165] = fx_cancel_effect_and_damage;
    (fx_handler - 1)[166] = fx_explode;
    (fx_handler - 1)[167] = fx_zap;
    (fx_handler - 1)[168] = fx_add_damage_dice_count;
    (fx_handler - 1)[169] = fx_halve_magic_weapon_damage;
    (fx_handler - 1)[170] = fx_disease_touch;
    (fx_handler - 1)[171] = fx_fear_aura;
    (fx_handler - 1)[172] = fx_rotting;
    (fx_handler - 1)[173] = fx_heal_eight_on_mask_4;
    (fx_handler - 1)[174] = fx_save_against_mask_1_or_2;
    (fx_handler - 1)[175] = fx_halve_damage;
    (fx_handler - 1)[176] = fx_engulf;
    (fx_handler - 1)[177] = fx_suffocate;
    (fx_handler - 1)[178] = fx_movement_stop;
    (fx_handler - 1)[179] = fx_attack_linked_character;
    (fx_handler - 1)[181] = fx_regenerate_with_enemies;
    (fx_handler - 1)[182] = fx_burst;
    (fx_handler - 1)[183] = fx_none;
    (fx_handler - 1)[184] = fx_disease_target_and_die;
    (fx_handler - 1)[185] = fx_die_on_disease_removal;
    (fx_handler - 1)[186] = fx_armor_acid;
    (fx_handler - 1)[187] = fx_disarm_hit;
    (fx_handler - 1)[188] = fx_queue_ice_explosion;
    (fx_handler - 1)[189] = fx_queue_meteor_explosion;
    (fx_handler - 1)[190] = fx_none;
    (fx_handler - 1)[191] = fx_none;
    (fx_handler - 1)[192] = fx_increase_mask_2_damage;
    (fx_handler - 1)[193] = fx_need_plus_any;
    (fx_handler - 1)[194] = fx_none;
    (fx_handler - 1)[203] = fx_gorgon_breath;
    (fx_handler - 1)[196] = fx_green_fire;
    (fx_handler - 1)[197] = fx_cancel_adjacent_plain_weapon;
    (fx_handler - 1)[198] = fx_disintegrate;
    (fx_handler - 1)[199] = fx_paralyze_after_target_save;
    (fx_handler - 1)[200] = fx_hp_damage_vs_immunity80;
    (fx_handler - 1)[201] = fx_green_fire_touch;
    (fx_handler - 1)[195] = fx_grant_gorgon_breath;
    (fx_handler - 1)[205] = fx_resist_damage_and_save_bonus;
    (fx_handler - 1)[206] = fx_apply_venom_to_target;
    (fx_handler - 1)[207] = fx_venom_attack_penalty;
    (fx_handler - 1)[208] = fx_constrict;
    (fx_handler - 1)[209] = fx_blinding_ray;
    (fx_handler - 1)[210] = fx_none;
    (fx_handler - 1)[211] = fx_poison_bite;
    (fx_handler - 1)[212] = fx_anemone_acid;
    (fx_handler - 1)[213] = fx_none;
    (fx_handler - 1)[214] = fx_attack_vs_immunity80;
    (fx_handler - 1)[215] = fx_none;    /* v1.0: no fx_rot_touch */
    (fx_handler - 1)[216] = fx_none;
    (fx_handler - 1)[217] = fx_none;
    (fx_handler - 1)[218] = fx_none;
    (fx_handler - 1)[219] = fx_none;
    (fx_handler - 1)[220] = fx_none;
    (fx_handler - 1)[221] = fx_none;
    (fx_handler - 1)[222] = fx_none;
    (fx_handler - 1)[223] = fx_none;
    (fx_handler - 1)[224] = fx_none;
    (fx_handler - 1)[225] = fx_none;
    (fx_handler - 1)[226] = fx_none;
    (fx_handler - 1)[227] = fx_none;
    (fx_handler - 1)[228] = fx_none;
    (fx_handler - 1)[229] = fx_none;
    (fx_handler - 1)[91] = fx_lightning_breath;
    (fx_handler - 1)[84] = fx_breath_weapon;
    (fx_handler - 1)[83] = fx_breath_weapon;
    (fx_handler - 1)[85] = fx_breath_weapon;
    (fx_handler - 1)[90] = fx_poison_steam_breath;
    (fx_handler - 1)[96] = fx_breath_weapon;
    (fx_handler - 1)[180] = fx_breath_weapon;
    (fx_handler - 1)[202] = fx_breath_weapon;
    (fx_handler - 1)[204] = fx_poison_steam_breath;
    (fx_handler - 1)[124] = fx_spell_gaze;
}

/* 2b3a:58e5 */
void far fx_dispatch(unsigned char id, struct Character far *character, struct Effect far *effect, char removed)
{
    if (item_handler_active)
        (*(fx_handler - 1)[0x7f])(character, effect, removed);      /* fx_item */
    else
        (*(fx_handler - 1)[id])(character, effect, removed);
}
