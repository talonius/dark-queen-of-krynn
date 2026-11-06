/* DQK.EXE overlay 10 module 2b3a: spell effects.  A handler for each filled
 * spell_fn id (136 of ids 1 to 140) -- spells, potions, wands and other
 * magic items, and three effects the game's spell_names table leaves
 * unnamed -- called through spell_fn[id] once the targets are chosen: it
 * rolls the dice, applies the effect to targets[1..target_count] or to the
 * caster, and reports it with one of the effect messages.  Also the
 * lightning bolt's bouncing path, the meteor swarm's steps, the cloud
 * records of stinking cloud, cloudkill and blade barrier, an item giving up
 * a stored spell, and the "Casts a Spell" announcement.
 *
 * EXE 0x0556c0..0x05a2ec, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 2b3a, the first module of RTLink section 10 (payload
 * 0x556c0, link segment 2b3a, which sections 9 and 11 also use).  Its last
 * retf is byte 0xb of paragraph 2ffc; module 2ffc starts at 2ffc:000c.
 * Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined in
 * shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the
 * shipped call, or the DGROUP offset of an extern variable.  Callee and
 * variable names are placeholders chosen from use; a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it, and an ovl10_2ffc_
 * callee is section 10's next module.  The handlers are named after the
 * game's own spell_names table (DGROUP 25c7); ids 138 to 140 have no name
 * there.
 *
 * spellfx_init (vector stub 1d99:13f5) fills spell_fn, 89 entries itself
 * and 47 through spell_fn_fill, and sets hit_proc, fx_lock, item_cast_active and
 * inventory_redraw.  Module 2ffc makes the calls (0x5c4a3).  Ids 65, 95, 97 and 99
 * are never filled.  Module 2ffc also far-calls item_casts and
 * cast_announce; vector stubs reach item_casts (1d99:0e69), item_spend
 * (0e5f), cast_announce (0e55), spell_remove_curse (14bd) and bolt_run
 * (1535).  Nothing found calls spellfx_unit_init.
 *
 * _DATA is diag_dx, diag_dy and 66 stored literals, DGROUP 8788..8b2d.
 * _BSS is spell_pool and spell_cost, c7e4..c7e5.
 *
 * targets is indexed from 1.  heroes, recs, combatants, steps and
 * tile_info are indexed from 1 too; the shipped code adds the address of
 * element 0 (b2f8, c526, c3f1, c2f3, 6197).
 */

#include "dqk.h"

static int diag_dx[4] = { -1, 1, -1, 1 };                        /* 8788 */
static int diag_dy[4] = { -1, -1, 1, 1 };                        /* 8790 */

static unsigned char spell_pool;                                    /* c7e4 */
static unsigned char spell_cost;                                    /* c7e5 */

/* The translated Pascal unit's empty initialization block (DKK magic.pas). */
void far spellfx_unit_init(void)                                   /* 2b3a:0000 */
{
}

void far side_affect(int side_id, char far *msg)                      /* 2b3a:0001 */
{
    unsigned char target_index;

    mirror_image_bypass = 1;
    for (target_index = 1; target_index <= target_count; target_index++) {
        if (targets[target_index]->side != side_id ||
            (current_spell == 1 && view_mode == 5 && combat_targets_near(targets[target_index], 1, 0) > 0))
            targets[target_index] = 0;
    }
    spell_damage_apply(current_spell, 0, 0, 0, 0, msg);
}

void far spell_bless(void)                                       /* 2b3a:00a2 */
{
    side_affect(cur_node->side, msg_format("is %s", fx_names[1]));
}

void far spell_curse(void)                                       /* 2b3a:00c9 */
{
    side_affect(enemy_team(cur_node), msg_format("is %s", fx_names[2]));
}

void far spell_cure_light(void)                                  /* 2b3a:00f8 */
{
    char bonus;

    if (target_count != 0) {
        if (cur_node->deity == 4)
            bonus = fx_roll_dice(1, 8);
        else
            bonus = 0;
        if (fx_heal(targets[1], fx_roll_dice(1, 8) + bonus, 0))
            heal_msg(targets[1]);
    }
}

void far spell_cause_light(void)                                 /* 2b3a:0162 */
{
    spell_damage_apply(current_spell, 0, 0, fx_roll_damage(1, 8), 8, "");
}

void far fx_affected(void)                                       /* 2b3a:018c */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is affected");
}

void far fx_protected(void)                                      /* 2b3a:01aa */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is protected");
}

void far spell_resist_cold(void)                                 /* 2b3a:01c8 */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is cold-resistant");
}

void far spell_burning_hands(void)                               /* 2b3a:01e6 */
{
    if (game_state[STATE_UNDERWATER])
        bottom_msg("Can't be cast underwater!");
    else
        spell_damage_apply(current_spell, 0, 0, caster_level(current_spell, 0), 9, "");
}

void far spell_charm_person(void)                                /* 2b3a:022a */
{
    long effect_address;
    struct Character far *character;

    character = targets[1];
    if (character != 0) {
        if ((character->race == 7 && !(character->type_flags & 2)) || character->icon_dim > 1) {
            hero_text(character, "is unaffected", 10, 1);
        } else {
            spell_damage_apply(current_spell, caster_level(current_spell, 1) + (cur_node->side << 7),
                            1, 0, 0, "is charmed");
            if (find_effect(character, 0xb, &effect_address))
                fx_dispatch(0xb, character, effect_address, 0);
        }
    }
}

void far spell_enlarge(void)                                     /* 2b3a:02e8 */
{
    char strength_code;
    struct Character far *character;
    long effect_address;

    character = targets[1];
    spell_pool = 0x12;
    spell_cost = 0;
    switch (caster_level(current_spell, 0)) {
    case 1: spell_cost = 0; break;
    case 2: spell_cost = 1; break;
    case 3: spell_cost = 0x33; break;
    case 4: spell_cost = 0x4c; break;
    case 5: spell_cost = 0x5b; break;
    case 6: spell_cost = 0x64; break;
    case 7: spell_pool = 0x13; break;
    case 8: spell_pool = 0x14; break;
    case 9: spell_pool = 0x15; break;
    default: spell_pool = 0x16; break;
    }
    if (fx_strength_upgrade(character, spell_pool, spell_cost, &strength_code) && !find_effect(character, 0x39, &effect_address) &&
        !find_effect(character, 0x3f, &effect_address)) {
        hero_text(character, "is stronger", 10, 1);
        fx_add(character, 0xc, spell_duration(current_spell), strength_code, 1);
        fx_stat_recalc(character, 0);
    } else {
        hero_text(character, "is unaffected", 10, 1);
    }
}

void far spell_reduce(void)                                      /* 2b3a:0438 */
{
    long effect_address;
    struct Character far *character;

    character = targets[1];
    if (character != 0) {
        if (target_count > 0 && !fx_saving_throw(character, 4, 0) && find_effect(character, 0xc, &effect_address)) {
            fx_remove(character, 0xc, 0);
            fx_stat_recalc(character, 0);
            hero_text(character, "has been reduced", 10, 1);
        }
    }
}

void far spell_friends(void)                                     /* 2b3a:04cd */
{
    spell_damage_apply(current_spell, fx_roll_dice(2, 4), 1, 0, 0, "is friendly");
    fx_stat_recalc(cur_node, 5);
}

void far spell_magic_missile(void)                               /* 2b3a:050a */
{
    unsigned char missile_level;

    missile_level = caster_level(current_spell, 0) + 1;
    spell_damage_apply(current_spell, 0, 0, fx_roll_damage(missile_level / 2, 4) + missile_level / 2, 8, "");
}

void far spell_shield(void)                                      /* 2b3a:056a */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is shielded");
}

void far spell_shocking_grasp(void)                              /* 2b3a:0588 */
{
    spell_damage_apply(current_spell, 0, 0, fx_roll_damage(1, 8) + caster_level(current_spell, 0), 0xc, "");
}

void far spell_sleep(void)                                       /* 2b3a:05c4 */
{
    unsigned char target_index;
    long effect_address;

    mirror_image_bypass = 1;
    spell_pool = fx_roll_dice(4, 4);
    for (target_index = 1; target_index <= target_count; target_index++) {
        switch (targets[target_index]->level_high) {
        case 1: spell_cost = 1; break;
        case 2: spell_cost = 2; break;
        case 3: spell_cost = 4; break;
        case 4: spell_cost = 6; break;
        case 5:
            if (targets[target_index]->race == 7) {
                spell_cost = 10;
                break;
            }
        default:
            spell_cost = 0x14;
        }
        if (targets[target_index]->status != 1 && !find_effect(targets[target_index], 0x35, &effect_address) &&
            spell_pool >= spell_cost)
            spell_pool -= spell_cost;
        else
            targets[target_index] = 0;
    }
    spell_damage_apply(current_spell, 0, 0, 0, 0, "falls asleep");
}

