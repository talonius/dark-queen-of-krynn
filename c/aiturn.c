/* DQK.EXE overlay 02 module 28d0: a computer-run combatant's turn.
 * Reading the keys the player may press meanwhile, choosing a temper,
 * checking morale, casting a spell, readying the best weapon, moving
 * toward the target direction by direction, and closing to attack.
 *
 * EXE 0x02de10..0x02f9af, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 28d0, the first module of RTLink section 2 (payload
 * 0x2de10, link segment 28d0).  Its final retf is byte 0xe of paragraph
 * 2a89, where approach.c begins at byte 0xf.  Built with Turbo C 2.01,
 * tcc -ml -O -Z -d.  Functions are defined in shipped order; bake consumes
 * them in that order.  Section 1's 28d0 frame holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.
 *
 * ai_turn is reached through the vector stub 1d99:0ff9; every other
 * routine is static, and those called ahead of their definition are
 * prototyped.
 *
 * _DATA is dir_tries and seven literals, DGROUP 5db4..5e55.  _BSS is
 * ab8c..acab: the spell list spell_pick builds with its cursor, the state
 * the move and weapon choices keep between calls, and junk, an out-value
 * find_effect fills that nothing reads.
 *
 * near_ids and heroes are indexed from 1; the shipped code adds the
 * address of element 0 (bce7, b2f8).
 */

#include "dqk.h"

static char try_turn_undead(struct Character far *character);
static char ai_no_action(struct Character far *character);
static char try_spell(struct Character far *character);
static char ai_attack(struct Character far *character);
static char turn_end(struct Character far *character);
static char turn_keys(struct Character far *character);
static char morale_check(struct Character far *character);
static void arm_best(struct Character far *character, struct Character far *target);
static unsigned char spell_pick(struct Character far *character, long far *unused_effect_address);

static unsigned char dir_tries[7][6] = { /* by temper: turns from the heading */
    { 0, 8, 7, 6, 1, 2 },
    { 0, 8, 7, 6, 1, 2 },
    { 0, 8, 1, 2, 7, 6 },
    { 0, 7, 1, 8, 6, 2 },
    { 0, 1, 7, 8, 2, 6 },
    { 0, 8, 7, 6, 5, 4 },
    { 0, 8, 1, 2, 3, 4 }
};

static int at;
static unsigned char spl;
static long junk;
static unsigned sorted[0x89];
static unsigned char old_dir;
static unsigned char retries;
static unsigned char standstill;
static char foe_halves_a;
static unsigned char need_bonus;
static char foe_resists_b;
static unsigned char missile_needs;

void far ai_turn(struct Character far *character)                             /* 28d0:0000 */
{
    unsigned char stance;
    char turn_finished;
    long unused_effect_address;

    if (character->combat->target == 0)
        combat_target_pick(character, 0xff, 1, 0, 0);
    if (find_effect(character, 0xd5, &unused_effect_address)) {
        combat_actions_end(character);
        return;
    }
    turn_finished = turn_keys(character);
    box_draw();
    combat_text_clear();
    if (!character->enabled)
        turn_finished = combat_actions_end(character);
    stance = character->combat->ai_stance;
    if (stance == 0 || stance > 4 || fx_roll_dice(1, 4) == 1) {
        stance = fx_roll_dice(1, 8);
        if (stance == 8)
            stance = fx_roll_dice(1, 2) + 4;
        else
            stance = fx_roll_dice(1, 4);
    }
    character->combat->ai_stance = stance;
    if (!turn_finished)
        turn_finished = morale_check(character);
    if (character->combat->fleeing && !character->combat->panicked)
        hero_text(character, "flees in panic", 10, 1);
    if (turn_finished)
        return;
    if (ai_no_action(character)) {
        turn_finished = combat_actions_end(character);
    } else if (character->combat->spell_cast > 0) {
        spell_cast(character->combat->spell_cast, 1, 1, &turn_finished);
        turn_finished = combat_actions_end(character);
    } else if (try_turn_undead(character)) {
        turn_done = combat_actions_end(character);
    } else if (try_spell(character)) {
        turn_finished = 1;
    } else {
        arm_best(character, character->combat->target);
        turn_finished = turn_keys(character);
        while (!turn_finished) {
            if (combat_target_pick(character, 0xff, 1, 0, 0) && character->combat->initiative > 0 && character->enabled)
                turn_finished = ai_attack(character);
            else
                turn_finished = turn_end(character);
        }
    }
}

