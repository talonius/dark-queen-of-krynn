/* DQK.EXE overlay 11 module 2b3a: the character view.  The status page (class,
 * race, level, experience, the six abilities, the money and the combat
 * values), its menu of items, spells, trade, deposit or drop, lay on hands
 * and cure disease; the items menu (ready, use, trade, deposit, drop, halve,
 * join, sell and identify); trading and dropping money with the amount
 * prompt and the carry limit; picking a party member; and the hit points a
 * class level adds or takes away.
 *
 * EXE 0x05cbb0..0x0608ed, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 2b3a, RTLink section 11's first module (payload
 * 0x5cbb0, link segment 2b3a, no parent section).  Its last retf is byte c
 * of paragraph 2f0d, where view3d.c begins at 2f0d:000d.  Section 11's table
 * files all 360 fixup sites in the range under frame 2b3a.  Built with Turbo
 * C 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped order; bake
 * consumes them in that order.  Sections 9 and 10 hold other bytes at link
 * segment 2b3a.
 *
 * The routines follow DKK's charview.pas (dkk-disassembly) for the status
 * page, the menus, trading and healing, with money.pas's parseMoneyType,
 * inputNumberMax, the weight and carry-limit helpers, tradeMoney and
 * dropMoney placed among them.  use_item, input_amount and pick_hero each
 * carry a switch whose values are few and sparse, which Turbo C compiles to
 * a search of a value table in the code segment (cmp ax,[cs:bx]).
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.  Twenty-one of these
 * routines have RTLink vectors: 1d99:1463 -> 0000, 141d -> 0590, 1427 ->
 * 065e, 1413 -> 096c, 0fef -> 0b0c, 1003 -> 0f41, 1107 -> 1965, 1391 ->
 * 1f75, 150d -> 1fda, 14ef -> 24d9, 14e5 -> 2586, 14d1 -> 2769, 14db ->
 * 277c, 14c7 -> 2799, 1521 -> 2c32, 1517 -> 2fbf, 13d7 -> 34fc, 13a5 ->
 * 354b, 143b -> 380e, 1481 -> 3a5b, 1495 -> 3ae5.
 *
 * _DATA is DGROUP 8d72..9112, literals only.  _BSS is c9e8..c9ef: last_tick
 * and view_hero, just below view3d.c's geo.
 */

#include <stdlib.h>
#include "dqk.h"

extern void far status_msg(char far *fmt, char fg, char bg, ...); /* 0a62:0904 */
extern void far *far get_selected(void);                         /* 0bdb:02e1 */

static long last_tick;                                           /* c9e8 */
static struct Character far *view_hero;                               /* c9ec */

void far money_show(void);
void far combat_show(void);
void far stat_show(short stat, char active);
void far items_menu(char far *done);
void far item_equip_toggle(struct Item far *item);
static void far trade_item(struct Item far *item);
static void far halve_item(struct Item far *item);
void far join_item(struct Item far *item);
void far use_item(struct Item far *item, char far *done);
char far cant_carry(struct Character far *hero, char mode, struct Item far *item);
void far pick_hero(char far *prompt, char allow_cancel, char selection_mode, struct Character far * far *out);
char far hp_bonus(struct Character far *character, short cls);
static void far trade(void);
void far drop(char in_view);
char far can_heal(struct Character far *character);
char far can_cure(struct Character far *character);
void far heal(struct Character far *character);
void far cure(struct Character far *character);

void far char_status(void)                                       /* 2b3a:0000 */
{
    short stat;
    short class_index;
    char buf[0x52];
    char sep;
    char any;

    view_hero = cur_node;
    frame_character_sheet();
    character_name_show(view_hero, 1, 1, 0);
    status_printf(0x14, 1, 0xb, 0, "Status:");
    status_printf(0x1b, 1, 7, 0, status_names[view_hero->status]);
    str_copy(buf, sex_names[view_hero->gender]);
    status_printf(1, 3, 0xb, 0, buf);
    status_printf(8, 3, 0xb, 0, msg_format("%d years", view_hero->age));
    str_copy(buf, align_names[view_hero->alignment]);
    status_printf(1, 4, 0xb, 0, buf);
    str_copy(buf, race_names[view_hero->race]);
    status_printf(0x14, 4, 0xb, 0, buf);
    str_copy(buf, type_names[view_hero->cls]);
    if (view_hero->level[0] > 0)
        str_copy(buf, msg_format("%s%s%s", deity_names[view_hero->deity], " ", buf));
    if (view_hero->level[5] > 0) {
        tick_slot = 0;
        do
            tick_slot++;
        while ((buf[tick_slot - 1] != 'M' || buf[tick_slot + 1] != 'g')
               && str_length(buf) >= tick_slot);
        str_insert(msg_format("%s%s", robe_names[view_hero->robe_order], " "), buf, tick_slot);
    }
    if (view_hero->level[1] > 0)
        switch (view_hero->knight_order) {
        case 1:
            str_copy(buf, msg_format("%s%s", buf, " of the Crown"));
            break;
        case 2:
            str_copy(buf, msg_format("%s%s", buf, " of the Sword"));
            break;
        case 3:
            str_copy(buf, msg_format("%s%s", buf, " of the Rose"));
            break;
        }
    status_printf(1, 5, 0xb, 0, buf);
    status_printf(1, 7, 0xb, 0, "Level");
    sep = 0;
    str_copy(buf, "");
    any = 0;
    for (class_index = 0; class_index <= 6; class_index++)
        if (view_hero->level[class_index] > 0) {
            any = 1;
            if (sep)
                str_copy(buf, msg_format("%s%s", buf, "/"));
            str_copy(buf, msg_format("%s%s", buf, byte_str(view_hero->level[class_index])));
            sep = 1;
        }
    if (!any)
        str_copy(buf, "0");
    status_printf(7, 7, 0xb, 0, buf);
    status_printf(0x18, 6, 0xb, 0, "Experience:");
    str_copy(buf, ulong_str(view_hero->xp));
    status_printf((0x14 - str_length(buf)) / 2 + 0x14, 7, 7, 0, buf);
    for (stat = 0; stat <= 5; stat++) {
        switch (stat) {
        case 0:
            str_copy(buf, "STR ");
            break;
        case 1:
            str_copy(buf, "INT ");
            break;
        case 2:
            str_copy(buf, "WIS ");
            break;
        case 3:
            str_copy(buf, "DEX ");
            break;
        case 4:
            str_copy(buf, "CON ");
            break;
        case 5:
            str_copy(buf, "CHA ");
            break;
        }
        status_printf(1, stat + 9, 7, 0, buf);
        stat_show(stat, 0);
    }
    money_show();
    combat_show();
    if (view_hero->equipped[0])
        item_name(view_hero, view_hero->equipped[0], 1, 0x15, 0, 1);
    if (view_hero->equipped[2])
        item_name(view_hero, view_hero->equipped[2], 1, 0x16, 0, 1);
}

