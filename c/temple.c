/* DQK.EXE overlay 08 module 2910: the temple.  The shop menu (heal, view,
 * repair, exit), the heal menu's eleven services -- curing blindness,
 * disease and poison, healing points, raising the dead, removing a curse,
 * stone to flesh and restoring lost levels -- and repairing every party
 * member at once.
 *
 * EXE 0x04c8b1..0x04d499, one Turbo C large-model _TEXT segment starting at
 * byte 1 of paragraph 2910, the last module of RTLink section 8 (payload
 * 0x491f0, link segment 25a4, parent section 3).  Module 280a's final retf
 * is byte 0 of paragraph 2910; section 8's table files every fixup site
 * after it under frame 2910, and seven zero bytes follow this module's
 * final retf to the section's end.  Built with Turbo C 2.01, tcc -ml -O -Z
 * -d.  Functions are defined in shipped order; bake consumes them in that
 * order.  Code outside the section reaches temple_shop through vector stub
 * 1d99:1251.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, an ovlNN_ name which section and stored
 * segment, and a mod1d99_ callee is an RTLink vector stub whose target is
 * named beside it.
 *
 * The routines follow the order of DKK's temple.pas (dkk-disassembly), with
 * heal_all added before temple_shop.  _DATA is heal_menu's spell map and
 * seven literals, DGROUP 82a4..8302.  The module declares no _BSS; pooled,
 * which money.c and 280a also set and nothing reads, is extern here.
 */

#include "dqk.h"

char far confirm_anyway(char far *msg)                           /* 2910:0001 */
{
    char confirmation;

    hero_text(cur_node, msg, 0, 0);
    confirmation = confirm(cure_anyway_text, 0);
    combat_text_clear();
    return confirmation;
}

void far cure_blind(void)                                        /* 2910:0045 */
{
    long eff;
    char unset_message[41];
    char confirmation;

    confirmation = 1;
    if (!find_effect(cur_node, 0x21, &eff))
        confirmation = confirm_anyway(not_blind_text);
    if (confirmation == 1)
        fx_remove(cur_node, 0x21, 0);
    hero_text(cur_node, unset_message, 10, 1);                               /* s is never set, as shipped */
}

void far cure_disease(void)                                      /* 2910:00b8 */
{
    long eff;
    char message[41];
    unsigned char index;
    char found;

    found = 0;
    for (index = 1; index <= 7; index++)
        if (find_effect(cur_node, disease_fx[index], &eff)) {
            found = 1;
            index = 7;
        }
    if (!found)
        str_copy(message, "is not Diseased");
    else {
        fx_lock = 1;
        for (index = 1; index <= 7; index++)
            fx_remove(cur_node, disease_fx[index], 0);
        fx_stat_recalc(cur_node, 0);
        fx_lock = 0;
        str_copy(message, "is cured");
    }
    hero_text(cur_node, message, 0, 1);
}

void far heal_points(unsigned char kind)                         /* 2910:0193 */
{
    unsigned char hp;

    switch (kind) {
    case 1:
        hp = fx_roll_dice(1, 8);
        fx_heal(cur_node, hp, 0);
        break;
    case 2:
        hp = fx_roll_dice(2, 8) + 1;
        fx_heal(cur_node, hp, 0);
        break;
    case 3:
        hp = fx_roll_dice(3, 8) + 3;
        fx_heal(cur_node, hp, 0);
        break;
    case 4:
        hp = fx_roll_dice(1, 4);
        if (cur_node->hp + hp > cur_node->hp_max)
            hp = 0;
        else
            hp = cur_node->hp_max - cur_node->hp - hp;
        fx_heal(cur_node, hp, 0);
        fx_remove(cur_node, 0x21, 0);
        fx_remove(cur_node, 0x44, 0);
        fx_stat_recalc(cur_node, 1);
        fx_stat_recalc(cur_node, 2);
        break;
    }
}

