/* DQK.EXE overlay 00 module 21e4: combat actions.
 * A combatant's turn set up, moving a step and the guards who strike as it
 * passes, fleeing, turning undead, melee and missile attacks with their
 * reports, sweeping several foes, casting and choosing a spell's targets
 * (one, several, or an area scored cell by cell), the targeting menu and
 * its map cursor with the mouse, an effect handler that fires rays and
 * spells by range, morale_base, and the leadership pass that can hand
 * computer-run party members to the player.
 *
 * EXE 0x024d63..0x02a329, one Turbo C large-model _TEXT segment starting at
 * byte 3 of paragraph 21e4, in RTLink section 0 (payload 0x22400, link
 * segment 1f4e; a root section, the parent of 1 and 2).  The first three
 * bytes of paragraph 21e4 are cbtutil.c's tail (module 20cb); its first
 * routine is the lone retf at 21e4:0003, which nothing reaches, left here by
 * the run's convention.  cbview.c (module 2740) starts after the last retf,
 * at 2740:0009.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are
 * defined in shipped order; bake consumes them in that order.  Section 3
 * holds other bytes at link segment 1f4e.
 *
 * One object: section 0 files all 410 fixup sites in the range under frame
 * 21e4, where an object starting later would file its own under its own
 * frame; 43 near calls go back to routines already defined and one, to the
 * static rank_target, goes forward.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.  The ovl00_21e4_
 * names defined here are the placeholders the landed callers already use.
 * combat_attack, view_click and eye_rays are reached through the vector
 * stubs 1d99:102b, 1d99:0fa9 and 1d99:10a3; combat_spell_targets is stored as
 * combat.c's hit_proc.
 *
 * _DATA is area_widths, turn_table and 36 literals, DGROUP 5a40..5d09; -d
 * stores "" on the NUL of "-Backstabs-".  _BSS is 20 statics, DGROUP
 * a896..a959: their names were chosen to fit Turbo C's name order, not
 * recovered, and dummy_int, unused_int and unused_buffer exist only to hold
 * the layout.
 *
 * heroes, near_ids, steps, combatants, recs, tile_info and a combatant's
 * attacks, dice_n, dice_s and dmg_bonus are indexed from 1, as are hits_left
 * and attacks_used by hand in melee_attack; the shipped code adds the address of
 * element 0.
 */

#include "dqk.h"

struct Box {                    /* a rectangle of combat-map cells        */
    char x0;                    /* +0                                     */
    char y0;                    /* +1                                     */
    char x1;                    /* +2                                     */
    char y1;                    /* +3                                     */
};

static int target_menu_choice;                                   /* a896 */
static char target_step_direction;                               /* a898 */
static struct Step reachable_target_list[53];                    /* a899 */
static unsigned char reach;                                      /* a938 */
static unsigned char cur_pick;                                   /* a939 */
static unsigned char pick_cnt;                                   /* a93a */
static struct Character far *saved_aim;                               /* a93b */
static int next_key;                                             /* a93f */
static int dummy_int;                                            /* a941 */
static int target_x;                                             /* a943 */
static int target_y;                                             /* a945 */
static int unused_int;                                           /* a947 */
static char fly_last_x;                                          /* a949 */
static char fly_next_x;                                          /* a94a */
static char fly_last_y;                                          /* a94b */
static char fly_next_y;                                          /* a94c */
static struct Character far *target_hero;                             /* a94d */
static char click_accept;                                        /* a951 */
static char moved_by_click;                                      /* a952 */
static char unused_buffer[7];                                    /* a953 */

static int area_widths[7][3] = {   /* by row offset -3..3            */
    { -3, -3,  1 },
    { -3,  1,  2 },
    {  1,  2,  3 },
    {  1,  2,  3 },
    {  1,  2,  3 },
    { -3,  1,  2 },
    { -3, -3,  1 }
};

static char turn_table[13][11] = { /* turning undead: kind, level band */
    { 99, 10,  7,  4,  1,  1,  0,  0, -1, -1, -1 },
    { 99, 13, 10,  7,  1,  1,  0,  0,  0, -1, -1 },
    { 99, 16, 13, 10,  4,  1,  1,  0,  0,  0, -1 },
    { 99, 19, 16, 13,  7,  4,  1,  1,  0,  0, -1 },
    { 99, 20, 19, 16, 10,  7,  4,  1,  1,  0,  0 },
    { 99, 99, 20, 19, 13, 10,  7,  4,  1,  1,  0 },
    { 99, 99, 99, 20, 16, 13, 10,  7,  4,  1,  0 },
    { 99, 99, 99, 99, 20, 16, 13, 10,  7,  4,  1 },
    { 99, 99, 99, 99, 99, 20, 16, 13, 10,  7,  1 },
    { 99, 99, 99, 99, 99, 99, 20, 16, 13, 10,  4 },
    { 99, 99, 99, 99, 99, 99, 99, 20, 16, 13, 10 },
    { 99, 99, 99, 99, 99, 99, 99, 99, 19, 16, 10 },
    { 99, 99, 99, 99, 99, 99, 99, 99, 20, 19, 13 }
};

void far attacks_left_recalc(struct Character far *character);
unsigned char far half_value(unsigned char value);
void far attack_register(struct Character far *character, struct Character far *character_cursor);
void far combat_attack(struct Character far *character, struct Character far *character_cursor, char mode,
                         struct Item far *missile, char far *done);
char far behind_target(struct Character far *character, struct Character far *character_cursor);
char far combat_target_allowed(struct Character far *character, struct Character far *character_cursor, char any);
unsigned char far fastest_enemy_speed(struct Character far *character);
unsigned char far enemies_near(struct Character far *character, unsigned char range);
void far range_penalty(struct Character far *character, struct Character far *character_cursor, unsigned char far *armor_class);
void far missile_hit(struct Character far *character, struct Character far *character_cursor, struct Item far *missile);
void far combat_aim(struct Character far *character, char range, char gate, char flag, char mode,
                         char far *result, struct Aim far *aim);
char far combat_target_pick(struct Character far *character, char range, char any, char fresh, char flag);
static char rank_target(struct Character far *character_cursor, unsigned char spell, int side);
char far area_aim(unsigned char spell, struct Aim far *aim);
char far turn_target_find(struct Character far *character, struct Character far * far *target, int far *level);

/* The translated Pascal unit's empty initialization block (DKK cbtacts.pas). */
void far cbaction_unit_init(void)                                   /* 21e4:0003 */
{
}

void far combat_round_begin(struct Character far *character)                     /* 21e4:0004 */
{
    unsigned char spell_slot;
    char cell;
    char blocked;
    char flag_b;

    character->combat->spell_cast = 0;
    combat_direction_probe(character, 8, &cell, &cell, &blocked, &flag_b);
    character->combat->can_cast = 0;
    for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
        if (character->spells[spell_slot] > 0 && character->spells[spell_slot] < 0x80)
            character->combat->can_cast = 1;
    character->combat->can_cast = character->combat->can_cast && (char)(blocked == 0);
    character->combat->can_use_item = blocked == 0;
    character->combat->attacked = 0;
    character->combat->attack_form = 2;
    attacks_left_recalc(character);
    move_rate = character->attack_rate[1];
    movement_stops_attack_rate = 0;
    fx_trigger_group(0x12, &character);
    character->attacks_left[1] = half_value(move_rate);
    if (character->enabled) {
        character->combat->initiative = fx_roll_dice(1, 6) + dex_missile(character);
        if (character->combat->initiative < 1)
            character->combat->initiative = 1;
        if ((char)game_state[STATE_INITIATIVE_SIDE_MASK] & (character->side + 1))
            character->combat->initiative -= 6;
        if (character->combat->initiative < 0 || character->combat->initiative > 20)
            character->combat->initiative = 0;
    } else
        character->combat->initiative = 0;
    character->combat->movement = calc_move(character);
}

void far roll_damage(struct Character far *character, struct Character far *character_cursor, unsigned char hand) /* 21e4:01dd */
{
    unsigned char mult;

    damage = fx_roll_damage((character->rolls - 1)[hand], (character->dice - 1)[hand])
             + (character->mods - 1)[hand];
    if (damage < 0)
        damage = 0;
    fx_trigger_group(4, &character);
    fx_trigger_group(5, &character_cursor);
    if (behind_target(character, character_cursor)) {
        if (class_level(character, 6) == 0)
            mult = 5;
        else
            mult = (class_level(character, 6) - 1) / 4 + 2;
        if (mult > 5)
            mult = 5;
        damage = mult * damage;
    }
    element_mask = 0;
}