void far spell_hold(void)                                        /* 2b3a:06e9 */
{
    char save_modifier;

    switch (target_count) {
    case 1:
        if (current_spell == 0x17)
            save_modifier = -2;
        else
            save_modifier = -3;
        break;
    case 2:
        save_modifier = -1;
        break;
    case 3:
    case 4:
        save_modifier = 0;
        break;
    }                                           /* n is unset for other counts */
    spell_effect_apply(save_modifier, "is held");
}

void far spell_resist_fire(void)                                 /* 2b3a:073c */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is fire resistant");
}

void far spell_silence(void)                                     /* 2b3a:075a */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is silenced");
}

void far spell_slow_poison(void)                                 /* 2b3a:0778 */
{
    long effect_address;
    struct Character far *character;

    character = targets[1];
    if (character->status == 1) {
        targets[1] = 0;
    } else if (find_effect(character, 0x37, &effect_address)) {
        if (character->hp == 0)
            character->hp = 1;
        character->status = 0;
        character->enabled = 1;
        spell_damage_apply(current_spell, -1, 1, 0, 0, "is affected");
        fx_add(character, 0xf, 10, -1, 1);
    }
}

void far spell_snake_charm(void)                                 /* 2b3a:0813 */
{
    struct Character far *character;
    long effect_address;

    spell_pool = cur_node->hp;
    target_count = 0;
    for (character = node_list; character != 0; character = character->next) {
        if ((character->type_flags & 0x10) && character->enabled && !find_effect(character, 0x33, &effect_address) &&
            character->hp <= spell_pool) {
            spell_pool = spell_pool - character->hp;
            target_count++;
            targets[target_count] = character;
        }
    }
    spell_damage_apply(current_spell, 0, 0, 0, 0, msg_format("is %s", fx_names[0xb]));
}

void far spell_spirit_hammer(void)                               /* 2b3a:08e3 */
{
    spell_damage_apply(current_spell, 0, 1, 0, 0, "");
    fx_dispatch(0x17, targets[1], 0, 0);
}

void far spell_invisibility(void)                                /* 2b3a:091b */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is invisible");
}

void far spell_knock(void)                                       /* 2b3a:0939 */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "Knock-Knock");
}

void far spell_mirror_image(void)                                /* 2b3a:0957 */
{
    char image_strength;

    image_strength = fx_roll_dice(1, 4) << 5;
    image_strength = caster_level(current_spell, 1) | image_strength;
    spell_damage_apply(current_spell, image_strength, 0, 0, 0, "is duplicated");
}

void far spell_enfeeble(void)                                    /* 2b3a:09a6 */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is weakened");
}

void far spell_cloud(void)                                       /* 2b3a:09c4 */
{
    short x;
    short y;
    struct Cloud far *cloud;
    unsigned char dir;
    unsigned char count;
    unsigned char tile;
    unsigned char index;
    unsigned char patch_index;
    unsigned char who;
    int seen[9];

    mem_zero(seen, 0x12);
    pool_alloc((void far * far *)&cloud, 0x21, &cloud_pool);
    mem_zero(cloud, 0x21);
    cloud->x = view_cx;
    cloud->y = view_cy;
    cloud->caster = cur_node;
    cloud->level = caster_level(current_spell, 1);
    cloud->strength = cloud->level;
    switch (current_spell) {
    case 0x22:
        count = 4;
        cloud->kind = 0x1d;
        break;
    case 0x5b:
        count = 9;
        cloud->kind = 0x1c;
        break;
    case 0x65:
        count = 4;
        cloud->kind = 0x1b;
        cloud->strength = cloud->strength * 3;
        break;
    }
    for (patch_index = 1; patch_index <= count; patch_index++) {
        dir = patch_dir[patch_index];
        x = cloud->x + dir_dx[dir];
        y = cloud->y + dir_dy[dir];
        if (x >= 0 && x <= 0x31 && y >= 0 && y <= 0x18) {
            cell_get(x, y, &who, &tile);
            for (index = 1; index <= patch_index - 1; index++)
                if (seen[index - 1] == who)
                    who = 0;
            seen[patch_index - 1] = who;
            tile = combat_terrain_under(x, y);
            cloud->on[patch_index] = tile > 0 && (tile_info - 1)[tile].cost < 0xff;
            cloud->tile[patch_index] = tile;
            if (cloud->on[patch_index])
                cur_map->cells[y][x] = cloud->kind;
        } else {
            cloud->on[patch_index] = 0;
        }
    }
    cloud->next = cloud_list;
    cloud_list = cloud;
    combat_view_scroll(view_cx, view_cy, -1, 8);
    for (patch_index = 1; patch_index <= count; patch_index++)
        if (seen[patch_index - 1] > 0)
            fx_terrain_apply((heroes - 1)[seen[patch_index - 1]], 1, 0);
}

void far spell_strength(void)                                    /* 2b3a:0c83 */
{
    struct Character far *character;
    unsigned char pct;
    unsigned char roll;
    unsigned char str;
    char out;

    pct = 0;
    character = targets[1];
    if (character->level[5] > 0)
        roll = fx_roll_dice(1, 4);
    if (character->level[0] > 0 || character->level[6] > 0)
        roll = fx_roll_dice(1, 6);
    if (character->level[2] > 0 || character->level[3] > 0 || character->level[1] > 0 || character->level[4] > 0)
        roll = fx_roll_dice(1, 8);
    str = character->stats[0].cur + roll;                   /* roll unset if no test passed */
    if (str > 0x12) {
        if (character->level[2] > 0 || character->level[3] > 0 || character->level[1] > 0 || character->level[4] > 0) {
            pct = character->str_ex + (str - 0x12) * 10;
            if (pct > 0x64)
                pct = 0x64;
        }
        str = 0x12;
    }
    if (fx_strength_upgrade(character, str, pct, &out)) {
        out = roll + 0x64;
        fx_add(character, 0x26, spell_duration(current_spell), out, 1);
        fx_stat_recalc(character, 0);
    }
}

char far disease_cure(void)                                      /* 2b3a:0dc0 */
{
    char done;

    /* v1.0 as shipped: done is set whether or not 0x22 was cured, fx_lock is
     * cleared only when 0xb9 was, and there is no 0xd7/0xac cure */
    done = 0;
    fx_lock = 1;
    fx_cure(targets[1], 0x22);
    done = 1;
    if (fx_cure(targets[1], 0x2b)) {
        done = 1;
        fx_remove(targets[1], 0x2c, 0);
        fx_remove(targets[1], 0x1f, 0);
        fx_stat_recalc(targets[1], 0);
    }
    if (fx_cure(targets[1], 0xb9))
        fx_lock = 0;
    return done;
}

void far spell_heal(void)                                        /* 2b3a:0e9f */
{
    struct Character far *character;
    unsigned char healing_amount;

    character = targets[1];
    if (fx_cure(character, 0xb9))
        hero_text(character, "is no longer diseased", 10, 0);
    if (disease_cure())
        hero_text(character, "is cured", 10, 0);
    if (fx_cure(character, 0x21))
        hero_text(character, "is no longer blind", 10, 0);
    if (fx_cure(character, 0x44)) {
        hero_text(character, "regains intellect", 10, 0);
        fx_stat_recalc(character, 1);
        fx_stat_recalc(character, 2);
    }
    healing_amount = fx_roll_dice(1, 4);
    if (character->hp + healing_amount > character->hp_max)
        healing_amount = 0;
    else
        healing_amount = character->hp_max - character->hp - healing_amount;
    if (fx_heal(character, healing_amount, 0))
        hero_text(character, "is Healed", 10, 0);
}

void far spell_cure_blindness(void)                              /* 2b3a:0ff6 */
{
    if (fx_cure(targets[1], 0x21))
        hit_flash(targets[1], 1, "can see");
}

void far spell_cause_blindness(void)                             /* 2b3a:1026 */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is blind");
}

void far spell_cure_disease(void)                                /* 2b3a:1044 */
{
    if (disease_cure())
        hero_text(targets[1], "is cured", 10, 0);
}

void far spell_cause_disease(void)                               /* 2b3a:1068 */
{
    long effect_address;

    spell_damage_apply(current_spell, 0, 1, 0, 0, msg_format("is %s", fx_names[0x22]));
    if (find_effect(targets[1], 0x22, &effect_address))
        fx_dispatch(0x22, targets[1], effect_address, 0);
}

char far dispel_roll(unsigned char level, unsigned char far *caster)
                                                                 /* 2b3a:10d9 */
{
    char ok;

    if (*caster > level)
        spell_pool = (*caster - level) * 5 + 0x32;
    else if (*caster < level)
        spell_pool = 0x32 - (spell_cost - *caster) * 2;     /* spell_cost, not level: as shipped */
    else
        spell_pool = 0x32;
    ok = fx_roll_dice(1, 100) <= spell_pool;
    return ok;
}

