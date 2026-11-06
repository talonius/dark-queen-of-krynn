/* DQK.EXE overlay 10 module 2ffc: choosing and casting spells.  The spell
 * lists -- memorized, in the grimoire, to memorize or scribe, and the spells
 * an item holds -- built into a menu with a level heading above each run,
 * the spell menu itself, and casting: whether a spell can be cast here,
 * choosing its targets, rolling each target's save and handing the effect
 * on, the targets along a line or cone, and calling the spell's handler
 * through spell_fn.
 *
 * EXE 0x05a2ec..0x05c4b4, one Turbo C large-model _TEXT segment starting at
 * byte 0xc of paragraph 2ffc, the second and last module of RTLink section
 * 10 (payload 0x556c0, link segment 2b3a, which sections 9 and 11 also use).
 * Its first 12 bytes are spellfx.c's tail; after its last retf the section
 * holds 12 zero bytes to its initialized end, 0x5c4c0.  Built with Turbo C
 * 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped order; bake
 * consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the
 * shipped call, or the DGROUP offset of an extern variable.  Callee and
 * variable names are placeholders chosen from use; a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.  The ovl10_2ffc_
 * functions keep the names spellfx.c calls them by.  Vector stubs reach
 * turn_checks (1d99:0f27), spell_list_build (14b3), spell_range (10c1),
 * camp_spell_targets (0fd1), spell_cast (100d), spell_path_targets (152b),
 * spell_menu (10cb) and spell_run (0f31); spellfx.c far-calls 101b, 10f9,
 * 19ff and 1e0d, and its spellfx_init stores 135e in hit_proc.
 *
 * The routines follow DKK's spells.pas (dkk-disassembly) in order from
 * modeEligible to private1B20; containsMode comes after them as
 * spell_memorized, spell_menu is DKK's CharView.spellListMenu with "in
 * Bundle" added, and no DKK procedure was found for turn_checks,
 * bundle_add or spell_run.  DKK's nested initializePath, clampCoordinates
 * and collectPathTargets are path_init, path_clamp and path_collect, which
 * take the parent's locals by pointer.
 *
 * _DATA is level_names, the four step tables and 15 stored literals, DGROUP
 * 8b2e..8d70; -d stores "" as the NUL of "Exit".  _BSS is list_head,
 * item_ptrs, spell_ids, item_count and spell_counts, c7e6..c9e6.
 *
 * targets is indexed from 1; hits, heroes and steps are too, and the
 * shipped code adds the address of element 0.  spell_path_targets's scans use
 * the global tick_slot as their loop counter.
 */

#include "dqk.h"

char far spell_memorized(struct Character far *character, unsigned char spell);
void far spell_run(unsigned char spell);

static char level_names[10][41] = {                              /* 8b2e */
    "Special", "1st Level", "2nd Level", "3rd Level", "4th Level",
    "5th Level", "6th Level", "7th Level", "8th Level", "9th Level"
};
static char near_dx[8] = { -1, 0, 0, 1, 1, 0, 0, -1 };             /* 8cc8 */
static char near_dy[8] = { 0, -1, -1, 0, 0, 1, 1, 0 };             /* 8cd0 */
static char far_dx[8] = { 1, 1, 0, 0, -1, -1, 0, 0 };             /* 8cd8 */
static char far_dy[8] = { 0, 0, 1, 1, 0, 0, -1, -1 };             /* 8ce0 */

static struct Node far *list_head;                                     /* c7e6 */
static struct Item far *item_ptrs[51];                                /* c7ea */
static unsigned char spell_ids[150];                                    /* c8b6 */
static unsigned char item_count;                                        /* c94c */
static unsigned char spell_counts[0x9a];                                /* c94d */

void far turn_checks(void)                                       /* 2ffc:000c */
{
    if (death_explosion_active == 0)
        fx_death_explosions();
    if (ice_explosion_active == 0)
        fx_ice_explosions();
    if (meteor_explosion_active == 0)
        fx_meteor_explosions();
}

