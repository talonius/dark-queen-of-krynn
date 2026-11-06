/* DQK.EXE overlay 07 module 25a4: making camp.  The camp menu and its
 * submenus: casting, memorizing and scribing spells and the slots they
 * use, the spell effects on each party member, resting, the party order,
 * game speed, icon and difficulty level, and fixing the party by resting
 * while its healing spells are cast.
 *
 * EXE 0x0447b0..0x04773d, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 25a4, the first module of RTLink section 7 (payload
 * 0x447b0, link segment 25a4, parent section 3).  Its final retf is byte c
 * of paragraph 289c, where the next module begins at 289c:000d (section 7's
 * table files every fixup site before it under frame 25a4, and every one
 * after it under 289c).  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Sections 4, 5, 6 and 8 hold other bytes at link segment 25a4.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, an ovlNN_ name which section and stored
 * segment, and a mod1d99_ callee is an RTLink vector stub whose target is
 * named beside it.  Code outside the section reaches three of these
 * routines through vector stubs: 1d99:14a9 -> 0688, 1d99:0f8b -> 18b6,
 * 1d99:120b -> 2d3c.
 *
 * The routines follow the order of DKK's camp.pas (dkk-disassembly), but
 * the empty camp_unit_init comes first where DKK's empty campOverlayInitProbe is
 * last, levels_lost and fix_needs come ahead of hero_ok, and DKK's
 * copy-protection check, drop menu and minimumWord are not in this module.
 * fix_needs and fix_cures step three of their counter pointers where they
 * mean to count, as shipped (side comments there).
 *
 * _DATA is 26 stored literals, DGROUP 7b96..7d97; -d stores "" as the NUL
 * of "cannot cast spells in this area", " " as the tail of "is in no
 * condition to ", and each literal used twice once.  The module declares
 * no _BSS.
 */

#include "dqk.h"

/* The translated Pascal unit's empty initialization block (DKK camp.pas). */
void far camp_unit_init(void)                                         /* 25a4:0000 */
{
}

int far prep_time(struct Character far *hero)                         /* 25a4:0001 */
{
    short result;
    unsigned char max_mem;
    unsigned char max_scroll;
    struct Item far *sub;
    struct Item far *item;
    unsigned char spell_slot;
    unsigned char level;
    unsigned char base;
    int mem;
    int scroll;

    max_mem = 0;
    max_scroll = 0;
    scroll = mem = 0;
    for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
        if ((hero->spells[spell_slot] & 0x80) && hero->enabled) {
            level = (spell_info - 1)[hero->spells[spell_slot] & 0x7f].level;
            if (level == 0)
                level = spell_study_level_fallback(hero->spells[spell_slot] & 0x7f);
            if (level > max_mem)
                max_mem = level;
            mem += level;
        }
    for (item = hero->items; item; item = item->next)
        if (item_casts(item)) {
            if (item->rec.type == 'I')
                sub = item->more;
            else
                sub = item;
            do {
                for (spell_slot = 1; spell_slot <= 3; spell_slot++)
                    if ((sub->rec.u.spells - 1)[spell_slot] & 0x80) {
                        level = (spell_info - 1)[(sub->rec.u.spells - 1)[spell_slot] & 0x7f].level;
                        if (level > max_scroll)
                            max_scroll = level;
                        scroll += level;
                    }
                sub = sub->more;
            } while (item->rec.type == 'I' && sub);
        }
    base = 0;
    if (mem > 0 || scroll > 0)
        base = 4;
    if (max_mem > 2 || max_scroll > 2)
        base = 6;
    hero->spell_prep = base;
    result = base * 60 + mem * 15 + scroll * 15;
    return result;
}

void far memorize_reset(struct Character far *hero)                   /* 25a4:01d4 */
{
    unsigned char spell_slot;

    for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
        if (hero->spells[spell_slot] & 0x80)
            hero->spells[spell_slot] = 0;
    hero->spell_prep = 0;
}

void far scribe_reset(struct Character far *hero)                     /* 25a4:0218 */
{
    unsigned char index;
    struct Item far *sub;
    struct Item far *item;

    for (item = hero->items; item; item = item->next)
        if (item_casts(item)) {
            if (item->rec.type == 'I') {
                for (sub = item->more, tick_slot = 1; item->rec.quantity >= tick_slot;
                     sub = sub->more, tick_slot++)
                    for (index = 1; index <= 3; index++)
                        (sub->rec.u.spells - 1)[index] = (sub->rec.u.spells - 1)[index] & 0x7f;
            } else
                for (index = 1; index <= 3; index++)
                    (item->rec.u.spells - 1)[index] = (item->rec.u.spells - 1)[index] & 0x7f;
        }
}

void far scribe_reset_all(void)                                  /* 25a4:02f2 */
{
    struct Character far *character;

    for (character = node_list; character; character = character->next)
        scribe_reset(character);
}

