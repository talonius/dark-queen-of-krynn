/* DQK.EXE overlay 00 module 1f4e: the combat round loop.
 * combat_main sets a fight up, runs rounds until a side's count reaches
 * zero, the round counter reaches its limit or Esc ends a demo, and tears
 * it down again.
 * Each round orders the combatants by initiative and gives each a turn:
 * the computer runs its own through ai_turn, and a party member's turn is
 * the command menu -- moving and attacking by direction, casting, turning,
 * guarding, delaying, the game speed.  After the fight, party members past
 * their race's age limit may die of it.
 *
 * EXE 0x022400..0x023bd5, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 1f4e, the first module of RTLink section 0 (payload
 * 0x22400, link segment 1f4e).  Its final retf is byte 4 of paragraph 20cb,
 * whose module starts at 20cb:0005.  Built with Turbo C 2.01,
 * tcc -ml -O -Z -d.  Functions are defined in shipped order; bake consumes
 * them in that order.  Section 3's 1f4e frame holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.
 *
 * combat_main is reached through the vector stub 1d99:0fc7.  No call, vector
 * or relocation in sections 0 to 2 reaches combat_unit_init.  The routines after
 * combat_main are near-called ahead of their definitions, so static.
 *
 * _DATA is max_age and eleven stored literals, DGROUP 58d8..5975: -d stores
 * "%s%s" and "" inside "%s%s%s".  _BSS is empty.
 *
 * combat_order is indexed from 1 up to 0x48, and heroes and tile_info from 1;
 * the shipped code adds the address of element 0 (bd99, b2f8, 6197).
 */

#include "dqk.h"

static void combatant_turn(struct Character far *character);
static void command_loop(struct Character far *character);
static void command_menu(struct Character far *character, unsigned char far *key);
static void round_end(char far *done);
static void move_hero(struct Character far *character, unsigned char key, char far *done);
static void strike(struct Character far *character, struct Character far *character_cursor, char far *done);
static void combat_speed_menu(void);
static void clear_target(struct Character far *character);

static unsigned max_age[7] = { 1600, 1600, 325, 525, 525, 199, 120 }; /* by race */

/* The translated Pascal unit's empty initialization block (DKK combat.pas). */
void far combat_unit_init(void)                                   /* 1f4e:0000 */
{
}

void far combat_cleanup(void)                                    /* 1f4e:0001 */
{
    struct Cloud far *nxt;
    struct Character far *cur;
    long eff;
    unsigned char index;

    while (cloud_list != 0) {
        nxt = cloud_list->next;
        pool_free(cloud_list, 0x21, &cloud_pool);
        cloud_list = nxt;
    }
    cur = node_list;
    while (cur != 0) {
        fx_remove(cur, 0xb0, 0);
        fx_remove(cur, 0xd0, 0);
        if (find_effect(cur, 0xb, &eff))
            if (cur->status == 0)
                if (side_count[1] > 1)
                    cur->status = 3;
                else
                    cur->status = 0;
        fx_hp_effects_remove(cur);
        cur = cur->next;
    }
    pic_free();
    for (index = 0; index <= 0x16; index++)
        icon_clear(index + 0xf);
    for (index = 0x98; index < 0x9d; index++)
        icon_clear(index);
    res_free(&pic_res);
    hit_proc = camp_spell_targets;
}

void far combat_aging(void)                                      /* 1f4e:012f */
{
    struct Character far *cur;
    struct Character far *character_cursor;
    struct Character far *witness;
    char is_old;
    char is_warning;
    unsigned char row;
    unsigned char count;
    char pronoun[6];
    char message[42];

    cur = node_list;
    while (cur != 0) {
        if (cur->status < 6 && cur->combat->not_pc != 1) {
            is_old = cur->age > max_age[cur->race];
            if (!is_old)
                is_warning = cur->age > max_age[cur->race] - 10;
            if (is_warning)                 /* unset when is_old: 1f4e:01a2 */
                hero_text(cur, old_text, 10, 1);
            if (is_old) {
                row = 0xe;
                text_clear(0x17, row, 0x26, 0x15);
                text_wrap(0x17, row, 0x26, 0x15, 0xb, 0, 1,
                          msg_format("%s%s%s", strain_text, cur->name, ".\t"));
                continue_prompt(0, 0, continue_items);
                text_clear(0x17, row, 0x26, 0x15);
                if (cur->gender == 0)
                    str_copy(pronoun, he_text);
                else
                    str_copy(pronoun, she_text);
                text_wrap(0x17, row, 0x26, 0x15, 0xb, 0, 1,
                          msg_format("%s%s", pronoun, breath_text));
                continue_prompt(0, 0, continue_items);
                text_clear(0x17, row, 0x26, 0x15);
                fx_disable(cur, 8, age_death_text);
                character_cursor = node_list;
                count = 0;
                while (character_cursor != 0) {
                    if (character_cursor->enabled == 1 && character_cursor->status == 0 && character_cursor->side != 1) {
                        count++;
                        witness = character_cursor;
                    }
                    character_cursor = character_cursor->next;
                }
                switch (count) {
                case 0:
                    str_copy(message, no_witness_text);
                    break;
                case 1:
                    str_copy(message, msg_format("%s%s", witness->name, one_witness_text));
                    break;
                default:
                    str_copy(message, msg_format("%s%s%s", many_witness_text, witness->name,
                                                 search_body_text));
                }
                text_clear(0x17, row, 0x26, 0x15);
                text_wrap(0x17, row, 0x26, 0x15, 0xe, 0, 1, message); /* v1.0: yellow (0xe), not 0xb */
                continue_prompt(0, 0, continue_items);
            }
        }
        cur = cur->next;
    }
    year_rolled = 0;
}