void far money_show(void)                                        /* 2b3a:0590 */
{
    char row;
    short index;
    char buf[0x2a];

    text_clear(0x14, 9, 0x26, 0xf);
    row = 9;
    for (index = 0; index <= 2; index++)
        if (cur_node->coins[index] > 0) {
            str_copy(buf, coin_names[index]);
            status_printf(0x14, row, 7, 0, buf);
            str_copy(buf, int_str(cur_node->coins[index]));
            status_printf(0x26 - str_length(buf), row, 7, 0, buf);
            row++;
        }
}

void far combat_show(void)                                       /* 2b3a:065e */
{
    struct Character far *character;
    char col;
    char buf[0x2a];

    character = cur_node;
    recalc_stats(character);
    status_printf(0x14, 3, 0xb, 0, "Hit Points ");
    text_clear(0x1f, 3, 0x25, 3);
    show_hp(character, 0x1f, 3, 0, 1);
    status_printf(1, 0x11, 0xb, 0, "Armor Class");
    if (character->ac[0] >= 1 && character->ac[0] <= 0x32)
        col = 0x10;
    else if (character->ac[0] >= 0x33 && character->ac[0] <= 0x3c)
        col = 0x11;
    else if (character->ac[0] >= 0x3d && character->ac[0] <= 0x45)
        col = 0x10;
    else
        col = 0xf;
    text_clear(0xf, 0x11, 0x11, 0x11);
    show_ac(character, col, 0x11);
    status_printf(0x14, 0x11, 0xb, 0, "Encumbrance");
    str_copy(buf, int_str(character->encumbrance));
    str_pad_left(buf, 5);
    status_printf(0x21, 0x11, 7, 0, buf);
    status_printf(1, 0x12, 0xb, 0, "THAC0\t ");
    int_to_str(0x3c - character->thac0_adj, buf);
    str_pad_left(buf, 2);
    status_printf(0x10, 0x12, 7, 0, buf);
    status_printf(0x14, 0x12, 0xb, 0, "Movement");
    calc_move(character);
    str_copy(buf, byte_str(character->movement));
    str_pad_left(buf, 3);
    status_printf(0x23, 0x12, 7, 0, buf);
    str_copy(buf, msg_format("%dd%s", character->rolls[0], byte_str(character->dice[0])));
    if (character->mods[0] > 0)
        str_copy(buf, msg_format("%s+%s", buf, byte_str(character->mods[0])));
    if (character->mods[0] < 0)
        str_copy(buf, msg_format("%s-%s", buf, byte_str(abs(character->mods[0]))));
    status_printf(1, 0x13, 0xb, 0, "Damage");
    str_pad_left(buf, 7);
    status_printf(0xb, 0x13, 7, 0, buf);
}

void far stat_show(short stat, char active)                      /* 2b3a:096c */
{
    char buf[0x2a];
    char col;
    char highlight_color;

    if (active)
        highlight_color = 0xb;
    else
        highlight_color = 7;
    col = 5;
    text_clear(col, stat + 9, 0xb, stat + 9);
    if (cur_node->stats[stat].cur < 10)
        col++;
    status_printf(col, stat + 9, highlight_color, 0, byte_str(cur_node->stats[stat].cur));
    if (stat == 0 && cur_node->stats[0].cur == 0x12 && cur_node->str_ex > 0) {
        str_copy(buf, byte_str(cur_node->str_ex));
        if (cur_node->str_ex < 10)
            str_copy(buf, msg_format("0%s", buf));
        if (cur_node->str_ex == 100)
            str_copy(buf, "00");
        status_printf(7, 9, highlight_color, 0, msg_format("(%s)", buf));
    }
    if (cur_node->stats[stat].cur != cur_node->stats[stat].base
        || stat == 0 && cur_node->str_ex_base != cur_node->str_ex)
        status_printf(0xc, stat + 9, highlight_color, 0, "*");
    else
        status_printf(0xc, stat + 9, highlight_color, 0, " ");
}

void far char_view(char far *done)                               /* 2b3a:0b0c */
{
    unsigned char menu_choice;
    char has_money;
    char has_spells;
    unsigned char spell_slot;
    short inner_index;
    int index;
    unsigned char options;
    char redraw;

    *done = 0;
    menu_choice = 0xff;
    selected_target = cur_node;
    char_status();
    mem_fill(hotkeys, 0x28, 0xff);
    redraw = 1;
    while (menu_choice != 7 && !*done) {
        options = 0;
        has_spells = 0;
        has_money = 0;
        for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
            if ((view_hero->spells[spell_slot] & 0x7f) > 0)
                has_spells = 1;
        for (inner_index = 0; inner_index <= 2; inner_index++)
            if (view_hero->coins[inner_index] > 0)
                has_money = 1;
        if (view_hero->items)
            hotkey_add(0, &options);
        if (has_spells)
            hotkey_add(1, &options);
        if ((view_hero->npc < 0x80 || view_hero->enabled == 0 || view_hero->status == 1) && has_money
            && view_mode != 5)
            hotkey_add(2, &options);
        if (has_money)
            if (view_mode == 10)
                hotkey_add(3, &options);
            else
                hotkey_add(4, &options);
        if (can_heal(cur_node) && view_mode != 10)
            hotkey_add(5, &options);
        if (can_cure(cur_node) && view_mode != 10)
            hotkey_add(6, &options);
        hotkey_add(7, &options);
        ui_mode = redraw;
        menu_choice = menu_run("", view_items, 0, 0);
        if (menu_escaped && menu_choice == 0x1b)
            menu_choice = 7;
        redraw = ui_mode;
        index = -1;
        switch (menu_choice) {
        case 0:
            items_menu(done);
            break;
        case 1:
            spell_slot = spell_menu(0, 0, &index, &has_spells);
            break;
        case 2:
            trade();
            break;
        case 3:
        case 4:
            drop(1);
            money_show();
            break;
        case 5:
            heal(cur_node);
            break;
        case 6:
            cure(cur_node);
            break;
        }
        if (!*done && menu_choice < 3)
            char_status();
    }
    if (view_mode == 5)
        combat_palette_set();
    view_redraw();
}