void far hit_report(struct Character far *character, struct Character far *character_cursor, unsigned char mode,
                    unsigned dealt, int shown, char hit)         /* 21e4:02c0 */
{
    unsigned char row;
    char text[0x52];

    switch (mode) {
    case 2:
        str_copy(text, "-Backstabs-");
        break;
    case 3:
        str_copy(text, "slays helpless");
        break;
    default:
        str_copy(text, "Attacks");
        break;
    }
    hero_text(character, text, 10, 0);
    row = 12;
    character_name_show(character_cursor, 0x17, row, 0);
    row++;
    if (mode == 1)
        str_copy(text, "(from behind) ");
    else
        str_copy(text, "");
    if (hit) {
        if (mode == 3)
            str_copy(text, "with one cruel blow");
        else {
            str_copy(text, msg_format("%sHitting for %s", text, byte_str(shown)));
            if (shown == 1)
                str_copy(text, msg_format("%s point ", text));
            else
                str_copy(text, msg_format("%s points ", text));
            str_copy(text, msg_format("%sof damage", text));
        }
        hero_damage(character_cursor, dealt);
    } else
        str_copy(text, msg_format("%sand Misses", text));
    if (character_cursor->status != 8)
        text_wrap(0x17, row, 0x26, row + 3, 7, 0, 1, text);
    row = text_row + 1;
    if (dealt > 0) {
        character_cursor->combat->can_cast = 0;
        if (character_cursor->combat->spell_cast > 0) {
            text_pause();
            hero_text(character_cursor, "lost a spell", 12, 1);
            spell_mark(character_cursor, character_cursor->combat->spell_cast);
            combat_actions_end(character_cursor);
        } else
            text_pause();
    } else
        text_pause();
    if (!character_cursor->enabled) {
        hero_text(character_cursor, "goes down", row, 0);
        if (!(character_cursor->immunity & 8)) {
            row += 2;
            if (character_cursor->status == 5)
                status_printf(0x17, row, 7, 0, "and is Dying");
            else if (character_cursor->status == 6 || character_cursor->status == 8 || character_cursor->status == 7)
                hero_text(character_cursor, "is killed", row, 0);
            row += 2;
        }
        fx_hp_effects_remove(character_cursor);
        fx_trigger_group(0xd, &character_cursor);
        if (!character_cursor->enabled && character_cursor->status != 10)
            hero_fall(character_cursor);
        else
            text_pause();
    }
    combat_text_clear();
}

void far guard_strikes(struct Character far *character)                       /* 21e4:05d9 */
{
    unsigned char count;
    unsigned char index;
    struct Character far *character_cursor;
    char result;

    count = combat_targets_near(character, 1, 0);
    for (index = 1; index <= count; index++)
        if (character->enabled) {
            character_cursor = (heroes - 1)[(near_ids - 1)[index]];
            if (character_cursor->combat->guarding && !is_helpless(character_cursor)) {
                combat_view_scroll(combatant_x(character), combatant_y(character), 2, 8);
                character_cursor->combat->guarding = 0;
                attack_register(character_cursor, character);
                combat_attack(character_cursor, character, 0, 0L, &result);
            }
        }
}

void far combat_move_step(struct Character far *character, unsigned char dir)  /* 21e4:06c0 */
{
    unsigned char cost;
    unsigned char slot;
    char x;
    char y;
    char x0;
    char y0;

    slot = combatant_slot(character);
    x0 = (combatants - 1)[slot].x;
    y0 = (combatants - 1)[slot].y;
    x = dir_dx[dir] + x0;
    y = dir_dy[dir] + y0;
    if (pool_lowbit(dir))
        cost = (tile_info - 1)[cur_map->cells[y][x]].cost * 3;
    else
        cost = (tile_info - 1)[cur_map->cells[y][x]].cost * 2;
    if (character->combat->movement < cost)
        character->combat->movement = 0;
    else
        character->combat->movement -= cost;
    cost = 1;
    if (character->quick_fight) {
        cost = 3;
        if (!in_combat_view(x - cur_map->ox, y - cur_map->oy) && move_flag)
            combat_view_scroll(x0, y0, 2, 8);
    }
    if (move_flag)
        tile_update(0, 0, slot);
    (combatants - 1)[slot].x = x;
    (combatants - 1)[slot].y = y;
    combat_grid_build();
    if (move_flag)
        combat_view_scroll(x, y, cost, 8);
    character->combat->attacks_taken = 0;
    character->combat->attack_angles = 0;
    sfx_play(0xb);
    guard_strikes(character);
    if (!character->enabled || is_helpless(character))
        character->combat->movement = 0;
}

void far opportunity_attacks(struct Character far *character, unsigned char dir)  /* 21e4:08e6 */
{
    unsigned char cands[12];
    struct Effect far *info;
    char dup;
    char done;
    unsigned char slot;
    unsigned char count;
    unsigned char count2;
    unsigned char scan;
    unsigned char index;
    unsigned char hand;
    unsigned char sel;
    unsigned char side;
    struct Character far *character_cursor;
    struct Character far *saved;

    slot = combatant_slot(character);
    count = combat_targets_near(character, 1, 0);
    if (count != 0) {
        mem_zero(cands, sizeof cands);
        index = 1;
        for (scan = 1; scan <= count; scan++)
            if (!is_helpless((heroes - 1)[(near_ids - 1)[scan]])
                && !(heroes - 1)[(near_ids - 1)[scan]]->combat->panicked
                && !find_effect((heroes - 1)[(near_ids - 1)[scan]], 0x1b, &info)) {
                (cands - 1)[index] = (near_ids - 1)[scan];
                index++;
            }
        count = index - 1;
        (combatants - 1)[slot].x += dir_dx[dir];
        (combatants - 1)[slot].y += dir_dy[dir];
        count2 = combat_targets_near(character, 1, 0);
        (combatants - 1)[slot].x -= dir_dx[dir];
        (combatants - 1)[slot].y -= dir_dy[dir];
        for (index = 1; index <= count; index++) {
            dup = 0;
            for (scan = 1; scan <= count2; scan++)
                if ((cands - 1)[index] == (near_ids - 1)[scan])
                    dup = 1;
            if (dup)
                (cands - 1)[index] = 0;
        }
        for (index = 1; index <= count; index++)
            if ((cands - 1)[index] > 0 && character->enabled) {
                panel_dirty = 1;
                move_flag = 1;
                done = 0;
                character_cursor = (heroes - 1)[(cands - 1)[index]];
                if (combat_target_allowed(character_cursor, character, 0)
                    && (!has_ranged_weapon(character_cursor) || has_throwable_weapon(character_cursor)))
                    for (side = character_cursor->combat->direction + 6; side <= character_cursor->combat->direction + 10; side++)
                        if (!done
                            && (character_cursor->combat->initiative > 0 || !character_cursor->combat->attacks_taken
                                || dir_ok(combatant_x(character_cursor), combatant_y(character_cursor),
                                                combatant_x(character), combatant_y(character),
                                                side % 8))) {
                            sel = 1;
                            if (character_cursor->attack_rate[0] == 0)
                                sel = 2;
                            for (hand = 1; hand <= 2; hand++)
                                if ((character_cursor->attacks_left - 1)[hand] > 0)
                                    sel = hand;
                            if ((character_cursor->attacks_left - 1)[sel] == 0)
                                (character_cursor->attacks_left - 1)[sel] = 1;
                            character_cursor->combat->attack_form = sel;
                            saved = character_cursor->combat->target;
                            if (!find_effect(character_cursor, 0xd2, &info) && !find_effect(character_cursor, 0x6a, &info)
                                && !find_effect(character_cursor, 0x1f, &info) && !find_effect(character_cursor, 0x33, &info)
                                && !find_effect(character_cursor, 0x34, &info) && !find_effect(character_cursor, 0x35, &info)
                                && !find_effect(character_cursor, 0x37, &info) && !find_effect(character_cursor, 0x76, &info)
                                && !find_effect(character_cursor, 0x82, &info) && !find_effect(character_cursor, 0xd5, &info))
                                combat_attack(character_cursor, character, 1, 0L, &combat_action_done);
                            done = 1;
                            character_cursor->combat->target = saved;
                            if (character->enabled) {
                                panel_dirty = 1;
                                hero_panel(character);
                            }
                        }
            }
    }
}

char far combat_flee(struct Character far *character)                     /* 21e4:0e26 */
{
    char result;
    char got_away;
    unsigned char value;
    unsigned char need;

    got_away = 0;
    if (encounter_mode != 0x20)
        if (combat_targets_near(character, 0xff, 0) == 0)
            got_away = 1;
        else {
            need = calc_move(character) / 2;
            value = fastest_enemy_speed(character);
            if (value < need || value == need && fx_roll_dice(1, 2) == 1)
                got_away = 1;
        }
    if (got_away)
        fx_out_of_play(character, 3, "Got Away");
    else
        bottom_msg("Escape is blocked");
    result = combat_actions_end(character);
    return result;
}

void far attacks_left_recalc(struct Character far *character)                     /* 21e4:0edf */
{
    unsigned char value;
    unsigned char orig;
    unsigned char cand;
    unsigned char rate;
    char found;
    struct Item far *sel;

    found = 0;
    orig = character->attacks_left[0];
    character->attacks_left[0] = character->attack_rate[0];
    if (has_ranged_weapon(character) && ranged_ammo_find(character, &sel)) {
        found = 1;
        rate = item_table[character->equipped[0]->rec.type].rate_of_fire;
        if (rate < 2)
            rate = 2;
        move_rate = rate;
    } else
        move_rate = character->attacks_left[0];
    movement_stops_attack_rate = 0;
    fx_trigger_group(0x12, &character);
    value = half_value(move_rate);
    if (found && sel != 0) {
        cand = 1;
        if (sel != 0 && sel->rec.quantity > cand)
            cand = sel->rec.quantity;
        if (cand < value && sel->rec.quantity > 0)
            value = cand;
    }
    if (character->combat->attacked == 0 || value < orig)
        character->attacks_left[0] = value;
    else
        character->attacks_left[0] = orig;
}

unsigned char far half_value(unsigned char value)                    /* 21e4:0ffc */
{
    unsigned char result;
    unsigned char rounded_value;

    rounded_value = value;
    if (pool_lowbit(combat_round))
        rounded_value++;
    result = rounded_value / 2;
    return result;
}

char far combat_target_allowed(struct Character far *character, struct Character far *character_cursor, char any) /* 21e4:1030 */
{
    char result;
    struct Character far *saved;

    result = 0;
    if (character_cursor != 0)
        if (character == character_cursor)
            result = 1;
        else {
            refused = 0;
            saved = cur_node;
            cur_node = character;
            fx_trigger_group(1, &character_cursor);
            cur_node = saved;
            if (!refused && (character_cursor->side != character->side || any)) {
                saved_aim = character->combat->target;
                character->combat->target = character_cursor;
                fx_trigger_group(0, &character);
                character->combat->target = saved_aim;
            } else
                refused = 1;
            result = !refused;
        }
    return result;
}