void far order_combatants(void)                                  /* 1f4e:0447 */
{
    char more;
    struct Character far *cur;
    struct Character far *saved;
    struct Character far *nxt;
    char cur_init;
    char slot_init;
    unsigned char index;
    unsigned char inner_index;

    cur = node_list;
    mem_fill(combat_order, 0x124, 0);
    while (cur != 0) {
        index = 1;
        more = 1;
        cur_init = cur->combat->initiative;
        while (index < 0x41 && more) {
            if (combat_order[index] != 0)
                slot_init = combat_order[index]->combat->initiative;
            else
                slot_init = -1;
            if (cur_init > slot_init
                    || (cur_init == slot_init && fx_roll_dice(1, 2) == 1))
                more = 0;
            else
                index++;
        }
        saved = combat_order[index];
        combat_order[index] = cur;
        if (saved != 0) {
            inner_index = index;
            more = 1;
            while (inner_index < 0x48 && more) {
                inner_index++;
                nxt = combat_order[inner_index];
                combat_order[inner_index] = saved;
                saved = nxt;
                if (nxt == 0)
                    more = 0;
            }
        }
        cur = cur->next;
    }
}

void far combat_main(void)                                       /* 1f4e:059d */
{
    char finished;
    char done;
    struct Character far *cur;
    int key;

    finished = 0;
    view_mode = 5;
    hit_proc = combat_spell_targets;
    combat_leadership_apply();
    combat_setup();
    done = 0;
    if (side_count[0] == 0 || side_count[1] == 0)
        done = 1;
    cur = node_list;
    while (cur != 0) {
        fx_trigger_group(0x18, &cur);
        cur = cur->next;
    }
    while (!done) {
        combat_sides_count();
        cur = node_list;
        while (cur != 0) {
            combat_round_begin(cur);
            cur = cur->next;
        }
        game_state[STATE_INITIATIVE_SIDE_MASK] = 0;
        order_combatants();
        cur = combat_order[1];
        order_idx = 1;
        while (cur != 0 && !done && (!demo_active || !esc_get())) {
            combatant_turn(cur);
            fx_death_explosions();
            fx_ice_explosions();
            fx_meteor_explosions();
            order_idx++;
            if (order_idx > 0x48)
                cur = 0;
            else
                cur = combat_order[order_idx];
            combat_sides_count();
            if (side_count[0] == 0 || side_count[1] == 0 || combat_round >= combat_round_limit)
                if (!finished) {
                    done = 1;
                    finished = 1;
                }
            if (demo_active)
                if (key_poll()) {
                    key = key_get();
                    if (key == 0x152)
                        sound_toggle();
                    if (key == 0x153)
                        music_toggle();
                    if (key == 0x1b || key == 0x60)
                        esc_set(1);
                }
            if (is_xvga() || !pixel_shift())
                input_poll();
        }
        round_end(&done);
        if (demo_active && esc_get())
            done = 1;
    }
    if (year_rolled)
        combat_aging();
    combat_cleanup();
    text_slow = 1;
}