unsigned char far slots_left(unsigned char level, short kind,
                             unsigned char far *used)            /* 25a4:0328 */
{
    char result;
    unsigned char count;
    unsigned char spell_slot;

    if (kind == 3) {
        if (cur_node->level[0] > 0)
            switch (cur_node->deity) {
            case 4:
                count = 3;
                break;
            case 6:
                count = 0;
                break;
            default:
                count = 1;
            }
        for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
            if ((cur_node->spells[spell_slot] & 0x7f) == 0x3e
                || (cur_node->spells[spell_slot] & 0x7f) == 0x39
                || (cur_node->spells[spell_slot] & 0x7f) == 0x3b
                || (cur_node->spells[spell_slot] & 0x7f) == 0x3c
                || (cur_node->spells[spell_slot] & 0x7f) == 0x3d
                || (cur_node->spells[spell_slot] & 0x7f) == 0x3f
                || (cur_node->spells[spell_slot] & 0x7f) == 0x40)
                count--;
        result = count;
    } else {
        count = 0;
        for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
            if (cur_node->spells[spell_slot] > 0
                && (spell_info - 1)[cur_node->spells[spell_slot] & 0x7f].level == level
                && (spell_info - 1)[cur_node->spells[spell_slot] & 0x7f].cast_class == kind)
                count++;
        *used = count;
        if ((cur_node->slots[kind] - 1)[level] < count)
            count = (cur_node->slots[kind] - 1)[level];
        result = (cur_node->slots[kind] - 1)[level] - count;
    }
    if (kind == 2) {
        if (cur_node->stats[1].cur < 0xc && level > 6)
            result = 0;
        if (cur_node->stats[1].cur < 0xe && level > 7)
            result = 0;
        if (cur_node->stats[1].cur < 0x10 && level > 8)
            result = 0;
        if (cur_node->stats[1].cur < 0x12 && level > 9)
            result = 0;
    } else if (kind == 0) {
        if (cur_node->stats[2].cur < 0x11 && level > 6)
            result = 0;
        if (cur_node->stats[2].cur < 0x12 && level > 7)
            result = 0;
    }
    return result;
}

char far spell_ok(unsigned char activity)                        /* 25a4:05b1 */
{
    char result;
    char msg[42];

    msg[0] = 0;
    if (activity == 1 && game_state[STATE_MAGIC_DISABLED] > 0)
        str_copy(msg, "cannot cast spells in this area");
    else if (cur_node->status == 1 || cur_node->enabled == 0) {
        str_copy(msg, "is in no condition to ");
        switch (activity) {
        case 1:
            str_cat(msg, "cast any spells");
            break;
        case 2:
            str_cat(msg, "memorize spells");
            break;
        case 3:
            str_cat(msg, "scribe any scrolls");
            break;
        }
    }
    if (msg[0] == 0)
        result = 1;
    else {
        result = 0;
        hero_text(cur_node, msg, 10, 1);
    }
    return result;
}

void far cast_menu(void)                                         /* 25a4:0688 */
{
    int index;
    char not_empty;
    char redraw;
    char done;
    unsigned char spell;

    redraw = 0;
    index = -1;
    ui_mode = 1;
    if (spell_ok(1))
        do {
            spell = spell_menu(0, 1, &index, &not_empty);
            if (spell != 0) {
                redraw = 1;
                text_clear(1, 0x11, 0x26, 0x16);
                spell_cast(spell, 0, 1, &done);
            } else if (not_empty)
                redraw = 1;
            else
                hero_text(cur_node, "has no spells memorized", 10, 1);
        } while (spell != 0);
    if (redraw)
        view_redraw();
}

unsigned char far show_slots(void)                               /* 25a4:073b */
{
    char label[42];
    char slot[10][4];
    char available_slots;
    unsigned char avail[4];
    short kind;
    unsigned char level;
    unsigned char column;
    unsigned char row;
    unsigned char slot_count;
    unsigned char base_slots;
    unsigned char used_slots;
    unsigned char temp;
    unsigned char any;

    bonus_slots = 0;
    any = 0;
    for (kind = 0; kind <= 3; kind++) {
        avail[kind] = 0;
        if (kind == 3) {
            if (cur_node->level[0] > 0 && cur_node->deity != 6) {
                avail[3] = 1;
                any = 1;
            }
            for (level = 1; level <= 9; level++)
                slot[level - 1][3] = -1;
            temp = slots_left(1, 3, &slot_count);
            if (temp > 0 || any)
                slot[0][3] = temp;
            else
                slot[0][3] = -1;
        } else
            for (level = 1; level <= 9; level++) {
                slot[level - 1][kind] = slots_left(level, kind, &slot_count);
                if ((cur_node->slots[kind] - 1)[level] == 0)
                    slot[level - 1][kind] = -1;
                else {
                    avail[kind] = 1;
                    any = 1;
                }
            }
    }
    if (cur_node->level[5] > 0 && cur_node->robe_order > 0) {
        switch (game_state[cur_node->robe_order + (STATE_MOON_PHASE_1 - 1)]) {
        case 1:
            bonus_slots = 1;
            break;
        case 2:
            bonus_slots = 2;
            break;
        default:
            bonus_slots = 0;
        }
        if (bonus_slots > 0)
            for (level = 1; level <= 9; level++) {
                base_slots = (cur_node->slots[2] - 1)[level];
                slot_count = slots_left(level, 2, &used_slots);
                if (base_slots < used_slots) {
                    if (used_slots - base_slots > bonus_slots)
                        bonus_slots = 0;
                    else
                        bonus_slots -= used_slots - base_slots;
                }
            }
        if (bonus_slots > 0)
            any = 1;
    }
    if (any) {
        text_clear(1, 0x11, 0x26, 0x16);
        row = 1;
        for (kind = 0; kind <= 3; kind++)
            if (avail[kind]) {
                switch (kind) {
                case 0:
                    str_copy(label, "    Cleric :");
                    break;
                case 1:
                    str_copy(label, "     Druid :");
                    break;
                case 3:
                    str_copy(label, special_label);
                    break;
                case 2:
                    str_copy(label, "Magic-User :");
                    break;
                }
                status_printf(1, row + 0x11, 7, 0, label);
                column = 0xd;
                for (level = 1; level <= 9; level++) {
                    available_slots = slot[level - 1][kind];
                    if (available_slots >= 0)
                        status_printf(column + 1, row + 0x11, 7, 0, "%d", available_slots);
                    column += 3;
                }
                row++;
            }
    }
    if (bonus_slots > 0) {
        status_printf(1, row + 0x11, 7, 0, bonus_label);
        status_printf(0xe, row + 0x11, 7, 0, byte_str(bonus_slots));
    }
    return any;
}