void far turn_undead(struct Character far *character)                     /* 21e4:1117 */
{
    unsigned char roll;
    unsigned char left;
    unsigned char count;
    char row;
    char turned;
    struct Character far *character_cursor;
    char col;
    int level;

    hero_text(character, "turns undead...", 10, 0);
    box_draw();
    text_pause();
    turned = 0;
    character->combat->turn_attempts++;
    level = 0xd;
    count = 6;
    roll = fx_roll_dice(1, 0x14);
    col = class_level(character, 0);
    if (class_level(character, 3) - 2 > col)
        col = class_level(character, 3) - 2;
    if (character->deity == 2)
        col += 2;
    if (col > 8)
        if (col < 14)
            col = 9;
        else
            col = 10;
    if (game_state[STATE_TURN_UNDEAD_PENALTY] > col)
        col = 0;
    else
        col -= game_state[STATE_TURN_UNDEAD_PENALTY];
    left = 0xff;
    while (turn_target_find(character, &character_cursor, &level) && left > 0) {
        row = turn_table[character_cursor->level_undead - 1][col];
        if (left == 0xff)
            if (row < 0)
                left = fx_roll_dice(1, 6) + 6;
            else
                left = fx_roll_dice(1, 0xc);
        if (char_abs(row) <= roll) {
            turned = 1;
            view_center(character_cursor, 3, 0);
            panel_dirty = 1;
            hero_panel(character_cursor);
            if (row > 0)
                fx_out_of_play(character_cursor, 3, "is turned");
            else {
                hero_text(character_cursor, "Is destroyed", 10, 0);
                hero_fall(character_cursor);
                character_cursor->status = 8;
                character_cursor->enabled = 0;
            }
            if (count > 0)
                count--;
            left--;
            if (left == 0 && count > 0 && row < 0)
                left++;
            combat_text_clear();
        } else
            level--;
    }
    if (!turned)
        bottom_msg("Nothing Happens...");
    combat_sides_count();
    combat_action_done = combat_actions_end(character);
    combat_text_clear();
}

char far turn_target_find(struct Character far *character, struct Character far * far *target,
                         int far *level)                         /* 21e4:135e */
{
    char found;
    unsigned char count;
    unsigned char index;
    short best;
    struct Character far *character_cursor;

    best = 0;
    *target = 0;
    count = combat_targets_near(character, 0xff, 0);
    found = 0;
    for (index = 1; index <= count; index++) {
        character_cursor = (heroes - 1)[(near_ids - 1)[index]];
        if (!character_cursor->combat->panicked && character_cursor->level_undead > 0 && character_cursor->level_undead > best
            && character_cursor->level_undead <= *level) {
            best = character_cursor->level_undead;
            *target = character_cursor;
            found = 1;
        }
    }
    *level = best;
    return found;
}

void far melee_attack(struct Character far *character, struct Character far *character_cursor, char mode, char far *done,
                      struct Item far *missile)                  /* 21e4:1417 */
{
    struct ItemInfo info;
    char hit;
    char stop;
    char rear;
    unsigned char armor_class;
    unsigned char hand;
    unsigned char hands;
    unsigned char how;
    short side;
    struct Effect far *effect;

    rear = mode;
    *done = 0;
    hits_left[0] = 0;
    hits_left[1] = 0;
    attacks_used[0] = 0;
    attacks_used[1] = 0;
    hit = 0;
    stop = 0;
    damage = 0;
    character->combat->attacked = 1;
    if (is_helpless(character_cursor)) {
        sfx_play(8);
        while ((character->attacks_left - 1)[character->combat->attack_form] == 0)
            character->combat->attack_form--;
        (attacks_used - 1)[character->combat->attack_form]++;
        hit_report(character, character_cursor, 3, character_cursor->hp + 5, 1, 1);
        fx_invisibility_remove(character);
        character->attacks_left[0] = 0;
        character->attacks_left[1] = 0;
        hit = 1;
        *done = 1;
    } else {
        if (character->equipped[0] != 0 && (character_cursor->icon_dim > 0x80 || (character_cursor->icon_dim & 7) > 1)) {
            info = item_table[character->equipped[0]->rec.type];
            character->rolls[0] = info.rolls_large;
            character->dice[0] = info.dice_large;
            character->mods[0] = character->mods[0] - info.mod + info.mod_large;
        }
        recalc_stats(character_cursor);
        fx_trigger_group(0xb, &character_cursor);
        side = 0;
        if (behind_target(character, character_cursor))
            armor_class = character_cursor->ac[1] - 4;
        else {
            if (character_cursor->combat->attacks_taken > 1 && combatant_direction(character, character_cursor) == character_cursor->combat->direction
                && character_cursor->combat->attack_angles > 4)
                rear = 1;
            if (rear)
                side = 1;
            armor_class = character_cursor->ac[side];
        }
        range_penalty(character, character_cursor, &armor_class);
        how = 0;
        if (rear)
            how = 1;
        if (behind_target(character, character_cursor))
            how = 2;
        hands = character->combat->attack_form;
        for (hand = hands; hand >= 1; hand--) {
            while ((character->attacks_left - 1)[hand] > 0 && !stop) {
                (character->attacks_left - 1)[hand]--;
                character->combat->attack_form = hand;
                (attacks_used - 1)[hand]++;
                if (fx_attack_roll(character, character_cursor, armor_class) || is_helpless(character_cursor)) {
                    (hits_left - 1)[hand]++;
                    sfx_play(8);
                    hit = 1;
                    roll_damage(character, character_cursor, hand);
                    if (character_cursor->enabled)
                        hit_report(character, character_cursor, how, damage, damage, 1);
                    if (character_cursor->enabled && (damage > 0 || find_effect(character, 0xb8, &effect)))
                        fx_trigger_group(hand + 1, &character);
                    if (!character_cursor->enabled)
                        stop = 1;
                    if (!character->enabled) {
                        hands = 0;
                        (character->attacks_left - 1)[hand] = 0;
                    }
                }
            }
            fx_death_explosions();
            fx_ice_explosions();
            fx_meteor_explosions();
        }
        if (!hit) {
            if (missile == 0)
                sfx_play(10);
            hit_report(character, character_cursor, how, 0, 0, 0);
        }
        *done = 1;
        for (hand = 1; hand <= 2; hand++)
            if ((character->attacks_left - 1)[hand] > 0)
                *done = 0;
    }
    if (!character->enabled)
        *done = 1;
    if (*done)
        *done = combat_actions_end(character);
}

void far attack_register(struct Character far *character, struct Character far *character_cursor) /* 21e4:1853 */
{
    unsigned char turn;
    unsigned char dir;

    character_cursor->combat->attacks_taken++;
    dir = combatant_direction(character_cursor, character);
    turn = (dir - character_cursor->combat->direction + 8) % 8;
    if (turn > 4)
        turn = 8 - turn;
    character_cursor->combat->attack_angles = (character_cursor->combat->attack_angles + turn) % 8;
}

void far combat_attack(struct Character far *character, struct Character far *character_cursor, char mode,
                         struct Item far *missile, char far *done) /* 21e4:18d6 */
{
    struct Item far *copy;
    struct Item far *item;
    struct Character far *saved;
    unsigned char dir;
    struct Effect far *effect;

    move_flag = 1;
    panel_dirty = 1;
    combat_round_limit = combat_round + 0xf;
    if (character_cursor->combat->attacks_taken < 2 && !mode) {
        dir = combatant_direction(character_cursor, character);
        if (find_effect(character_cursor, 0xd5, &effect))
            character_cursor->combat->direction = 2;
        else
            character_cursor->combat->direction = (dir + 4) % 8;
    } else if (combatant_visible(character_cursor, 0)) {
        dir = character_cursor->combat->direction;
        if (!mode)
            if (find_effect(character_cursor, 0xd5, &effect))
                character_cursor->combat->direction = 2;
            else
                character_cursor->combat->direction = (dir + 4) % 8;
    }
    if (combatant_visible(character_cursor, 0))
        combatant_facing_set(character_cursor, dir, 0, 0);
    dir = combatant_direction(character, character_cursor);
    hero_panel(character);
    combatant_facing_set(character, dir, 1, 0);
    character->combat->target = character_cursor;
    delay(100);
    if (missile != 0)
        missile_hit(character, character_cursor, missile);
    if (character->equipped[0]->rec.type == 0x1c || character->equipped[0]->rec.type == 0x1d) /* equip[0] unchecked */
        missile_hit(character, character_cursor, character->equipped[0]);
    *done = 1;
    if (character->attacks_left[0] > 0 || character->attacks_left[1] > 0) {
        saved = cur_node;
        cur_node = character;
        melee_attack(character, character_cursor, mode, done, missile);
        if (missile != 0 && !wields_43(character)) {
            if (missile->rec.quantity > 0)
                missile->rec.quantity -= attacks_used[0];
            if (missile->rec.quantity == 0)
                if (has_throwable_weapon(character) && missile->rec.u.magic.effect != 0x17) {
                    pool_alloc((void far * far *)&copy, 0x3d, &item_record_pool);
                    mem_move5(missile, copy, 0x3d);
                    item = pool_items;
                    if (item == 0) {
                        pool_items = copy;
                        copy->next = 0;
                    } else {
                        while (item->next != 0 && item != dropped_item_boundary)
                            item = item->next;
                        copy->next = item->next;
                        item->next = copy;
                    }
                    copy->rec.equipped = 0;
                    item_remove(character, missile);
                    dropped_item_boundary = copy;
                } else
                    item_remove(character, missile);
        }
        recalc_stats(character);
        cur_node = saved;
    }
    if (*done)
        *done = combat_actions_end(character);
    if (combatant_visible(character, 0)) {
        combatant_facing_set(character, character->combat->direction, 1, 1);
        combatant_facing_set(character, character->combat->direction, 0, 0);
    }
}