static void combatant_turn(struct Character far *character)                   /* 1f4e:078b */
{
    character->combat->attacks_taken = 0;
    character->combat->attack_angles = 0;
    character->combat->guarding = 0;
    fx_trigger_group(7, &character);
    if (character->combat->initiative > 0) {
        if (character->combat->initiative == 0x14)
            character->combat->initiative = 0x13;
        cur_node = character;
        move_flag = character->side == 0 || combatant_visible(character, 0);
        view_center(character, 2, 1);
        recalc_stats(character);
        panel_dirty = 1;
        hero_panel(character);
        fx_trigger_group(0xf, &character);
        if (character->combat->spell_cast == 0)
            fx_trigger_group(0x15, &character);
        if (character->combat->initiative > 0)
            if (character->quick_fight)
                ai_turn(character);
            else
                command_loop(character);
        cell_redraw(combatant_x(character), combatant_y(character));
    }
}

static void command_loop(struct Character far *character)                     /* 1f4e:08cf */
{
    unsigned char key;
    char done;
    struct Character far *character_cursor;
    struct Aim aim;
    unsigned char spell;

    if (character->enabled) {
        if (character->combat->spell_cast > 0) {
            spell = character->combat->spell_cast;
            character->combat->spell_cast = 0;
            spell_cast(spell, 0, 1, &done);
            done = combat_actions_end(character);
        } else {
            done = 0;
            while (!done) {
                ui_mode = 1;
                command_menu(character, &key);
                if (!menu_escaped) {
                    switch (key) {
                    case 0:
                        move_hero(character, key, &done);
                        break;
                    case 1:
                        combat_aim(character, 0xff, 1, 0, 1, &done, &aim);
                        break;
                    case 2:
                        ui_mode = 2;
                        items_menu(&done);
                        attacks_left_recalc(character);
                        if (!done)
                            combat_view_show();
                        break;
                    case 3:
                        combat_cast_begin(0, 0, &done);
                        break;
                    case 4:
                        if (class_level(character, 0) > 0 || class_level(character, 3) > 2)
                            if (character->combat->turn_attempts < ai_level) {
                                turn_undead(character);
                                done = combat_actions_end(character);
                            }
                        break;
                    case 5:
                        done = combat_guard(character);
                        break;
                    case 6:
                        clear_target(character);
                        box_draw();
                        input_pending_clear();
                        delay(200);
                        done = 1;
                        ai_turn(character);
                        break;
                    case 7:
                        character->combat->initiative = 1;
                        tick_slot = order_idx;
                        character_cursor = combat_order[tick_slot + 1];
                        while (character_cursor != 0 && tick_slot < 0x48) {
                            combat_order[tick_slot] = character_cursor;
                            tick_slot++;
                            character_cursor = combat_order[tick_slot + 1];
                        }
                        combat_order[tick_slot] = character;
                        order_idx--;
                        done = 1;
                        break;
                    case 8:
                        done = bandage_dying(1);
                        done = combat_actions_end(character);
                        break;
                    case 9:
                        fx_taunt(character);
                        done = combat_actions_end(character);
                        break;
                    case 10:
                        char_view(&done);
                        attacks_left_recalc(character);
                        if (!done)
                            combat_view_show();
                        break;
                    case 11:
                        combat_speed_menu();
                        break;
                    case 12:
                        done = combat_actions_end(character);
                        break;
                    }
                } else {
                    switch (key) {
                    case 0x81:
                    case 0x82:
                    case 0x83:
                    case 0x84:
                    case 0x85:
                    case 0x86:
                    case 0x87:
                    case 0x88:
                        move_hero(character, key, &done);
                        break;
                    case ' ':
                        character_cursor = node_list;
                        while (character_cursor != 0) {
                            if (character_cursor->npc < 0x80)
                                character_cursor->quick_fight = 0;
                            character_cursor = character_cursor->next;
                        }
                        break;
                    case 0xcc:
                        magic_on = !magic_on;
                        if (magic_on)
                            bottom_msg(magic_on_text);
                        else
                            bottom_msg(magic_off_text);
                        break;
                    case 0xc0:
                    case 0xd0:
                        order_idx--;
                        character->combat->initiative = 0x14;
                        character_cursor = node_list;
                        while (character_cursor != 0) {
                            clear_target(character_cursor);
                            character_cursor = character_cursor->next;
                        }
                        box_draw();
                        delay(200);
                        done = 1;
                        break;
                    case 0xd7:
                        if (gods_intervene()) {
                            view_center(character, 3, 0);
                            done = 1;
                        } else
                            bottom_msg("That doesn't work");
                        break;
                    }
                }
                if (!done) {
                    view_center(character, 2, 1);
                    panel_dirty = 1;
                    hero_panel(character);
                }
            }
        }
    } else
        done = combat_actions_end(character);
}