void far study_rest(char far *interrupted)                        /* 25a4:0aac */
{
    struct Character far *character;
    unsigned short total;
    unsigned short time;

    total = 0;
    for (character = node_list; character; character = character->next) {
        time = prep_time(character);
        if (total < time)
            total = time;
    }
    rest_time[4] = total / 1440;
    total %= 1440;
    rest_time[3] = total / 60;
    total %= 60;
    rest_time[2] = total / 10;
    rest_time[1] = total % 10;
    *interrupted = camp_rest(1);
    mem_fill(rest_time, sizeof rest_time, 0);
    party_info_display();
}

void far memorized_sort(void)                                    /* 25a4:0b64 */
{
    unsigned char spell_slot;
    unsigned char comparison_slot;
    unsigned char temp;

    for (spell_slot = 0; spell_slot <= 0x87; spell_slot++)
        for (comparison_slot = spell_slot + 1; comparison_slot <= 0x88; comparison_slot++)
            if (cur_node->spells[spell_slot] == 0
                || (cur_node->spells[spell_slot] & 0x7f) > (cur_node->spells[comparison_slot] & 0x7f)
                   && cur_node->spells[comparison_slot] != 0) {
                temp = cur_node->spells[spell_slot];
                cur_node->spells[spell_slot] = cur_node->spells[comparison_slot];
                cur_node->spells[comparison_slot] = temp;
            }
}

void far memorize_menu(void)                                     /* 25a4:0c33 */
{
    char done;
    char not_empty;
    char redraw;
    unsigned char spell;
    unsigned char spell_slot;
    unsigned char used;
    int index;

    if (spell_ok(2)) {
        done = 0;
        index = -1;
        ui_mode = 1;
        not_empty = spell_list_build(5);
        if (not_empty) {
            spell = spell_menu(5, 8, &index, &not_empty);
            if (spell == 0)
                memorize_reset(cur_node);
            else
                done = 1;
        } else {
            spell = spell_menu(5, 5, &index, &not_empty);
            used = 1;
            redraw = 0;
        }
        index = -1;
        redraw = 1;
        frame_lower_panel_palette();
        while (!done) {
            done = !show_slots();
            if (done)
                hero_text(cur_node, "cannot memorize any spells", 10, 1);
            else {
                spell = spell_menu(1, 2, &index, &not_empty);
                redraw = 1;
                if (spell == 0)
                    done = 1;
                else if (slots_left((spell_info - 1)[spell].level, (spell_info - 1)[spell].cast_class, &used) > 0
                         || (spell_info - 1)[spell].cast_class == 2 && bonus_slots > 0) {
                    if ((spell_info - 1)[spell].cast_class == 3)
                        pick_special = 1;
                    else
                        pick_special = 0;
                    for (spell_slot = 0; cur_node->spells[spell_slot] != 0; spell_slot++)
                        ;
                    cur_node->spells[spell_slot] = spell + 0x80;
                    memorized_sort();
                }
            }
        }
        if (index != -1) {
            index = -1;
            spell = spell_menu(5, 8, &index, &not_empty);
            if (not_empty && spell == 0)
                memorize_reset(cur_node);
        }
        if (redraw)
            view_redraw();
    } else if (cur_node->enabled)
        hero_text(cur_node, "cannot memorize any spells", 10, 1);
}

void far scribe_menu(void)                                       /* 25a4:0e63 */
{
    char done;
    char not_empty;
    char redraw;
    unsigned char spell;
    unsigned char inner_index;
    unsigned char scan_index;
    int index;
    struct Item far *item;
    struct Item far *sub;
    char found;

    if (spell_ok(3)) {
        done = 0;
        index = -1;
        not_empty = spell_list_build(6);
        if (not_empty) {
            redraw = 1;
            spell = spell_menu(6, 9, &index, &not_empty);
            if (spell == 0)
                scribe_reset(cur_node);
            else
                done = 1;
        } else
            redraw = 0;
        index = -1;
        while (!done) {
            spell = spell_menu(3, 3, &index, &not_empty);
            if (spell == 0) {
                done = 1;
                if (!not_empty)
                    hero_text(cur_node, "has no copyable scrolls", 10, 1);
                else
                    redraw = 1;
            } else {
                redraw = 1;
                if (cur_node->known[(spell - 1) / 8] & bit_mask[(spell - 1) % 8])
                    bottom_msg("You already know that spell");
                else {
                    item = cur_node->items;
                    found = 0;
                    while (item && !found) {
                        if (item_casts(item)) {
                            if (item->rec.type == 'I') {
                                for (sub = item->more, scan_index = 1; item->rec.quantity >= scan_index;
                                     sub = sub->more, scan_index++)
                                    for (inner_index = 1; inner_index <= 3; inner_index++)
                                        if (!found && ((sub->rec.u.spells - 1)[inner_index] & 0x80)
                                            && ((sub->rec.u.spells - 1)[inner_index] & 0x7f) == spell) {
                                            bottom_msg("You are already scribing that spell");
                                            found = 1;
                                        }
                            } else
                                for (inner_index = 1; inner_index <= 3; inner_index++)
                                    if (((item->rec.u.spells - 1)[inner_index] & 0x80)
                                        && ((item->rec.u.spells - 1)[inner_index] & 0x7f) == spell) {
                                        bottom_msg("You are already scribing that spell");
                                        found = 1;
                                    }
                        }
                        item = item->next;
                    }
                    if (!found) {
                        if ((cur_node->slots[(spell_info - 1)[spell].cast_class] - 1)[(spell_info - 1)[spell].level] > 0) {
                            item = cur_node->items;
                            while (item && !found) {
                                if (item->rec.type == 'I') {
                                    for (sub = item->more, scan_index = 1; item->rec.quantity >= scan_index; scan_index++) {
                                        inner_index = 1;
                                        do {
                                            if ((sub->rec.u.spells - 1)[inner_index] == spell) {
                                                (sub->rec.u.spells - 1)[inner_index] =
                                                    (sub->rec.u.spells - 1)[inner_index] | 0x80;
                                                found = 1;
                                            }
                                            inner_index++;
                                        } while (inner_index <= 3 && !found);
                                        sub = sub->more;
                                        if (found)
                                            scan_index = item->rec.quantity;
                                    }
                                } else {
                                    inner_index = 1;
                                    do {
                                        if ((item->rec.u.spells - 1)[inner_index] == spell) {
                                            (item->rec.u.spells - 1)[inner_index] =
                                                (item->rec.u.spells - 1)[inner_index] | 0x80;
                                            found = 1;
                                        }
                                        inner_index++;
                                    } while (inner_index <= 3 && !found);
                                }
                                item = item->next;
                            }
                        } else
                            bottom_msg("You can not scribe that spell.");
                    }
                }
            }
        }
        if (index != -1) {
            index = -1;
            spell = spell_menu(6, 9, &index, &not_empty);
            if (spell == 0)
                scribe_reset(cur_node);
        }
        if (redraw)
            view_redraw();
    }
}