void far range_penalty(struct Character far *character, struct Character far *character_cursor,
                       unsigned char far *armor_class)                    /* 21e4:1c9d */
{
    char step;
    char dist;

    dist = get_distance(character, character_cursor);
    if (has_ranged_weapon(character))
        step = (item_table[character->equipped[0]->rec.type].range - 1) / 3;
    else
        step = dist;
    if (dist - step > 0) {
        dist = dist - step;
        *armor_class = *armor_class + 2;
    }
    if (dist - step > 0) {
        dist = dist - step;
        *armor_class = *armor_class + 3;
    }
}

char far aim_target(struct Aim far *aim, int far *side, struct Character far * far *target,
                    unsigned char far *spell, char far *ok)      /* 21e4:1d42 */
{
    char result;
    char found;
    char friendly;

    found = 0;
    *ok = 1;
    friendly = (spell_info - 1)[*spell].hostile == 0;
    if (combat_target_pick(cur_node, spell_range(*spell), 0, 1, friendly)) {
        *target = cur_node->combat->target;
        if (friendly)
            *side = cur_node->side;
        else
            *side = enemy_team(cur_node);
        *ok = rank_target(*target, *spell, *side) > 0;
        if (*ok) {
            aim->target = cur_node->combat->target;
            aim->x = combatant_x(aim->target);
            aim->y = combatant_y(aim->target);
            found = 1;
        }
    }
    result = found;
    return result;
}

char far choose_aim(unsigned char spell, char chosen, char area,
                    struct Aim far *aim)                         /* 21e4:1e78 */
{
    char result;
    char found;
    char ok;
    struct Character far *target;
    char friendly;
    int side;

    found = 0;
    if (!chosen) {
        friendly = spell != 0x53;
        combat_aim(cur_node, spell_range(spell), 0, area, friendly, &found, aim);
        cur_node->combat->target = aim->target;
    } else if (!area || (spell_info - 1)[spell].targeting == 8)
        found = aim_target(aim, &side, &target, &spell, &ok);
    else
        found = area_aim(spell, aim);
    if (found) {
        result = 1;
        view_cx = aim->x;
        view_cy = aim->y;
    } else {
        result = 0;
        mem_fill(aim, sizeof(struct Aim), 0);
    }
    return result;
}

void far combat_spell_targets(unsigned char spell, char mode, char far *result) /* 21e4:1f78 */
{
    unsigned char left;
    unsigned char count;
    unsigned char target_index;
    unsigned char limit;
    unsigned char cost;
    struct Aim aim;
    char dup;
    char stop;

    mem_zero(targets, 4);
    *result = 1;
    target_count = 0;
    mirror_image_bypass = 0;
    view_cx = combatant_x(cur_node);
    view_cy = combatant_y(cur_node);
    switch ((spell_info - 1)[spell].targeting & 0xf) {
    case 0:
        targets[1] = cur_node;
        target_count = 1;
        break;
    case 5:
        cost = 0;
        count = 0;
        if (spell == 0x4f)
            limit = caster_level(0x4f, 0);
        else
            limit = fx_roll_dice(2, 4);
        stop = 0;
        do {
            if (choose_aim(spell, mode, 0, &aim)) {
                dup = 0;
                for (target_index = 1; target_index <= count; target_index++)
                    if (targets[target_index] == aim.target)
                        dup = 1;
                if (!dup) {
                    count++;
                    targets[count] = aim.target;
                    view_cx = combatant_x(aim.target);
                    view_cy = combatant_y(aim.target);
                    target_count++;
                    if (spell != 0x4f)
                        switch (targets[count]->level_high) {
                        case 0:
                        case 1:
                            cost++;
                            break;
                        case 2:
                            cost += 2;
                            break;
                        case 3:
                            cost += 4;
                            break;
                        default:
                            cost += 8;
                        }
                    else
                        switch (targets[count]->icon_dim) {
                        case 1:
                            cost++;
                            break;
                        case 2:
                        case 3:
                            cost += 2;
                            break;
                        case 4:
                            cost += 4;
                        }
                    if (count > 1 && cost > limit)
                        stop = 1;
                } else if (mode)
                    limit--;
                else
                    bottom_msg("Already been targeted");
                cell_redraw(combatant_x(aim.target), combatant_y(aim.target));
            } else
                stop = 1;
        } while (!stop && limit != 0);
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        if (choose_aim(spell, mode, 1, &aim)) {
            if (spell == 0x57 && game_state[STATE_UNDERWATER])
                plan_steps(view_cx, view_cy, 3, 0xff, 1);
            else if (spell == 0x33 && game_state[STATE_UNDERWATER] || spell == 0x73 && !game_state[STATE_DUNGEON]
                     || spell == 0x2f && !game_state[STATE_DUNGEON])
                plan_steps(view_cx, view_cy, 2, 0xff, 1);
            else
                plan_steps(view_cx, view_cy, (spell_info - 1)[spell].targeting & 7, 0xff, 1);
            for (count = 1; count <= step_count; count++)
                targets[count] = (heroes - 1)[(steps - 1)[count].who];
            target_count = step_count;
            mirror_image_bypass = 1;
        } else
            *result = 0;
        break;
    case 15:
        if (choose_aim(spell, mode, 0, &aim)) {
            if (cur_node->combat->target != 0) {
                targets[1] = cur_node->combat->target;
                target_count = 1;
            } else {
                plan_steps(view_cx, view_cy, (spell_info - 1)[spell].targeting >> 4, 0xff, 1);
                for (count = 1; count <= step_count; count++)
                    targets[count] = (heroes - 1)[(steps - 1)[count].who];
                target_count = step_count;
                mirror_image_bypass = 1;
            }
        } else
            *result = 0;
        break;
    default:
        left = ((spell_info - 1)[spell].targeting & 3) + 1;
        count = 0;
        while (left > 0)
            if (choose_aim(spell, mode, 0, &aim)) {
                dup = 0;
                for (target_index = 1; target_index <= count; target_index++)
                    if (targets[target_index] == aim.target)
                        dup = 1;
                if ((spell == 0x17 || spell == 0x31) && aim.target == cur_node)
                    dup = 1;
                if (!dup) {
                    count++;
                    targets[count] = aim.target;
                    left--;
                    view_cx = combatant_x(aim.target);
                    view_cy = combatant_y(aim.target);
                } else if (!mode) {
                    if (aim.target == cur_node)
                        bottom_msg("Cannot target yourself");
                    else
                        bottom_msg("Already been targeted");
                } else
                    left--;
                cell_redraw(combatant_x(aim.target), combatant_y(aim.target));
            } else
                left = 0;
        target_count = count;
        if (count == 0)
            *result = 0;
        else {
            view_cx = combatant_x(targets[count]);
            view_cy = combatant_y(targets[count]);
        }
    }
}

void far combat_cast_begin(unsigned char spell, char mode, char far *done) /* 21e4:2543 */
{
    unsigned char kind;
    unsigned char delay_by;
    unsigned char index;
    int choice;
    char more;
    struct Character far *cur;
    struct Character far *next;

    cur = cur_node;
    more = 1;
    choice = -1;
    *done = 0;
    kind = spell;
    if (kind == 0)
        kind = spell_menu(0, 1, &choice, &more);
    if (kind > 0 && (spell_info - 1)[kind].combat_ok == 0) {
        bottom_msg("Camp Only Spell");
        kind = 0;
    }
    if (game_state[STATE_UNDERWATER] && (kind == 0x4a || kind == 9 || kind == 0x2f || kind == 0x76
                             || kind == 0x73 || kind == 0x7c)) {
        bottom_msg("Can't be cast underwater!");  /* v1.0 wording */
        kind = 0;
    }
    if (!mode) {
        combat_view_show();
        move_flag = 1;
        panel_dirty = 1;
        view_center(cur, 3, 1);
        hero_panel(cur);
    }
    if (kind > 0) {
        delay_by = (spell_info - 1)[kind].casting_time / 3;
        if (delay_by == 0) {
            spell_cast(kind, mode, 1, done);
            *done = combat_actions_end(cur);
        } else {
            *done = 1;
            hero_text(cur, "Begins Casting", 10, 1);
            cur->combat->spell_cast = kind;
            if (cur->combat->initiative > delay_by)
                cur->combat->initiative -= delay_by;
            else
                cur->combat->initiative = 1;
            index = order_idx;
            goto check;
        shift:
            combat_order[index] = next;
            index++;
        check:
            next = combat_order[index + 1];
            if (next != 0
                && (next->combat->initiative > cur->combat->initiative
                    || next->combat->initiative == cur->combat->initiative
                       && fx_roll_dice(1, 2) == 1))
                goto shift;
            combat_order[index] = cur;
            order_idx--;
            *done = 1;
        }
    }
}

char far behind_target(struct Character far *character, struct Character far *character_cursor)   /* 21e4:278f */
{
    char result;
    struct Item far *item;
    unsigned char type;
    char able;
    struct Effect far *effect;

    able = 0;
    item = character->equipped[0];
    if (item != 0)
        type = item->rec.type;
    if (class_level(character, 6) > 0 || find_effect(character, 0xbe, &effect))
        if (item == 0 || type == 3 || type == 4 || type == 0x11 || type == 0x12 || type == 0x13
            || type == 0x3f || type == 0x43)
            able = 1;
    if (able && character_cursor->combat->attacks_taken > 1 && get_distance(character, character_cursor) < 2 && (character_cursor->icon_dim & 0x7f) <= 1
        && combatant_direction(character, character_cursor) == character_cursor->combat->direction)
        result = 1;
    else
        result = 0;
    return result;
}

