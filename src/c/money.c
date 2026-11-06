/* DQK.EXE overlay 08 module 25a4: the party's money.  Pooling the members'
 * steel, gems and jewelry, sharing the pool out again, taking coins from it
 * by menu, and appraising gems and jewelry for sale or keeping.
 *
 * EXE 0x0491f0..0x04a32b, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 25a4, the first module of RTLink section 8 (payload
 * 0x491f0, link segment 25a4).  Its final retf is byte 0xa of paragraph
 * 26b7, where the next module begins at 26b7:000b (its first routine holds
 * a fixup site the section's table files under frame 26b7).  Built with Turbo C
 * 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped order; bake
 * consumes them in that order.  Section 4's 25a4 is config.c, other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, and a mod1d99_ callee is an RTLink vector
 * stub whose target is named beside it.
 *
 * _DATA is seven literals, DGROUP 7f54..7fb1; -d stores the "%s%s" format
 * as the tail of "%s%s%s".  It declares no _BSS; pooled, which section 8's
 * 280a and 2910 also set and nothing reads, is extern here.
 *
 * A member carries steel, gems and jewelry as coins[0..2]; pool[] holds
 * the party's pooled amounts in the same order.
 */

#include "dqk.h"

char far too_heavy(struct Character far *hero, long amount, long far *room)
{                                                                /* 25a4:0000 */
    char over;
    unsigned short most;

    most = carry_limit(hero);
    if (hero->encumbrance + amount > most) {
        over = 1;
        if (hero->encumbrance <= most)
            *room = most - hero->encumbrance;
        else
            *room = 0;
    } else {
        over = 0;
        *room = 0;
    }
    return over;
}

void far give_steel(unsigned short amount)                       /* 25a4:007a */
{
    long room;

    if (too_heavy(cur_node, amount, &room)) {
        bottom_msg("Overloaded. Money will be put in Pool.");
        cur_node->coins[0] += room;
        weight_add(cur_node, room);
        pool[0] += amount - room;
    } else {
        cur_node->coins[0] += amount;
        weight_add(cur_node, amount);
    }
}

void far pool_money(void)                                        /* 25a4:00f6 */
{
    struct Character far *character_cursor;
    short index;

    character_cursor = node_list;
    pooled = 1;
    while (character_cursor) {
        if (character_cursor->npc == 0 || character_cursor->npc == 0xb3) {
            for (index = 0; index <= 2; index++) {
                pool[index] += character_cursor->coins[index];
                weight_sub(character_cursor, character_cursor->coins[index]);
            }
            mem_fill(character_cursor->coins, sizeof character_cursor->coins, 0);
        }
        character_cursor = character_cursor->next;
    }
}

unsigned char far party_count(void)                              /* 25a4:01a1 */
{
    char result;
    struct Character far *character_cursor;
    unsigned char member_count;

    character_cursor = node_list;
    member_count = 0;
    while (character_cursor) {
        if (character_cursor->npc == 0 || character_cursor->npc == 0xb3)
            member_count++;
        character_cursor = character_cursor->next;
    }
    result = member_count;
    return result;
}