void far fx_line_add(char title, char far *text,
                     struct Node far * far *cur)                 /* 25a4:1246 */
{
    pool_alloc((void far * far *)*cur, sizeof(struct Node), node_pool);
    *cur = (*cur)->next;
    (*cur)->next = 0;
    str_copy((*cur)->text, text);
    (*cur)->key = title;
}

void far effects_menu(void)                                      /* 25a4:12b6 */
{
    struct Node far *head;
    struct Node far *cur;
    struct Character far *character;
    unsigned char count;
    char redraw;
    char show;
    char text[42];
    int index;
    unsigned char id;
    struct Effect far *effect;

    pool_alloc((void far * far *)&head, sizeof(struct Node), node_pool);
    cur = head;
    cur->key = 1;
    cur->next = 0;
    character = node_list;
    str_copy(cur->text, character->name);
    while (character) {
        count = 0;
        if (character != node_list)
            fx_line_add(1, character->name, &cur);
        for (effect = character->effects; effect; effect = effect->next) {
            show = 1;
            id = effect->id;
            if (id == 1 || id == 2 || id == 4 || id == 5 || id == 6 || id == 8
                || id == 9 || id == 0xa || id == 0xc || id == 0xd || id == 0xe
                || id == 0xf || id == 0x10 || id == 0x11 || id == 0x13 || id == 0x14
                || id == 0x15 || id == 0x17 || id == 0x18 || id == 0x19 || id == 0x1c
                || id == 0x1d || id == 0x20 || id == 0x21 || id == 0x22 || id == 0x23
                || id == 0x24 || id == 0x25 || id == 0x26 || id == 0x27 || id == 0x29
                || id == 0x2a || id == 0x2d || id == 0x2e || id == 0x31 || id == 0x32
                || id == 0x36 || id == 0x37 || id == 0x38 || id == 0x39 || id == 0x3d
                || id == 0x3e || id == 0x3f || id == 0x44 || id == 0x45 || id == 0x47
                || id == 0x4a || id == 0x4b || id == 0x4e || id == 0x59 || id == 0x5c
                || id == 0x5d || id == 0x62 || id == 0x73 || id == 0x77 || id == 0x79
                || id == 0x7a || id == 0x7b || id == 0x7e || id == 0xac)
                str_copy(text, fx_names[effect->id]);
            else
                show = 0;
            if (show) {
                count++;
                fx_line_add(0, msg_format(" %s", text), &cur);
            }
        }
        if (count == 0)
            fx_line_add(0, " <No Spell Effects>", &cur);
        fx_line_add(0, " ", &cur);
        character = character->next;
    }
    redraw = 1;
    index = 0;
    frame_border();
    hotkey_reset(1);
    list_menu(empty_text, exit_items, 1, 4, 0x26, 0x16, head, 1, 0xff,
              &redraw, &index, &cur);
    list_free(&head);
    view_redraw();
}

void far magic_menu(char far *interrupted)                       /* 25a4:1671 */
{
    unsigned char scan_index;
    unsigned char count;
    unsigned char choice;
    char saved;

    choice = 0xff;
    saved = 1;
    while (*interrupted == 0 && choice != 5 && (!menu_escaped || choice != 0x1b)) {
        mem_fill(hotkeys, sizeof hotkeys, 0xff);
        count = 0;
        for (scan_index = 0; scan_index <= 5; scan_index++)
            if (scan_index != 4 || game_state[STATE_REST_ENCOUNTER_CHANCE] < 0x64 || game_state[STATE_REST_ENCOUNTER_INTERVAL] != 1)
                hotkey_add(scan_index, &count);
        ui_mode = saved;
        choice = node_prompt("", camp_menus[1], 1, 1);
        saved = ui_mode;
        if (menu_escaped) {
            party_rows(cur_node, 0);
            party_key_select(choice);
            party_rows(cur_node, 1);
        } else
            switch (choice) {
            case 0:
                cast_menu();
                break;
            case 1:
                memorize_menu();
                break;
            case 2:
                scribe_menu();
                break;
            case 3:
                effects_menu();
                break;
            case 4:
                study_rest(interrupted);
                break;
            }
    }
}