char far spell_usable(struct Character far *character, unsigned char spell, char flag)
                                                                 /* 2ffc:0031 */
{
    char result;

    spell = spell & 0x7f;
    result = 0;
    switch ((spell_info - 1)[spell].cast_class) {
    case 0:
    case 3:
        result = character->stats[2].cur > 8 && (class_level(character, 0) > 0 || class_level(character, 1) > 5
                              || class_level(character, 3) > 8);
        break;
    case 1:
        result = (char)(character->stats[2].cur > 8) && class_level(character, 4) > 7;
        break;
    case 2:
        if (character->stats[1].cur > 8) {
            if (class_level(character, 5) > 0 && !(character->race == 6 && character->equipped[2] != 0
                                            && class_level(character, 4) <= 8 && view_mode == 5))
                result = 1;
            if (class_level(character, 4) > 8)
                result = 1;
            if (flag && class_level(character, 6) > 10)
                result = 1;
        }
        break;
    case 4:
        result = 0;
        break;
    }
    return result;
}

char far spell_choose(unsigned char type, int far *index)                /* 2ffc:019c */
{
    char result;
    struct Node far *picked;
    struct Node far *node;
    char title[42];
    char text[42];
    short count;
    char rows;
    char key;
    char can_exit;
    char redraw;
    char fkeys;

    str_copy(title, txt_choose_spell);
    rows = 0x16;
    fkeys = 0;
    hotkey_reset(1);
    redraw = 0;
    ui_mode = 1;
    switch (type) {
    case 1:
        str_copy(text, txt_cast_exit);
        break;
    case 2:
        str_copy(text, txt_memorize_exit);
        break;
    case 3:
        str_copy(text, txt_scribe_exit);
        break;
    case 4:
        str_copy(text, txt_learn_exit);
        break;
    case 8:
        str_copy(title, txt_spells_ok);
        str_copy(text, txt_keep_exit);
        break;
    case 9:
        str_copy(title, txt_scribe_ok);
        str_copy(text, txt_keep_exit);
        break;
    default:
        fkeys = -1;
        str_copy(title, empty_text);
        str_copy(text, "Exit");
        break;
    }
    if (type == 2) {
        rows = 0xf;
        redraw = pick_special;
    } else
        rows = 0x16;
    can_exit = type != 4;
    if (*index < 0) {
        redraw = 1;
        *index = 0;
    }
    if (type == 4 || type == 1)
        redraw = 1;
    picked = 0;
    do
        key = list_menu(title, text, 1, 5, 0x26, rows, list_head, can_exit, fkeys,
                        &redraw, index, &picked);
    while (key != 0 && key != 1 && key != 0x1b);
    node = list_head;
    count = 0;
    while (node != picked) {
        if (node->key == 0)
            count++;
        node = node->next;
    }
    if (key != 0)
        result = 0;
    else
        result = spell_ids[count];
    if (type == 3)
        item_cur = item_ptrs[count];
    list_free(&list_head);
    return result;
}

void far list_add(unsigned char spell)                           /* 2ffc:03e6 */
{
    struct Node far *node;
    unsigned short count;
    unsigned char level;

    node = list_head;
    if (list_head == 0) {
        pool_alloc(&node, 0x28, node_pool);
        list_head = node;
        level = 0;
        count = 0;
    } else {
        count = 1;
        while (node->next != 0) {
            if (node->key == 0)
                count++;
            node = node->next;
        }
        level = (spell_info - 1)[spell_ids[count - 1]].level;
        pool_alloc(node, 0x28, node_pool);
        node = node->next;
    }
    node->next = 0;
    if ((spell_info - 1)[spell & 0x7f].level != level) {
        str_copy(node->text, level_names[(spell_info - 1)[spell & 0x7f].level]);
        node->key = 1;
        pool_alloc(node, 0x28, node_pool);
        node = node->next;
        node->next = 0;
    }
    if (spell & 0x80)
        str_copy(node->text, " *");
    else
        str_copy(node->text, "\t");
    str_copy(node->text, msg_format("%s%s", node->text, (spell_names - 1)[spell & 0x7f]));
    node->key = 0;
    spell_ids[count] = spell & 0x7f;
}