static void command_menu(struct Character far *character, unsigned char far *key) /* 1f4e:0cd7 */
{
    unsigned char count;
    unsigned char spell_slot;
    char flag;
    char cy;
    char cx;

    count = 0;
    hotkey_add(1, &count);
    if (character->item_count > 0)
        hotkey_add(2, &count);
    flag = 0;
    for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
        if ((character->spells[spell_slot] & 0x7f) > 0)
            flag = 1;
    if (flag && character->combat->can_cast && !game_state[STATE_MAGIC_DISABLED])
        hotkey_add(3, &count);
    if ((class_level(character, 0) > 0 || class_level(character, 3) > 2) && character->combat->turn_attempts < ai_level)
        hotkey_add(4, &count);
    if (!has_ranged_weapon(character) || has_throwable_weapon(character))
        hotkey_add(5, &count);
    hotkey_add(6, &count);
    hotkey_add(7, &count);
    if (bandage_dying(0))
        hotkey_add(8, &count);
    if (character->race == 5 && taunting_character == 0)
        hotkey_add(9, &count);
    for (spell_slot = 10; spell_slot <= 0xc; spell_slot++)
        hotkey_add(spell_slot, &count);
    turn_done = 0;
    do {
        cy = combatant_y(character) - cur_map->oy;
        cx = combatant_x(character) - cur_map->ox;
        if (character->combat->movement > 0)
            set_menu_kind(4);
        else
            set_menu_kind(0xb);
        *key = grid_choose("", command_items, 1, 0, cy, cx);
        if (menu_escaped) {
            if (*key == 0xc0 || *key == 0xd0 || *key == 0xcc || *key == 0xd7
                    || *key == 0x12 || *key == 0x88 || *key == 0x84 || *key == 0x86
                    || *key == 0x82 || *key == 0x83 || *key == 0x81 || *key == 0x87
                    || *key == 0x85)
                turn_done = 1;
            else
                *key = 0xff;
        }
        if (*key <= 0xc)
            turn_done = 1;
    } while (!turn_done);
    box_draw_both();
}

static void round_end(char far *done)                            /* 1f4e:0f6c */
{
    struct Character far *character_cursor;
    unsigned char combatant_count;

    clock_advance(1, 1);
    fx_clouds_tick(0);
    combat_round++;
    hostile_hp_percent_update();
    character_cursor = node_list;
    combatant_count = 1;
    while (character_cursor != 0) {
        fx_trigger_group(0x13, &character_cursor);
        fx_terrain_apply(character_cursor, 0, 1);
        if (character_cursor->status == 5) {
            character_cursor->combat->dying_turns++;
            if (character_cursor->combat->dying_turns > 9)
                character_cursor->status = 6;
        }
        combatant_count++;
        character_cursor = character_cursor->next;
    }
    if (bandage_dying(0))
        bottom_msg("Your Teammate is Dying");
    combat_sides_count();
    combat_view_scroll(cur_map->ox + 3, cur_map->oy + 3, 0xff, 8);
    if (side_count[0] == 0 || side_count[1] == 0 || combat_round >= combat_round_limit)
        *done = 1;
    if (side_count[0] > 0 && side_count[1] == 0 && !demo_active)
        if (confirm(continue_text, 0))
            *done = 0;
}