void far order_up(void)                                          /* 25a4:179b */
{
    struct Character far *prev;
    struct Character far *character;
    struct Character far *after;

    character = node_list;
    prev = 0;
    if (character == cur_node)
        while (character->next)
            character = character->next;
    else
        while (character->next != cur_node) {
            prev = character;
            character = character->next;
        }
    after = cur_node->next;
    if (node_list == cur_node) {
        if (after)
            node_list = after;
        character->next = cur_node;
        cur_node->next = 0;
    } else {
        character->next = after;
        cur_node->next = character;
        if (character == node_list || prev == 0)
            node_list = cur_node;
        else
            prev->next = cur_node;
    }
}

void far order_down(void)                                        /* 25a4:18b6 */
{
    struct Character far *character;
    struct Character far *after;

    character = node_list;
    if (character != cur_node)
        while (character->next != cur_node)
            character = character->next;
    after = cur_node->next;
    if (after == 0) {
        cur_node->next = node_list;
        node_list = cur_node;
        character->next = after;
    } else {
        cur_node->next = after->next;
        if (node_list == cur_node)
            node_list = after;
        if (character != cur_node)
            character->next = after;
        after->next = cur_node;
    }
}

void far order_menu(void)                                        /* 25a4:19a3 */
{
    unsigned char choice;
    char saved;

    set_step_mode(0);
    choice = 0xff;
    saved = 1;
    while (choice != 2 && (!menu_escaped || choice != 0x1b)) {
        mem_fill(hotkeys, sizeof hotkeys, 0xff);
        if (step_mode())
            hotkeys[0].key = 1;
        else
            hotkeys[0].key = 0;
        hotkeys[1].key = 2;
        ui_mode = saved;
        choice = node_prompt(order_title, order_items, 1, 1);
        saved = ui_mode;
        if (menu_escaped) {
            if (!step_mode()) {
                party_rows(cur_node, 0);
                party_key_select(choice);
                party_rows(cur_node, 1);
            } else {
                switch (choice) {
                case 0x87:
                case 0x88:
                    order_up();
                    break;
                case 0x84:
                case 0x85:
                    order_down();
                    break;
                }
                party_display(cur_node);
            }
        } else if (choice < 2 || menu_escaped && choice == 0x1b) {
            set_step_mode(!step_mode());
            if (step_mode())
                hero_text(cur_node, selected_text, 10, 0);
            else
                combat_text_clear();
        }
    }
    set_step_mode(0);
    order_mode = 0;
}

void far level_menu(void)                                        /* 25a4:1b15 */
{
    unsigned char choice;
    unsigned char value;

    value = game_state[STATE_DIFFICULTY];
    ui_mode = value;
    hotkey_reset(4);
    choice = menu_run(empty_text, camp_menus[3], 0, 1);
    if (menu_escaped) {
        party_rows(cur_node, 0);
        party_key_select(choice);
        party_rows(cur_node, 1);
    } else if (choice < 5)
        value = choice + 1;
    game_state[STATE_DIFFICULTY] = value;
}

void far speed_menu(void)                                        /* 25a4:1bab */
{
    unsigned char count;
    unsigned char choice;
    char saved;

    saved = 1;
    do {
        status_printf(1, 0x12, 7, 0,
                      msg_format("%s%s", speed_text, byte_str(game_state[STATE_SPEED])));
        mem_fill(hotkeys, sizeof hotkeys, 0xff);
        count = 0;
        if (game_state[STATE_SPEED] > 0)
            hotkey_add(0, &count);
        if (game_state[STATE_SPEED] < 9)
            hotkey_add(1, &count);
        hotkey_add(2, &count);
        ui_mode = saved;
        choice = menu_run(speed_title, speed_items, 1, 1);
        saved = ui_mode;
        if (menu_escaped)
            switch (choice) {
            case 0x1b:
                choice = 2;
                break;
            case 0x84:
                if (game_state[STATE_SPEED] > 0)
                    game_state[STATE_SPEED]--;
                break;
            case 0x88:
                if (game_state[STATE_SPEED] < 9)
                    game_state[STATE_SPEED]++;
                break;
            }
        else
            switch (choice) {
            case 0:
                if (game_state[STATE_SPEED] > 0)
                    game_state[STATE_SPEED]--;
                break;
            case 1:
                if (game_state[STATE_SPEED] < 9)
                    game_state[STATE_SPEED]++;
                break;
            }
    } while (choice != 2);
    combat_text_clear();
}

void far alter_menu(void)                                        /* 25a4:1ce2 */
{
    unsigned char choice;
    char saved;

    choice = 0xff;
    saved = 1;
    ui_mode = 1;
    while (choice != 5) {
        hotkey_reset(5);
        choice = node_prompt("", camp_menus[2], 1, 1);
        saved = ui_mode;
        if (menu_escaped) {
            if (choice == 0x1b)
                choice = 5;
            else {
                party_rows(cur_node, 0);
                party_key_select(choice);
                party_rows(cur_node, 1);
            }
        } else {
            switch (choice) {
            case 0:
                order_menu();
                break;
            case 1:
                if (game_state[STATE_PARTY_COUNT] > 1)
                    party_drop();
                break;
            case 2:
                speed_menu();
                break;
            case 3:
                if (!(cur_node->npc & 0x80)) {
                    icon_customize();
                    pic_release();
                    view_redraw();
                }
                break;
            case 4:
                level_menu();
                break;
            }
            ui_mode = saved;
        }
    }
}