void far spell_dispel_magic(void)                                /* 2b3a:114e */
{
    unsigned char target_index;
    unsigned char lvl;
    struct Character far *character;
    char any;
    struct Effect far *effect;
    struct Effect far *next;
    char x;
    char y;
    unsigned char tile;
    unsigned char count;
    struct Cloud far *cloud;

    mirror_image_bypass = 1;
    lvl = caster_level(current_spell, 0);
    for (target_index = 1; target_index <= target_count; target_index++) {
        any = 0;
        character = targets[target_index];
        for (effect = character->effects; effect != 0; effect = next) {
            next = effect->next;
            if (effect->strength < 0xff) {
                spell_cost = effect->strength & 0x1f;
                if (lvl > spell_cost)
                    spell_pool = (lvl - spell_cost) * 5 + 0x32;
                else if (lvl < spell_cost)
                    spell_pool = 0x32 - (spell_cost - lvl) * 2;
                else
                    spell_pool = 0x32;
                if (fx_roll_dice(1, 100) <= spell_pool) {
                    fx_remove(character, effect->id, effect);
                    any = 1;
                }
            }
        }
        if (any)
            hit_flash(character, 1, "is affected");
    }
    if (view_mode == 5) {
        for (tick_slot = 0; tick_slot <= 8; tick_slot++) {
            x = dir_dx[tick_slot] + view_cx;
            y = dir_dy[tick_slot] + view_cy;
            cell_get(x, y, &count, &tile);
            if (tile == 0x1d || tile == 0x1c) {
                cloud = cloud_list;
                if (tile == 0x1d)
                    count = 4;
                else
                    count = 9;
                for (; cloud != 0; cloud = cloud->next) {
                    for (target_index = 1; target_index <= count; target_index++)
                        if (cloud->x + dir_dx[patch_dir[target_index]] == x &&
                            cloud->y + dir_dy[patch_dir[target_index]] == y &&
                            dispel_roll(cloud->level, &lvl))
                            cloud->strength = 0xff;
                }
                combat_view_scroll(view_cx, view_cy, 3, 8);
            }
        }
        fx_clouds_tick(1);
    }
}

void far spell_prayer(void)                                      /* 2b3a:13b7 */
{
    spell_damage_apply(current_spell, caster_level(current_spell, 1) + (cur_node->side << 7),
                    0, 0, 0, "is praying");
}

void far spell_remove_curse(void)                                /* 2b3a:13ef */
{
    struct Item far *item;
    char found;
    short target_index;

    if (fx_cure(targets[1], 0x2b)) {
        hit_flash(targets[1], 1, "is un-cursed");
    } else {
        found = 0;
        for (item = targets[1]->items; item != 0 && !found; item = item->next) {
            if (item->rec.cursed && item->rec.equipped) {
                found = 1;
                item->rec.equipped = 0;
                if (item->rec.u.spells[2] & 0x80) {
                    item_handler_active = 1;
                    fx_dispatch(0x7f, targets[1], (long)item, 1);
                    for (target_index = 0; target_index <= 5; target_index++)
                        fx_stat_recalc(targets[1], target_index);
                }
            }
        }
        if (found)
            hit_flash(targets[1], 1, "has an item un-cursed");
    }
}

void far spell_bestow_curse(void)                                /* 2b3a:14dd */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "has been cursed!");
}

void far spell_blink(void)                                       /* 2b3a:14fb */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is blinking");
}

void far spell_fireball(void)                                    /* 2b3a:1519 */
{
    unsigned char dice;
    char bonus;
    unsigned char target_index;

    if (game_state[STATE_UNDERWATER] && current_spell != 0x33) {
        bottom_msg("Can't be cast underwater!");
    } else {
        mirror_image_bypass = 1;
        if (current_spell == 0x85)
            dice = fx_roll_dice(1, 3) * 2 + 1;
        else
            dice = caster_level(current_spell, 0);
        if (game_state[STATE_UNDERWATER]) {
            plan_steps(view_cx, view_cy, 2, -1, 1);
            for (target_index = 1; target_index <= step_count; target_index++)
                targets[target_index] = (heroes - 1)[(steps - 1)[target_index].who];
            target_count = step_count;
        }
        combat_view_scroll(view_cx, view_cy, 0, 8);
        if (current_spell == 0x73)
            bonus = dice;
        else
            bonus = 0;
        spell_damage_apply(current_spell, 0, 0, fx_roll_damage(dice, 6) + bonus, 9, "");
    }
}

void far side_apply(char kind, int side_id, char far *msg)            /* 2b3a:163f */
{
    unsigned char left;
    unsigned char target_index;

    mirror_image_bypass = 1;
    left = caster_level(current_spell, 0);
    for (target_index = 1; target_index <= target_count; target_index++) {
        if (targets[target_index]->side != side_id || left <= 0) {
            targets[target_index] = 0;
        } else {
            /* v1.0 cures at once; v1.1 first offers the effect to group 9 */
            left--;
            if (fx_cure(targets[target_index], kind))
                targets[target_index] = 0;
        }
    }
    spell_damage_apply(current_spell, 0, 0, 0, 0, msg);
    for (target_index = 1; target_index <= target_count; target_index++)
        if (targets[target_index] != 0)
            fx_trigger_group(0x12, &targets[target_index]);
}

void far spell_haste(void)                                       /* 2b3a:175a */
{
    side_apply(0x2a, cur_node->side, "is Hasted");
}

void far bolt_hit(int x, int y, int dmg);

void far bolt_run(unsigned range, int dmg, char bounce)          /* 2b3a:1772 */
{
    unsigned char spent;
    unsigned char who_was;
    unsigned char tile;
    unsigned char who;
    char dir;
    char px;
    char py;
    short prev_x;
    short prev_y;
    short tx;
    short ty;
    struct Line back;
    struct Line line;
    char moved;
    char stopped;
    register int cx;
    register int cy;

    icon_facings(0x14);
    cx = combatant_x(cur_node);
    cy = combatant_y(cur_node);
    tx = view_cx;
    ty = view_cy;
    bolt_hit(tx, ty, dmg);
    cell_get(tx, ty, &who_was, &tile);
    range = range * 2;
    mirror_image_bypass = 1;
    spent = 0;
    dir = 1;
    while (range > 0) {
        mem_zero(&line, 0x18);
        line.x0 = tx;
        line.y0 = ty;
        line.x1 = (tx - cx) * dir * range + tx;
        line.y1 = (ty - cy) * dir * range + ty;
        line_init(&line);
        stopped = 0;
        do {
            px = line.x;
            py = line.y;
            if (line.x0 != line.x1 || line.y0 != line.y1) {
                do {
                    prev_x = line.x;
                    prev_y = line.y;
                    moved = line_step(&line);
                    cell_get(line.x, line.y, &who, &tile);
                } while (moved && (who <= 0 || who == who_was) && tile != 0 &&
                         (tile_info - 1)[tile].cost <= 1 && line.steps < range);
            }
            if (tile == 0)
                range = 0;
            if (line.x < 0 || line.x > 0x31 || line.y < 0 || line.y > 0x18) {
                stopped = 1;
                line.x = prev_x;
                line.y = prev_y;
            }
            icon_fly(px, py, line.x, line.y, 4, 0x32);
            if (who_was != who)
                bolt_hit(line.x, line.y, dmg);
            who_was = who;
            if ((tile_info - 1)[tile].cost > 1 && game_state[STATE_DUNGEON] == 1 && !stopped) {
                tx = line.x;
                ty = line.y;
                dir = -1;
                mem_zero(&back, 0x18);
                back.x0 = tx;
                back.y0 = ty;
                back.x1 = cx;
                back.y1 = cy;
                line_init(&back);
                while (line_step(&back))
                    ;
                if (bounce && back.steps <= 8)
                    line.steps = line.steps * 2;
                bounce = 0;
            }
            spent = line.steps - spent;
            if (spent < 2)
                spent = 2;
            if (spent < range)
                range -= spent;
            else
                range = 0;
            spent = line.steps;
        } while (range != 0 &&
                 ((tile_info - 1)[tile].cost <= 1 || game_state[STATE_DUNGEON] != 1));
    }
    mirror_image_bypass = 0;
}

void far bolt_hit(int x, int y, int dmg)                         /* 2b3a:1a54 */
{
    unsigned char who;
    unsigned char tile;
    struct Character far *character;

    cell_get(x, y, &who, &tile);
    if (who > 0) {
        character = (heroes - 1)[who];
        fx_damage(character, dmg, 2, fx_saving_throw(character, 4, 0));
        icon_facings(0x14);
    }
}

void far spell_lightning_bolt(void)                              /* 2b3a:1ac3 */
{
    short dmg;

    if (game_state[STATE_UNDERWATER]) {
        spell_fireball();
    } else {
        element_mask = 0xc;
        dmg = fx_roll_damage(caster_level(current_spell, 0), 6);
        bolt_run(7, dmg, 1);
    }
}