void far missile_hit(struct Character far *character, struct Character far *character_cursor,
                     struct Item far *missile)                   /* 21e4:2888 */
{
    unsigned char frames;
    unsigned char dir;
    unsigned char pace;

    dir = combatant_direction(character, character_cursor);
    frames = 1;
    pace = 10;
    switch (missile->rec.type) {
    case 4:
    case 5:
    case 7:
    case 0xc:
    case 0x1e:
    case 0x42:
    case 0x4a:
        missile_facing(dir, 0xf);
        sfx_play(0xd);
        break;
    case 2:
    case 3:
    case 6:
        icon_facings(0x12);
        frames = 4;
        pace = 0x32;
        sfx_play(10);
        break;
    case 0x1c:
    case 0x1d:
    case 0x43:
        icon_facing(0x16, 0, 0, 0);
        icon_facing(0x16, 1, 1, 0);
        frames = 2;
        pace = 10;
        sfx_play(7);
        break;
    default:
        icon_facing(0x15, 0, 0, 0);
        icon_facing(0x15, 1, 1, 0);
        frames = 2;
        pace = 0x14;
        sfx_play(10);
    }
    icon_fly(combatant_x(character), combatant_y(character), combatant_x(character_cursor), combatant_y(character_cursor),
             frames, pace);
}

void far hostile_hp_percent_update(void)                                   /* 21e4:29f1 */
{
    unsigned short total;
    unsigned short left;
    struct Character far *character;

    total = 0;
    left = 0;
    for (character = node_list; character != 0; character = character->next)
        if (character->side == 1 && character->status != 9) {
            if (character->enabled)
                left = left + character->hp;
            total = total + character->hp_max;
        }
    if (total > 0)
        morale_base = left * 20 / total * 5;
}

unsigned char far fastest_enemy_speed(struct Character far *character)            /* 21e4:2a76 */
{
    unsigned char result;
    unsigned char best;
    unsigned char rate;
    struct Character far *character_cursor;

    best = 0;
    for (character_cursor = node_list; character_cursor != 0; character_cursor = character_cursor->next)
        if (enemy_team(character) == character_cursor->side && character_cursor->enabled) {
            rate = calc_move(character_cursor) / 2;
            if (rate > best)
                best = rate;
        }
    result = best;
    return result;
}

char far attack_confirm(struct Character far *character, struct Character far *character_cursor) /* 21e4:2aef */
{
    char result;
    char saved;

    saved = menu_escaped;
    if (enemy_team(character_cursor) == character->side || character->quick_fight)
        result = 1;
    else if (!confirm(ally_prompt, 0))
        result = 0;
    else {
        result = 1;
        same_team_attack_approved = 1;
        game_state[STATE_ALLY_ATTACK_CONFIRMED] = 1;
        combat_sides_count();
    }
    menu_escaped = saved;
    return result;
}

unsigned char far target_menu(struct Character far *character, struct Character far *character_cursor, unsigned char range,
                              char attack, char empty_ok, char any_ok, char enabled, char x,
                              char y)                            /* 21e4:2b60 */
{
    unsigned char result;
    struct Item far *sel;
    unsigned dist;
    char open;
    unsigned char index;
    unsigned char count;

    dist = 0xff;
    open = cur_map->dist_mode;
    cur_map->dist_mode = 1;
    line_clear(combatant_x(character), combatant_y(character), &x, &y, &dist);
    cur_map->dist_mode = open;
    dist = dist >> 1;
    if (attack)
        status_printf(0, 0x17, 7, 0, msg_format("%s%d ", range_text, dist));
    mem_fill(hotkeys, sizeof hotkeys, 0xff);
    count = 0;
    for (index = 0; index <= 5; index++)
        if (index == 3) {
            if (range >= dist && enabled)
                if (!attack) {
                    if (any_ok && character_cursor != 0 || empty_ok && character_cursor == 0)
                        hotkey_add(3, &count);
                } else if (character != character_cursor)
                    if (!has_ranged_weapon(character))
                        hotkey_add(3, &count);
                    else if (ranged_ammo_find(character, &sel))
                        if (combat_targets_near(character, 1, 0) == 0 || has_throwable_weapon(character))
                            hotkey_add(3, &count);
        } else
            hotkey_add(index, &count);
    if (character_cursor != 0) {
        view_center(character_cursor, 3, 1);
        panel_dirty = 1;
        hero_panel(character_cursor);
    }
    result = menu_run(blank_text, aim_items, 1, 0);
    return result;
}

void far aim_at(struct Character far *character, struct Character far *character_cursor, char mode, char far *result,
                struct Aim far *aim)                             /* 21e4:2d46 */
{
    struct Item far *sel;

    sel = 0;
    *result = 1;
    if (mode == 1 && !attack_confirm(character, character_cursor))
        *result = 0;
    if (*result) {
        aim->target = character_cursor;
        aim->x = combatant_x(character_cursor);
        aim->y = combatant_y(character_cursor);
        cur_map->cursor_on = 0;
        combat_view_scroll(cur_map->ox + 3, cur_map->oy + 3, 3, 8);
        if (mode == 1) {
            attack_register(character, character_cursor);
            if (has_ranged_weapon(character) && ranged_ammo_find(character, &sel) && has_throwable_weapon(character)
                && get_distance(character, character_cursor) == 1)
                sel = 0;
            combat_attack(character, character_cursor, 0, sel, result);
        }
    } else
        mem_fill(aim, sizeof(struct Aim), 0);
}

void far aim_menu(struct Character far *character, struct Character far *character_cursor, unsigned char range, char attack,
                  char mode, char pair_ok, char far *active, char far *accepted,
                  struct Aim far *aim, char x, char y)           /* 21e4:2e9c */
{
    unsigned char count;
    unsigned char key;
    unsigned char dir;
    unsigned char who;
    unsigned char kind;
    unsigned dist;
    char tx;
    char ty;
    unsigned char index;
    struct Item far *sel;

    mem_fill(aim, sizeof(struct Aim), 0);
    x = combatant_x(character_cursor);
    y = combatant_y(character_cursor);
    target_x = x;
    target_y = y;
    key = 0xff;
    dir = 8;
    *accepted = 0;
    cur_map->cursor_on = 1;
    cur_map->cursor_size = 1;
    moved_by_click = 0;
    while (key != 0 && key != 2 || moved_by_click) {
        if (!moved_by_click) {
            combat_view_scroll(x, y, 3, dir);
            x += dir_dx[dir];
            y += dir_dy[dir];
            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;
            if (x > 0x31)
                x = 0x31;
            if (y > 0x18)
                y = 0x18;
        } else {
            x = target_x;
            y = target_y;
        }
        cell_get(x, y, &who, &kind);
        dist = 0xff;
        *active = 0;
        tx = x;
        ty = y;
        target_x = x;
        target_y = y;
        if (line_clear(combatant_x(character), combatant_y(character), &tx, &ty, &dist)) {
            *active = 1;
            if (attack)
                status_printf(0, 0x17, 7, 0, msg_format("%s%d\t", range_text, dist >> 1));
        } else if (attack)
            box_draw_top();
        dist = dist >> 1;
        character_cursor = 0;
        if (*active)
            if (who > 0)
                character_cursor = (heroes - 1)[who];
            else if (kind == 0x1f || kind == 0x1e)
                for (index = 1; index <= rec_count; index++)
                    if ((recs - 1)[index].x == x && (recs - 1)[index].y == y)
                        character_cursor = (recs - 1)[index].hero;
        if (character_cursor != 0) {
            panel_dirty = 1;
            hero_panel(character_cursor);
        } else
            text_clear(0x17, 1, 0x26, 0x15);
        if (range < dist || (tile_info - 1)[kind].cost == 0xff)
            *active = 0;
        if (character_cursor != 0) {
            if (!combat_target_allowed(character, character_cursor, 1) || !pair_ok)
                *active = 0;
            if (attack == 1)
                if (character == character_cursor || who == 0 && kind == 0x1f)
                    *active = 0;
                else if (has_ranged_weapon(character) /* v1.0: no adjacent-throw exemption */
                         && (!ranged_ammo_find(character, &sel)
                             || combat_targets_near(character, 1, 0) > 0 && !has_throwable_weapon(character)))
                    *active = 0;
        } else if (!mode)
            *active = 0;
        mem_fill(hotkeys, sizeof hotkeys, 0xff);
        count = 0;
        if (*active)
            hotkey_reset(2);
        else {
            hotkey_add(1, &count);
            hotkey_add(2, &count);
        }
        set_menu_kind(5);
        moved_by_click = 0;
        click_accept = 0;
        if (next_key == 0)
            key = grid_pick(blank_text, target_items, 1, 0);
        else {
            key = next_key;
            next_key = 0;
        }
        if (!moved_by_click) {
            if (click_accept || menu_escaped && key == 0xd) {
                menu_escaped = 0;
                key = 0;
                click_accept = 0;
            }
            if (menu_escaped)
                switch (key) {
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
                case 0x80:
                    combat_view_scroll(x, y, 0, 8);
                    dir = 8;
                    break;
                case 0x1b:
                    cell_redraw(x, y);
                    mem_fill(aim, sizeof(struct Aim), 0);
                    *accepted = 0;
                    key = 2;
                    break;
                default:
                    dir = 8;
                }
            else
                switch (key) {
                case 0:
                    *accepted = 1;
                    aim->x = x;
                    aim->y = y;
                    if (character_cursor != 0)
                        aim->target = character_cursor;
                    else
                        aim->target = 0;
                    cur_map->cursor_on = 0;
                    cell_redraw(x, y);
                    turn_done = 0;
                    break;
                case 1:
                    combat_view_scroll(x, y, 0, 8);
                    dir = 8;
                    break;
                case 2:
                    cell_redraw(x, y);
                    mem_fill(aim, sizeof(struct Aim), 0);
                    *accepted = 0;
                    break;
                default:
                    dir = 8;
                }
        }
    }
}