char far item_ok(struct Item far *item)                          /* 2b3a:0da0 */
{
    char ok;
    struct Item far *sub;

    ok = 0;
    if (item->rec.equipped)
        bottom_msg("Must be unreadied");
    else if (!item_casts(item))
        ok = 1;
    else if (item->rec.u.magic.charges & 0x80 || item->rec.u.magic.effect & 0x80
             || item->rec.u.magic.special & 0x80) {
        character_name_show(cur_node, 1, 0x15, 0);
        text_col = str_length(cur_node->name) + 2;
        text_row = 0x15;
        text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 0, " was going to scribe from that scroll");
        if (confirm("is it Okay to lose it? ", 0))
            ok = 1;
    } else if (item->rec.type == 'I') {
        sub = item;
        ok = 0;
        while (sub->more) {     /* 2b3a:0e7b tests item, never sub       */
            if (item->rec.u.magic.charges & 0x80 || item->rec.u.magic.effect & 0x80
                || item->rec.u.magic.special & 0x80) {
                character_name_show(cur_node, 1, 0x15, 0);
                text_col = str_length(cur_node->name) + 2;
                text_row = 0x15;
                text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 0,
                          " was going to scribe from that bundle");
                if (confirm("is it Okay to lose it? ", 0))
                    ok = 0;     /* 2b3a:0eff a yes leaves ok 0 too      */
                goto done;
            }
            sub = sub->more;
        }
        ok = 1;
    } else
        ok = 1;
done:
    text_clear(1, 0x15, 0x26, 0x16);
    return ok;
}

void far items_menu(char far *done)                              /* 2b3a:0f41 */
{
    unsigned char options;
    unsigned char menu_choice;
    char need_redraw;
    char redraw_box;
    char anim;
    struct Item far *item;
    struct Item far *sel;
    int index;
    struct Character far *character;
    unsigned char count;
    char redraw;
    struct Item far *picked;
    register int required_records;

    character = cur_node;
    sel = character->items;
    index = 0;
    redraw_box = 1;
    need_redraw = 1;
    menu_choice = 0xff;
    redraw = 1;
    while (menu_choice != 9 && !*done && character->item_count > 0) {
        count = character->item_count;
        if (character->items) {
            mem_fill(hotkeys, 0x28, 0xff);
            options = 0;
            hotkey_add(0, &options);
            if (character->enabled && game_state[STATE_MAGIC_DISABLED] == 0
                && (view_mode == 2 || view_mode == 3 || view_mode == 4
                    || view_mode == 5 && character->combat->can_use_item != 0))
                hotkey_add(1, &options);
            if ((character->npc < 0x80 || character->enabled == 0 || character->status == 1) && view_mode != 5)
                hotkey_add(2, &options);
            if (view_mode == 10)
                hotkey_add(3, &options);
            else
                hotkey_add(4, &options);
            if (character->item_count < 0x10)
                hotkey_add(5, &options);
            hotkey_add(6, &options);
            if ((character->npc < 0x80 || character->enabled == 0 || character->status == 1) && view_mode == 1
                && game_state[STATE_TEMPLE_SERVICE] != 1)
                hotkey_add(7, &options);
            if (view_mode == 1 && game_state[STATE_TEMPLE_SERVICE] != 1)
                hotkey_add(8, &options);
            hotkey_add(9, &options);
            for (item = character->items; item; item = item->next)
                item_name(character, item, 0, 0, 1, 0);
            if (need_redraw || inventory_redraw) {
                frame_list_panel();
                character_name_show(character, 1, 1, 1);
                status_printf(str_length(character->name) + 4, 1, 7, 0, items_label);
                status_printf(1, 3, 0xb, 0, ready_label);
                redraw_box = 1;
                need_redraw = 0;
                inventory_redraw = 0;
            }
            ui_mode = redraw;
            picked = sel;
            menu_choice = list_menu(empty_text, items_items, 1, 5, 0x26, 0x16,
                           (struct Node far *)character->items, 1, 8, &redraw_box, &index,
                           (struct Node far * far *)&picked);
            sel = picked;
            redraw = ui_mode;
            if (menu_escaped && menu_choice == 0x1b)
                menu_choice = 9;
            if (sel)
                switch (menu_choice) {
                case 0:
                    item_equip_toggle(sel);
                    break;
                case 1:
                    if (sel->rec.equipped == 0) {
                        bottom_msg(must_ready_text);
                        menu_choice = 0xff;
                    } else if (item_casts(sel)
                               || sel->rec.u.magic.effect > 0 && sel->rec.u.magic.special < 0x80) {
                        use_item(sel, done);
                        if (view_mode != 5)
                            *done = 0;
                        if (!*done)
                            need_redraw = 1;
                    }
                    break;
                case 2:
                    if (item_ok(sel))
                        trade_item(sel);
                    else
                        menu_choice = 0xff;
                    need_redraw = 1;
                    break;
                case 3:
                    if (item_ok(sel)) {
                        if (sel->rec.type == 'I')
                            required_records = sel->rec.quantity + 1;
                        else
                            required_records = 1;
                        if (pool_records() + required_records <= 0x32 || view_mode != 10) {
                            for (cur_item = pool_items; cur_item && cur_item->next;
                                 cur_item = cur_item->next)
                                ;
                            item_name(character, sel, 0, 0, 0, 0);
                            if (view_mode == 10) {
                                text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                                          msg_format("Your %swill be left in the vault",
                                                     sel->name));
                                pool_set_records(pool_records() + required_records);
                                text_pause();
                                if (!pool_items) {
                                    pool_alloc(&pool_items, 0x3d, &item_record_pool);
                                    cur_item = pool_items;
                                } else {
                                    pool_alloc(&cur_item->next, 0x3d, &item_record_pool);
                                    cur_item = cur_item->next;
                                }
                                *cur_item = *sel;
                                cur_item->next = 0;
                                if (sel->rec.type == 'I')
                                    sel->more = 0;
                                item_remove(character, sel);
                                redraw_box = 1;
                            } else {
                                text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                                          msg_format("Your %swill be gone forever",
                                                     sel->name));
                                if (confirm("Drop It? ", 1)) {
                                    item_remove(cur_node, sel);
                                    redraw_box = 1;
                                }
                            }
                            text_clear(1, 0x15, 0x26, 0x16);
                        } else if (view_mode == 10)
                            if (sel->rec.type == 'I')
                                text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                                          "Scroll Bundle won't fit into Vault!");
                            else
                                text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                                          "Vault is full!");
                    } else
                        menu_choice = 0xff;
                    break;
                case 4:
                    if (item_ok(sel)) {
                        item_name(character, sel, 0, 0, 0, 0);
                        text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                                  msg_format("%s%s%s", your_text, sel->name, gone_text));
                        anim = confirm_anim;
                        confirm_anim = 0;
                        if (confirm(txt_lose_it, 0)) {
                            item_remove(cur_node, sel);
                            redraw_box = 1;
                        }
                        confirm_anim = anim;
                        text_clear(1, 0x15, 0x26, 0x16);
                    } else
                        menu_choice = 0xff;
                    break;
                case 5:
                    halve_item(sel);
                    break;
                case 6:
                    join_item(sel);
                    break;
                case 7:
                    if (item_ok(sel))
                        item_sell(character, sel);
                    else
                        menu_choice = 0xff;
                    break;
                case 8:
                    item_identify(character, sel, &redraw_box);
                    break;
                }
            recalc_stats(character);
        }
        if (character->item_count != count)
            redraw_box = 1;
        if (has_ready_8c(character) && view_mode == 5)
            *done = 1;
    }
}