static char try_turn_undead(struct Character far *character)                          /* 28d0:0268 */
{
    char turned_undead;
    struct Character far *undead_target;
    int undead_level_limit;

    turned_undead = 0;
    if (!character->combat->fleeing) {
        undead_level_limit = 0xd;
        if (character->combat->turn_attempts < ai_level
            && (class_level(character, 0) > 0 || class_level(character, 3) > 2)
            && turn_target_find(character, &undead_target, &undead_level_limit)) {
            turned_undead = 1;
            turn_undead(character);
        }
    }
    return turned_undead;
}

static char ai_no_action(struct Character far *character)                          /* 28d0:02f5 */
{
    char action_available;

    action_available = 0;
    return action_available;
}

static char try_spell(struct Character far *character)                        /* 28d0:0306 */
{
    char cast_started;
    long unused_effect_address;
    char cast_accepted;
    unsigned char spell;
    unsigned char casting_threshold;

    cast_started = 0;
    if (!character->combat->fleeing && !game_state[STATE_MAGIC_DISABLED] && character->combat->can_cast
        && (character->npc >= 0x7f || magic_on)) {
        cast_accepted = 0;
        spell = spell_pick(character, &unused_effect_address);
        casting_threshold = class_level(character, 2) + class_level(character, 3) + class_level(character, 4)
              + class_level(character, 1);
        casting_threshold = casting_threshold * 4;
        if (spell && fx_roll_dice(1, 100) > casting_threshold
            && fx_roll_dice(1, 5) <= (spell_info - 1)[spell].ai_priority)
            combat_cast_begin(spell, 1, &cast_accepted);
        cast_started = cast_accepted;
    }
    return cast_started;
}

static char step_ok(struct Character far *character, unsigned char direction_attempt, unsigned char heading,
                    char far *off_map)                           /* 28d0:0438 */
{
    char step_allowed;
    long unused_effect_address;
    unsigned char direction_offset;
    unsigned char candidate_direction;
    char occupant_slot;
    unsigned char terrain;
    unsigned char movement_cost;
    char can_step;
    unsigned char hazard_type;
    char special_terrain;

    *off_map = 0;
    can_step = 0;
    direction_offset = dir_tries[character->combat->ai_stance][direction_attempt];
    candidate_direction = (heading + direction_offset) % 8;
    combat_direction_probe(character, candidate_direction, &occupant_slot, &terrain, &hazard_type, &special_terrain);
    if (terrain == 0)
        *off_map = 1;
    else if ((tile_info - 1)[terrain].cost == 0xff) {
        step_allowed = 0;
        return step_allowed;
    } else {
        if (pool_lowbit(candidate_direction))
            movement_cost = (tile_info - 1)[terrain].cost * 3;
        else
            movement_cost = (tile_info - 1)[terrain].cost * 2;
        if (!occupant_slot && character->combat->movement > movement_cost) {
            switch (hazard_type) {
            case 0x1d:
                if (!find_effect(character, 0x1e, &unused_effect_address) && !find_effect(character, 0xa0, &unused_effect_address)
                    && !find_effect(character, 0x8f, &unused_effect_address) && !find_effect(character, 0x3f, &unused_effect_address)
                    && !character->combat->panicked /* v1.0: no effect-0x39 check here */
                    && !(character->immunity & 0x18))
                    movement_cost = character->combat->movement + 1;
                break;
            case 0x1c:
                if (find_effect(character, 0x39, &unused_effect_address) || find_effect(character, 0x8f, &unused_effect_address)
                    || (character->immunity & 0x10))
                    break;
            case 0x1b:
                if (!character->combat->panicked)
                    movement_cost = character->combat->movement + 1;
                break;
            }
            if (special_terrain && character->level_high < 7 && !character->combat->panicked)
                movement_cost = character->combat->movement + 1;
            if (character->combat->movement >= movement_cost)
                can_step = 1;
        }
    }
    step_allowed = can_step;
    return step_allowed;
}