void far spell_slow(void)                                        /* 2b3a:1b11 */
{
    long effect_address;
    struct Character far *character;

    character = targets[1];
    if (!find_effect(character, 0x96, &effect_address))
        side_apply(0x27, enemy_team(cur_node), "is Slowed");
}

void far spell_harm(void)                                        /* 2b3a:1b5c */
{
    struct Character far *character;

    character = targets[1];
    element_mask = 8;
    fx_add_message(character, 0x22, 0, 0, 0, 0, 0, "");
    hero_text(character, msg_format("is %s", fx_names[0x22]), 10, 1);
    if (character->hp > 4)
        fx_damage(character, character->hp - fx_roll_dice(1, 4), 0, 0);
}

void far potion_speed(void)                                      /* 2b3a:1bf9 */
{
    if (!fx_cure(targets[1], 0x2a))
        spell_damage_apply(current_spell, 0, 0, 0, 0, "is Speedy");
}

void far spell_cure_serious(void)                                /* 2b3a:1c2e */
{
    char bonus;

    if (target_count != 0) {
        if (cur_node->deity == 4)
            bonus = fx_roll_dice(1, 8);
        else
            bonus = 0;
        if (fx_heal(targets[1], fx_roll_dice(2, 8) + bonus + 1, 0))
            heal_msg(targets[1]);
    }
}

void far potion_frost(void)                                      /* 2b3a:1c9a */
{
    char strength_code;

    if (fx_strength_upgrade(targets[1], 0x15, 0, &strength_code))
        hero_text(targets[1], "is stronger", 10, 1);
    fx_add(targets[1], 0x71, fx_roll_dice(1, 4) * 10 + 0x28, strength_code, 1);
    fx_stat_recalc(targets[1], 0);
}

void far javelin_lightning(void)                                 /* 2b3a:1d20 */
{
    element_mask = 0xc;
    bolt_run(3, 0x14, 0);
}

void far wand_paralysis(void)                                    /* 2b3a:1d39 */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is paralyzed");
}

void far potion_healing(void)                                    /* 2b3a:1d57 */
{
    if (fx_heal(targets[1], fx_roll_dice(2, 4) + 2, 0))
        hit_flash(targets[1], 1, "is Healed");
}

void far elixir_youth(void)                                      /* 2b3a:1d97 */
{
    struct Character far *character;

    character = cur_node;
    character->age = character->age - (fx_roll_dice(1, 4) + 1);
    if (character->age < 0x12) {
        character->age = 0x12;
        hero_text(character, "is now 18 years old", 10, 1);
    } else {
        hero_text(character, "becomes younger", 10, 1);
    }
}

void far wand_missiles(void)                                     /* 2b3a:1e05 */
{
    spell_damage_apply(current_spell, 0, 0, fx_roll_damage(2, 4) + 2, 8, "");
}

void far spell_cause_serious(void)                               /* 2b3a:1e31 */
{
    spell_damage_apply(current_spell, 0, 0, fx_roll_damage(2, 8) + 1, 8, "");
}

void far spell_neutralize(void)                                  /* 2b3a:1e5d */
{
    struct Character far *character;
    long effect_address;

    character = targets[1];
    if (character->status == 1) {
        targets[1] = 0;
    } else if (find_effect(character, 0x37, &effect_address)) {
        if (character->hp == 0)
            character->hp = 1;
        fx_lock = 1;
        fx_remove(character, 0x37, 0);
        fx_remove(character, 0x16, 0);
        fx_remove(character, 0xf, 0);
        fx_lock = 0;
        hero_text(character, "is unpoisoned", 10, 1);
        character->enabled = 1;
        character->status = 0;
    } else {
        hero_text(character, "is unaffected", 10, 1);
    }
}

void far spell_poison(void)                                      /* 2b3a:1f41 */
{
    long effect_address;
    struct Character far *target;

    if (cur_node->combat != 0) {
        spell_damage_apply(current_spell, 0, 0, 0, 8, "");
        target = cur_node->combat->target;
        effect_id = 0x40;
        fx_trigger_group(9, &target);
        effect_address = 0;
        if (effect_id == 0x40)
            fx_dispatch(0x40, cur_node, effect_address, 0);
    }
}

void far spell_sticks_snakes(void)                               /* 2b3a:1fcb */
{
    long effect_address;

    if (targets[1]->level_high < 6) {
        spell_damage_apply(current_spell, caster_level(current_spell, 1), 0, 0, 8, "");
        if (find_effect(targets[1], 3, &effect_address))
            fx_dispatch(3, targets[1], effect_address, 0);
    } else if (!is_helpless(targets[1])) {
        hero_text(targets[1], "smashes them flat", 10, 1);
    }
}

void far spell_cure_critical(void)                               /* 2b3a:2073 */
{
    char bonus;

    if (cur_node->deity == 4)
        bonus = fx_roll_dice(1, 8) + 3;
    else
        bonus = 3;
    if (target_count != 0) {
        if (fx_heal(targets[1], fx_roll_dice(3, 8) + bonus, 0))
            heal_msg(targets[1]);
    }
}

void far spell_cause_critical(void)                              /* 2b3a:20df */
{
    spell_damage_apply(current_spell, 0, 0, fx_roll_damage(3, 8) + 3, 8, "");
}

void far spell_dispel_evil(void)                                 /* 2b3a:210b */
{
    fx_add_message(cur_node, 4, spell_duration(0x49), 0, 0, 0, 0, "");
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is affected");
}

void far spell_flame_strike(void)                                /* 2b3a:2157 */
{
    if (game_state[STATE_UNDERWATER]) {
        bottom_msg("Can't be cast underwater!");
    } else {
        mirror_image_bypass = 1;
        spell_damage_apply(current_spell, 0, 0, fx_roll_damage(6, 8), 9, "");
    }
}

void far spell_raise_dead(void)                                  /* 2b3a:219f */
{
    struct Character far *character;

    character = targets[1];
    if ((character->status == 6 || character->status == 1) && character->stats[4].base > 0 && character->race != 0 &&
        character->race != 1) {
        fx_lock = 1;
        fx_remove(character, 0x37, 0);
        fx_lock = 0;
        character->status = 0;
        character->enabled = 1;
        character->stats[4].base--;
        fx_stat_recalc(character, 4);
        character->hp = 1;
        hero_text(character, "is raised", 10, 1);
    }
}

void far spell_slay_living(void)                                 /* 2b3a:223e */
{
    struct Character far *character;

    character = targets[1];
    element_mask = 0x40;
    damage = 0x43;
    fx_trigger_group(9, &character);
    if (damage != 0) {
        if (!fx_saving_throw(character, 4, 0) && !(character->immunity & 8)) {
            fx_disable(character, 6, "is slain");
        } else {
            element_mask = 8;
            fx_damage(character, fx_roll_damage(2, 8) + 1, 0, 0);
        }
    } else {
        hero_text(character, "is unaffected", 10, 1);
    }
}

void far spell_entangle(void)                                    /* 2b3a:22f5 */
{
    struct Character far *character;
    unsigned char target_index;
    char save;

    mirror_image_bypass = 1;
    if (game_state[STATE_DUNGEON] == 0) {
        for (target_index = 1; target_index <= target_count; target_index++) {
            if (targets[target_index] != 0) {
                character = targets[target_index];
                save = fx_saving_throw(character, 4, 0);
                fx_add_message(character, 0x1b, spell_duration(0x4e), 0, 0, 1, save, "is entangled");
            }
        }
    }
    mirror_image_bypass = 0;
}

void far spell_faerie_fire(void)                                 /* 2b3a:2396 */
{
    spell_effect_apply(0, "is highlighted");
}

void far spell_invis_animals(void)                               /* 2b3a:23a7 */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is invisible");
}

void far spell_charm_monsters(void)                              /* 2b3a:23c5 */
{
    unsigned char target_index;
    long effect_address;

    spell_effect_apply(0, "is charmed");
    for (target_index = 1; target_index <= target_count; target_index++)
        if (find_effect(targets[target_index], 0xb, &effect_address))
            fx_dispatch(0xb, targets[target_index], effect_address, 0);
}

void far spell_confusion(void)                                   /* 2b3a:243f */
{
    unsigned char target_index;
    char save;
    struct Character far *character;
    unsigned char maximum_targets;

    mirror_image_bypass = 1;
    maximum_targets = fx_roll_dice(2, 8);
    if (target_count > maximum_targets)
        target_count = maximum_targets;
    for (target_index = 1; target_index <= target_count; target_index++) {
        if (targets[target_index] != 0) {
            character = targets[target_index];
            save = fx_saving_throw(character, 4, 0);
            fx_add_message(character, 0x23, spell_duration(0x52), 0, 0, 1, save, "is confused");
        }
    }
    mirror_image_bypass = 0;
}