void far raise_dead(void)                                        /* 2910:02c0 */
{
    char confirmation;
    char dead;
    struct Character far *hero;
    char cannot;
    char gone;

    hero = cur_node;
    dead = 0;
    cannot = 0;
    confirmation = 1;
    if (hero->status == 6 || hero->status == 1)
        dead = 1;
    gone = hero->status == 8;
    if (hero->race < 2)
        cannot = 1;
    if (!dead || cannot) {
        if (cannot)
            confirmation = confirm_anyway(cannot_raise_text);
        else if (gone)
            confirmation = confirm_anyway(beyond_help_text);
        else
            confirmation = confirm_anyway(not_dead_text);
    }
    if (confirmation == 1 && dead && !cannot && !gone) {
        spell_target = hero;
        current_spell = 0x4b;
        spell_run(0x4b);
    }
}

void far cure_poison(void)                                       /* 2910:0394 */
{
    long eff;
    char confirmation;
    char found;

    found = 0;
    confirmation = 1;
    if (find_effect(cur_node, 0x37, &eff))
        found = 1;
    if (!found)
        confirmation = confirm_anyway("is not poisoned.");
    if (confirmation == 1 && found && find_effect(cur_node, 0x37, &eff)) {
        if (cur_node->hp == 0)
            cur_node->hp = 1;
        fx_lock = 1;
        fx_remove(cur_node, 0x37, 0);
        fx_remove(cur_node, 0x16, 0);
        fx_remove(cur_node, 0x0f, 0);
        fx_lock = 0;
        cur_node->enabled = 1;
        cur_node->status = 0;
    }
}

void far remove_curse(void)                                      /* 2910:0479 */
{
    char cursed;
    struct Item far *item;
    long eff;
    char confirmation;

    confirmation = 1;
    cursed = 0;
    item = cur_node->items;
    while ((item != 0) & !cursed) {          /* & not &&, as shipped */
        if (item->rec.cursed != 0)
            cursed = 1;
        item = item->next;
    }
    if (!cursed && !find_effect(cur_node, 0x24, &eff))
        confirmation = confirm_anyway(not_cursed_text);
    else
        confirmation = 1;
    if (confirmation == 1) {
        spell_target = cur_node;
        spell_remove_curse();
    }
}

void far stone_flesh(void)                                       /* 2910:0524 */
{
    char confirmation;

    confirmation = 1;
    if (cur_node->status != 7)
        confirmation = confirm_anyway(not_stoned_text);
    if (confirmation == 1 && cur_node->status == 7) {
        if (has_ready_8c(cur_node))
            hero_text(0, "spell dosn't work!", 10, 0);
        else {
            cur_node->status = 0;
            cur_node->enabled = 1;
            cur_node->hp = 1;
        }
    }
}

void far restore_levels(void)                                    /* 2910:059e */
{
    char confirmation;
    char lower;
    short class_index;

    lower = 0;
    confirmation = 1;
    for (class_index = 0; class_index <= 6; class_index++)
        if (cur_node->level[class_index] < cur_node->level_max[class_index])
            lower = 1;
    if (!lower)
        confirmation = confirm_anyway(no_lost_text);
    if (confirmation == 1) {
        spell_target = cur_node;
        current_spell = 0x66;
        spell_run(0x66);
    }
}