void far list_add_count(unsigned char spell)                     /* 2ffc:05cc */
{
    unsigned char saved;
    unsigned char swap;
    struct Node far *node;
    struct Node far *scan;
    struct Node far *added;
    unsigned short index;
    char found;
    unsigned char level;
    register int pos;

    node = list_head;
    found = 0;
    level = (spell_info - 1)[spell & 0x7f].level;
    if (list_head == 0) {
        mem_zero(spell_counts, 0x9a);
        pool_alloc(&node, 0x28, node_pool);
        node->next = 0;
        list_head = node;
        pos = 0;
        spell_counts[pos] = 1;
    } else {
        pos = 0;
        while (node != 0 && !found) {
            if (node->key == 0) {
                if ((spell_info - 1)[spell_ids[pos]].level <= level && spell_ids[pos] != (spell & 0x7f)) {
                    pos++;
                    node = node->next;
                } else
                    found = 1;
            } else {
                node = node->next;
                pos++;
            }
        }
        if (spell_ids[pos] != (spell & 0x7f)) {
            scan = list_head;
            if (scan != node) {
                while (scan->next != node)
                    scan = scan->next;
                pool_alloc(&added, 0x28, node_pool);
                added->next = scan->next;
                scan->next = added;
            } else {
                pool_alloc(&added, 0x28, node_pool);
                added->next = node;
                list_head = added;
            }
            node = added;
            saved = 1;
            for (index = pos; index <= 0x88; index++) {
                swap = spell_counts[index];
                spell_counts[index] = saved;
                saved = swap;
            }
        } else
            spell_counts[pos]++;
    }
    if (spell & 0x80)
        str_copy(node->text, " *");
    else
        str_copy(node->text, "\t");
    str_copy(node->text, msg_format("%s%s", node->text, (spell_names - 1)[spell & 0x7f]));
    node->key = 0;
    if (spell_counts[pos] > 1)
        str_copy(node->text, msg_format("%s (%s)", node->text, byte_str(spell_counts[pos])));
    if (spell_ids[pos] != (spell & 0x7f)) {
        saved = spell_ids[pos];
        spell_ids[pos] = spell & 0x7f;
        for (index = pos + 1; index <= 0x86; index++) {
            swap = spell_ids[index];
            spell_ids[index] = saved;
            saved = swap;
        }
    }
}

void far item_add(char marked, char check)                       /* 2ffc:08cb */
{
    unsigned char index;
    struct Effect far *effect;

    if (check && item_table[item_cur->rec.type].slot == 0xc)
        return;
    if (find_effect(cur_node, 0x10, &effect)
        || class_level(cur_node, 0) > 0 && item_table[item_cur->rec.type].slot == 0xc)
        item_cur->rec.unid_mask = item_cur->rec.unid_mask & 0xf8;
    if ((item_cur->rec.unid_mask & 7) != 0)
        return;
    for (index = 1; index <= 3; index++) {
        if (marked && (item_cur->rec.u.spells - 1)[index] > 0x80
            || !marked && (item_cur->rec.u.spells - 1)[index] > 0) {
            if (spell_usable(cur_node, (item_cur->rec.u.spells - 1)[index], 1)) {
                list_add((item_cur->rec.u.spells - 1)[index]);
                item_ptrs[item_count] = item_cur;
                item_count++;
            }
        }
    }
}

void far bundle_add(char marked)                                 /* 2ffc:09fc */
{
    register int index;

    for (index = 0; index <= 0x32; index++)
        item_ptrs[index] = 0;
    item_count = 0;
    item_cur = item_cur->more;
    while (item_cur != 0) {
        item_add(marked, 0);
        item_cur = item_cur->more;
    }
}