void far spell_dimension_door(void)                              /* 2b3a:24f0 */
{
    struct Character far *character;

    character = cur_node;
    tile_update(0, 0, combatant_slot(character));
    hero_place(character, view_cx, view_cy, 0);
    combat_view_scroll(combatant_x(character), combatant_y(character), 0, 8);
    hero_text(character, "teleports", 10, 1);
}

void far spell_fear(void)                                        /* 2b3a:257a */
{
    struct Character far *caster;
    struct Character far *target;
    unsigned char target_index;
    char save;
    long effect_address;

    mirror_image_bypass = 1;
    if (view_mode == 5) {
        caster = cur_node;
        spell_path_targets(combatant_x(caster), combatant_y(caster), view_cx, view_cy, 3, 6);
        for (target_index = 1; target_index <= target_count; target_index++) {
            target = targets[target_index];
            save = fx_saving_throw(target, 4, 0);
            if (save == 0) {
                fx_add_message(target, 0x6f, spell_duration(0x54), 0, 1, 1, save, "flees in terror");
                if (find_effect(target, 0x6f, &effect_address)) {
                    target->combat->panicked = 1;
                    target->quick_fight = 1;
                    if (!(target->npc & 0x80))
                        target->npc = 0xb3;
                    target->combat->target = 0;
                }
            } else {
                hero_text(target, "is unaffected", 10, 1);
            }
        }
        mirror_image_bypass = 0;
    }
}

void far spell_fire_shield(void)                                 /* 2b3a:26b4 */
{
    unsigned char choice;
    char done;

    done = 0;
    do {
        if (view_mode == 5 && cur_node->quick_fight != 0) {
            choice = fx_roll_dice(1, 2) - 1;
        } else {
            hotkey_reset(1);
            choice = menu_run(txt_flame_type, txt_hot_cold, 0, 0);
        }
        switch (choice) {
        case 0:
            if (game_state[STATE_UNDERWATER]) {
                bottom_msg("Can't be cast underwater!");
                return;
            }
            fx_add_message(cur_node, 0x32, spell_duration(0x55), 0, 0, 0, 0, "is Protected");
            fx_add_message(cur_node, 0x70, spell_duration(0x55), 0, 0, 0, 0, "");
            done = 1;
            break;
        case 1:
            fx_add_message(cur_node, 0x36, spell_duration(0x55), 0, 0, 0, 0, "is Protected");
            fx_add_message(cur_node, 0x70, spell_duration(0x55), 0, 0, 0, 0, "");
            done = 1;
            break;
        default:
            done = confirm(txt_abort_spell, 1);
        }
    } while (!done);
}

void far spell_fumble(void)                                      /* 2b3a:27ef */
{
    struct Character far *character;
    long fx2;
    long effect_address;

    character = targets[1];
    if (!find_effect(character, 0x96, &effect_address)) {
        element_mask = 0x40;
        if (!fx_saving_throw(character, 4, 0)) {
            fx_add_message(character, 0x1b, spell_duration(0x56), 0, 0, 0, 0, "is clumsy");
            if (find_effect(character, 0x1b, &fx2))
                fx_dispatch(0x1b, character, 0, 0);
        } else {
            fx_add_message(character, 0x2a, spell_duration(0x56), 0, 0, 0, 0, "is slowed");
            if (find_effect(character, 0x2a, &fx2))
                fx_dispatch(0x2a, character, 0, 0);
        }
    }
}

void far spell_ice_storm(void)                                   /* 2b3a:28fb */
{
    mirror_image_bypass = 1;
    if (game_state[STATE_UNDERWATER])
        spell_damage_apply(current_spell, 0, 0, fx_roll_damage(1, 10), 8, "");
    else
        spell_damage_apply(current_spell, 0, 0, fx_roll_damage(3, 10), 8, "");
}

void far spell_minor_globe(void)                                 /* 2b3a:2960 */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is protected");
}

void far spell_barkskin(void)                                    /* 2b3a:297e */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is toughened");
}

void far spell_cone_of_cold(void)                                /* 2b3a:299c */
{
    struct Character far *character;
    unsigned char cone_radius;
    unsigned char dice;

    mirror_image_bypass = 1;
    character = cur_node;
    dice = caster_level(current_spell, 0);
    cone_radius = (caster_level(current_spell, 0) + 1) / 2;
    if (cone_radius < 1)
        cone_radius = 1;
    spell_path_targets(combatant_x(character), combatant_y(character), view_cx, view_cy, 2, cone_radius);
    spell_damage_apply(current_spell, 0, 0, fx_roll_damage(dice, 4) + dice, 10, "");
}

void far spell_feeblemind(void)                                  /* 2b3a:2a48 */
{
    unsigned char old;
    unsigned char spell_slot;
    struct Character far *character;
    long effect_address;

    character = targets[1];
    old = character->saves[4];
    switch (character->cls) {
    case 0:
        character->saves[4]--;
        break;
    case 5:
        character->saves[4] += 4;
        break;
    default:
        character->saves[4] += 2;
    }
    spell_damage_apply(current_spell, 0, 0, 0, 0, "");
    if (find_effect(character, 0x44, &effect_address)) {
        for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
            if (character->spells[spell_slot] & 0x80)
                character->spells[spell_slot] = 0;
        fx_dispatch(0x44, character, 0, 0);
    }
    character->saves[4] = old;
}

void far scroll_protection(void)                                 /* 2b3a:2b1c */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "");
}

void far spell_charm_mammal(void)                                /* 2b3a:2b3a */
{
    long effect_address;
    struct Character far *character;

    character = targets[1];
    if (character != 0) {
        if (character->race < 7 || (character->type_flags & 2)) {
            spell_damage_apply(current_spell, caster_level(current_spell, 1) + (cur_node->side << 7),
                            1, 0, 0, msg_format("is %s", fx_names[0xb]));
            if (find_effect(character, 0xb, &effect_address))
                fx_dispatch(0xb, character, effect_address, 0);
        } else {
            hero_text(character, msg_format("is not %s", fx_names[0xb]), 10, 1);
        }
    }
}

void far potion_invisibility(void)                               /* 2b3a:2c14 */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "");
}

void far spell_cure_light_98(void)                               /* 2b3a:2c32 */
{
    if (target_count != 0) {
        if (fx_heal(targets[1], fx_roll_dice(1, 8), 0))
            heal_msg(targets[1]);
    }
}

void far potion_extra_healing(void)                              /* 2b3a:2c6e */
{
    if (fx_heal(targets[1], fx_roll_dice(2, 4) + 2, 0))
        hit_flash(targets[1], 1, "is Healed");
}

void far spell_bestow_curse_100(void)                            /* 2b3a:2cae */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "");
}

void far spell_restoration(void)                                 /* 2b3a:2ccc */
{
    short best;
    short class_index;
    long best_xp;
    struct Character far *character;
    unsigned char diff;
    unsigned char gain;
    unsigned char total;
    char cut;
    unsigned char count;

    total = 0;
    character = targets[1];
    best = 0x11;
    best_xp = 0x7fffffffL;
    count = 0;
    for (class_index = 0; class_index <= 6; class_index++)
        if (character->level_max[class_index] > 0)
            count++;
    for (class_index = 0; class_index <= 6; class_index++) {
        if (character->level[class_index] < character->level_max[class_index]) {
            diff = character->level_max[class_index] - character->level[class_index];
            total += diff;
            if (level_xp_threshold(character, character->level[class_index] + 1, class_index) <= best_xp) {
                best_xp = level_xp_threshold(character, character->level[class_index] + 1, class_index);
                best = class_index;
            }
        }
    }
    if (best != 0x11) {
        if (character->hp_max < character->old_hp_max) {
            gain = (character->old_hp_max - character->hp_max) / total;
            if (character->level[best] + 1 < hit_dice_level_limit[best])
                cut = con_hp_adj(character, best) / count;
            else
                cut = 0;
            character->hp += gain;
            character->hp_base += gain - cut;
            character->hp_max += gain;
        }
        character->level[best]++;
        recalc_from_levels(character);
        if (total == 1 && character->xp < character->old_xp)
            character->xp = character->old_xp;
        else
            character->xp = best_xp;
    }
    character->level_up = can_level_up(character);
}

void far spell_energy_drain(void)                                /* 2b3a:2ebc */
{
    cur_node->combat->target = targets[1];
    fx_dispatch(0x65, cur_node, 0, 0);
}

void far spell_destruction(void)                                 /* 2b3a:2eee */
{
    struct Character far *character;

    character = targets[1];
    element_mask = 0x48;
    damage = 0xff;
    fx_trigger_group(9, &character);
    if (!(character->immunity & 8) && damage != 0)
        fx_disable(character, 8, "is destroyed");
    else
        hero_text(character, "is unaffected", 10, 1);
}

void far spell_resurrection(void)                                /* 2b3a:2f60 */
{
    struct Character far *character;

    character = targets[1];
    if (character->status == 6 && character->stats[4].base > 0 && character->race != 0 && character->race != 1) {
        fx_remove(character, 0x37, 0);
        character->status = 0;
        character->enabled = 1;
        character->hp = character->hp_max;
        hero_text(character, "is resurrected", 10, 1);
    }
}