static void ai_move(struct Character far *character)                          /* 28d0:06b0 */
{
    char off_map;
    char movement_finished;
    unsigned char heading;
    unsigned char step_direction;
    unsigned char direction_attempt;

    movement_finished = 0;
    if (character->combat->movement / 2 < 10)
        status_printf(0, 0x18, 7, 0, msg_format("Move/Attack, Move Left = %s ",
                                                byte_str(character->combat->movement / 2)));
    else
        status_printf(0, 0x18, 7, 0, msg_format("Move/Attack, Move Left = %s",
                                                byte_str(character->combat->movement / 2)));
    if (turn_keys(character))
        return;
    if (character->combat->movement / 2 > 0 && character->combat->initiative > 0
        && (!(character->npc & 0x80)
            || (character->npc & 0x80) && fx_roll_dice(1, 100) + morale >= morale_base
            || character->side == 1)
        && (character->combat->fleeing || character->cls != 5)) {
        if (!character->combat->fleeing)
            heading = combatant_direction(character, character->combat->target);
        else {
            character->combat->ai_stance = fx_roll_dice(1, 2);
            heading = geo.facing - (geo.facing + 2) % 4 / 2 + 1;
            if (character->side == 0)
                heading = heading + 4;
            heading = heading % 8;
        }
        off_map = 0;
        movement_finished = 0;
        direction_attempt = 1;
        while (direction_attempt < 6 && !movement_finished && !step_ok(character, direction_attempt, heading, &off_map)) {
            if (character->combat->fleeing && off_map)
                movement_finished = combat_flee(character);
            else
                direction_attempt++;
        }
        if (movement_finished) {
            character->combat->movement = 0;
            character->combat->fleeing = 0;
            movement_finished = combat_actions_end(character);
        } else {
            if (direction_attempt < 6 && character->combat->ai_stance < 7)
                step_direction = (dir_tries[character->combat->ai_stance][direction_attempt] + heading) % 8;
            if (direction_attempt == 6 || (step_direction + 4) % 8 == old_dir) {
                retries++;
                character->combat->ai_stance = character->combat->ai_stance % 6 + 1;
                if (retries > 1) {
                    character->combat->target = 0;
                    if (retries > 2) {
                        character->combat->movement = 0;
                        movement_finished = 1;
                    } else if (!combat_target_pick(character, 0xff, 1, 0, 0))
                        movement_finished = turn_end(character);
                }
            }
            if (direction_attempt < 6)
                old_dir = step_direction;
            else
                movement_finished = 1;
        }
        if (movement_finished)
            return;
        move_flag = can_reach || combatant_visible(character, 0) || character->side == 0;
        combatant_facing_set(character, step_direction, 0, 0);
        opportunity_attacks(character, character->combat->direction);
        if (!character->enabled) {
            movement_finished = combat_actions_end(character);
            return;
        }
        if (character->combat->movement > 0)
            combat_move_step(character, character->combat->direction);
        if (!character->enabled || is_helpless(character))
            movement_finished = combat_actions_end(character);
        fx_terrain_apply(character, 0, 0);
    } else {
        if (character->combat->movement / 2 == 0 && !character->combat->fleeing)
            movement_finished = try_spell(character);
        if (!movement_finished)
            movement_finished = turn_end(character);
    }
}