void far items_add(char marked, char check)                      /* 2ffc:0a4a */
{
    struct Item far *bundle;
    register int index;

    for (index = 0; index <= 0x32; index++)
        item_ptrs[index] = 0;
    item_count = 0;
    item_cur = cur_node->items;
    while (item_cur != 0) {
        if (item_casts(item_cur)) {
            if (cur_node->robe_order == 1 && (item_cur->rec.unid_mask & 0x20)
                || cur_node->robe_order == 2 && (item_cur->rec.unid_mask & 0x10)) {
                if (item_cur->rec.type == 0x49) {
                    bundle = item_cur;
                    item_cur = item_cur->more;
                    while (item_cur != 0) {
                        item_add(marked, check);
                        item_cur = item_cur->more;
                    }
                    item_cur = bundle;
                } else
                    item_add(marked, check);
            }
        }
        item_cur = item_cur->next;
    }
}

char far spell_list_build(unsigned char list)                    /* 2ffc:0b39 */
{
    char result;
    unsigned char spell;
    unsigned short spell_slot;
    struct Node far *cur;
    struct Node far *node;
    unsigned char prev;
    unsigned char level;
    char headers;
    unsigned char kind;

    headers = 1;
    result = 0;
    list_head = 0;
    mem_zero(spell_ids, 0x96);
    switch (list) {
    case 0:
        for (spell_slot = 0; spell_slot <= 0x88; spell_slot++) {
            if (cur_node->spells[spell_slot] > 0 && spell_usable(cur_node, cur_node->spells[spell_slot] & 0x7f, 0)
                && cur_node->spells[spell_slot] < 0x80)
                list_add_count(cur_node->spells[spell_slot]);
        }
        break;
    case 5:
        for (spell_slot = 0; spell_slot <= 0x88; spell_slot++) {
            if ((cur_node->spells[spell_slot] & 0x80)
                && spell_usable(cur_node, cur_node->spells[spell_slot] & 0x7f, 0))
                list_add_count(cur_node->spells[spell_slot]);
        }
        break;
    case 1:
        for (spell_slot = 1; spell_slot <= 0x7e; spell_slot++) {
            if ((cur_node->known[(spell_slot - 1) >> 3] & bit_mask[(spell_slot - 1) & 7])
                && spell_usable(cur_node, spell_slot, 0) && !spell_memorized(cur_node, spell_slot))
                list_add_count(spell_slot);
        }
        break;
    case 2:
        item_count = 0;
        item_add(0, 0);
        headers = 0;
        break;
    case 7:
        bundle_add(0);
        headers = 0;
        break;
    case 3:
        items_add(0, 1);
        headers = 0;
        break;
    case 6:
        items_add(1, 1);
        headers = 0;
        break;
    case 4:
        if (cur_node->robe_order == 1)
            kind = 2;
        else
            kind = 3;
        for (level = 1; level <= 9; level++) {
            if ((cur_node->slots[2] - 1)[level] > 0) {
                for (spell_slot = 1, spell = 0xff; spell_slot <= 10 && spell != 0; spell_slot++) {
                    spell = spells_by_level[kind - 1][level - 1][spell_slot - 1];
                    if (spell != 0) {
                        if (!(cur_node->known[(spell - 1) / 8] & bit_mask[(spell - 1) % 8]))
                            list_add_count(spell);
                    }
                }
            }
        }
        break;
    }
    if (list_head != 0) {
        if (headers) {
            spell_slot = 0;
            level = (spell_info - 1)[spell_ids[spell_slot]].level;
            pool_alloc(&node, 0x28, node_pool);
            node->next = list_head;
            list_head = node;
            str_copy(list_head->text, level_names[(spell_info - 1)[spell_ids[spell_slot]].level]);
            list_head->key = 1;
            cur = list_head;
            do {
                prev = level;
                if (spell_ids[spell_slot] != 0)
                    level = (spell_info - 1)[spell_ids[spell_slot]].level;
                if (prev < level) {
                    pool_alloc(&node, 0x28, node_pool);
                    node->next = cur->next;
                    cur->next = node;
                    cur = cur->next;
                    str_copy(cur->text, level_names[level]);
                    cur->key = 1;
                }
                spell_slot++;
                cur = cur->next;
            } while (cur != 0);
        }
        result = 1;
    }
    return result;
}