void far spell_hold_animal(void)                                 /* 2b3a:2fd4 */
{
    char save_modifier;

    switch (target_count) {
    case 1: save_modifier = -4; break;
    case 2: save_modifier = -2; break;
    case 3: save_modifier = -1; break;
    default: save_modifier = 0;
    }
    spell_effect_apply(save_modifier, "is held");
}

void far spell_prot_fire(void)                                   /* 2b3a:301a */
{
    struct Character far *character;

    character = targets[1];
    if (character == 0)
        return;
    if (class_level(character, 4) > 0)
        fx_add_message(character, 0x77, 0, caster_level(0x6d, 0), 0, 0, 0, "is Protected");
    else
        fx_add_message(character, 0x14, spell_duration(0x6d), class_level(character, 4), 0, 0, 0,
                     "is Protected");
}

void far spell_death(void)                                       /* 2b3a:30ba */
{
    struct Character far *character;
    unsigned char target_index;

    mirror_image_bypass = 1;
    element_mask = 0x40;
    spell_pool = fx_roll_dice(4, 0x14) * 2;
    for (target_index = 1; target_index <= target_count; target_index++) {
        character = targets[target_index];
        switch (character->level_high) {
        case 0: spell_cost = 1; break;
        case 1: spell_cost = 2; break;
        case 2:
        case 3: spell_cost = 8; break;
        case 6:
            if (character->hp_max > 0x33) {
                spell_cost = 0x14;
                break;
            }
        case 4:
        case 5: spell_cost = 0x10; break;
        case 7:
        case 8: spell_cost = 0x14; break;
        default: spell_cost = 0xa2;
        }
        damage = 0x64;
        fx_trigger_group(9, &character);
        if (!(character->immunity & 8) && spell_pool >= spell_cost && damage != 0) {
            spell_pool -= spell_cost;
            fx_disable(character, 6, "drops dead");
        } else {
            targets[target_index] = 0;
        }
    }
}

void far spell_disintegrate(void)                                /* 2b3a:31cc */
{
    char save;
    struct Character far *character;

    element_mask = 0x48;
    character = targets[1];
    damage = 0xff;
    fx_trigger_group(9, &character);
    if (damage != 0) {
        save = fx_saving_throw(character, 4, 0);
        if (save == 0) {
            fx_disable(character, 6, "disintegrates");
            while (character->items != 0)
                item_remove(character, character->items);
            return;
        }
    }
    hero_text(character, "is unaffected", 10, 1);
}

void far spell_globe(void)                                       /* 2b3a:3279 */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is protected");
}

void far spell_stone_to_flesh(void)                              /* 2b3a:3297 */
{
    struct Character far *character;
    unsigned char record_index;
    unsigned char scan_index;

    character = targets[1];
    if (character->status == 7) {
        if (has_ready_8c(character)) {
            hero_text(0, "spell dosn't work!", 10, 0);
        } else {
            character->status = 0;
            character->enabled = 1;
            character->hp = 1;
            hero_text(character, "is no longer a statue", 10, 0);
            if (view_mode == 5) {
                for (record_index = 1; (recs - 1)[record_index].hero != character; record_index++)
                    ;
                cur_map->cells[(recs - 1)[record_index].y][(recs - 1)[record_index].x] = (recs - 1)[record_index].terrain;
                for (scan_index = record_index; scan_index <= rec_count - 1; scan_index++)
                    (recs - 1)[scan_index] = (recs - 1)[scan_index + 1];
                rec_count--;
                if (hero_place(character, combatant_x(character), combatant_y(character), 1))
                    combat_view_scroll(view_cx, view_cy, 0, 8);
            }
        }
    }
    spell_damage_apply(current_spell, 0, 0, 0, 0, "");
}

void far spell_flesh_to_stone(void)                              /* 2b3a:344d */
{
    char save;
    struct Character far *character;

    element_mask = 0x40;
    character = targets[1];
    damage = 0xff;
    fx_trigger_group(9, &character);
    if (damage != 0) {
        save = fx_saving_throw(character, 4, 0);
        if (save == 0)
            fx_disable(character, 7, "is turned to stone");
    }
}

void far spell_mass_invis(void)                                  /* 2b3a:34b9 */
{
    unsigned char target_index;

    mirror_image_bypass = 1;
    for (target_index = 1; target_index <= target_count; target_index++) {
        if (targets[target_index]->side != cur_node->side ||
            (view_mode == 5 && combat_targets_near(targets[target_index], 1, 0) > 0))
            targets[target_index] = 0;
    }
    spell_damage_apply(current_spell, 0, 0, 0, 0, msg_format("is %s", fx_names[0x19]));
}

void far spell_word_stun(void)                                   /* 2b3a:3569 */
{
    unsigned char stun_duration;
    struct Character far *character;

    mirror_image_bypass = 1;
    character = targets[1];
    switch (character->hp) {
        case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
        case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
        case 18: case 19: case 20: case 21: case 22: case 23: case 24: case 25:
        case 26: case 27: case 28: case 29: case 30:
        stun_duration = fx_roll_dice(4, 4);
        break;
        case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38:
        case 39: case 40: case 41: case 42: case 43: case 44: case 45: case 46:
        case 47: case 48: case 49: case 50: case 51: case 52: case 53: case 54:
        case 55: case 56: case 57: case 58: case 59: case 60:
        stun_duration = fx_roll_dice(2, 4);
        break;
        case 61: case 62: case 63: case 64: case 65: case 66: case 67: case 68:
        case 69: case 70: case 71: case 72: case 73: case 74: case 75: case 76:
        case 77: case 78: case 79: case 80: case 81: case 82: case 83: case 84:
        case 85: case 86: case 87: case 88: case 89: case 90:
        stun_duration = fx_roll_dice(1, 4);
        break;
    default:
        stun_duration = 0;
    }
    if (stun_duration != 0)
        fx_add_message(character, (spell_info - 1)[0x75].effect_id, stun_duration, caster_level(0x75, 1), 0,
                     (spell_info - 1)[0x75].save_effect, 0,
                     msg_format("is %s", fx_names[0x6a]));
}

void far spell_fire_touch(void)                                  /* 2b3a:36d6 */
{
    if (game_state[STATE_UNDERWATER])
        bottom_msg("Can't be cast underwater!");
    else
        spell_damage_apply(current_spell, 0, 0, 0, 0, "is covered in flame");
}

void far spell_iron_skin(void)                                   /* 2b3a:370d */
{
    spell_damage_apply(current_spell, 0, 0, 0, 0, "is protected");
}

void far spell_mass_charm(void)                                  /* 2b3a:372b */
{
    long effect_address;
    struct Character far *character;
    char done;
    unsigned char target_index;
    unsigned char lvl;
    unsigned char used;
    unsigned char cap;

    mirror_image_bypass = 1;
    done = 0;
    used = 0;
    target_index = 1;
    cap = caster_level(current_spell, 0) * 2;
    for (; target_index <= target_count && !done; target_index++) {
        character = targets[target_index];
        if (character == 0)
            continue;
        if (!(character->immunity & 2) || character->side == cur_node->side) {
            hero_text(character, "is unaffected", 10, 1);
            continue;
        }
        lvl = character->level_high;
        if (lvl + used > cap)
            continue;
        used += lvl;
        done = used == cap;
        fx_add_message(character, 0xb, spell_duration(current_spell),
                     caster_level(current_spell, 1) + (cur_node->side << 7), 1, 1,
                     fx_saving_throw(character, 4, -2), "is charmed");
        if (find_effect(character, 0xb, &effect_address))
            fx_dispatch(0xb, character, effect_address, 0);
    }
}

void far spell_dance(void)                                       /* 2b3a:388c */
{
    struct Character far *character;
    long effect_address;

    character = targets[1];
    if (!find_effect(character, 0x96, &effect_address)) {
        recalc_stats(character);
        fx_trigger_group(0xb, &character);
        if (fx_attack_roll(cur_node, character, character->ac[0])) {
            fx_add_message(character, 0x1b, spell_duration(0x79), caster_level(0x79, 1), 0, 0, 0,
                         empty_text);
            fx_add_message(character, 0x76, spell_duration(0x79), caster_level(0x79, 1), 0, 0, 0,
                         "begins to dance wildly");
        }
    }
}