static void move_hero(struct Character far *character, unsigned char key, char far *done) /* 1f4e:1099 */
{
    unsigned char start_moves;
    char start_facing;
    char cell_x;
    char cell_y;
    unsigned char dir;
    unsigned char foe;
    unsigned char terrain;
    unsigned char key_code;
    char adj_b;
    char adj_a;
    unsigned char cost;
    char prompt[40];
    int col;
    int row;
    long eff;

    start_moves = character->combat->movement;
    start_facing = character->combat->direction;
    cell_x = combatant_x(character);
    cell_y = combatant_y(character);
    key_code = key;
    *done = 0;
    dir = 8;
    while (!*done && key_code != 0 && key_code != 1 && key_code != 0xd) {
        if (key_code == 0xff) {
            str_format(prompt, "%s%2d", "Move/Attack, Move Left = ", character->combat->movement / 2);
            row = combatant_y(character) - cur_map->oy;
            col = combatant_x(character) - cur_map->ox;
            set_menu_kind(4);
            hotkey_reset(1);
            key_code = grid_choose(prompt, move_items, 1, 0, row, col);
        }
        dir = 8;
        if (menu_escaped) {
            switch (key_code) {
            case 0x1b:
                character->combat->movement = start_moves;
                tile_update(0, 0, combatant_slot(character));
                *done = !hero_place(character, cell_x, cell_y, 0);
                combat_view_scroll(combatant_x(character), combatant_y(character), 0, 8);
                if (find_effect(character, 0xd5, &eff))
                    character->combat->direction = 2;
                else
                    character->combat->direction = start_facing;
                dir = 8;
                key_code = 0;
                menu_escaped = 0;
                break;
            case 0x88:
                dir = 0;
                break;
            case 0x81:
                dir = 1;
                break;
            case 0x82:
                dir = 2;
                break;
            case 0x83:
                dir = 3;
                break;
            case 0x84:
                dir = 4;
                break;
            case 0x85:
                dir = 5;
                break;
            case 0x86:
                dir = 6;
                break;
            case 0x87:
                dir = 7;
                break;
            default:
                dir = 8;
            }
        } else if (key_code == 0) {
            character->combat->movement = start_moves;
            tile_update(0, 0, combatant_slot(character));
            *done = !hero_place(character, cell_x, cell_y, 0);
            combat_view_scroll(combatant_x(character), combatant_y(character), 0, 8);
            if (find_effect(character, 0xd5, &eff))
                character->combat->direction = 2;
            else
                character->combat->direction = start_facing;
            dir = 8;
        }
        if (dir < 8) {
            combatant_facing_set(character, dir, 0, 0);
            combat_direction_probe(character, dir, &foe, &terrain, &adj_a, &adj_b);
            if (foe > 0)
                strike(character, (heroes - 1)[foe], done);
            else if (terrain == 0) {
                if (confirm(flee_text, 0))
                    *done = combat_flee(character);
                else {
                    *done = 0;
                    key_code = 0xff;
                }
            } else {
                if (pool_lowbit(dir))
                    cost = (tile_info - 1)[terrain].cost * 3;
                else
                    cost = (tile_info - 1)[terrain].cost * 2;
                if ((tile_info - 1)[terrain].cost == 0xff)
                    cost = 0xff;
                if (character->combat->movement < cost)
                    bottom_msg(cant_go_text);
                else {
                    opportunity_attacks(character, dir);
                    if (character->enabled) {
                        if (character->combat->movement > 0)
                            combat_move_step(character, dir);
                        if (!character->enabled)
                            *done = combat_actions_end(character);
                        fx_terrain_apply(character, 1, 0);
                        if (is_helpless(character) || !character->enabled)
                            *done = combat_actions_end(character);
                    } else
                        *done = combat_actions_end(character);
                }
            }
        }
        if (menu_escaped)
            key_code = 0xff;
    }
    if (character->combat->movement < 2)
        character->combat->movement = 0;
}

static void strike(struct Character far *character, struct Character far *character_cursor, char far *done) /* 1f4e:15a3 */
{
    if (has_ranged_weapon(character) && !has_throwable_weapon(character) && game_state[STATE_UNDERWATER])
        bottom_msg("Can't use that underwater");
    if (has_ranged_weapon(character) && !has_throwable_weapon(character))
        bottom_msg("Not with that weapon");
    else if (attack_confirm(character, character_cursor)) {
        character->combat->target = character_cursor;
        attack_register(character, character_cursor);
        combat_attack(character, character_cursor, 0, 0L, done);
    }
}

static void combat_speed_menu(void)                              /* 1f4e:166c */
{
    char query[42];
    unsigned char count;
    unsigned char choice;

    choice = 0xff;
    while (choice != 2) {
        str_copy(query, msg_format("%s%s%s%s", speed_title, "(", byte_str(game_state[STATE_SPEED]),
                                   ") "));
        mem_fill(hotkeys, sizeof hotkeys, 0xff);
        count = 0;
        if (game_state[STATE_SPEED] > 0)
            hotkey_add(0, &count);
        if (game_state[STATE_SPEED] < 9)
            hotkey_add(1, &count);
        hotkey_add(2, &count);
        choice = menu_run(query, speed_items, 0, 0);
        if (menu_escaped)
            switch (choice) {       /* 1f4e:1746: a char is never 0x104 or 0x108 */
            case 0x1b:
                choice = 2;
                break;
            case 0x108:
                choice = 1;
                break;
            case 0x104:
                choice = 0;
                break;
            }
        switch (choice) {
        case 0:
            game_state[STATE_SPEED]--;
            break;
        case 1:
            game_state[STATE_SPEED]++;
            break;
        }
    }
}

static void clear_target(struct Character far *character)                     /* 1f4e:1791 */
{
    character->quick_fight = 1;
    if (character->combat->target != 0 && character->combat->target->side == character->side)
        character->combat->target = 0;
}