void far item_effect(struct Item far *item, char enabled)             /* 2b3a:16f7 */
{
    unsigned char id;
    struct Character far *character;
    unsigned char level;
    unsigned char spell_slot;
    unsigned char cnt;
    char flag;

    cnt = 0;
    character = cur_node;
    id = item->rec.u.magic.special & 0x7f;
    switch (id) {
    case 0:
        item_handler_active = 1;
        if (enabled && item->rec.u.magic.effect == 0x8c) {
            if (view_mode == 5 && cur_node->quick_fight == 0)
                combat_view_show();
            if (view_mode == 5) {
                status_printf(0, 0x17, 7, 0, "Item:");
                item_name(cur_node, item, 5, 0x17, 0, 1);
            }
            text_pause();
            if (view_mode == 5)
                combat_text_clear();
            item_cast_active = 1;
            spell_cast(0x8c, cur_node->quick_fight, 0, &flag);
            item_cast_active = 0;
        } else
            fx_dispatch(item->rec.u.magic.effect, character, (struct Effect far *)item, !enabled);
        break;
    case 1:
        level = class_level(character, 5);
        if (level > 0x1d)
            level = 0x1d;
        else if (level < 1)
            level = 1;
        spell_slot = spell_slots_by_level[class_spell_rows[character->robe_order == 1 ? 7 : 8]][level - 1][4];
        if (enabled)
            character->slots[2][4] = spell_slot + spell_slot;
        else {
            character->slots[2][4] = spell_slot;
            cnt = 0;
            for (spell_slot = 0; spell_slot <= 0x88; spell_slot++)
                if (character->spells[spell_slot] && (spell_info - 1)[character->spells[spell_slot]].cast_class == 2
                    && (spell_info - 1)[character->spells[spell_slot]].level == 5) {
                    cnt++;
                    if (character->slots[2][4] < cnt)
                        character->spells[spell_slot] = 0;
                }
        }
        break;
    case 3:
    case 5:
        fx_stat_recalc(character, 0);
        break;
    case 9:
        if (!enabled)
            fx_remove(character, 0x17, 0);
        break;
    }
}

void far item_equip_toggle(struct Item far *item)                  /* 2b3a:1965 */
{
    char special;
    unsigned char err;
    unsigned char slot;
    struct Character far *character;

    special = item->rec.u.magic.special & 0x80;
    character = cur_node;
    if (item->rec.equipped) {
        if (item->rec.cursed)
            bottom_msg("It's Cursed!");
        else {
            item->rec.equipped = 0;
            if (special)
                item_effect(item, 0);
        }
    } else {
        err = 0;
        if (character->hands + item_table[item->rec.type].hands > 2)
            err = 3;
        slot = item_table[item->rec.type].slot;
        switch (slot) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            if (character->equipped[slot])
                err = 2;
            break;
        case 9:
            if (character->equipped[10])
                err = 2;
            break;
        }
        if (item->rec.type == 0x27)
            if (!(cur_node->robe_order == 2 && item->rec.unid_mask & 0x10
                  || cur_node->robe_order == 1 && item->rec.unid_mask & 0x20))
                if (cur_node->level[6] <= 9)
                    if (cur_node->level[6] < 10 && cur_node->level[6] > 0)
                        err = 6;
                    else
                        err = 4;
        if (item->rec.type == 0x43 && cur_node->race != 5)
            err = 5;
        if (item->rec.type == 0x1e && character->equipped[11]) {
            err = 2;
            slot = 0xb;
        }
        if ((item->rec.type == 0xc || item->rec.type == 0x4a) && character->equipped[12]) {
            err = 2;
            slot = 0xc;
        }
        if (!(item_table[item->rec.type].class_mask & character->class_mask)
            && (item->rec.type != 0x43 || cur_node->race != 5))
            err = 1;
        switch (err) {
        case 0:
            item->rec.equipped = 1;
            if (special)
                item_effect(item, 1);
            break;
        case 1:
            bottom_msg("Wrong Class");
            break;
        case 2:
            item_name(character, character->equipped[slot], 0, 0, 0, 0);
            bottom_msg(msg_format("already using %s", character->equipped[slot]->name));
            break;
        case 3:
            if (view_mode != 5 || character->quick_fight == 0)
                bottom_msg("Your hands are full!");
            break;
        case 4:
            bottom_msg("Wrong Magical Order");
            break;
        case 5:
            bottom_msg("Usable Only By Kender");
            break;
        case 6:
            bottom_msg("Thief Level Too Low");
            break;
        }
    }
}

static void far trade_item(struct Item far *item)                /* 2b3a:1c58 */
{
    struct Character far *target;

    target = selected_target;
    view_redraw();
    pick_hero("Trade with whom?", 1, 1, &target);
    if (target) {
        selected_target = target;
        if (cant_carry(target, 0, item))
            bottom_msg("Overloaded");
        else {
            item_append(target, item);
            if (item->rec.type == 'I')
                item->more = 0;
            item_remove(cur_node, item);
            recalc_stats(target);
        }
    }
}

static void far halve_item(struct Item far *item)                /* 2b3a:1d13 */
{
    struct Item far *new;
    struct Item far *last;
    unsigned char half;
    unsigned char rest;

    half = item->rec.quantity / 2;
    if (half > 0) {
        if (item->rec.type == 'I') {
            rest = item->rec.quantity - half;
            pool_alloc(&new, 0x3d, &item_record_pool);
            *new = *item;
            if (half == 1)
                mem_move(new->more, new, 0x3d);
            else {
                new->rec.quantity = half;
                new->rec.weight = half;
                new->rec.name_ids[1] = half;
                last = new;
                new->rec.value = 0;
                for (tick_slot = 1; tick_slot <= half; tick_slot++) {
                    last = last->more;
                    new->rec.value += last->rec.value;
                }
            }
            new->rec.equipped = 0;
            new->next = item->next;
            if (rest == 1) {
                if (half == 1)
                    mem_move(item->more->more, item, 0x3d);
                else
                    mem_move(item->more->more->more, item, 0x3d);
                item->more = 0;
            } else {
                item->rec.quantity = rest;
                item->rec.weight = rest;
                item->rec.name_ids[1] = rest;
                for (tick_slot = 1; tick_slot <= half; tick_slot++) {
                    item->rec.value -= item->more->rec.value;
                    item->more = item->more->more;
                }
            }
            if (half == 1)
                new->more = 0;
            else
                last->more = 0;
        } else {
            rest = item->rec.quantity - half;
            pool_alloc(&new, 0x3d, &item_record_pool);
            mem_move5(item, new, 0x3d);
            new->rec.quantity = half;
            new->rec.equipped = 0;
            new->next = item->next;
            item->rec.quantity = rest;
        }
        item->next = new;
    } else
        bottom_msg("Can't halve that");
}