void far spell_word_blind(void)                                  /* 2b3a:396a */
{
    struct Character far *character;
    unsigned char unused_blind_duration;
    unsigned char target_index;
    unsigned char sum;

    mirror_image_bypass = 1;
    for (target_index = 1, sum = 0; target_index <= target_count && sum <= 0x64; target_index++) {
        character = targets[target_index];
        if (character->hp_max <= 0x64 && character->hp_max + sum <= 0x64)
            sum += character->hp_max;
    }
    unused_blind_duration = fx_roll_dice(1, 4) + 1;                 /* never read: as shipped */
    if (sum < 0x33)
        unused_blind_duration = unused_blind_duration * 10;
    for (target_index = 1, sum = 0; target_index <= target_count && sum <= 0x64; target_index++) {
        character = targets[target_index];
        if (character->hp_max <= 0x64 && character->hp_max + sum <= 0x64) {
            sum += character->hp_max;
            fx_add_message(character, 0x21, spell_duration(current_spell), 0, 0, 0, 0, "is blinded");
        }
    }
}

char far swarm_step(unsigned char mode, unsigned char dir, char far *cy, char far *cx,
                 char far *y0, char far *y1, char far *x0, char far *x1, unsigned far *power)
                                                                 /* 2b3a:3a73 */
{
    char result;
    char blocked;

    blocked = 0;
    *power = 0xff;
    switch (mode) {
    case 1:
        *x1 = diag_dx[dir - 1] + *x0;
        *y1 = diag_dy[dir - 1] + *y0;
        break;
    case 3:
        *x1 = diag_dx[dir - 1] + *x0;
        *y1 = *y0;
        break;
    case 2:
    case 4:
        *x1 = *x0;
    }
    if (*x1 < 0 || *x1 > 0x31 || *y1 < 0 || *y1 > 0x18 ||
        (meteor_explosion_active != 0 && *x1 == *x0 && *y1 == *y0))
        blocked = 1;
    if (blocked)
        result = 0;
    else
        result = line_clear(*cx, *cy, x1, y1, power);
    *power = *power / 2;
    return result;
}

void far spell_meteor_swarm(void)                                /* 2b3a:3b99 */
{
    struct Character far *character;
    char tx;
    char ty;
    char cx;
    char cy;
    char x;
    char y;
    unsigned char who;
    unsigned char before;
    unsigned char meteor_index;
    unsigned char target_index;
    unsigned power;
    char path_found;

    if (game_state[STATE_UNDERWATER]) {
        bottom_msg("Can't be cast underwater!");
        return;
    }
    mirror_image_bypass = 1;
    element_mask = 9;
    character = cur_node;
    cx = combatant_x(character);
    cy = combatant_y(character);
    tx = view_cx;
    ty = view_cy;
    for (meteor_index = 1; meteor_index <= 4; meteor_index++) {
        mem_zero(targets, 4);               /* targets[0] only: as shipped */
        target_index = 1;
        do {
            path_found = swarm_step(target_index, meteor_index, &cy, &cx, &ty, &y, &tx, &x, &power);
            target_index++;
        } while (target_index <= 4 && path_found == 0);
        for (who = 0; !dir_ok(cx, cy, x, y, who); who++)
            ;
        move_flag = 1;
        combatant_facing_set(character, who, 1, 0);
        icon_facings(0x20);
        missile_facing(who, 0x20);
        icon_fly(cx, cy, x, y, 1, 0x1e);
        view_cx = x;
        view_cy = y;
        icon_scatter(1, 0x19, 0xc);
        spell_path_targets(cx, cy, x, y, 1, power);
        before = target_count;
        plan_steps(x, y, 1, -1, 1);
        for (target_index = 1; target_index <= step_count; target_index++) {
            target_count++;
            targets[target_count] = (heroes - 1)[(steps - 1)[target_index].who];
        }
        for (target_index = 1; target_index <= target_count; target_index++) {
            if (targets[target_index] == 0)
                continue;
            if (targets[target_index]->enabled == 0)
                continue;
            if (target_index <= before)
                fx_damage(targets[target_index], fx_roll_damage(10, 4), 0, 0);
            else
                fx_damage(targets[target_index], fx_roll_damage(10, 4), 2, fx_saving_throw(targets[target_index], 4, 0));
        }
        if (combatant_visible(character, 0)) {
            combatant_facing_set(character, character->combat->direction, 1, 1);
            combatant_facing_set(character, character->combat->direction, 0, 0);
        }
    }
}

void far spell_word_kill(void)                                   /* 2b3a:3ea6 */
{
    unsigned char target_index;

    mirror_image_bypass = 1;
    if (target_count > 0) {
        if (targets[1] != 0 && targets[1]->hp_max <= 0x3c && targets[1]->hp_max > 10) {
            damage = 0xff;
            fx_trigger_group(9, &targets[1]);
            if (damage != 0)
                fx_disable(targets[1], 6, "is slain");
        } else {
            for (target_index = 1; target_index <= target_count; target_index++) {
                if (targets[target_index] != 0 && targets[target_index]->hp_max <= 10) {
                    damage = 0xff;
                    fx_trigger_group(9, &targets[target_index]);
                    if (damage != 0)
                        fx_disable(targets[target_index], 6, "is slain");
                }
            }
        }
    }
}

void far spell_summoning(void)                                   /* 2b3a:3fa4 */
{
    char cx;
    char cy;
    char x;
    char y;
    char dir;
    struct Character far *summoning_anchor;
    struct Character far *summoned_character;
    char done;
    char revived;
    unsigned char placed;
    unsigned char tile;
    unsigned char who;
    unsigned char slot;
    unsigned char want;

    summoning_anchor = summoned_character = node_list;
    done = 0;
    dir = 0;
    placed = 0;
    if (game_state[STATE_DUNGEON])
        want = 0x17;
    else
        want = 0x45;
    while (summoned_character != 0 && summoned_character->status != 9)
        summoned_character = summoned_character->next;
    do {
        if (dir > 7) {
            if (summoning_anchor == 0 || summoning_anchor->next == 0 || summoning_anchor->status == 9) {
                summoning_anchor = cur_node;
                done = 1;
            } else {
                summoning_anchor = summoning_anchor->next;
            }
            dir = 0;
        }
        if (summoned_character == 0 || placed == 2) {
            done = 1;
        } else {
            summoned_character->side = cur_node->side;
            cx = combatant_x(summoning_anchor);
            cy = combatant_y(summoning_anchor);
            revived = 0;
            do {
                x = dir_dx[dir] + cx;
                y = dir_dy[dir] + cy;
                if (x <= 0x31 && x >= 0 && y <= 0x18 && y >= 0) {  /* off map: loops forever, as shipped */
                    cell_get(x, y, &who, &tile);
                    if (who == 0 && tile == want) {
                        slot = combatant_slot(summoned_character);
                        (combatants - 1)[slot].size = summoned_character->icon_dim & 7;
                        (combatants - 1)[slot].x = x;
                        (combatants - 1)[slot].y = y;
                        revived = fx_revive(summoned_character, summoned_character->hp_max, "gates in");
                        if (revived) {
                            placed++;
                            summoned_character = summoned_character->next;
                            if (summoned_character == 0 || summoned_character->status != 9)
                                done = 1;
                        }
                    }
                    dir++;
                }
            } while (!revived && dir <= 7);
        }
    } while (!done);
    if (placed == 0)
        hero_text(cur_node, "summons is not answered", 10, 1);
}

void far power_strike(void)                                      /* 2b3a:41e7 */
{
    if (cur_node->combat->target != 0) {
        damage = fx_roll_dice(1, 6) + str_damage(cur_node) + 9;
        fx_damage(cur_node->combat->target, fx_roll_damage(1, 6) + 9, 0, 0);
    }
}

void far power_stone_gaze(void)                                  /* 2b3a:4250 */
{
    struct Character far *character;
    long effect_address;

    character = cur_node;
    fx_target = targets[1];
    if (find_effect(fx_target, 0x48, &effect_address)) {
        hero_text(fx_target, "reflects it!", 12, 0);
        icon_fly(combatant_x(fx_target), combatant_y(fx_target), combatant_x(character),
                     combatant_y(character), 4, 0x2d);
        fx_target = character;
    }
    if (!fx_saving_throw(fx_target, 1, 0) && !find_effect(fx_target, 0x7e, &effect_address) &&
        !find_effect(fx_target, 0x4f, &effect_address))
        fx_disable(fx_target, 7, "is turned to stone");
}

void far power_stone_self(void)                                  /* 2b3a:4369 */
{
    if (view_mode == 5)
        hero_text(cur_node, "is turned to stone", 10, 0);
    switch (cur_node->status) {
    case 6:
    case 7:
    case 8:
        break;
    default:
        cur_node->status = 7;
        cur_node->enabled = 0;
        cur_node->hp = 0;
        fx_hp_effects_remove(cur_node);
        if (cur_node->enabled == 0 && cur_node->status != 10)
            hero_fall(cur_node);
        text_pause();
        if (view_mode == 5)
            combat_text_clear();
    }
}

char far item_casts(struct Item far *item)                         /* 2b3a:43f7 */
{
    char result;

    if (item != 0 && (item->rec.type == 0x27 || item->rec.type == 0x28 || item->rec.type == 0x49))
        result = 1;
    else
        result = 0;
    return result;
}