static char ai_attack(struct Character far *character)                        /* 28d0:0b3d */
{
    char turn_finished;
    char action_finished;
    char continue_turn;
    unsigned char weapon_range;
    unsigned short line_cost;
    unsigned char nearby_count;
    unsigned char target_index;
    struct Item far *ammunition;
    struct Character far *target;
    char target_x;
    char target_y;
    unsigned char action_attempts;

    old_dir = 8;
    retries = 0;
    standstill = 0;
    action_attempts = 0;
    continue_turn = 1;
    action_finished = 0;
    fx_trigger_group(0xe, &character);
    if (character->side == 0 && bandage_dying(1))
        character->combat->initiative = 0;
    if (character->combat->initiative == 0)
        continue_turn = 0;
    while (!action_finished && continue_turn) {
        if (character->combat->fleeing) {
            while (character->combat->movement > 0 && character->combat->initiative > 0
                   && character->combat->initiative < 20)
                ai_move(character);
        }
        if (character->combat->initiative == 0 || character->combat->initiative == 20)
            continue_turn = 0;
        else
            action_finished = 0;
        if (!action_finished && continue_turn) {
            action_attempts++;
            if (action_attempts > 20) {
                action_finished = 1;
                continue_turn = !turn_end(character);
            }
            can_reach = 0;
            weapon_range = 1;
            if (character->equipped[0] != 0)
                weapon_range = item_table[character->equipped[0]->rec.type].range;
            if (weapon_range == 0 || weapon_range == 0xff)
                weapon_range = 1;
            target = character->combat->target;
            if (!target->enabled || target->side == 0)
                target = 0;
            if (target != 0 && combat_target_allowed(character, target, 0)) {
                target_x = combatant_x(target);
                target_y = combatant_y(target);
                line_cost = weapon_range;
                cur_map->dist_mode = 0;
                if (line_clear(combatant_x(character), combatant_y(character), &target_x, &target_y, &line_cost)
                    && line_cost / 2 <= weapon_range)
                    can_reach = 1;
            }
            if (!can_reach) {
                nearby_count = combat_targets_near(character, weapon_range, 0);
                if (nearby_count == 0) {
                    if (combat_target_pick(character, 0xff, 0, 0, 0)) {
                        ai_move(character);
                        if (character->combat->spell_cast > 0)
                            continue_turn = 0;
                    } else
                        action_finished = turn_end(character);
                } else {
                    target_index = fx_roll_dice(1, nearby_count);
                    target = (heroes - 1)[(near_ids - 1)[target_index]];
                    if (has_ranged_weapon(character) && !has_throwable_weapon(character)
                        && (combat_targets_near(character, 1, 0) > 0 || game_state[STATE_UNDERWATER])) {
                        arm_best(character, character->combat->target);
                        action_finished = 1;
                    } else if (get_distance(character, target) == 1
                               || combat_target_allowed(character, target, 0))
                        can_reach = 1;
                }
            }
            if (can_reach)
                combat_view_scroll(combatant_x(character), combatant_y(character), 2,
                                combatant_direction(character, target));
            if (can_reach) {
                if (combat_sweep_attack(character, target))
                    action_finished = combat_actions_end(character);
                else {
                    attack_register(character, target);
                    ammunition = 0;
                    if (has_ranged_weapon(character)) {
                        can_reach = ranged_ammo_find(character, &ammunition);
                        if (has_throwable_weapon(character) && get_distance(character, target) == 1)
                            ammunition = 0;
                    }
                    combat_attack(character, target, 0, ammunition, &action_finished);
                    if (action_finished)
                        continue_turn = 0;
                    else if (!target->enabled)
                        action_finished = 1;
                }
            }
        }
    }
    turn_finished = !continue_turn;
    return turn_finished;
}

static char turn_end(struct Character far *character)                         /* 28d0:0fec */
{
    char turn_finished;

    combat_text_clear();
    if (is_helpless(character) || has_ranged_weapon(character) || character->combat->initiative == 0)
        turn_finished = combat_actions_end(character);
    else
        turn_finished = combat_guard(character);
    return turn_finished;
}

static char turn_keys(struct Character far *character)                        /* 28d0:1050 */
{
    char turn_interrupted;
    struct Character far *party_member;
    unsigned char key;

    turn_interrupted = 0;
    if (screen_sync()) {
        key = game_key_get();
        switch (key) {
        case 0xcc:
            magic_on = !magic_on;
            if (magic_on)
                bottom_msg("Magic On");
            else
                bottom_msg("Magic Off");
            break;
        case 0x20:
            for (party_member = node_list; party_member != 0; party_member = party_member->next)
                if (party_member->npc < 0x80 && party_member->status != 1)
                    party_member->quick_fight = 0;
            if (!character->quick_fight) {
                character->combat->initiative = 20;
                turn_interrupted = 1;
            }
            break;
        case 0xd7:
            gods_intervene();
            break;
        }
    }
    return turn_interrupted;
}

static char morale_check(struct Character far *character)                     /* 28d0:1117 */
{
    char turn_finished;
    unsigned char enemy_speed;

    turn_finished = 0;
    character->combat->fleeing = 0;
    if (character->combat->panicked) {
        character->combat->fleeing = 1;
        hero_text(character, "is forced to flee", 10, 1);
    } else if (character->npc & 0x80) {
        morale = (character->npc & 0x7f) * 2;
        fx_trigger_group(0x11, &character);
        if (100 - character->hp * 100 / character->hp_max > morale || !morale) {
            morale = morale_base;
            fx_trigger_group(0x11, &character);
            if (100 - game_state[STATE_MORALE_CHECK_PERCENT] > morale || !morale || character->side == 0) {
                enemy_speed = fastest_enemy_speed(character);
                if (calc_move(character) / 2 >= enemy_speed)
                    character->combat->fleeing = 1;
                else if (character->stats[1].cur > 5) {
                    fx_out_of_play(character, 4, "Surrenders");
                    turn_finished = combat_actions_end(character);
                }
            }
        }
    }
    return turn_finished;
}