struct Character far *far step_target(char delta, struct Character far *character) /* 21e4:3465 */
{
    struct Character far *character_cursor;
    struct Character far *next;
    char found;

    saved_aim = (heroes - 1)[reachable_target_list[cur_pick].who];
    if (saved_aim == 0)
        saved_aim = character;
    if (pick_cnt < 1)
        game_exit();
    fly_last_x = (combatants - 1)[reachable_target_list[cur_pick].who].x;
    fly_last_y = (combatants - 1)[reachable_target_list[cur_pick].who].y;
    cur_pick += delta;
    if (cur_pick == 0)
        cur_pick = pick_cnt;
    if (cur_pick > pick_cnt)
        cur_pick = 1;
    character_cursor = (heroes - 1)[reachable_target_list[cur_pick].who];
    if (delta != 0 && character_cursor->side != 1) {
        found = 0;
        for (tick_slot = 0; tick_slot < pick_cnt && !found; tick_slot++)
            if (character_cursor->side != 0)
                found = 1;
            else {
                cur_pick += delta;
                if (cur_pick == 0)
                    cur_pick = pick_cnt;
                if (cur_pick > pick_cnt)
                    cur_pick = 1;
                character_cursor = (heroes - 1)[reachable_target_list[cur_pick].who];
            }
    }
    next = (heroes - 1)[reachable_target_list[cur_pick].who];
    fly_next_x = (combatants - 1)[reachable_target_list[cur_pick].who].x;
    fly_next_y = (combatants - 1)[reachable_target_list[cur_pick].who].y;
    if (next == 0 || fly_next_x < 0 || fly_next_y < 0) {
        character_cursor = saved_aim;
        fly_next_x = fly_last_x;
        fly_next_y = fly_last_y;
    } else {
        character_cursor = next;
        icon_fly(fly_last_x, fly_last_y, fly_next_x, fly_next_y, 1, 0);
        fly_last_x = fly_next_x;
        fly_last_y = fly_next_y;
    }
    return character_cursor;
}

void far build_picks(struct Character far *character)                         /* 21e4:36c6 */
{
    unsigned char step_index;
    struct Character far *character_cursor;

    cur_pick = 1;
    plan_steps(combatant_x(character), combatant_y(character), 0xff, 0xff, combatant_size(character));
    pick_cnt = 0;
    for (step_index = 1; step_index <= step_count; step_index++) {
        character_cursor = (heroes - 1)[(steps - 1)[step_index].who];
        if (combat_target_allowed(character, character_cursor, 0)) {
            pick_cnt++;
            reachable_target_list[pick_cnt] = (steps - 1)[step_index];
            if (character->combat->target == character_cursor)
                cur_pick = pick_cnt;
        }
    }
    target_hero = step_target(0, character);
    if (target_hero != character)
        ui_mode = 4;
}

void far combat_aim(struct Character far *character, char range, char gate, char flag, char mode,
                         char far *result, struct Aim far *aim)  /* 21e4:37dd */
{
    char active;
    char accepted;
    char cur_x;
    char cur_y;
    char org_x;
    char org_y;
    char stop;

    ui_mode = 1;
    next_key = 0;
    icon_facing(0x1e, 0, 0, 0);
    mem_fill(aim, sizeof(struct Aim), 0);
    *result = 0;
    reach = range;
    if (reach == 0xff)
        if (character->equipped[0] != 0)
            reach = item_table[character->equipped[0]->rec.type].range;
        else
            reach = 1;
    if (reach == 0 || reach == 0xff)
        reach = 1;
    org_x = combatant_x(character);
    org_y = combatant_y(character);
    cell_redraw(org_x, org_y);
    build_picks(character);
    cur_x = combatant_x(target_hero);
    cur_y = combatant_y(target_hero);
    active = 1;
    target_step_direction = 0;
    stop = 0;
    while (!stop) {
        target_menu_choice = target_menu(character, target_hero, reach, gate, flag, mode, active, cur_x,
                                         cur_y);
        if (menu_escaped)
            if (target_menu_choice == 0x1b) {
                target_menu_choice = 5;
                menu_escaped = 0;
            } else if (target_menu_choice == 0x80) {
                target_menu_choice = 4;
                menu_escaped = 0;
            }
        if (!menu_escaped)
            switch (target_menu_choice) {
            case 0:
                active = 1;
                target_hero = step_target(1, character);
                cur_x = combatant_x(target_hero);
                cur_y = combatant_y(target_hero);
                break;
            case 1:
                active = 1;
                target_hero = step_target(-1, character);
                cur_x = combatant_x(target_hero);
                cur_y = combatant_y(target_hero);
                break;
            case 2:
                aim_menu(character, target_hero, reach, gate, flag, mode, &active, &accepted, aim,
                         cur_x, cur_y);
                if (accepted) {
                    target_hero = aim->target;
                    cur_x = aim->x;
                    cur_y = aim->y;
                } else
                    active = 1;
                if (active && accepted) {
                    target_menu_choice = 3;
                    stop = 1;
                    ui_mode = 4;
                    if (target_hero != 0)
                        aim->target = target_hero;
                    else
                        aim->target = 0;
                    *result = 1;
                    if (gate == 1)
                        aim_at(character, target_hero, gate, result, aim);
                    if (!*result) {
                        cur_map->cursor_on = 0;
                        combat_view_scroll(cur_x, cur_y, 0, 8);
                    }
                } else if (character != 0 && !accepted) {
                    panel_dirty = 1;
                    hero_panel(character);
                }
                ui_mode = 3;                /* after the 4 set on acceptance, as shipped */
                break;
            case 3:
                aim->x = cur_x;
                aim->y = cur_y;
                if (target_hero != 0)
                    aim->target = target_hero;
                else
                    aim->target = 0;
                *result = 1;
                if (gate == 1)
                    aim_at(character, target_hero, gate, result, aim);
                if (!*result) {
                    cur_map->cursor_on = 0;
                    combat_view_scroll(cur_x, cur_y, 0, 8);
                }
                stop = 1;
                break;
            case 4:
                cur_map->cursor_on = 1;
                cur_map->cursor_size = 1;
                combat_view_scroll(cur_x, cur_y, 0, 8);
                cur_map->cursor_on = 0;
                target_step_direction = 0;
                break;
            case 5:
                stop = 1;
                break;
            }
        else if (target_menu_choice == 0x88 || target_menu_choice == 0x84
                 || target_menu_choice == 0x82 || target_menu_choice == 0x86
                 || target_menu_choice == 0x87 || target_menu_choice == 0x85
                 || target_menu_choice == 0x81 || target_menu_choice == 0x83) {
            next_key = target_menu_choice;
            aim_menu(character, target_hero, reach, gate, flag, mode, &active, &accepted, aim, cur_x,
                     cur_y);
            if (accepted) {
                target_hero = aim->target;
                cur_x = aim->x;
                cur_y = aim->y;
            } else
                active = 1;
            if (active && accepted) {
                target_menu_choice = 3;
                stop = 1;
                ui_mode = 4;
                if (target_hero != 0)
                    aim->target = target_hero;
                else
                    aim->target = 0;
                *result = 1;
                if (gate == 1)
                    aim_at(character, target_hero, gate, result, aim);
                if (!*result) {
                    cur_map->cursor_on = 0;
                    combat_view_scroll(cur_x, cur_y, 0, 8);
                }
            } else
                ui_mode = 3;
        }
        cell_redraw(combatant_x(target_hero), combatant_y(target_hero));
        target_hero = step_target(target_step_direction, character);
        if (target_hero != 0
            && (target_menu_choice == 0 || target_menu_choice == 1 || target_menu_choice == 3)) {
            cur_x = combatant_x(target_hero);
            cur_y = combatant_y(target_hero);
        }
    }
    if (gate)
        box_draw_top();
}

char far combat_target_pick(struct Character far *character, char range, char any, char fresh,
                         char flag)                              /* 21e4:3d9e */
{
    char result;
    unsigned char count;
    unsigned char tries_left;
    unsigned char pick;
    char last;
    char found;
    char again;
    struct Character far *character_cursor;
    struct Effect far *effect;

    effect = 0;
    again = 0;
    found = 0;
    last = 0;
    character_cursor = character->combat->target;
    if (fresh || character_cursor != 0 && (character_cursor->side == character->side || !character_cursor->enabled || !combat_target_allowed(character, character_cursor, flag)))
        character->combat->target = 0;
    if (character->combat->target != 0)
        found = 1;
    while (!found && !last) {
        last = again;
        if (again && !fresh)
            cur_map->dist_mode = 1;
        count = combat_targets_near(character, range, flag);
        if (count > 0)
            pick = fx_roll_dice(1, count);
        tries_left = count;
        while (tries_left > 0 && !found && count > 0) {
            tries_left--;
            if (find_effect(character, 0x8c, &effect)
                && (heroes - 1)[(near_ids - 1)[1]] == taunting_character
                && tries_left == 0x13)
                pick = 1;
            else
                pick = fx_roll_dice(1, count);
            if ((near_ids - 1)[pick] > 0) {
                character_cursor = (heroes - 1)[(near_ids - 1)[pick]];
                if (any && cur_map->dist_mode || combat_target_allowed(character, character_cursor, flag)) {
                    found = 1;
                    character->combat->target = character_cursor;
                } else {
                    (near_ids - 1)[pick] = 0;
                    for (pick = 1; pick <= count; pick++)
                        if ((near_ids - 1)[pick] > 0)
                            found = 1;
                    if (found)
                        found = 0;
                    else
                        count = 0;
                }
            }
            if (!found) {
                pick++;
                if (pick > count)
                    pick = 1;
            }
        }
        if (!again)
            again = 1;
    }
    cur_map->dist_mode = 0;
    result = found;
    return result;
}