unsigned char far levels_lost(struct Character far *character)                /* 25a4:1dd9 */
{
    char result;
    unsigned char sum;
    short class_index;

    sum = 0;
    for (class_index = 0; class_index <= 6; class_index++)
        if (character->level[class_index] < character->level_max[class_index])
            sum += character->level_max[class_index] - character->level[class_index];
    result = sum;
    return result;
}

/* The counters are pointers, as DKK's var parameters were; two of the seven
 * are moved rather than counted: restore_need at 25a4:1e8c and raise_need at
 * 1ea0 step the pointer, as shipped, so the caller's counts stay 0. */
void far fix_needs(unsigned far *raise_need, unsigned far *curse_need,
                   unsigned far *sight_need, unsigned far *heal_need,
                   unsigned far *detox_need, unsigned far *flesh_need,
                   unsigned far *restore_need)                   /* 25a4:1e31 */
{
    struct Character far *character;
    long junk;

    *restore_need = 0;
    *raise_need = 0;
    *curse_need = 0;
    *sight_need = 0;
    *heal_need = 0;
    *detox_need = 0;
    *flesh_need = 0;
    for (character = node_list; character; character = character->next) {
        restore_need += levels_lost(character);
        if (character->status == 6 && character->race > 1)
            raise_need++;
        if (character->status == 7 && !has_ready_8c(character))
            (*flesh_need)++;
        if (find_effect(character, 0x24, &junk))
            (*curse_need)++;
        if (find_effect(character, 0x37, &junk))
            (*detox_need)++;
        if (find_effect(character, 0x44, &junk))
            (*heal_need)++;
        else if (find_effect(character, 0x21, &junk))
            (*sight_need)++;
    }
}

char far hero_ok(struct Character far *character)                             /* 25a4:1f60 */
{
    char result;

    result = character->status == 0;
    return result;
}

void far fix_spells(unsigned far *total, unsigned far *resurrect,
                    unsigned far *sight, unsigned far *flesh,
                    unsigned far *curse, unsigned far *raise,
                    unsigned far *detox, unsigned far *heal)     /* 25a4:1f81 */
{
    unsigned short score;
    struct Character far *character;

    score = 0;
    for (character = node_list; character; character = character->next)
        if (hero_ok(character)) {
            if (character->known[0] & bit_mask[2])
                score += character->slots[0][0] * 9;
            if (character->known[7] & bit_mask[1])
                score += character->slots[0][3] * 0x11;
            if (character->known[8] & bit_mask[6])
                score += character->slots[0][4] * 0x1b;
            if (character->known[4] & bit_mask[3]) {
                score += character->slots[0][5] * 0x32;
                *heal += character->slots[0][5];
            }
            if (character->known[8] & bit_mask[2])
                *detox += character->slots[0][3];
            if (character->known[9] & bit_mask[2])
                *raise += character->slots[0][4];
            if (character->known[5] & bit_mask[2])
                *curse += character->slots[0][2];
            if (character->known[0xb] & bit_mask[0])
                *curse += character->slots[2][2];
            if (character->known[0xe] & bit_mask[0])
                *flesh += character->slots[2][5];
            if (character->known[4] & bit_mask[4])
                *sight += character->slots[0][2];
            if (character->known[0xd] & bit_mask[0])
                *resurrect += character->slots[0][5];
        }
    *total += score;
}

void far heal_roll(unsigned far *total, unsigned far *cure_4d8_count, unsigned far *cure_3d8_count,
                   unsigned far *cure_2d8_count, unsigned far *cure_1d8_count)           /* 25a4:2103 */
{
    unsigned short score;
    unsigned char index;

    score = 0;
    for (index = 1; index <= *cure_1d8_count; index++)
        score += fx_roll_dice(1, 8);
    for (index = 1; index <= *cure_2d8_count; index++)
        score += fx_roll_dice(2, 8) + 1;
    for (index = 1; index <= *cure_3d8_count; index++)
        score += fx_roll_dice(3, 8) + 3;
    for (index = 1; index <= *cure_4d8_count; index++)
        score += fx_roll_dice(4, 8) + 3;
    *total += score;
}

unsigned far party_wounds(void)                                  /* 25a4:21ba */
{
    unsigned short result;
    unsigned short sum;
    struct Character far *character;

    sum = 0;
    for (character = node_list; character; character = character->next)
        sum += character->hp_max - character->hp;
    result = sum;
    return result;
}

void far heal_clock(unsigned far *cure_4d8_count, unsigned far *cure_3d8_count, unsigned far *cure_2d8_count,
                    unsigned far *cure_1d8_count, unsigned far *score)       /* 25a4:2202 */
{
    struct Character far *character;
    unsigned short maximum;
    unsigned short time1;
    unsigned short time2;
    unsigned short time3;
    unsigned short time4;
    unsigned short time5;
    unsigned short threshold;
    unsigned short value;
    unsigned short divisor;

    character = node_list;
    time1 = 0;
    time2 = 0;
    time3 = 0;
    time4 = 0;
    time5 = 0;
    *cure_1d8_count = 0;
    *cure_2d8_count = 0;
    *cure_3d8_count = 0;
    *cure_4d8_count = 0;
    maximum = 0;
    while (character) {
        threshold = 0;
        if (hero_ok(character) && character->stats[2].cur > 3) {
            value = character->slots[0][0];
            *cure_1d8_count += value;
            time1 = value * 15;
            value = character->slots[1][1];
            *cure_1d8_count += value;
            time2 = value * 30;
            *cure_2d8_count += character->slots[0][3];
            time3 = character->slots[0][3] * 60;
            *cure_3d8_count += character->slots[0][4];
            time4 = character->slots[0][4] * 75;
            *cure_4d8_count += character->slots[0][5];
            time5 = character->slots[0][5] * 90;
        }
        threshold += time1 + time2 + time3 + time4 + time5;
        if (maximum < threshold)
            maximum = threshold;
        character = character->next;
    }
    if (party_wounds() < *score) {
        divisor = *score / party_wounds();
        maximum = maximum / divisor;
    }
    rest_time[3] = maximum / 60;
    rest_time[2] = (maximum - 60 * rest_time[3]) / 10;
    rest_time[1] = maximum % 10;
}