static unsigned char item_score(struct Character far *character, struct Item far *item)
{                                                                /* 28d0:1271 */
    unsigned char weapon_score;
    unsigned char score;
    struct ItemInfo item_info;

    item_info = item_table[item->rec.type];
    score = item_info.dice * item_info.rolls;
    if (item->rec.plus > 0)
        score = score + item->rec.plus * 8;
    if (item_info.mod > 0)
        score = score + item_info.mod * 2;
    if ((item_info.flags & 8) > 0)
        score = score + (item_info.rate_of_fire - 1) * 2;
    if (item_info.hands <= 1)
        score = score + 3;
    if (character->hands + item_info.hands >= 3)
        score = 0;
    if (item->rec.cursed)
        score = 0;
    if (item_info.wpn_class <= 1 && foe_halves_a || (item_info.wpn_class & 0x80) && foe_resists_b)
        score = score / 2;
    if ((item_info.flags & 8) || (item_info.flags & 0x10)) {
        if (item->rec.plus < missile_needs)
            score = 0;
    } else if (item->rec.plus < need_bonus)
        score = 0;
    if (item_info.range > 1 && game_state[STATE_UNDERWATER])
        score = 0;
    weapon_score = score;
    return weapon_score;
}

static void arm_best(struct Character far *character, struct Character far *target) /* 28d0:1396 */
{
    struct Item far *missile;
    struct Item far *melee;
    struct Item far *ammo;
    struct Item far *candidate_item;
    struct Item far *missile_support_item;
    unsigned char best_missile;
    unsigned char best_melee;
    unsigned char best_ammo;
    unsigned char score;
    unsigned char type;
    unsigned char flags;
    char can_change_equipment;
    char changed;
    char close_range_missile;
    char missile_supported;

    need_bonus = 0;
    if (target != 0) {
        if (find_effect(target, 0x7d, &junk))
            need_bonus = 2;
        if (find_effect(target, 0x3c, &junk))
            need_bonus = 3;
        if (find_effect(target, 0x50, &junk) || find_effect(target, 0x51, &junk)
            || find_effect(target, 0x67, &junk))
            need_bonus = 1;
        missile_needs = need_bonus;
        if (!missile_needs && find_effect(target, 0x29, &junk))
            missile_needs = 1;
        foe_halves_a = find_effect(target, 0x42, &junk);
        foe_resists_b = find_effect(target, 0x43, &junk);
    } else {
        missile_needs = 0;
        foe_resists_b = 0;
        foe_halves_a = 0;
    }
    if (character->equipped[0] != 0)
        character->hands = character->hands - item_table[character->equipped[0]->rec.type].hands;
    if (character->equipped[1] != 0)
        character->hands = character->hands - item_table[character->equipped[1]->rec.type].hands;
    missile = 0;
    melee = 0;
    ammo = 0;
    best_missile = 1;
    best_melee = character->unarmed_dice[0] * character->unarmed_rolls[0];
    if (character->unarmed_mods[0] > 0)
        best_melee = best_melee + character->unarmed_mods[0] * 2;
    best_ammo = 0;
    for (candidate_item = character->items; candidate_item != 0; candidate_item = candidate_item->next) {
        type = candidate_item->rec.type;
        switch (item_table[type].slot) {
        case 0:
            if (item_table[type].class_mask & character->class_mask) {
                score = item_score(character, candidate_item);
                if (((item_table[type].flags & 8) || (item_table[type].flags & 0x10))
                    && score > best_missile) {
                    missile = candidate_item;
                    best_missile = score;
                }
                if (!(item_table[type].flags & 8) && score > best_melee) {
                    melee = candidate_item;
                    best_melee = score;
                }
            }
            break;
        }
        if ((int)item_table[type].slot == 1 && (item_table[type].class_mask & character->class_mask) > 0) {
            if (candidate_item->rec.plus >= 0)
                score = candidate_item->rec.plus + 1;
            else
                score = 0;
            if (score > best_ammo) {
                ammo = candidate_item;
                best_ammo = score;
            }
        }
    }
    if (missile != 0 && item_table[missile->rec.type].range > 1
        && (item_table[missile->rec.type].flags & 0x14) == 0x14)
        close_range_missile = 1;
    else
        close_range_missile = 0;
    missile_supported = 0;
    missile_support_item = 0;
    flags = 0;
    if (missile != 0) {
        flags = item_table[missile->rec.type].flags;
        if (flags & 0x10)
            missile_support_item = missile;
        if (flags & 8) {
            if (flags & 1)
                missile_support_item = character->equipped[11];
            if (flags & 0x80)
                missile_support_item = character->equipped[12];
        }
    }
    if (missile_support_item != 0 || flags == 0xa && !game_state[STATE_UNDERWATER])
        missile_supported = 1;
    if (missile != 0 && best_melee / 2 < best_missile && missile_supported
        && (close_range_missile || !combat_targets_near(character, 1, 0)))
        candidate_item = missile;
    else
        candidate_item = melee;
    changed = 0;
    can_change_equipment = 1;
    if (character->equipped[0] != 0 && (character->equipped[0] == candidate_item || character->equipped[0]->rec.cursed))
        can_change_equipment = 0;
    if (can_change_equipment) {
        if (character->equipped[0] != 0)
            item_equip_toggle(character->equipped[0]);
        recalc_stats(character);
        if (character->equipped[1] != 0 && !character->equipped[1]->rec.cursed)
            character->hands = character->hands - item_table[character->equipped[1]->rec.type].hands;
        if (candidate_item != 0)
            item_equip_toggle(candidate_item);
        changed = 1;
    }
    recalc_stats(character);
    attacks_left_recalc(character);
    can_change_equipment = 1;
    if (character->equipped[1] != 0 && (character->equipped[1] == ammo || character->equipped[1]->rec.cursed))
        can_change_equipment = 0;
    if (character->hands > 2) {
        if (character->equipped[1] == 0 || character->equipped[1]->rec.cursed)
            item_equip_toggle(candidate_item);
        else
            item_equip_toggle(character->equipped[1]);
        changed = 1;
    } else if (character->hands < 2 && can_change_equipment) {
        if (character->equipped[1] != 0)
            item_equip_toggle(character->equipped[1]);
        recalc_stats(character);
        if (ammo != 0)
            item_equip_toggle(ammo);
        changed = 1;
    }
    recalc_stats(character);
    if (changed)
        hero_panel(character);
}