int far party_scrolls(void)                                      /* 2b3a:1f75 */
{
    struct Item far *item;
    struct Character far *character;
    register int scroll_count;

    scroll_count = 0;
    for (character = node_list; character; character = character->next)
        for (item = character->items; item; item = item->next)
            if (item->rec.type == 'I')
                scroll_count += item->rec.quantity;
    return scroll_count;
}

void far join_item(struct Item far *item)                        /* 2b3a:1fda */
{
    struct Item far *cur;
    struct Item far *next;
    struct Item far *dst;
    unsigned char qty;

    dst = item;
    qty = dst->rec.quantity;
    cur = cur_node->items;
    while (cur) {
        next = cur->next;
        if (cur != dst && cur->rec.quantity > 0 && cur->rec.name_ids[0] == dst->rec.name_ids[0]
            && cur->rec.name_ids[1] == dst->rec.name_ids[1] && cur->rec.name_ids[2] == dst->rec.name_ids[2]
            && cur->rec.unid_mask == dst->rec.unid_mask && cur->rec.type == dst->rec.type
            && cur->rec.plus == dst->rec.plus && cur->rec.save_bonus == dst->rec.save_bonus
            && cur->rec.cursed == dst->rec.cursed && cur->rec.weight == dst->rec.weight
            && cur->rec.u.magic.charges == cur->rec.u.magic.charges && cur->rec.u.magic.charges < 2
            && cur->rec.u.magic.effect == cur->rec.u.magic.effect
            && cur->rec.u.magic.special == cur->rec.u.magic.special)
                                /* 2b3a:20d5 the last three against cur */
            if (cur->rec.quantity + qty <= 0xff) {
                qty += cur->rec.quantity;
                item_remove(cur_node, cur);
            } else {
                dst->rec.quantity = 0xff;
                cur->rec.quantity = cur->rec.quantity - (0xff - qty);
                qty = cur->rec.quantity;
                dst = cur;
            }
        cur = next;
    }
    dst->rec.quantity = qty;
}

void far use_item(struct Item far *item, char far *done)          /* 2b3a:2175 */
{
    unsigned char spell;
    int index;
    char not_empty;

    index = -1;
    item_cast_active = 0;
    spell = 0;
    if (item_casts(item)) {
        item_cur = item;
        if (item->rec.type == 'I')
            spell = spell_menu(7, 1, &index, &not_empty);
        else
            spell = spell_menu(2, 1, &index, &not_empty);
    } else if (item->rec.u.magic.effect > 0 && item->rec.u.magic.special < 0x80) {
        item_cast_active = 1;
        spell = item->rec.u.magic.effect;
        if (spell == 0x40 && game_state[STATE_UNDERWATER]) {
            status_msg("Nothing Happens", 0xb, 8);
            spell = 0;
        }
    }
    if (spell == 0)
        *done = 0;
    else {
        if (view_mode == 5 && cur_node->quick_fight == 0)
            combat_view_show();
        if (item_cast_active) {
            hero_text(cur_node, "uses an item", 0xa, 0);
            if (view_mode == 5) {
                status_printf(0, 0x17, 7, 0, "Item:");
                item_name(cur_node, item, 5, 0x17, 0, 1);
            } else
                item_name(cur_node, item, 1, 0x16, 0, 1);
            text_pause();
            combat_text_clear();
        }
        item_cast_active = 1;
        if (item_casts(item)) {
            combat_text_clear();
            if (class_level(cur_node, 5) > 0 || class_level(cur_node, 0) > 0
                || class_level(cur_node, 6) > 9 && fx_roll_dice(1, 100) <= 0x4b)
                spell_cast(spell, cur_node->quick_fight, 0, done);
            else
                hero_text(cur_node, "oops!", text_row + 1, 1);
        } else {
            switch (spell) {
            case 0x39:
                spell = 0x7f;
                break;
            case 0x3b:
                spell = 0x80;
                break;
            case 0x3c:
                spell = 0x81;
                break;
            case 0x3d:
                spell = 0x82;
                break;
            case 0x3e:
                spell = 0x83;
                break;
            case 0x3f:
                spell = 0x84;
                break;
            case 0x40:
                spell = 0x85;
                break;
            case 0x41:
                spell = 0x86;
                break;
            case 0x5f:
                spell = 0x87;
                break;
            case 0x61:
                spell = 0x88;
                break;
            case 0x63:
                spell = 0x89;
                break;
            }
            spell_cast(spell, cur_node->quick_fight, 0, done);
        }
        item_cast_active = 0;
        if (view_mode == 5 && (spell_info - 1)[spell].combat_ok != 0)
            *done = combat_actions_end(cur_node);
    }
    if (*done)
        if (item_casts(item))
            item_spend(cur_node, item, spell);
        else if (item->rec.u.magic.charges > 0)
            if (item->rec.quantity > 1)
                item->rec.quantity--;
            else {
                item->rec.u.magic.charges--;
                if (item->rec.u.magic.charges == 0)
                    item_remove(cur_node, item);
            }
}

int far coin_kind(char far *text, char far *name)                 /* 2b3a:24d9 */
{
    short kind;
    unsigned char index;

    for (index = 1; text[index - 1] == ' '; index++)
        ;
    switch (text[index - 1]) {
    case 'G':
        kind = 1;
        str_copy(name, "Gems ");
        break;
    case 'P':
    case 'S':
        str_copy(name, "Steel ");
        kind = 0;
        break;
    case 'J':
        str_copy(name, "Jewelry ");
        kind = 2;
        break;
    }
    return kind;
}