void far share_money(void)                                       /* 25a4:01ee */
{
    struct Character far *character_cursor;
    unsigned char member_count;
    long room;
    unsigned short share[3];
    unsigned short rest[3];
    short index;

    character_cursor = node_list;
    member_count = party_count();
    mem_fill(share, 4, 0);              /* 4 of 6 bytes; the loop sets all */
    mem_fill(rest, 4, 0);
    for (index = 0; index <= 2; index++) {
        if (pool[index] > 0) {
            share[index] = pool[index] / member_count;
            rest[index] = pool[index] % member_count;
        } else {
            share[index] = 0;
            rest[index] = 0;
        }
    }
    while (character_cursor) {
        if (character_cursor->npc < 0x80) {
            for (index = 2; index >= 0; index--) {
                if (!too_heavy(character_cursor, share[index], &room)) {
                    character_cursor->coins[index] = share[index] + character_cursor->coins[index];
                    weight_add(character_cursor, share[index]);
                    if (rest[index] > 0) {
                        if (!too_heavy(character_cursor, 1L, &room)) {
                            character_cursor->coins[index] = character_cursor->coins[index] + 1;
                            weight_add(character_cursor, 1);
                            rest[index]--;
                        }
                    }
                } else {
                    character_cursor->coins[index] = character_cursor->coins[index] + room;
                    rest[index] = rest[index] + share[index] - room;
                    weight_add(character_cursor, room);
                }
            }
        }
        character_cursor = character_cursor->next;
    }
    for (index = 2; index >= 0; index--) {
        if (rest[index] > 0) {
            character_cursor = node_list;
            while (character_cursor) {
                room = carry_limit(character_cursor) - character_cursor->encumbrance;
                if (room > 0) {
                    if (rest[index] > room) {
                        character_cursor->coins[index] = character_cursor->coins[index] + room;
                        weight_add(character_cursor, room);
                        rest[index] = rest[index] - room;
                    } else {
                        character_cursor->coins[index] = rest[index] + character_cursor->coins[index];
                        weight_add(character_cursor, rest[index]);
                        rest[index] = 0;
                    }
                }
                character_cursor = character_cursor->next;
            }
        }
    }
    pooled = 0;
    for (index = 0; index <= 2; index++) {
        pool[index] = rest[index];
        if (pool[index])
            pooled = 1;
    }
}

void far pool_take(struct Character far *hero, long amount, short kind)
{                                                                /* 25a4:05c0 */
    long room;

    if (too_heavy(hero, amount, &room)) {
        bottom_msg("Overloaded");
        text_pause();
        input_pending_clear();
    } else {
        if (pool[kind] < amount)
            amount = pool[kind];
        pool[kind] = pool[kind] - amount;
        hero->coins[kind] = hero->coins[kind] + amount;
        weight_add(hero, amount);
    }
}

void far take_menu(void)                                         /* 25a4:0683 */
{
    short index;
    short kind;
    struct Node far *list;
    struct Node far *cur;
    struct Node far *first;
    int top;
    char buf[50];
    unsigned short amount;
    char done;
    char redraw;
    char key;

    frame_border();
    top = 0;
    do {
        redraw = 1;
        list = 0;
        cur = 0;
        for (index = 0; index <= 2; index++) {
            if (pool[index] > 0) {
                if (!list) {
                    pool_alloc(&list, sizeof(struct Node), node_pool);
                    list->next = 0;
                } else {
                    cur = list;
                    pool_alloc(&list, sizeof(struct Node), node_pool);
                    list->next = cur;
                }
                str_format(buf, "%ld", pool[index]);
                str_copy(list->text, msg_format("%s %s", coin_names[index], buf));
                list->key = 0;
            }
        }
        first = cur = list;
        hotkey_reset(1);
        key = list_menu(coin_title, select_items, 2, 2, 0x12, 8, list, 1, 0,
                        &redraw, &top, &cur);
        if (!cur || key == 1 || menu_escaped && key == 0x1b) {
            done = 1;
        } else {
            done = 0;
            kind = coin_kind(cur->text, buf);
            if (kind == 1)
                str_copy(buf, msg_format("%s%s%s", how_many_text, buf, take_text));
            else
                str_copy(buf, msg_format("%s%s%s", how_much_text, buf, take_text));
            amount = input_amount(pool[kind], buf, 7);
            pool_take(cur_node, amount, kind);
            list_free(&first);
            pooled = 0;
            done = 1;
            for (index = 0; index <= 2; index++) {
                if (pool[index] > 0) {
                    pooled = 1;
                    done = 0;
                }
            }
        }
    } while (!done);
}

void far pool_check(char far *has_coins, char far *has_items)   /* 25a4:092c */
{
    short index;

    *has_coins = 0;
    *has_items = 0;
    for (index = 0; index <= 2; index++)
        if (pool[index])
            *has_coins = 1;
    if (pool_items)
        *has_items = 1;
}