unsigned char far spell_range(unsigned char spell)               /* 2ffc:0f34 */
{
    char result;
    unsigned char range_value;

    result = 4;
    if (item_cast_active == 0)
        range_value = caster_level(spell, 0) * (spell_info - 1)[spell].range_per_level + (spell_info - 1)[spell].range;
    else
        range_value = (spell_info - 1)[spell].range_per_level * 6 + (spell_info - 1)[spell].range;
    if (spell == 0x7d)
        range_value = range_value / 4;
    if (range_value == 0 && (spell_info - 1)[spell].targeting != 0)
        range_value = 1;
    if (range_value == 0xff)
        range_value = 1;
    result = range_value;
    return result;
}

int far spell_duration(unsigned char spell)                     /* 2ffc:101b */
{
    short result;

    switch (spell) {
    case 0x1a:
        result = 0xec4;
        break;
    case 0x7f:
    case 0x82:
        result = fx_roll_dice(5, 4);
        break;
    case 0x80:
        result = fx_roll_dice(1, 4) * 10 + 0x28;
        break;
    case 0x7a:
        result = 0x5a0;
        break;
    case 0x79:
        result = fx_roll_dice(1, 4) + 1;
        break;
    default:
        result = caster_level(spell, 0) * (spell_info - 1)[spell].duration_per_level + (spell_info - 1)[spell].duration;
        break;
    }
    return result;
}

void far spell_damage_apply(unsigned char spell, char level, char on_remove, unsigned char dmg, int damage_element_mask,
                         char far *msg)                          /* 2ffc:10f9 */
{
    struct Effect far *effect;
    char lvl;
    unsigned char target_index;
    char saved;

    if (dmg == 0)
        element_mask = 0;
    else
        element_mask = damage_element_mask;
    if (target_count == 0)
        return;
    if (level == 0)
        lvl = caster_level(spell, 1);
    else
        lvl = level;
    for (target_index = 1; target_index <= target_count; target_index++) {
        if (targets[target_index] != 0) {
            if ((spell_info - 1)[spell].save_effect == 0)
                saved = 0;
            else if (targets[target_index]->status == 10
                     || find_effect(targets[target_index], 0x96, &effect) && current_spell == 0x37)
                saved = 1;
            else
                saved = fx_saving_throw(targets[target_index], (spell_info - 1)[spell].save_type, 0);
            if ((spell_info - 1)[current_spell].range == 0xff) {
                recalc_stats(targets[target_index]);
                fx_trigger_group(0xb, &targets[target_index]);
                if (!fx_attack_roll(cur_node, targets[target_index], targets[target_index]->ac[0])) {
                    dmg = 0;
                    saved = 1;
                }
            }
            if (dmg > 0)
                fx_damage(targets[target_index], dmg, (spell_info - 1)[spell].save_effect, saved);
            if ((spell_info - 1)[spell].effect_id > 0)
                fx_add_message(targets[target_index], (spell_info - 1)[spell].effect_id, spell_duration(spell), lvl, on_remove,
                             (spell_info - 1)[spell].save_effect, saved, msg);
        }
    }
    element_mask = 0;
    turn_checks();
}

void far camp_spell_targets(unsigned char spell, char mode, char far *ok)
                                                                 /* 2ffc:135e */
{
    targets[1] = cur_node;
    target_count = 1;
    *ok = 1;
    switch ((spell_info - 1)[spell].camp_target) {
    case 1:
        break;
    case 2:
        view_redraw();
        if (selected_target != 0)
            targets[1] = selected_target;
        pick_hero("Cast Spell on whom", 1, 1, &targets[1]);
        selected_target = targets[1];
        if (targets[1] == 0) {
            target_count = 0;
            *ok = 0;
        }
        break;
    case 3:
        targets[target_count] = node_list;
        while (targets[target_count]->next != 0) {
            targets[target_count + 1] = targets[target_count]->next;
            target_count++;
        }
        break;
    default:
        *ok = 0;
        break;
    }
}