long far input_amount(long most, char far *prompt, char color)   /* 2b3a:2586 */
{
    long result;
    char col;
    char plen;
    char most_text[8];
    char text[7];
    unsigned char key_code;
    long val;
    int err;
    long value;

    box_draw();
    status_printf(0, 0x18, color, 0, prompt);
    plen = str_len(prompt);
    col = plen;
    str_format(most_text, "%l", most);
    text[0] = 0;
    screen_flush();
    do {
        key_code = game_key_get();
        switch (key_code) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            str_format(text, "%s%c", text, key_code);
            str_to_long(text, &val, &err);
            if (val <= most)
                col++;
            else {
                str_copy(text, most_text);
                col = str_len(most_text) + plen;
            }
            status_printf(plen, 0x18, 0xb, 0, text);
            break;
        case 8:
            if (str_len(text) > 0) {
                str_copy(text, substr(text, 1, str_len(text) - 1));
                col--;
                status_char(col, 0x18, 0, 1);
            }
            break;
        }
    } while (key_code != 0xd && key_code != 0x1b && key_code != 0x60);
    box_draw();
    if (key_code == 0x1b || key_code == 0x60)
        value = 0;
    else
        str_to_uint(text, &value, &err);
    result = value;
    return result;
}

void far weight_sub(struct Character far *character, int weight)                   /* 2b3a:2769 */
{
    character->encumbrance = character->encumbrance - weight;
}

unsigned far carry_limit(struct Character far *character)                     /* 2b3a:277c */
{
    unsigned short limit;

    limit = str_carry(character) + 1500;
    return limit;
}

void far weight_add(struct Character far *character, int weight)                   /* 2b3a:2799 */
{
    character->encumbrance = character->encumbrance + weight;
}

void far money_trade(struct Character far * far *giver, struct Character far * far *taker,
                     long amount, short kind)                    /* 2b3a:27ac */
{
    if (carry_limit(*taker) >= (*taker)->encumbrance + amount) {
        (*giver)->coins[kind] = (*giver)->coins[kind] - amount;
        weight_sub(*giver, amount);
        (*taker)->coins[kind] = (*taker)->coins[kind] + amount;
        weight_add(*taker, amount);
    } else {
        bottom_msg("Overloaded");
        text_pause();
        input_pending_clear();
    }
}

static void far trade(void)                                      /* 2b3a:286a */
{
    struct Character far *target;
    short coin;
    short kind;
    unsigned char column;
    unsigned char pad;
    struct Node far *cur;
    struct Node far *prev;
    struct Node far *list;
    int index;
    char num_text[0x18];
    char line[0x18];
    char desc[0x29];
    char key;
    unsigned short amount;
    char exit_outer;
    char exit_inner;
    char redraw;
    register int count;

    do {
        target = selected_target;
        view_redraw();
        pick_hero(trade_to_text, 1, 1, &target);
        if (!target)
            exit_outer = 1;
        else {
            char_status();
            do {
                money_show();
                selected_target = target;
                cur = 0;
                prev = 0;
                list = 0;
                count = 0;
                for (coin = 2; coin >= 0; coin--)
                    if (cur_node->coins[coin] != 0) {
                        count++;
                        if (!cur) {
                            pool_alloc(&cur, 0x28, node_pool);
                            cur->next = 0;
                        } else {
                            prev = cur;
                            pool_alloc(&cur, 0x28, node_pool);
                            cur->next = prev;
                        }
                        str_copy(num_text, int_str(cur_node->coins[coin]));
                        pad = 0x12 - str_len(coin_names[coin]) - str_len(num_text);
                        str_copy(line, "");
                        for (column = 1; column <= pad; column++)
                            str_copy(line, msg_format("%s ", line));
                        str_copy(cur->text, msg_format("%s%s%s", coin_names[coin], line,
                                                       num_text, 0L));
                                /* 2b3a:29f3 a fifth argument, unread   */
                        cur->key = 0;
                    }
                list = cur;
                index = 0;
                redraw = 1;
                hotkey_reset(1);
                key = list_menu(coin_title, select_items, 0x14, 9, 0x26, 0xf, cur, 1, 1,
                                &redraw, &index, &prev);
                if (key == 1 || menu_escaped && key == 0x1b)
                    prev = 0;
                if (!prev)
                    exit_inner = 1;
                else {
                    str_copy(desc, "");
                    kind = coin_kind(prev->text, desc);
                    str_copy(desc, msg_format("%s%s%s", how_much_text, desc,
                                              trade_amount_text));
                    amount = input_amount(cur_node->coins[kind], desc, 7);
                    money_trade(&cur_node, &target, amount, kind);
                    exit_inner = 1;
                    exit_outer = 1;
                    for (coin = 0; coin <= 2; coin++)
                        if (cur_node->coins[coin] > 0) {
                            exit_outer = 0;
                            exit_inner = 0;
                        }
                }
                if (list)
                    list_free(&list);
            } while (!exit_inner);
        }
    } while (!exit_outer);
}

void far money_drop(struct Character far *character, long amount, short kind) /* 2b3a:2bc6 */
{
    character->coins[kind] = character->coins[kind] - amount;
    weight_sub(character, amount);
    if (view_mode == 6 || view_mode == 1 || view_mode == 10)
        pool[kind] = pool[kind] + amount;
}

void far drop(char in_view)                                      /* 2b3a:2c32 */
{
    short coin;
    short kind;
    unsigned char pad;
    unsigned char column;
    int index;
    struct Node far *cur;
    struct Node far *prev;
    struct Node far *list;
    char exit_flag;
    char redraw;
    char num_text[0x12];
    char line[0x12];
    char desc[0x29];
    char key;
    unsigned short amount;
    unsigned char left;
    unsigned char right;
    unsigned char top;
    unsigned char bottom;
    register int count;

    do {
        if (in_view)
            money_show();
        cur = 0;
        prev = 0;
        list = 0;
        count = 0;
        for (coin = 2; coin >= 0; coin--)
            if (cur_node->coins[coin] != 0) {
                count++;
                if (!cur) {
                    pool_alloc(&cur, 0x28, node_pool);
                    cur->next = 0;
                } else {
                    prev = cur;
                    pool_alloc(&cur, 0x28, node_pool);
                    cur->next = prev;
                }
                str_copy(num_text, int_str(cur_node->coins[coin]));
                pad = 0x12 - str_length(coin_names[coin]) - str_length(num_text);
                str_copy(line, "");
                for (column = 1; column <= pad; column++)
                    str_copy(line, msg_format("%s ", line));
                str_copy(cur->text, msg_format("%s%s%s", coin_names[coin], line, num_text));
                cur->key = 0;
            }
        list = cur;
        index = 0;
        redraw = 1;
        hotkey_reset(1);
        if (in_view) {
            left = 0x14;
            right = 0x26;
            top = 9;
            bottom = 0xf;
        } else {
            left = 1;
            right = 0x26;
            top = 0x11;
            bottom = 0x16;
        }
        key = list_menu(coin_title, select_items, left, top, right, bottom, cur, 1, 1,
                        &redraw, &index, &prev);
        if (!prev || key == 1 || menu_escaped && key == 0x1b)
            exit_flag = 1;
        else {
            desc[0] = 0;
            kind = coin_kind(prev->text, desc);
            if (view_mode == 10)
                if (str_length(desc) > 7)
                    str_copy(desc, msg_format("How much %sdo you deposit? ", desc));
                else if (kind == 1)
                    str_copy(desc, msg_format("How many %swill you deposit? ", desc));
                else
                    str_copy(desc, msg_format("How much %swill you deposit? ", desc));
            else
                str_copy(desc, msg_format("How much %swill you drop? ", desc));
            amount = input_amount(cur_node->coins[kind], desc, 7);
            money_drop(cur_node, amount, kind);
            exit_flag = 1;
            for (coin = 0; coin <= 2; coin++)
                if (cur_node->coins[coin] > 0)
                    exit_flag = 0;
        }
        if (list)
            list_free(&list);
    } while (!exit_flag);
}