void far heal_menu(void)                                         /* 2910:0615 */
{
    int map[11] = { 0x25, 0x27, 0x03, 0x3a, 0x47, 0x24, 0x43, 0x4b, 0x2b,
                    0x71, 0x66 };       /* spell_names index per service */
    struct Node far *list;
    struct Node far *sel;
    char redraw;
    unsigned char index;
    int idx;
    char done;
    unsigned char menu_choice;
    char num[11];
    unsigned char len;
    char color;

    done = 0;
    list = 0;
    sel = 0;
    list_alloc(11, &list);
    sel = list;
    index = 1;
    while (list != 0) {
        str_copy(list->text, (spell_names - 1)[map[index - 1]]);
        list->key = 0;
        list = list->next;
        index++;
    }
    list = sel;
    box_draw();
    redraw = 1;
    frame_lower_panel();
    idx = 0;
    do {
        text_clear(1, 0x13, 0x26, 0x13);
        status_printf(1, 0x13, 0x0b, 0, "%s", cur_node->name);
        str_copy(num, int_str(cur_node->hp));
        len = str_length(num);
        if (cur_node->hp != cur_node->hp_max)
            color = 0;
        else
            color = 0x0b;
        status_printf(0x12, 0x13, color, 0, num);
        str_format(num, "/%s", int_str(cur_node->hp_max));
        status_printf(len + 0x12, 0x13, 0x0b, 0, num);
        status_printf(1, 1, 0x0b, 0, "%s%s", cur_node->name, help_you_text);
        hotkey_reset(1);
        menu_choice = list_menu(empty_text, heal_items, 2, 3, 0x26, 0x0f, list, 1, 0,
                      &redraw, &idx, &sel);
        if (menu_escaped) {
            if (menu_choice == 0x1b)
                done = 1;
        } else
            switch (menu_choice) {
            case 0:
                switch (idx) {
                case 0:
                    cure_blind();
                    break;
                case 1:
                    cure_disease();
                    break;
                case 2:
                    heal_points(1);
                    break;
                case 3:
                    heal_points(2);
                    break;
                case 4:
                    heal_points(3);
                    break;
                case 5:
                    heal_points(4);
                    break;
                case 6:
                    cure_poison();
                    break;
                case 7:
                    raise_dead();
                    break;
                case 8:
                    remove_curse();
                    break;
                case 9:
                    stone_flesh();
                    break;
                case 10:
                    restore_levels();
                    break;
                case 11:
                    done = 1;
                    break;
                }
                break;
            case 1:
                done = 1;
                break;
            }
    } while (!done);
    list_free(&list);
    view_redraw();
    party_display(cur_node);
}

void far heal_all(void)                                          /* 2910:08fa */
{
    unsigned char index;
    short scan_index;
    struct Character far *hero;
    short class_index;
    char lower;
    long eff;

    hero = node_list;
    while (hero != 0) {
        fx_lock = 1;
        fx_remove(hero, 0x44, 0);
        fx_remove(hero, 0x21, 0);
        for (index = 1; index <= 7; index++)
            fx_remove(hero, disease_fx[index], 0);
        fx_lock = 0;
        if (find_effect(hero, 0x37, &eff)) {
            if (hero->hp == 0)
                hero->hp = 1;
            fx_lock = 1;
            fx_remove(hero, 0x37, 0);
            fx_remove(hero, 0x16, 0);
            fx_remove(hero, 0x0f, 0);
            fx_lock = 0;
            hero->enabled = 1;
            hero->status = 0;
        }
        if (hero->status == 6 && hero->race != 0 && hero->race != 1) {
            spell_target = hero;
            current_spell = 0x69;
            spell_run(0x69);
        }
        if (hero->status == 7) {
            hero->status = 0;
            hero->enabled = 1;
        }
        do {
            lower = 0;
            for (class_index = 0; class_index <= 6; class_index++)
                if (hero->level[class_index] < hero->level_max[class_index])
                    lower = 1;
            if (lower) {
                spell_target = hero;
                current_spell = 0x66;
                spell_run(0x66);
            }
        } while (lower);
        for (scan_index = 0; scan_index <= 5; scan_index++)
            fx_stat_recalc(hero, scan_index);
        if (hero->status != 8 && hero->status != 6)
            hero->hp = hero->hp_max;
        if (hero->status == 4) {
            hero->status = 0;
            hero->enabled = 1;
        }
        hero = hero->next;
    }
}

void far temple_shop(void)                                       /* 2910:0af6 */
{
    unsigned char key;
    char done;

    view_mode = 1;
    shop_temple_frame = 0;
    view_redraw();
    shop_temple_frame = 1;
    party_display(cur_node);
    mem_zero(pool, 12);
    pooled = 0;
    done = 0;
    do {
        hotkey_reset(3);
        key = node_prompt(empty_text, temple_items, 1, 0);
        if (menu_escaped)
            switch (key) {
            case 0x1b:
                key = 3;
                break;
            case 0x84:
            case 0x88:
                party_key_select(key);
                key = 0xff;
                break;
            }
        switch (key) {
        case 0:
            heal_menu();
            break;
        case 1:
            char_view(&menu_escaped);
            break;
        case 2:
            heal_all();
            break;
        case 3:
            done = 1;
            break;
        }
        party_display(cur_node);
    } while (!done);
}