void far cure_clock(unsigned far *curse_need, unsigned far *sight_need,
                    unsigned far *detox_need, unsigned far *flesh_need,
                    unsigned far *heal_need, unsigned far *raise_need,
                    unsigned far *restore_need, unsigned far *sight,
                    unsigned far *curse, unsigned far *detox,
                    unsigned far *raise, unsigned far *flesh,
                    unsigned far *heal, unsigned far *resurrect,
                    unsigned far *restore)                       /* 25a4:239c */
{
    struct Character far *character;
    unsigned short score;

    for (character = node_list; character; character = character->next)
        if (hero_ok(character)) {
            *restore += character->slots[0][6];
            *resurrect += character->slots[0][6];
            *heal += character->slots[0][5];
            *flesh += character->slots[2][5];
            *raise += character->slots[0][4];
            *detox += character->slots[0][3];
            *curse += character->slots[2][3];
            *curse += character->slots[0][2];
            *sight += character->slots[0][2];
        }
    score = 0;
    score = score + umin(*restore, *restore_need) * 0x69;
    score = score + umin(*resurrect, *raise_need) * 0x69;
    score = score + umin(*heal, *heal_need) * 0x5a;
    score = score + umin(*flesh, *flesh_need) * 0x5a;
    if (*resurrect == 0) {
        score = score + umin(*raise, *raise_need) * 0x4b;
        *raise = 0;
    }
    score = score + umin(*detox, *detox_need) * 0x3c;
    score = score + umin(*sight, *sight_need) * 0x2d;
    score = score + umin(*curse, *curse_need) * 0x2d;
    rest_time[3] = score / 60;
    rest_time[2] = (score - 60 * rest_time[3]) / 10;
    rest_time[1] = score % 10;
}

void far heal_party(unsigned far *total)                         /* 25a4:259d */
{
    struct Character far *character;
    unsigned short amount;

    for (character = node_list; character; character = character->next)
        if (character->hp_max > character->hp) {
            amount = character->hp_max - character->hp;
            if (*total < amount)
                amount = *total;
            if (amount < 1)
                amount = 0;
            if (amount > 0)
                if (fx_heal(character, amount, 0))
                    if (*total >= amount)
                        *total -= amount;
        }
}

/* restore is stepped as a pointer at 25a4:266a, as shipped, not counted
 * down.  The blindness cure removes effect 0x21 from cur_node rather than
 * h and never counts sight down, both as DKK's applyRecovery does. */
void far fix_cures(unsigned far *sight, unsigned far *curse,
                   unsigned far *raise, unsigned far *detox,
                   unsigned far *flesh, unsigned far *heal,
                   unsigned far *restore, unsigned far *resurrect) /* 25a4:2638 */
{
    struct Character far *character;
    long junk;
    char changed;
    unsigned char damage;
    unsigned char factor;
    short index;

    for (character = node_list; character; character = character->next) {
        changed = 0;
        while (levels_lost(character) > 0 && *restore > 0) {
            spell_target = character;
            current_spell = 0x66;
            spell_run(0x66);
            restore--;
        }
        if (*heal > 0 && find_effect(character, 0x44, &junk)) {
            fx_remove(character, 0x44, 0);
            if (find_effect(character, 0x21, &junk))
                fx_remove(character, 0x21, 0);
            changed = 1;
            hero_text(character, "is healed", 10, 1);
            (*heal)--;
            fx_stat_recalc(character, 1);
            fx_stat_recalc(character, 2);
        }
        if (*flesh > 0 && character->status == 7 && !has_ready_8c(character)) {
            character->status = 0;
            character->enabled = 1;
            character->hp = 1;
            hero_text(character, "is changed back to flesh", 10, 1);
            (*flesh)--;
        }
        if (*detox > 0 && find_effect(character, 0x37, &junk)) {
            if (character->hp == 0)
                character->hp = 1;
            fx_lock = 1;
            fx_remove(character, 0x37, 0);
            fx_remove(character, 0x16, 0);
            fx_remove(character, 0xf, 0);
            fx_lock = 0;
            character->enabled = 1;
            character->status = 0;
            hero_text(character, "is detoxified", 10, 1);
            (*detox)--;
        }
        if ((*raise > 0 || *resurrect > 0) && character->status == 6 && character->race > 1) {
            fx_lock = 1;
            fx_remove(character, 0x37, 0);
            fx_lock = 0;
            character->hp = 1;
            character->status = 0;
            character->enabled = 1;
            if (*resurrect > 0) {
                *raise = 0;
                hero_text(character, "has been resurrected", 10, 1);
                (*resurrect)--;
            } else {
                if (character->stats[4].cur > 0 && *raise > 0)
                    character->stats[4].cur--;
                if (character->hp_max > character->hp_base)
                    damage = character->hp_max - character->hp_base;
                else
                    damage = 0;
                factor = 0;
                if (character->stats[4].cur >= 0xe) {
                    for (index = 0; index <= 6; index++)
                        if (character->level[index] > 0)
                            if (index == 2 || index == 3 || index == 4 || index == 1)
                                factor += (character->stats[4].cur - 14) * character->level[index];
                            else if (character->stats[4].cur > 0xf)
                                factor += character->level[index] * 2;
                            else
                                factor += character->level[index];
                    if (factor > 0)
                        damage = damage / factor;
                    if (character->stats[4].cur < 0x11 || character->level[2] > 0 || character->level[3] > 0
                        || character->level[2] > character->byte_92)
                        character->hp_max -= damage;
                }
                hero_text(character, "has been raised from the dead", 10, 1);
                (*raise)--;
            }
        }
        if (*curse > 0 && find_effect(character, 0x24, &junk)) {
            spell_target = character;
            spell_remove_curse();
            hero_text(character, "is relieved of a curse", 10, 1);
            (*curse)--;
        }
        if (!changed && *sight > 0 && find_effect(character, 0x21, &junk)) {
            fx_remove(cur_node, 0x21, 0);
            hero_text(cur_node, "regains sight", 10, 1);
        }
    }
}