static void spell_next(void)                                     /* 28d0:19b0 */
{
    char next_spell_found;

    next_spell_found = 0;
    do {
        if (sorted[at] == 0) {
            next_spell_found = 1;
            spl = 0;
        } else if (sorted[at] != spl) {
            spl = sorted[at];
            next_spell_found = 1;
        } else
            at++;
    } while (!next_spell_found);
}

static void spells_sort(unsigned far *sorted_spells, struct Character far *character)  /* 28d0:1a07 */
{
    unsigned char spell;
    unsigned char displaced_spell;
    unsigned char spell_slot;
    unsigned char insertion_slot;
    char placed;

    for (spell_slot = 0; spell_slot <= 0x88; spell_slot++) {
        spell = character->spells[spell_slot];
        if (spell > 0 && spell < 0x80 && (spell_info - 1)[spell].combat_ok != 0) {
            placed = 0;
            insertion_slot = 0;
            do {
                if (sorted_spells[insertion_slot] == 0) {
                    sorted_spells[insertion_slot] = spell;
                    placed = 1;
                } else if ((spell_info - 1)[sorted_spells[insertion_slot]].ai_priority > (spell_info - 1)[spell].ai_priority)
                    insertion_slot++;
                else {
                    do {
                        displaced_spell = sorted_spells[insertion_slot];
                        sorted_spells[insertion_slot] = spell;
                        spell = displaced_spell;
                        insertion_slot++;
                    } while (spell != 0 && insertion_slot <= 0x88);
                    placed = 1;
                }
            } while (!placed);
        }
    }
}

static unsigned char spell_pick(struct Character far *character, long far *unused_effect_address) /* 28d0:1b1e */
{
    unsigned char selected_spell;
    char target_found;

    selected_spell = 0;
    mem_zero(sorted, sizeof sorted);
    spells_sort(sorted, character);
    target_found = 0;
    spl = 0;
    at = 0;
    do {
        spell_next();
        if (spl)
            target_found = spell_target_rating(character, spl) > 0;
    } while (!target_found && spl);
    selected_spell = spl;
    return selected_spell;
}