void far spell_cast(unsigned char spell, char quiet, char announce, char far *done)
                                                                 /* 2ffc:145b */
{
    char again;
    char x;
    char y;
    char cast_direction;
    struct Character far *caster;
    char kind;

    caster = cur_node;
    again = 1;
    if (view_mode != 5 && (spell_info - 1)[spell].camp_target == 0) {
        if (item_cast_active == 0) {
            status_printf(1, 0x13, 7, 0, (spell_names - 1)[spell]);
            status_printf(1, 0x14, 7, 0, txt_cant_cast_here);
            if (confirm(txt_lose_it, 1))
                spell_mark(caster, spell);
        } else {
            status_printf(1, 0x13, 7, 0, txt_that_item);
            status_printf(1, 0x14, 7, 0, txt_combat_only);
            if (confirm(txt_use_it, 1))
                *done = 1;
        }
        announce = 0;
        again = 0;
    }
    if (announce && item_cast_active == 0)
        cast_announce(caster, txt_casts, spell);
    while (again) {
        hit_proc(spell, quiet, done);
        if (*done) {
            again = 0;
            if (view_mode == 5 && spell != 0x7c) {
                if (spell == 0x2f || spell == 0x73 || spell == 0x4a || spell == 0x85)
                    icon_facings(0x20);
                else
                    icon_facings(0x13);
                x = combatant_x(caster);
                y = combatant_y(caster);
                for (cast_direction = 0; !dir_ok(x, y, view_cx, view_cy, cast_direction); cast_direction++)
                    ;
                if (spell == 0x2f || spell == 0x73 || spell == 0x4a || spell == 0x85) {
                    kind = 1;
                    missile_facing(cast_direction, 0x20);
                } else
                    kind = 4;
                move_flag = 1;
                combatant_facing_set(caster, cast_direction, 1, 0);
                switch (spell) {
                case 0x2f:
                    sfx_play(0xc);
                    break;
                case 0x33:
                    sfx_play(9);
                    break;
                default:
                    sfx_play(3);
                    break;
                }
                icon_fly(x, y, view_cx, view_cy, kind, 0x1e);
                switch (spell) {
                case 0x22:
                    icon_scatter(1, 0x1b, 0xc);
                    break;
                case 0x5b:
                    icon_scatter(1, 0x1c, 0xc);
                    break;
                case 0x2f:
                case 0x73:
                case 0x85:
                    if (game_state[STATE_DUNGEON] == 0 || spell == 0x85)
                        icon_scatter(2, 0x19, 0xc);
                    else
                        icon_scatter(3, 0x19, 0xc);
                    break;
                case 0x57:
                    if (game_state[STATE_UNDERWATER])
                        icon_scatter(3, 0x1a, 0xc);
                    else
                        icon_scatter(2, 0x1a, 0xc);
                    break;
                case 0x33:
                    if (game_state[STATE_UNDERWATER])
                        icon_scatter(2, 0x14, 9);
                    break;
                }
                if (combatant_visible(caster, 0)) {
                    combatant_facing_set(caster, caster->combat->direction, 1, 1);
                    combatant_facing_set(caster, caster->combat->direction, 0, 0);
                }
            }
            fx_invisibility_remove(caster);
            if (item_cast_active == 0 && spell > 0)
                spell_mark(caster, spell);
            current_spell = spell;
            spell_run(current_spell);
            current_spell = 0;
            mirror_image_bypass = 0;
        } else if (view_mode == 5) {
            if (quiet || confirm(txt_abort_spell, 1)) {
                bottom_msg(txt_spell_aborted);
                if (item_cast_active == 0 && spell > 0)
                    spell_mark(caster, spell);
                again = 0;
            }
        } else
            again = 0;
    }
    combat_text_clear();
    if (view_mode == 5)
        box_draw_top();
    if (view_mode == 5)
        turn_checks();
}

void far path_init(char x0, char y0, char x1, char y1, struct Line far *line)
                                                                 /* 2ffc:18db */
{
    line->x0 = x0;
    line->y0 = y0;
    line->x1 = x1;
    line->y1 = y1;
    line_init(line);
}