void far fix_menu(char far *interrupted)                         /* 25a4:2ad0 */
{
    unsigned total;
    unsigned cure_1d8_count;
    unsigned cure_2d8_count;
    unsigned cure_3d8_count;
    unsigned saved[7];
    unsigned resurrect;
    unsigned raise;
    unsigned raise_need;
    unsigned curse;
    unsigned curse_need;
    unsigned sight;
    unsigned sight_need;
    unsigned heal;
    unsigned heal_need;
    unsigned detox;
    unsigned detox_need;
    unsigned flesh;
    unsigned flesh_need;
    unsigned restore;
    unsigned restore_need;
    unsigned cure_4d8_count;

    if (game_state[STATE_MAGIC_DISABLED] == 0) {
        raise = 0;
        curse = 0;
        sight = 0;
        heal = 0;
        detox = 0;
        flesh = 0;
        resurrect = 0;
        restore = 0;
        *interrupted = 0;
        total = 0;
        if (party_wounds()) {
            fix_spells(&total, &resurrect, &sight, &flesh, &curse, &raise, &detox, &heal);
            mem_move(rest_time, saved, sizeof saved);
            heal_clock(&cure_4d8_count, &cure_3d8_count, &cure_2d8_count, &cure_1d8_count, &total);
            no_study = 1;
            *interrupted = camp_rest(0);
            no_study = 0;
            if (!*interrupted) {
                heal_roll(&total, &cure_4d8_count, &cure_3d8_count, &cure_2d8_count, &cure_1d8_count);
                fix_cures(&sight, &curse, &raise, &detox, &flesh, &heal, &restore,
                          &resurrect);
                heal_party(&total);
                party_display(cur_node);
                party_info_display();
                mem_move(saved, rest_time, sizeof saved);
                mem_move(rest_time, saved, sizeof saved);
                fix_needs(&raise_need, &curse_need, &sight_need, &heal_need, &detox_need,
                          &flesh_need, &restore_need);
                cure_clock(&curse_need, &sight_need, &detox_need, &flesh_need,
                           &heal_need, &raise_need, &restore_need, &sight, &curse,
                           &detox, &raise, &flesh, &heal, &resurrect, &restore);
                no_study = 1;
                *interrupted = camp_rest(0);
                no_study = 0;
                if (!*interrupted) {
                    fix_cures(&sight, &curse, &raise, &detox, &flesh, &heal, &restore,
                              &resurrect);
                    party_display(cur_node);
                    party_info_display();
                    mem_move(saved, rest_time, sizeof saved);
                }
            }
        }
    }
}

void far camp_menu(char far *interrupted)                        /* 25a4:2d3c */
{
    unsigned char choice;
    char done;
    char saved;
    unsigned char count;

    saved_view_mode = view_mode;
    view_mode = 2;
    heal_ticks = 0;
    mem_fill(rest_time, sizeof rest_time, 0);
    str_copy(pic_saved, pic_name);
    view_kind_saved = view_kind;
    view_redraw();
    text_clear(1, 0x11, 0x26, 0x16);
    status_printf(1, 0x12, 7, 0, camp_text);
    scribe_reset_all();
    *interrupted = 0;
    choice = 0xff;
    saved = 1;
    while (*interrupted == 0 && choice != 7) {
        mem_fill(hotkeys, sizeof hotkeys, 0xff);
        count = 0;
        for (tick_slot = 0; tick_slot <= 7; tick_slot++)
            if (tick_slot != 2 && tick_slot != 4 || game_state[STATE_REST_ENCOUNTER_CHANCE] < 0x64
                || game_state[STATE_REST_ENCOUNTER_INTERVAL] != 1)
                hotkey_add(tick_slot, &count);
        ui_mode = saved;
        choice = node_prompt(empty_text, camp_menus[0], 1, 1);
        saved = ui_mode;
        if (menu_escaped) {
            if (choice == 0x1b)
                choice = 7;
            else {
                party_rows(cur_node, 0);
                party_key_select(choice);
                party_rows(cur_node, 1);
            }
        } else {
            switch (choice) {
            case 0:
                char_view(&done);
                break;
            case 1:
                magic_menu(interrupted);
                break;
            case 2:
                study_rest(interrupted);
                break;
            case 3:
                alter_menu();
                break;
            case 4:
                fix_menu(interrupted);
                break;
            case 5:
                if (cur_node == 0 || game_saved
                    || confirm("Game not saved. Load anyway? ", 0)) {
                    pending_load_set(1);
                    adventure_exit = 1;
                    return;
                }
                break;
            case 6:
                game_save_menu();
                if (confirm(exit_text, 0))
                    game_exit();
                break;
            }
            if (choice != 7)
                box_draw();
        }
    }
    scribe_reset_all();
    view_mode = saved_view_mode;
    party_info_display();
    combat_text_clear();
    box_draw();
}