char far cant_carry(struct Character far *hero, char mode, struct Item far *item) /* 2b3a:2fbf */
{
    char result;
    char cannot;
    unsigned short weight;
    struct Item far *cur;
    char dup;

    recalc_stats(hero);
    cannot = 0;
    dup = 0;
    cur = hero->items;
    if (item->rec.quantity > 0 && mode)
        while (!dup && cur) {
            dup = cur->rec.name_ids[0] == item->rec.name_ids[0]
                  && cur->rec.name_ids[1] == item->rec.name_ids[1]
                  && cur->rec.name_ids[2] == item->rec.name_ids[2] && cur->rec.type == item->rec.type
                  && cur->rec.plus == item->rec.plus && cur->rec.save_bonus == item->rec.save_bonus
                  && cur->rec.cursed == item->rec.cursed && cur->rec.weight == item->rec.weight
                  && cur->rec.u.magic.charges == item->rec.u.magic.charges && cur->rec.u.magic.charges < 2
                  && cur->rec.u.magic.effect == item->rec.u.magic.effect
                  && cur->rec.u.magic.special == item->rec.u.magic.special;
            cur = cur->next;
        }
    if (hero->item_count > 0xf && !dup)
        cannot = 1;
    weight = item->rec.weight;
    if (item->rec.quantity > 0)
        weight = weight * item->rec.quantity;
    if (hero->encumbrance + weight > str_carry(hero) + 1500)
        cannot = 1;
    result = cannot;
    return result;
}

char far can_heal(struct Character far *character)                            /* 2b3a:314e */
{
    char result;
    struct Effect far *found;

    result = (character->cls == 3 || character->old_level[3] > 0 && old_class_abilities_available(character)) && view_mode != 5
        && character->status == 0 && !find_effect(character, 0x8c, &found);
    return result;
}

char far can_cure(struct Character far *character)                            /* 2b3a:31af */
{
    char result;

    result = (character->cls == 3 || character->old_level[3] > 0 && old_class_abilities_available(character)) && view_mode != 5
        && character->status == 0 && character->cure > 0;
    return result;
}

void far heal(struct Character far *character)                                /* 2b3a:3200 */
{
    struct Character far *target;
    struct Effect far *found;

    view_redraw();
    target = node_list;
    pick_hero("Heal whom? ", 1, 0, &target);
    if (target) {
        if (fx_heal(target, class_level(character, 3) * 2, 0)
            || find_effect(target, 0xb9, &found)) {
            if (find_effect(target, 0xb9, &found))
                fx_remove(target, 0xb9, found);
            bottom_msg(msg_format("%s%s", target->name, " feels better"));
        } else
            bottom_msg(msg_format("%s is unaffected", target->name));
        fx_add(character, 0x8c, 0x5a0, 0, 0);
    }
    char_status();
}

void far cure(struct Character far *character)                                /* 2b3a:3319 */
{
    struct Character far *target;
    char diseased;
    char cure_confirmed;
    struct Effect far *found;

    view_redraw();
    target = node_list;
    pick_hero("Cure whom? ", 1, 0, &target);
    if (target) {
        diseased = 0;           /* 2b3a:3351 nothing sets it           */
        cure_confirmed = 1;
        if (!diseased) {
            hero_text(target, "is not diseased", 0, 0);
            cure_confirmed = confirm("cure anyway: ", 0);
            combat_text_clear();
        }
        if (cure_confirmed == 1) {
            fx_lock = 1;
            fx_lock = 0;
            if (character->cure > 0)
                character->cure--;
            if (!find_effect(character, 0x6e, &found))
                fx_add(character, 0x6e, 0x2760, 0, 1);
            if (diseased)
                bottom_msg(msg_format("%s is cured", target->name));
            else
                bottom_msg(msg_format("%s cures the healthy", character->name));
        }
    }
    char_status();
}

char far pick_char(void)                                         /* 2b3a:343a */
{
    char result;
    unsigned char key;
    char saved;

    saved = party_click_enabled;
    party_click_enabled = 1;
    do {
        hotkey_reset(1);
        key = menu_run(pick_text, select_items, 1, 0);
        if (menu_escaped)
            if (key == 0x1b) {
                key = 1;
                menu_escaped = 0;
            } else {
                party_rows(cur_node, 0);
                party_key_select(key);
                party_rows(cur_node, 1);
            }
    } while (menu_escaped || key <= 1);
    party_rows(cur_node, 1);
    result = key == 0;
    party_click_enabled = saved;
    return result;
}

void far ticks_start(void)                                       /* 2b3a:34fc */
{
    last_tick = timer_ticks();
}

char far ticks_quick(void)                                       /* 2b3a:3509 */
{
    long now;
    char result;

    now = timer_ticks();
    if (now - last_tick < 0x30)
        result = 1;
    else
        result = 0;
    last_tick = now;
    return result;
}