void far path_clamp(char far *x, char far *y)                   /* 2ffc:190d */
{
    if (*x > 0x31)
        *x = 0x31;
    else if (*x < 0)
        *x = 0;
    if (*y > 0x18)
        *y = 0x18;
    else if (*y < 0)
        *y = 0;
}

void far path_collect(unsigned char far *hits, unsigned char far *tile, unsigned char far *who,
                      struct Line far *line, char far *end)         /* 2ffc:194a */
{
    unsigned char index;
    char found;

    do {
        *end = !line_step(line);
        if (*end == 0) {
            cell_get(line->x, line->y, who, tile);
            index = 1;
            found = 0;
            if (*who > 0) {
                do {
                    if (hits[index - 1] == *who)
                        found = 1;
                    else if (hits[index - 1] == 0) {
                        hits[index - 1] = *who;
                        found = 1;
                    } else
                        index++;
                } while (!found);
            }
        }
    } while (*end == 0);
}

void far spell_path_targets(char x0, char y0, char x1, char y1, unsigned char width,
                         unsigned char len)                      /* 2ffc:19ff */
{
    char nx;
    char ny;
    char far_x;
    char far_y;
    struct Line line;
    char done;
    unsigned char dist;
    unsigned char idx;
    unsigned char last;
    unsigned char dirs[50];
    unsigned char hits[30];
    unsigned char who;
    unsigned char tile;
    unsigned char count;
    unsigned char out;
    unsigned char dir;
    unsigned range;
    char found;

    idx = 0;
    path_init(x0, y0, x1, y1, &line);
    do {
        done = !line_step(&line);
        if (done == 0) {
            dirs[idx] = line.dir;
            idx++;
        }
    } while (done == 0);
    last = idx - 1;
    idx = 0;
    len = len * 2;
    dist = line.steps;
    done = 0;
    while (dist < len && done == 0) {
        if (x1 < 0x31 && x1 > 0 && y1 < 0x18 && y1 > 0) {
            switch (dirs[idx]) {
            case 0:
            case 2:
            case 4:
            case 6:
                dist += 2;
                break;
            case 1:
            case 3:
            case 5:
            case 7:
                dist += 3;
                break;
            }
            x1 += dir_dx[dirs[idx]];
            y1 += dir_dy[dirs[idx]];
            if (idx == last)
                idx = 0;
            else
                idx++;
        } else
            done = 1;
    }
    path_clamp(&x1, &y1);
    range = len;
    line_clear(x0, y0, &x1, &y1, &range);
    path_init(x0, y0, x1, y1, &line);
    mem_zero(hits, 0x1e);
    count = 1;
    do {
        done = !line_step(&line);
        if (done == 0) {
            cell_get(line.x, line.y, &who, &tile);
            if (who > 0) {
                if (count > 1) {
                    for (tick_slot = 1; tick_slot <= count - 1; tick_slot++)
                        if ((hits - 1)[tick_slot] == who)
                            found = 1;
                } else
                    found = 0;          /* only here: once set, it stays set */
                if (!found) {
                    (hits - 1)[count] = who;
                    count++;
                }
                dir = line.dir;
            }
        }
    } while (done == 0);
    if (width > 1 && width != 0xff) {
        nx = near_dx[dir] + x1;         /* dir is unset if nothing was hit */
        ny = near_dy[dir] + y1;
        path_clamp(&nx, &ny);
        far_x = far_dx[dir] + x1;
        far_y = far_dy[dir] + y1;
        path_clamp(&far_x, &far_y);
        path_init(x0, y0, far_x, far_y, &line);
        path_collect(hits, &tile, &who, &line, &done);
        if (width > 2) {
            path_init(x0, y0, nx, ny, &line);
            path_collect(hits, &tile, &who, &line, &done);
        }
    }
    if (width == 0xff) {
        plan_steps(x1, y1, 3, 0xff, 1);
        for (tick_slot = 1; tick_slot <= step_count; tick_slot++)
            (hits - 1)[tick_slot] = (steps - 1)[tick_slot].who;
    }
    count = 1;
    out = 1;
    target_count = 0;
    if ((hits - 1)[1] != 0) {
        do {
            if ((heroes - 1)[(hits - 1)[count]] != cur_node) {
                target_count++;
                targets[out] = (heroes - 1)[(hits - 1)[count]];
                out++;
            }
            count++;
        } while ((hits - 1)[count] != 0);
    }
}