char far gods_intervene(void)                                   /* 21e4:3feb */
{
    struct Character far *character;

    if (str_cmp(msg_text(3), debug_password) != 0)
        return 0;
    bottom_msg("The Gods intervene!");
    for (character = node_list; character != 0; character = character->next) {
        if (character->side == 1) {
            character->enabled = 0;
            character->status = 6;
            (combatants - 1)[combatant_slot(character)].size = 0;
        }
        combat_actions_end(character);
    }
    combat_view_scroll(cur_map->ox + 3, cur_map->oy + 3, 0xff, 8);
    return 1;
}

void far cursor_step(int dir)                                    /* 21e4:40a7 */
{
    combat_view_scroll(target_x, target_y, 3, dir);
    target_x += dir_dx[dir];
    target_y += dir_dy[dir];
}

void far view_click(int zone, int ty, int tx)                    /* 21e4:40d6 */
{
    short x;
    short dir;
    register int y;
    register int index;

    y = target_y - cur_map->oy;
    x = target_x - cur_map->ox;
    if (zone == 5 && ty == y && tx == x) {
        click_accept = 1;
        return;
    }
    switch (zone) {
    case 1:
        for (index = x; index >= 0; index--) {
            combat_view_scroll(target_x, target_y, 3, 6);
            target_x += dir_dx[6];
            target_y += dir_dy[6];
        }
        break;
    case 2:
        for (index = y; index >= 0; index--) {
            combat_view_scroll(target_x, target_y, 3, 0);
            target_x += dir_dx[0];
            target_y += dir_dy[0];
        }
        break;
    case 3:
        for (index = x; index <= tx; index++) {
            combat_view_scroll(target_x, target_y, 3, 2);
            target_x += dir_dx[2];
            target_y += dir_dy[2];
        }
        break;
    case 4:
        for (index = y; index <= ty; index++) {
            combat_view_scroll(target_x, target_y, 3, 4);
            target_x += dir_dx[4];
            target_y += dir_dy[4];
        }
        break;
    case 5:
        while (ty != y || tx != x) {
            if (x < tx) {
                x++;
                if (y < ty) {
                    y++;
                    dir = 3;
                } else if (y > ty) {
                    y--;
                    dir = 1;
                } else
                    dir = 2;
            } else if (x > tx) {
                x--;
                if (y < ty) {
                    y++;
                    dir = 5;
                } else if (y > ty) {
                    y--;
                    dir = 7;
                } else
                    dir = 6;
            } else if (y < ty) {
                y++;
                dir = 4;
            } else if (y > ty) {
                y--;
                dir = 0;
            }
            cursor_step(dir);
        }
        break;
    }
    if (target_x < 0)
        target_x = 0;
    if (target_y < 0)
        target_y = 0;
    if (target_x > 0x31)
        target_x = 0x31;
    if (target_y > 0x18)
        target_y = 0x18;
    moved_by_click = 1;
}

static char rank_target(struct Character far *character_cursor, unsigned char spell, int side) /* 21e4:42cc */
{
    char rank;
    struct Effect far *effect;
    unsigned char level;
    unsigned char blocker;

    if (character_cursor->side != side)
        return -40;
    rank = 0;
    blocker = (spell_info - 1)[spell].effect_id;
    switch (spell) {
    case 1:
        if (!find_effect(character_cursor, 1, &effect) && combat_targets_near(character_cursor, 1, 0) == 0)
            rank = 4;
        break;
    case 0x2a:
        if (!find_effect(cur_node, 0x31, &effect))
            rank = 4;
        break;
    case 0x24:
        if ((char)(character_cursor->hp_max > character_cursor->hp) || find_effect(character_cursor, 0xb9, &effect))
            rank = 4;
        break;
    case 3:
    case 0x3a:
    case 0x47:
        if ((char)(character_cursor->hp_max > character_cursor->hp))
            rank = 4;
        break;
    case 0x60:
        if ((char)(character_cursor->race < 7 || character_cursor->type_flags & 2))
            rank = 4;
        break;
    case 0xa:
        if (character_cursor->immunity & 2 && (char)(character_cursor->icon_dim == 1))
            rank = 4;
        break;
    case 0xf:
    case 0x86:
        if (!find_effect(character_cursor, 0x11, &effect))
            rank = 4;
        break;
    case 0x15:
        if ((char)(character_cursor->level_high < 5) && !find_effect(character_cursor, 0x8d, &effect)
            && !find_effect(character_cursor, 0x35, &effect))
            rank = 4;
        break;
    case 0x17:
    case 0x31:
        if ((char)(character_cursor->race < 7) && !find_effect(character_cursor, 0x8e, &effect) && character_cursor->immunity & 2
            && !find_effect(character_cursor, 0x34, &effect))
            rank = 4;
        break;
    case 0x25:
        if (find_effect(character_cursor, 0x21, &effect))
            rank = 4;
        break;
    case 0x2b:
        if (find_effect(character_cursor, 0x24, &effect))
            rank = 4;
        break;
    case 0x2f:
    case 0x4a:
    case 0x73:
    case 0x85:
        if (!find_effect(character_cursor, 0x5d, &effect))
            rank = 4;
        break;
    case 0x33:
        if (!find_effect(character_cursor, 6, &effect))
            rank = 4;
        break;
    case 0x4c:
    case 0x6e:
        if (!(character_cursor->immunity & 8))
            rank = 4;
        break;
    case 0x44:
        if (!(character_cursor->immunity & 0x10) && !find_effect(character_cursor, 0x8f, &effect))
            rank = 4;
        break;
    case 0x5e:
        if (character_cursor->immunity & 2 && !find_effect(character_cursor, 0x8e, &effect))
            rank = 4;
        break;
    case 0x82:
        if (!find_effect(character_cursor, 0x8e, &effect))
            rank = 4;
        break;
    case 0x51:
    case 0x78:
        if (character_cursor->immunity & 2)
            rank = 4;
        break;
    case 0x52:
        if (!find_effect(character_cursor, 0x8a, &effect))
            rank = 4;
        break;
    case 0x54:
        if (!find_effect(character_cursor, 0x5c, &effect))
            rank = 4;
        break;
    case 0x5b:
        if (character_cursor->level_high < 6)
            rank = 4;
        break;
    default:
        if (blocker == 0 || (char)!find_effect(character_cursor, blocker, &effect))
            rank = 4;
    }
    level = (spell_info - 1)[spell].level;
    if (rank > 0) {
        if (level < 5 && find_effect(character_cursor, 0x39, &effect))
            rank = 0;
        if (level < 4 && find_effect(character_cursor, 0x3f, &effect))
            rank = 0;
        if (character_cursor->spell_resist > 0x28)
            if (character_cursor->spell_resist > 0x82)
                rank = 0;
            else if (character_cursor->spell_resist > 0x64)
                rank = 1;
            else if (character_cursor->spell_resist > 0x46)
                rank = 2;
            else
                rank = 3;
    }
    return rank;
}

char far spell_target_rating(struct Character far *character, unsigned char spell) /* 21e4:4789 */
{
    char result;
    char best;
    short side;
    unsigned char step_index;
    unsigned char count;
    unsigned char range;

    result = 0;
    if ((spell_info - 1)[spell].hostile == 0)
        side = character->side;
    else
        side = enemy_team(character);
    if ((spell_info - 1)[spell].targeting == 0)
        best = rank_target(character, spell, side);
    else {
        range = spell_range(spell);
        plan_steps(combatant_x(character), combatant_y(character), range, 0xff, combatant_size(character));
        count = step_count;
        step_index = 1;
        best = 0;
        if (count > 0)
            while (step_index <= count && best <= 0) {
                best = rank_target((heroes - 1)[(steps - 1)[step_index].who], spell, side);
                step_index++;
            }
    }
    result = best;
    return result;
}