void far item_spend(struct Character far *character, struct Item far *item, unsigned char spell)
                                                                 /* 2b3a:442e */
{
    unsigned char want;
    unsigned char index;
    unsigned char slot;
    struct Item far *prev;
    struct Item far *first;
    char found;

    want = spell;
    slot = 0;
    if (item->rec.type == 0x49) {
        first = item;
        found = 0;
        while (item->more != 0 && !found) {
            prev = item;
            item = item->more;
            for (index = 1; index <= 3; index++) {
                if (((item->rec.u.spells - 1)[index] & 0x7f) == want) {
                    slot = index;
                    found = 1;
                }
            }
        }
        if (found) {
            (item->rec.u.spells - 1)[slot] = 0;
            item->rec.name_ids[0]--;
            if (item->rec.name_ids[0] < 0x64) {
                prev->more = item->more;
                first->rec.quantity--;
                if (first->rec.quantity == 1) {
                    prev = first->next;
                    mem_move(first->more, first, 0x3d);
                    first->more = 0;
                    first->next = prev;
                }
                if (item != 0)
                    pool_free(item, 0x3d, &item_record_pool);
            }
        }
    } else {
        for (index = 1; index <= 3; index++)
            if (((item->rec.u.spells - 1)[index] & 0x7f) == want)
                slot = index;
        if (slot != 0) {
            (item->rec.u.spells - 1)[slot] = 0;
            item->rec.name_ids[0]--;
            if (item->rec.name_ids[0] < 0x64)
                item_remove(character, item);
        }
    }
}

void far cast_announce(struct Character far *character, char far *text, unsigned char spell)
                                                                 /* 2b3a:45d7 */
{
    if (view_mode == 5) {
        hero_text(character, "Casts a Spell", 10, 1);
        box_draw_top();
        status_printf(0, 0x17, 7, 0, msg_format("Spell:%s", (spell_names - 1)[spell]));
    } else {
        text_clear(1, 0x12, 0x26, 0x16);
        character_name_show(character, 1, 0x13, 0);
        status_printf(str_length(character->name) + 2, 0x13, 7, 0, text);
        status_printf(1, 0x14, 7, 0, (spell_names - 1)[spell]);
        msg_pause();
        combat_text_clear();
    }
}

void far spell_fn_fill(void)                                     /* 2b3a:46bb */
{
    spell_fn[1] = spell_bless;
    spell_fn[2] = spell_curse;
    spell_fn[3] = spell_cure_light;
    spell_fn[4] = spell_cause_light;
    spell_fn[5] = fx_affected;
    spell_fn[6] = fx_protected;
    spell_fn[7] = fx_protected;
    spell_fn[8] = spell_resist_cold;
    spell_fn[9] = spell_burning_hands;
    spell_fn[10] = spell_charm_person;
    spell_fn[11] = fx_affected;
    spell_fn[12] = spell_enlarge;
    spell_fn[13] = spell_reduce;
    spell_fn[14] = spell_friends;
    spell_fn[15] = spell_magic_missile;
    spell_fn[16] = fx_protected;
    spell_fn[17] = fx_protected;
    spell_fn[18] = fx_affected;
    spell_fn[19] = spell_shield;
    spell_fn[20] = spell_shocking_grasp;
    spell_fn[21] = spell_sleep;
    spell_fn[22] = fx_affected;
    spell_fn[23] = spell_hold;
    spell_fn[24] = spell_resist_fire;
    spell_fn[25] = spell_silence;
    spell_fn[26] = spell_slow_poison;
    spell_fn[27] = spell_snake_charm;
    spell_fn[28] = spell_spirit_hammer;
    spell_fn[29] = fx_affected;
    spell_fn[30] = spell_invisibility;
    spell_fn[31] = spell_knock;
    spell_fn[32] = spell_mirror_image;
    spell_fn[33] = spell_enfeeble;
    spell_fn[34] = spell_cloud;
    spell_fn[35] = spell_strength;
    spell_fn[36] = spell_heal;
    spell_fn[37] = spell_cure_blindness;
    spell_fn[38] = spell_cause_blindness;
    spell_fn[39] = spell_cure_disease;
    spell_fn[40] = spell_cause_disease;
    spell_fn[41] = spell_dispel_magic;
    spell_fn[42] = spell_prayer;
    spell_fn[43] = spell_remove_curse;
    spell_fn[44] = spell_bestow_curse;
    spell_fn[138] = power_strike;
    spell_fn[139] = power_stone_gaze;
    spell_fn[140] = power_stone_self;
}

void far spellfx_init(void)                                      /* 2b3a:4892 */
{
    hit_proc = camp_spell_targets;
    fx_lock = 0;
    item_cast_active = 0;
    inventory_redraw = 1;
    spell_fn[45] = spell_blink;
    spell_fn[46] = spell_dispel_magic;
    spell_fn[47] = spell_fireball;
    spell_fn[48] = spell_haste;
    spell_fn[49] = spell_hold;
    spell_fn[50] = spell_invisibility;
    spell_fn[51] = spell_lightning_bolt;
    spell_fn[52] = fx_protected;
    spell_fn[53] = fx_protected;
    spell_fn[54] = fx_protected;
    spell_fn[55] = spell_slow;
    spell_fn[56] = spell_harm;
    spell_fn[127] = potion_speed;
    spell_fn[58] = spell_cure_serious;
    spell_fn[128] = potion_frost;
    spell_fn[129] = javelin_lightning;
    spell_fn[130] = wand_paralysis;
    spell_fn[131] = potion_healing;
    spell_fn[132] = elixir_youth;
    spell_fn[133] = spell_fireball;
    spell_fn[134] = wand_missiles;
    spell_fn[66] = spell_cause_serious;
    spell_fn[67] = spell_neutralize;
    spell_fn[68] = spell_poison;
    spell_fn[69] = fx_protected;
    spell_fn[70] = spell_sticks_snakes;
    spell_fn[71] = spell_cure_critical;
    spell_fn[72] = spell_cause_critical;
    spell_fn[73] = spell_dispel_evil;
    spell_fn[74] = spell_flame_strike;
    spell_fn[75] = spell_raise_dead;
    spell_fn[76] = spell_slay_living;
    spell_fn[77] = fx_affected;
    spell_fn[78] = spell_entangle;
    spell_fn[79] = spell_faerie_fire;
    spell_fn[80] = spell_invis_animals;
    spell_fn[81] = spell_charm_monsters;
    spell_fn[82] = spell_confusion;
    spell_fn[83] = spell_dimension_door;
    spell_fn[84] = spell_fear;
    spell_fn[85] = spell_fire_shield;
    spell_fn[86] = spell_fumble;
    spell_fn[87] = spell_ice_storm;
    spell_fn[88] = spell_minor_globe;
    spell_fn[89] = spell_remove_curse;
    spell_fn[90] = spell_barkskin;
    spell_fn[91] = spell_cloud;
    spell_fn[92] = spell_cone_of_cold;
    spell_fn[93] = spell_feeblemind;
    spell_fn[94] = spell_hold;
    spell_fn[135] = scroll_protection;
    spell_fn[96] = spell_charm_mammal;
    spell_fn[136] = potion_invisibility;
    spell_fn[98] = spell_cure_light_98;
    spell_fn[137] = potion_extra_healing;
    spell_fn[100] = spell_bestow_curse_100;
    spell_fn[101] = spell_cloud;
    spell_fn[102] = spell_restoration;
    spell_fn[103] = spell_energy_drain;
    spell_fn[104] = spell_destruction;
    spell_fn[105] = spell_resurrection;
    spell_fn[106] = spell_cure_disease;
    spell_fn[107] = spell_neutralize;
    spell_fn[108] = spell_hold_animal;
    spell_fn[109] = spell_prot_fire;
    spell_fn[110] = spell_death;
    spell_fn[111] = spell_disintegrate;
    spell_fn[112] = spell_globe;
    spell_fn[113] = spell_stone_to_flesh;
    spell_fn[114] = spell_flesh_to_stone;
    spell_fn[115] = spell_fireball;
    spell_fn[116] = spell_mass_invis;
    spell_fn[117] = spell_word_stun;
    spell_fn[118] = spell_fire_touch;
    spell_fn[119] = spell_iron_skin;
    spell_fn[120] = spell_mass_charm;
    spell_fn[121] = spell_dance;
    spell_fn[122] = fx_protected;
    spell_fn[123] = spell_word_blind;
    spell_fn[124] = spell_meteor_swarm;
    spell_fn[125] = spell_word_kill;
    spell_fn[126] = spell_summoning;
    spell_fn[57] = fx_protected;
    spell_fn[59] = spell_silence;
    spell_fn[60] = fx_affected;
    spell_fn[61] = spell_remove_curse;
    spell_fn[62] = spell_bless;
    spell_fn[63] = spell_charm_person;
    spell_fn[64] = spell_burning_hands;
    spell_fn_fill();
}