void far spell_effect_apply(char save_modifier, char far *msg)                  /* 2ffc:1e0d */
{
    struct Character far *cur;
    char saved;
    unsigned char target_index;
    short pred;
    char level;
    char extra;
    struct Effect far *effect;

    extra = 0;
    for (target_index = target_count; target_index >= 1; target_index--) {
        if (targets[target_index] != 0) {
            cur = targets[target_index];
            if (target_index < target_count) {
                sfx_play(3);
                icon_facings(0x13);
                icon_fly(combatant_x(cur_node), combatant_y(cur_node), combatant_x(cur),
                             combatant_y(cur), 4, 0x1e);
            }
            if ((current_spell == 0x4f || current_spell == 0x51) && target_index == target_count) {
                saved = 1;
                pred = 1;
            } else {
                saved = fx_saving_throw(cur, (spell_info - 1)[current_spell].save_type, save_modifier);
                pred = (spell_info - 1)[current_spell].save_effect;
            }
            if (cur->race == 7 && (current_spell == 0x17 || current_spell == 0x31))
                saved = 1;
            if (!(cur->type_flags & 0x40) && current_spell == 0x6c)
                saved = 1;
            if (find_effect(cur, 0x96, &effect)
                && (current_spell == 0x31 || current_spell == 0x17 || current_spell == 0x5e))
                saved = 1;
            if (current_spell == 0x51) {
                level = caster_level(current_spell, 1) + (cur_node->side << 7);
                extra = 1;
            } else
                level = caster_level(current_spell, 1);
            fx_add_message(cur, (spell_info - 1)[current_spell].effect_id, spell_duration(current_spell), level,
                         extra, pred, saved, msg);
        }
    }
}

char far spell_memorized(struct Character far *character, unsigned char spell) /* 2ffc:200f */
{
    char result;
    unsigned char spell_slot;
    char found;

    spell = spell & 0x7f;
    result = 0;
    if ((spell_info - 1)[spell].cast_class == 3) {
        spell_slot = 0;
        found = 0;
        while (!found && spell_slot <= 0x88) {
            if ((character->spells[spell_slot] & 0x7f) == spell)
                found = 1;
            spell_slot++;
        }
        result = found;
    }
    return result;
}

unsigned char far spell_menu(unsigned char list, unsigned char type, int far *choice,
                             char far *more)                     /* 2ffc:2080 */
{
    char result;
    char far *label;

    switch (list) {
    case 0:
        label = "in Memory";
        break;
    case 1:
        label = "in Grimoire";
        break;
    case 2:
        label = "on Scroll";
        break;
    case 7:
        label = "in Bundle";
        break;
    case 3:
        label = "on Scrolls";
        break;
    case 4:
        label = "to Choose";
        break;
    case 5:
        label = "to Memorize";
        break;
    case 6:
        label = "to Scribe";
        break;
    default:
        label = "";
        break;
    }
    *more = spell_list_build(list);
    if (*more) {
        if (*choice < 0 || type == 1) {
            if (view_mode != 5) {
                if (type != 2)
                    frame_list_panel();
            } else
                frame_border();
        }
        character_name_show(cur_node, 1, 1, 1);
        status_printf(str_length(cur_node->name) + 4, 1, 7, 0, msg_format("Spells %s", label));
        result = spell_choose(type, choice);
    } else
        result = 0;
    return result;
}

void far spell_run(unsigned char spell)                          /* 2ffc:21b5 */
{
    (*spell_fn[spell])();
    if (view_mode == 5)
        turn_checks();
}