unsigned char far hp_floor(struct Character far *character)                   /* 2b3a:354b */
{
    unsigned char cnt;
    unsigned char result;
    unsigned short sum;
    char mod;
    short class_index;
    unsigned char excess;
    unsigned char lvl;

    cnt = 0;
    sum = 0;
    for (class_index = 0; class_index <= 6; class_index++)
        if (character->level[class_index] > 0 && character->level[class_index] > character->byte_92 || character->old_level[class_index] > 0) {
            mod = con_hp_adj(character, class_index);
            if (character->old_level[class_index] > 0)
                lvl = character->old_level[class_index];
            else {
                cnt++;
                lvl = character->level[class_index];
            }
            if (hit_dice_level_limit[class_index] > lvl)
                switch (class_index) {
                case 0:
                    if (character->deity <= 4)
                        sum += (character->level[class_index] + 1) * (mod + 8);
                    else
                        sum += character->level[class_index] * (mod + 8);
                    break;
                case 2:
                    sum += character->level[class_index] * (mod + 8);
                    break;
                case 3:
                    sum += character->level[class_index] * (mod + 10);
                    break;
                case 1:
                    sum += (character->level[class_index] + 1) * (mod + 10);
                    break;
                case 4:
                    sum += character->level[class_index] * (mod + 8) + 1;
                    break;
                case 5:
                    sum += character->level[class_index] * (mod + 4);
                    break;
                case 6:
                    sum += character->level[class_index] * (mod + 6);
                    break;
                }
            else {
                excess = character->level[class_index] - hit_dice_level_limit[class_index] + 1;
                switch (class_index) {
                case 0:
                    sum += mod * 9 + 0x48 + excess * 2;
                    break;
                case 2:
                case 3:
                    sum += mod * 9 + 0x5a + excess * 3;
                    break;
                case 1:
                    sum += mod * 10 + 0x64 + excess * 2;
                    break;
                case 4:
                    sum += mod * 11 + 0x58 + excess * 2;
                    break;
                case 5:
                    sum += mod * 11 + 0x2c + excess;
                    break;
                case 6:
                    sum += mod * 10 + 0x3c + excess * 2;
                    break;
                }
            }
        }
    if (cnt > 0)
        sum = sum / cnt;
    else if (character->byte_92 == 0)
        sum = 0;
    result = sum;
    return result;
}

void far hp_adjust(struct Character far *character, char mask, unsigned char divisor, char apply)
                                                                 /* 2b3a:380e */
{
    unsigned char roll1;
    unsigned char roll2;
    unsigned char total;
    unsigned char lvl;
    unsigned char secondary;
    short class_index;
    unsigned char amount;
    unsigned char floor;

    total = 0;
    secondary = 0;
    for (class_index = 0; class_index <= 6; class_index++)
        if (character->level[class_index] > 0 && (class_bit[class_index] & mask) > 0)
            if ((int)character->level[class_index] < hit_dice_level_limit[class_index]) {   /* 2b3a:385a an int compare */
                if (character->level[class_index] > 1)
                    lvl = 1;
                else
                    lvl = class_hit_dice[class_index];
                roll1 = fx_roll_dice(lvl, hit_die_sides[class_index]);
                roll2 = fx_roll_dice(lvl, hit_die_sides[class_index]);
                if (roll2 > roll1)
                    roll1 = roll2;
                total += roll1;
                secondary += hp_bonus(character, class_index) * lvl;
            } else
                switch (class_progression_row(character, class_index)) {
                case 2:
                case 10:
                    total = 3;
                    break;
                case 3:
                case 4:
                case 5:
                case 6:
                case 9:
                    total = 2;
                    break;
                case 0:
                case 1:
                case 7:
                case 8:
                    total = 1;
                    break;
                }
    if (divisor > 0) {
        total = total / divisor;
        if (total < 1)
            total = 1;
        secondary = secondary / divisor;
        roll1 = character->hp_max - character->hp;
        if (apply) {
            character->hp_base += total;
            character->hp_max += total + secondary;
            if (character->hp_max < roll1)
                character->hp = 0;
            else
                character->hp = character->hp_max - roll1;
        } else {
            if (character->hp_base > total)
                character->hp_base -= total;
            else
                character->hp_base = 0;
            amount = total + secondary;
            floor = hp_floor(character);
            if (character->hp_max - amount > floor)
                amount = character->hp_max - floor;
            if (character->hp_max < amount)
                character->hp_max = 0;
            else
                character->hp_max -= amount;
            if (character->hp_max < roll1)
                character->hp = 0;
            else
                character->hp = character->hp_max - roll1;
        }
    } else {
        character->hp = 0;
        character->hp_base = 0;
        character->hp_max = 0;
    }
}

char far hp_bonus(struct Character far *character, short cls)                 /* 2b3a:3a5b */
{
    char result;
    char bonus;

    bonus = 0;
    bonus += constitution_hp_bonus[character->stats[4].cur];
    if (cls == 2 || cls == 4 || cls == 3 || cls == 1)
        switch (character->stats[4].cur) {
        case 0x11:
            bonus++;
            break;
        case 0x12:
            bonus += 2;
            break;
        case 0x13:
        case 0x14:
            bonus += 3;
            break;
        case 0x15:
        case 0x16:
        case 0x17:
            bonus += 4;
            break;
        case 0x18:
        case 0x19:
            bonus += 5;
            break;
        }
    result = bonus;
    return result;
}

void far pick_hero(char far *prompt, char allow_cancel, char selection_mode, struct Character far * far *out)
                                                                 /* 2b3a:3ae5 */
{
    unsigned char key;
    char mode;
    struct Character far *cur;
    char saved;
    char saved_menu_context;
    char text[0x2a];
    register int sel;

    set_selected(*out);
    saved = party_click_enabled;
    order_mode = selection_mode;
    picker_start_character = cur_node;
    key = 0xff;
    saved_menu_context = exploration_menu_context;
    exploration_menu_context = 0;
    while (key >= 2 && key != 0x1b) {
        party_display(*out);
        if (view_mode == 2 || view_mode == 6)
            mode = 1;
        else
            mode = 0;
        if (view_mode == 0)
            party_click_enabled = 1;      /* 2b3a:3b57 overwritten at once        */
        party_click_enabled = saved;
        cur = *out;
        set_selected(*out);
        if (allow_cancel)
            hotkey_reset(1);
        else
            hotkey_reset(0);
        str_copy(text, msg_format("%s%s", prompt, " "));
        key = node_menu(text, select_items, 1, mode);
        cur = get_selected();
        if (menu_escaped) {
            sel = key;
            switch (sel) {
            case 0x1b:
                if (allow_cancel)
                    cur = 0;
                break;
            case 0xd:
                key = 0;
                break;
            case 0x84:
            case 0x85:
                cur = cur->next;
                if (!cur)
                    cur = node_list;
                break;
            case 0x87:
            case 0x88:
                if (picker_direct_select)
                    if (order_mode)
                        cur = picker_start_character;
                    else
                        cur = cur_node;
                else {
                    cur = node_list;
                    if (*out != node_list)
                        while (cur->next != *out)
                            cur = cur->next;
                    else
                        while (cur->next)
                            cur = cur->next;
                }
                break;
            }
        } else if (allow_cancel && key == 1)
            cur = 0;
        *out = cur;
        set_selected(*out);
    }
    exploration_menu_context = saved_menu_context;
    order_mode = 0;
}