char far area_aim(unsigned char spell, struct Aim far *aim)      /* 21e4:48aa */
{
    char result;
    struct Character far *cur;
    char cx;
    char cy;
    short radius;
    short outer;
    short inner;
    short base_x;
    short base_y;
    short best_dx;
    short best_dy;
    short my_x;
    short my_y;
    short min_x;
    short max_x;
    short min_dy;
    short max_dy;
    short score;
    short best;
    unsigned short cost;
    struct Box obox;
    struct Box ibox;
    short side;
    register int dy;
    register int dx;

    if ((spell_info - 1)[spell].hostile == 0)
        side = cur_node->side;
    else
        side = enemy_team(cur_node);
    result = 0;
    if (spell == 0x57 && game_state[STATE_UNDERWATER])
        radius = 3;
    else if (spell == 0x33 && game_state[STATE_UNDERWATER])
        radius = 2;
    else
        radius = (spell_info - 1)[spell].targeting - 8;
    if ((spell == 0x2f || spell == 0x73) && !game_state[STATE_DUNGEON])
        radius--;
    cur = 0;
    outer = spell_range(spell) + radius;
    if (outer > 0x12) {
        outer = 0x12;
        cur = cur_node->combat->target;
    }
    inner = outer - radius;
    if (cur == 0)
        cur = cur_node;
    base_x = combatant_x(cur);
    base_y = combatant_y(cur);
    if (spell == 0x22 || spell == 0x65) {
        obox.x0 = base_x - inner;
        obox.y0 = base_y - inner;
    } else {
        obox.x0 = base_x - outer;
        obox.y0 = base_y - outer;
    }
    ibox.x0 = base_x - inner;
    ibox.y0 = base_y - inner;
    obox.x1 = base_x + outer;
    obox.y1 = base_y + outer;
    ibox.x1 = base_x + inner;
    ibox.y1 = base_y + inner;
    mem_zero(target_scores, 0xab2);
    for (cur = node_list; cur != 0; cur = cur->next) {
        cx = combatant_x(cur);
        cy = combatant_y(cur);
        if (cur->enabled && cx >= obox.x0 && cx <= obox.x1 && cy >= obox.y0 && cy <= obox.y1) {
            score = rank_target(cur, spell, side);
            if (spell == 0x22 || spell == 0x65)
                min_dy = 0;
            else
                min_dy = -radius;
            max_dy = radius;
            if (cy + min_dy < ibox.y0)
                min_dy = ibox.y0 - cy;
            if (cy + max_dy > ibox.y1)
                max_dy = ibox.y1 - cy;
            for (dy = min_dy; dy <= max_dy; dy++) {
                if (spell == 0x22 || spell == 0x65)
                    min_x = cx;
                else
                    min_x = cx - area_widths[dy + 3][radius - 1];
                max_x = cx + area_widths[dy + 3][radius - 1];
                if (ibox.x0 > min_x)
                    min_x = ibox.x0;
                if (ibox.x1 < max_x)
                    max_x = ibox.x1;
                for (dx = min_x; dx <= max_x; dx++)
                    target_scores[dx - base_x + 18][cy + dy - base_y + 18] += score;
            }
        }
    }
    my_x = combatant_x(cur_node);
    my_y = combatant_y(cur_node);
    best = 0;
    for (dx = -inner; dx <= inner; dx++)
        for (dy = -inner; dy <= inner; dy++) {
            score = target_scores[dx + 18][dy + 18];
            if (score > best || score == best && score > 0 && chance(0x19)) {
                cx = my_x;
                cy = my_y;
                cost = spell_range(spell);
                if (base_x + dx >= 0 && base_x + dx <= 0x31 && base_y + dy >= 0
                    && base_y + dy < 0x18
                    && line_clear(base_x + dx, base_y + dy, &cx, &cy, &cost)) {
                    best_dx = dx;
                    best_dy = dy;
                    best = score;
                }
            }
        }
    aim->target = 0;
    if (best > 0) {
        result = 1;
        aim->x = best_dx + base_x;
        aim->y = best_dy + base_y;
    } else {
        result = 0;
        aim->target = 0;
    }
    return result;
}

char far can_lead(struct Character far *character)                            /* 21e4:4d50 */
{
    char result;
    unsigned char bonus;
    unsigned char odds;

    switch (character->knight_order) {                 /* odds stays unset past 1..3, as shipped */
    case 1:
        odds = 0x14;
        break;
    case 2:
        odds = 0x28;
        break;
    case 3:
        odds = 0x50;
        break;
    }
    if (character->stats[5].cur > 0xd)
        bonus = (character->stats[5].cur - 0xd) * 5;
    else
        bonus = 0;
    if (odds + bonus > fx_roll_dice(1, 100))
        result = 1;
    else
        result = 0;
    return result;
}

void far combat_leadership_apply(void)                                   /* 21e4:4dd2 */
{
    struct Character far *leader;
    struct Character far *character;

    for (character = node_list; character != 0; character = character->next)
        if (character->side == 0)
            character->quick_fight = character->npc & 0x80;
        else
            character->quick_fight = 1;
    for (leader = node_list; leader != 0; leader = leader->next)
        if (leader->side == 0 && !(leader->npc & 0x80) && class_level(leader, 1) > 0
            && leader->status == 0)
            for (character = node_list; character != 0; character = character->next)
                if (character->side == 0 && character->npc & 0x80 && character->quick_fight && character->status == 0
                    && character->stats[1].cur > 3 && can_lead(leader))
                    character->quick_fight = 0;
}

void far fly_icon(unsigned char cell, struct Character far *character_cursor, struct Character far *character) /* 21e4:4edb */
{
    icon_facings(cell);
    icon_fly(combatant_x(character), combatant_y(character), combatant_x(character_cursor), combatant_y(character_cursor), 1,
             0x1e);
}

void far sight_line(char fresh, unsigned far *dist, struct Character far *character,
                    char far *blocked)                           /* 21e4:4f32 */
{
    char tx;
    char ty;

    *blocked = 1;
    if (combat_target_pick(character, 0xff, 0, fresh, 0)) {
        tx = combatant_x(character->combat->target);
        ty = combatant_y(character->combat->target);
        if (line_clear(combatant_x(character), combatant_y(character), &tx, &ty, dist))
            *blocked = 0;
    }
}

void far eye_rays(struct Character far *character, struct Effect far *effect, char removed) /* 21e4:4fd1 */
{                                       /* reads neither fx nor removed */
    unsigned char left;
    unsigned short dist;
    unsigned char used;
    char blocked;
    struct Character far *character_cursor;

    used = 0;
    left = 4;
    blocked = 0;
    character->combat->target = 0;
    sight_line(1, &dist, character, &blocked);
    do {
        element_mask = 8;
        character_cursor = character->combat->target;
        dist = get_distance(character, character_cursor);
        left--;
        if (character_cursor != 0)
            if (dist <= 2 && !(used & 1)) {
                used = used | 1;
                hero_text(character, "fires a disintegrate ray", 10, 1);
                fly_icon(0x13, character_cursor, character);
                if (!fx_saving_throw(character_cursor, 3, 0))
                    fx_disable(character_cursor, 8, "is disintegrated");
                sight_line(0, &dist, character, &blocked);
            } else if (dist <= 3 && !(used & 2)) {
                used = used | 2;
                hero_text(character, "fires a flesh to stone ray", 10, 1);
                fly_icon(0x13, character_cursor, character);
                if (!fx_saving_throw(character_cursor, 1, 0))
                    fx_disable(character_cursor, 7, "is Stoned");
                sight_line(0, &dist, character, &blocked);
            } else if (dist <= 4 && !(used & 4)) {
                element_mask = 0x48;
                used = used | 4;
                hero_text(character, "fires a death ray", 10, 1);
                fly_icon(0x13, character_cursor, character);
                if (!fx_saving_throw(character_cursor, 0, 0))
                    fx_disable(character_cursor, 6, "is killed");
                sight_line(0, &dist, character, &blocked);
            } else if (dist <= 5 && !(used & 8)) {
                used = used | 8;
                hero_text(character, "Causes Serious Wounds", 10, 1);
                fly_icon(0x13, character_cursor, character);
                current_spell = 0x42;
                fx_damage(character_cursor, fx_roll_damage(2, 8) + 1, 0, 0);
                sight_line(0, &dist, character, &blocked);
            } else if (!(used & 0x10)) {
                current_spell = 0x54;
                spell_cast(0x54, 1, 1, &blocked);
                used = used | 0x10;
            } else if (!(used & 0x20)) {
                current_spell = 0x37;
                spell_cast(0x37, 1, 1, &blocked);
                used = used | 0x20;
            } else if (!(used & 0x40)) {
                current_spell = 0x15;
                spell_cast(0x15, 1, 1, &blocked);
                used = used | 0x40;
            }
    } while (left > 0 && character->combat->target != 0);
}

char far combat_sweep_attack(struct Character far *character, struct Character far *character_cursor) /* 21e4:52e4 */
{
    char result;
    unsigned char count;
    unsigned char left;
    unsigned char index;
    unsigned char target_slot;
    struct Character far *nearby_target;

    result = 0;
    if (character->attacks_left[0] < character->combat->sweep_targets && character_cursor->level_high == 0 && get_distance(character, character_cursor) == 1) {
        count = enemies_near(character, 1);
        left = 0;
        for (index = 1; index <= count; index++) {
            nearby_target = (heroes - 1)[(near_ids - 1)[index]];
            if (nearby_target == character_cursor)
                target_slot = index;
            if (nearby_target->level_high == 0)
                left++;
        }
        if (character->attacks_left[0] < left) {
            if (character->combat->sweep_targets < left)
                left = character->combat->sweep_targets;
            hero_text(character, "sweeps", 10, 1);
            nearby_target = (heroes - 1)[(near_ids - 1)[1]];
            if (nearby_target != character_cursor) {
                (near_ids - 1)[target_slot] = (near_ids - 1)[1];
                (near_ids - 1)[1] = combatant_slot(character_cursor);
            }
            for (index = 1; index <= count; index++)
                if (left > 0 && (heroes - 1)[(near_ids - 1)[index]]->level_high == 0) {
                    nearby_target = (heroes - 1)[(near_ids - 1)[index]];
                    attack_register(character, nearby_target);
                    character->attacks_left[0] = 1;
                    combat_attack(character, nearby_target, 0, 0L, &combat_action_done);
                    left--;
                }
            result = 1;
        }
    }
    return result;
}

unsigned char far enemies_near(struct Character far *character, unsigned char range) /* 21e4:54b5 */
{
    unsigned char enemy_count;
    unsigned char count;
    unsigned char matching;
    unsigned char step_index;

    plan_steps(combatant_x(character), combatant_y(character), range, 0xff, combatant_size(character));
    count = step_count;
    matching = 0;
    if (count > 0) {
        for (step_index = 1; step_index <= count; step_index++)
            if (enemy_team(character) == (heroes - 1)[(steps - 1)[step_index].who]->side) {
                matching++;
                mem_move5(&(steps - 1)[step_index], &(steps - 1)[matching], 3);
            }
        count = matching;
        step_count = matching;
    }
    enemy_count = count;
    for (step_index = 1; step_index <= count; step_index++)
        (near_ids - 1)[step_index] = (steps - 1)[step_index].who;
    return enemy_count;
}