void far appraise(char far *done)                                /* 25a4:097c */
{
    char text1[42];
    char text2[42];
    unsigned char key;
    unsigned char roll;
    long room;
    short value;
    struct Item far *inventory_tail;
    struct Item far *item;
    char leave;
    char no_keep;
    unsigned char count;

    *done = 1;
    if (cur_node->coins[1] == 0 && cur_node->coins[2] == 0) {
        bottom_msg("No Gems or Jewelry");
        *done = 0;
        return;
    }
    do {
        if (cur_node->coins[1] == 0 && cur_node->coins[2] == 0) {
            leave = 1;
        } else {
            leave = 0;
            int_to_str(cur_node->coins[1], text1);
            int_to_str(cur_node->coins[2], text2);
            str_copy(text1, msg_format("%s%s", gems_text, text1));
            str_copy(text2, msg_format("%s%s", jewelry_text, text2));
            frame_border();
            character_name_show(cur_node, 1, 1, 0);
            status_printf(1, 7, 0xb, 0, collection_text);
            status_printf(1, 9, 0xb, 0, text1);
            status_printf(1, 0xa, 0xb, 0, text2);
            mem_fill(hotkeys, sizeof hotkeys, 0xff);
            count = 0;
            if (cur_node->coins[1])
                hotkey_add(0, &count);
            if (cur_node->coins[2])
                hotkey_add(1, &count);
            hotkey_add(2, &count);
            key = menu_run(appraise_title, appraise_items, 1, 0);
            if (menu_escaped && key == 0x1b)
                key = 2;
            switch (key) {
            case 0:
                if (cur_node->coins[1]) {
                    cur_node->coins[1]--;
                    roll = fx_roll_dice(1, 100);
                    switch (roll) {
                    case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                    case 8: case 9: case 10: case 11: case 12: case 13:
                    case 14: case 15: case 16: case 17: case 18: case 19:
                    case 20: case 21: case 22: case 23: case 24: case 25:
                        value = 5;
                        break;
                    case 26: case 27: case 28: case 29: case 30: case 31:
                    case 32: case 33: case 34: case 35: case 36: case 37:
                    case 38: case 39: case 40: case 41: case 42: case 43:
                    case 44: case 45: case 46: case 47: case 48: case 49:
                    case 50:
                        value = 25;
                        break;
                    case 51: case 52: case 53: case 54: case 55: case 56:
                    case 57: case 58: case 59: case 60: case 61: case 62:
                    case 63: case 64: case 65: case 66: case 67: case 68:
                    case 69: case 70:
                        value = 50;
                        break;
                    case 71: case 72: case 73: case 74: case 75: case 76:
                    case 77: case 78: case 79: case 80: case 81: case 82:
                    case 83: case 84: case 85: case 86: case 87: case 88:
                    case 89: case 90:
                        value = 250;
                        break;
                    case 91: case 92: case 93: case 94: case 95: case 96:
                    case 97: case 98: case 99:
                        value = 500;
                        break;
                    case 100:
                        value = 2500;
                        break;
                    }
                    int_to_str(value, text1);
                    str_copy(text1, msg_format("%s%s%s", gem_value_text, text1,
                                               " steel."));
                    status_printf(1, 0xc, 0xb, 0, text1);
                    mem_fill(hotkeys, sizeof hotkeys, 0xff);
                    hotkeys[0].key = 0;
                    if (too_heavy(cur_node, 1L, &room) || cur_node->item_count >= 0x10) {
                        no_keep = 1;
                    } else {
                        hotkeys[1].key = 1;
                        no_keep = 0;
                    }
                    key = menu_run(empty_text, sell_items, 0, 0);
                    box_draw();
                    if (key == 1 && !no_keep) {
                        item = rec_alloc();
                        mem_fill(item, sizeof(struct Item), 0);
                        item->next = 0;
                        item->rec.equipped = 0;
                        item->rec.name_ids[2] = 0x7a;
                        item->rec.name_ids[1] = 0;
                        item->rec.name_ids[0] = 0;
                        item->rec.type = 0x2f;
                        item->rec.value = value * 2;
                        item->rec.weight = 1;
                        item->rec.unid_mask = 0;
                        inventory_tail = cur_node->items;
                        if (!inventory_tail) {
                            cur_node->items = item;
                        } else {
                            while (inventory_tail->next)
                                inventory_tail = inventory_tail->next;
                            inventory_tail->next = item;
                        }
                    } else
                        give_steel(value);
                }
                break;
            case 1:
                if (cur_node->coins[2]) {
                    cur_node->coins[2]--;
                    roll = fx_roll_dice(1, 100);
                    switch (roll) {
                    case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                    case 8: case 9: case 10:
                        value = random_n(450) + 50;
                        break;
                    case 11: case 12: case 13: case 14: case 15: case 16:
                    case 17: case 18: case 19: case 20:
                        value = random_n(500) + 100;
                        break;
                    case 21: case 22: case 23: case 24: case 25: case 26:
                    case 27: case 28: case 29: case 30: case 31: case 32:
                    case 33: case 34: case 35: case 36: case 37: case 38:
                    case 39: case 40:
                        value = random_n(750) + 150;
                        break;
                    case 41: case 42: case 43: case 44: case 45: case 46:
                    case 47: case 48: case 49: case 50:
                        value = random_n(1500) + 250;
                        break;
                    case 51: case 52: case 53: case 54: case 55: case 56:
                    case 57: case 58: case 59: case 60: case 61: case 62:
                    case 63: case 64: case 65: case 66: case 67: case 68:
                    case 69: case 70:
                        value = random_n(2500) + 500;
                        break;
                    case 71: case 72: case 73: case 74: case 75: case 76:
                    case 77: case 78: case 79: case 80: case 81: case 82:
                    case 83: case 84: case 85: case 86: case 87: case 88:
                    case 89: case 90:
                        value = random_n(3000) + 1000;
                        break;
                    case 91: case 92: case 93: case 94: case 95: case 96:
                    case 97: case 98: case 99: case 100:
                        value = random_n(5000) + 1000;
                        break;
                    }
                    int_to_str(value, text2);
                    str_copy(text1, msg_format("%s%s%s", jewel_value_text, text2,
                                               " steel."));
                    status_printf(1, 0xc, 0xb, 0, text1);
                    mem_fill(hotkeys, sizeof hotkeys, 0xff);
                    hotkeys[0].key = 0;
                    if (too_heavy(cur_node, 1L, &room) || cur_node->item_count >= 0x10) {
                        no_keep = 1;
                    } else {
                        hotkeys[1].key = 1;
                        no_keep = 0;
                    }
                    key = menu_run(empty_text, sell_items, 0, 0);
                    box_draw();
                    if (key == 1 && !no_keep) {
                        item = rec_alloc();
                        mem_fill(item, sizeof(struct Item), 0);
                        item->next = 0;
                        item->rec.equipped = 0;
                        item->rec.name_ids[2] = 0x7b;
                        item->rec.name_ids[1] = 0;
                        item->rec.name_ids[0] = 0;
                        item->rec.type = 0x2f;
                        item->rec.value = value * 2;
                        item->rec.weight = 1;
                        item->rec.unid_mask = 0;
                        inventory_tail = cur_node->items;
                        if (!inventory_tail) {
                            inventory_tail = item;   /* not cur_node->items: 25a4:10e6 */
                        } else {
                            while (inventory_tail->next)
                                inventory_tail = inventory_tail->next;
                            inventory_tail->next = item;
                        }
                    } else
                        give_steel(value);
                }
                break;
            case 2:
                leave = 1;
                break;
            }
            recalc_stats(cur_node);
        }
    } while (!leave);
}
