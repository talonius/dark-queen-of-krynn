/* DQK.EXE overlay 08 module 280a: the shop and the vault.  Pricing the
 * shop's stock and buying from it, the shop menu, the vault menu, and
 * selling or identifying one of a member's items for steel.
 *
 * EXE 0x04b855..0x04c8b1, one Turbo C large-model _TEXT segment starting at
 * byte 5 of paragraph 280a in RTLink section 8 (payload 0x491f0, link
 * segment 25a4, parent section 3).  Module 26b7's final retf is byte 4 of
 * paragraph 280a, after a fixup site the section's table files under
 * 26b7; this module's final retf is byte 0 of paragraph 2910, where the
 * next module begins at 2910:0001.  Built with Turbo C 2.01, tcc -ml -O -Z
 * -d.  Functions are defined in shipped order; bake consumes them in that
 * order.  Code outside the section reaches four of them through vector
 * stubs: 1d99:149f -> 06f4, 1d99:125b -> 09a0, 1d99:14f9 -> 0be4,
 * 1d99:1503 -> 0e61; module 26b7 far-calls item_give.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, an ovlNN_ name which section and stored
 * segment, and a mod1d99_ callee is an RTLink vector stub whose target is
 * named beside it.
 *
 * The first five routines are DKK's shop.pas (dkk-disassembly), with
 * funds_show, DKK's first, third; vault_menu is DKK's PoolVault, and
 * item_sell and item_identify are DKK's charview.pas sellItem and
 * identifyItem.  Its three cs:[bx] reads are Turbo C's value search for a
 * sparse switch, in shop_pick, shop_buy and shop_menu.
 *
 * _DATA is 27 stored literals, DGROUP 8056..82a3; -d stores "" as the NUL
 * of "%u" and each "buys" format once.  _BSS is shop_redraw, one byte at
 * c7e2.  pooled, which money.c and temple.c also set and nothing reads, is
 * extern here.
 */

#include "dqk.h"

extern char far *shop_items;                                     /* 3efe */

static char shop_redraw;                                         /* c7e2 */

void far shop_pick(char far *key, struct Item far * far *pick,
                   struct Item far * far *cursor, int far *top)  /* 280a:0005 */
{
    struct Item far *head;
    struct Item far *item;
    char num[17];
    char line[35];
    unsigned char len;
    unsigned char index;
    unsigned short price;

    item = pool_items;
    while (item) {
        str_copy(line, spaces);
        if (item->rec.value == 0)
            item->rec.value = 1;
        switch (game_state[STATE_SHOP_PRICE_SCALE]) {
        case 1:
            price = item->rec.value / 16;
            break;
        case 2:
            price = item->rec.value / 8;
            break;
        case 4:
            price = item->rec.value / 4;
            break;
        case 8:
            price = item->rec.value / 2;
            break;
        case 0x20:
            price = item->rec.value * 2;
            break;
        case 0x40:
            price = item->rec.value * 4;
            break;
        case 0x80:
            price = item->rec.value * 8;
            break;
        default:
            price = item->rec.value;
        }
        if (item->rec.quantity > 1)
            price = item->rec.quantity * price;
        if (price == 0)
            price = 1;
        str_format(num, "%u", price);
        len = str_len(num);
        for (index = 0; index < len; index++)
            line[0x22 - len + index] = num[index];
        len = str_len(item->name);
        for (index = 0; index < len; index++)
            line[index] = item->name[index];
        str_copy(item->name, line);
        item = item->next;
    }
    head = pool_items;
    ui_mode = 0;
    hotkey_reset(1);
    *key = list_menu(items_title, buy_items, 1, 1, 0x26, 0x12,
                     (struct Node far *)head, 1, 0, &shop_redraw, top,
                     (struct Node far * far *)cursor);
    *pick = *cursor;
    item = pool_items;
    while (item) {
        item_name(0, item, 0, 0, 0, 0);
        item = item->next;
    }
}

void far item_give(struct Item far *item, char far *failed)     /* 280a:0295 */
{
    struct Item far *inventory_item;
    struct Item far *new_item;

    if (item->rec.type == 0x49 && party_scrolls() + item->rec.quantity > 0) {
        bottom_msg("Too many Bundles!");
        *failed = 1;
    } else if (cant_carry(cur_node, 1, item)) {
        bottom_msg("OverLoaded");
        *failed = 1;
    } else {
        *failed = 0;
        inventory_item = cur_node->items;
        if (!inventory_item) {
            inventory_item = rec_alloc();
            *inventory_item = *item;
            inventory_item->next = 0;
            cur_node->items = inventory_item;
        } else {
            while (inventory_item->next)
                inventory_item = inventory_item->next;
            new_item = rec_alloc();
            *new_item = *item;
            new_item->next = 0;
            inventory_item->next = new_item;
        }
        if (item->rec.quantity > 0)
            join_item(new_item);                                        /* q unset when items was empty: 280a:03ae */
        recalc_stats(cur_node);
    }
}

void far funds_show(void)                                        /* 280a:03ce */
{
    char funds_text[12];

    str_copy(funds_text, msg_format("%d", cur_node->coins[0]));
    str_pad_left(funds_text, 10);
    status_printf(1, 0x14, 0xb, 0, msg_format("Personal funds:%s", funds_text));
    str_copy(funds_text, msg_format("%ld", pool[0]));
    str_pad_left(funds_text, 10);
    status_printf(1, 0x15, 0xb, 0, msg_format("Pooled funds:  %s", funds_text));
}

void far shop_buy(void)                                          /* 280a:0490 */
{
    struct Item far *pick;
    char key;
    char done;
    unsigned short price;
    unsigned short steel;
    long pooled_steel;
    char failed;
    struct Item far *cursor;
    int top;

    pick = 0;
    cursor = pool_items;
    top = 0;
    frame_border();
    shop_redraw = 1;
    do {
        funds_show();
        shop_pick(&key, &pick, &cursor, &top);
        if (key == 1 || key == 0x1b && menu_escaped) {
            done = 1;
        } else {
            done = 0;
            switch (game_state[STATE_SHOP_PRICE_SCALE]) {
            case 1:
                price = pick->rec.value / 16;
                break;
            case 2:
                price = pick->rec.value / 8;
                break;
            case 4:
                price = pick->rec.value / 4;
                break;
            case 8:
                price = pick->rec.value / 2;
                break;
            case 0x20:
                price = pick->rec.value * 2;
                break;
            case 0x40:
                price = pick->rec.value * 4;
                break;
            case 0x80:
                price = pick->rec.value * 8;
                break;
            default:
                price = pick->rec.value;
            }
            if (pick->rec.quantity > 1)
                price = pick->rec.quantity * price;
            if (price == 0)
                price = 1;
            steel = cur_node->coins[0];
            pooled_steel = pool[0];
            if (price <= steel) {
                item_give(pick, &failed);
                if (!failed) {
                    cur_node->coins[0] -= price;
                    if (pick->rec.quantity > 0)
                        bottom_msg(msg_format("%s buys %s ", cur_node->name,
                                                pick->name));
                    else
                        bottom_msg(msg_format("%s buys a %s ", cur_node->name,
                                                pick->name));
                }
            } else if (price <= steel + pooled_steel) {
                item_give(pick, &failed);
                if (!failed) {
                    cur_node->coins[0] = 0;
                    pool[0] -= price - steel;
                    if (pick->rec.quantity > 0)
                        bottom_msg(msg_format("%s buys %s ", cur_node->name,
                                                pick->name));
                    else
                        bottom_msg(msg_format("%s buys a %s ", cur_node->name,
                                                pick->name));
                }
            } else
                bottom_msg(not_enough_text);
        }
    } while (!done);
}

void far shop_menu(void)                                         /* 280a:06f4 */
{
    char viewed;
    char done;
    char has_coins;
    char has_items;
    unsigned char key;
    unsigned char count;
    unsigned char index;
    char state;
    char appraised;
    struct Item far *item;

    saved_view_mode = view_mode;
    view_mode = 1;
    shop_temple_frame = 0;
    view_redraw();
    shop_temple_frame = 1;
    party_display(cur_node);
    mem_zero(pool, sizeof pool);
    pooled = 0;
    done = 0;
    item = pool_items;
    while (item) {
        item_name(0, item, 0, 0, 0, 0);
        item = item->next;
    }
    state = 1;
    do {
        pool_check(&has_coins, &has_items);
        if (has_coins) {
            hotkey_reset(6);
        } else {
            count = 0;
            for (index = 0; index <= 6; index++)
                if (index != 2 && index != 4)
                    hotkey_add(index, &count);
        }
        ui_mode = state;
        key = node_prompt(empty_text, shop_items, 1, 0);
        state = ui_mode;
        if (menu_escaped) {
            switch (key) {
            case 0x84:
            case 0x85:
            case 0x87:
            case 0x88:
                party_rows(cur_node, 0);
                party_key_select(key);
                party_rows(cur_node, 1);
                key = 0xff;
                break;
            case 0x1b:
                key = 6;
            }
        }
        switch (key) {
        case 0:
            shop_buy();
            break;
        case 1:
            char_view(&viewed);
            break;
        case 2:
            take_menu();
            break;
        case 3:
            pool_money();
            break;
        case 4:
            share_money();
            break;
        case 5:
            appraise(&appraised);
            break;
        case 6:
            pool_check(&has_coins, &has_items);
            if (has_coins) {
                text_wrap(1, 0x11, 0x26, 0x16, 7, 0, 1, leave_text1);
                text_wrap(1, 0x11, 0x26, 0x16, 0xb, 0, 0, leave_text2);
                if (confirm(empty_text, 1))
                    text_clear(1, 0x11, 0x26, 0x16);
                else
                    done = 1;
            } else
                done = 1;
            break;
        }
        switch (key) {
        case 0:
        case 2:
        case 5:
            view_redraw();
            break;
        case 3:                         /* appraise's flag, as shipped: 280a:0987 */
            if (appraised)
                view_redraw();
            break;
        }
    } while (!done);
    view_mode = saved_view_mode;
}

void far vault_menu(void)                                        /* 280a:09a0 */
{
    unsigned char key_code;
    char leave;
    char state;
    unsigned char count;
    unsigned char key;
    char done;

    view_mode = 10;
    shop_temple_frame = 1;
    view_redraw();
    shop_temple_frame = 1;
    party_display(cur_node);
    pool_clear();
    vault_load_current();
    pooled = 0;
    leave = 0;
    state = 1;
    do {
        mem_zero(hotkeys, sizeof hotkeys);
        count = 0;
        hotkey_add(0, &count);
        pool_check(&has_coins, &treasure_has_items);
        if (has_coins || treasure_has_items)
            hotkey_add(1, &count);
        hotkey_add(2, &count);
        if (cur_node->coins[0] > 0 || cur_node->coins[1] > 0 || cur_node->coins[2] > 0)
            hotkey_add(3, &count);
        if (cur_node->items)
            hotkey_add(4, &count);
        hotkey_add(5, &count);
        ui_mode = state;
        set_menu_kind(2);
        key = node_prompt("", vault_items, 1, 0);
        state = ui_mode;
        if (menu_escaped && key == 0x1b) {
            menu_escaped = 0;
            key = 5;
        }
        done = 0;
        if (!menu_escaped) {
            switch (key) {
            case 0:
                char_view(&done);
                break;
            case 1:
                loot_take(&has_coins, &treasure_has_items);
                break;
            case 2:
                pool_money();
                break;
            case 3:
                drop(0);
                text_clear(1, 0x11, 0x26, 0x16);
                break;
            case 4:
                items_menu(&done);
                view_redraw();
                break;
            case 5:
                leave = 1;
                break;
            }
        } else {
            key_code = key;
            if (key_code == 0x84 || key_code == 0x88 || key_code == 0x87 || key_code == 0x85) {
                party_rows(cur_node, 0);
                party_key_select(key);
                party_rows(cur_node, 1);
            }
        }
    } while (!leave);
    vault_slot = 0x54;
    vault_save_current();
    post_reset();
}

void far item_sell(struct Character far *hero, struct Item far *item) /* 280a:0be4 */
{
    unsigned short price;
    unsigned short amount;
    long room;
    char ask[42];

    price = 0;
    if (item->rec.value > 0)
        price = item->rec.value / 2;
    if (item->rec.quantity > 1 && item->rec.type != 0x49)
        price = item->rec.quantity * price;
    item_name(hero, item, 0, 0, 0, 0);
    if (price > 0) {
        /* v1.0 formats the offer with "%d"; v1.1 with "%u" */
        text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                  msg_format("I'll give you %d steel pieces for your %s", price,
                             item->name));
        str_copy(ask, "Will you sell ");
        if (item->rec.quantity < 2 || item->rec.type == 0x49)
            str_copy(ask, msg_format("%sit? ", ask));
        else
            str_copy(ask, msg_format("%sthem? ", ask));
    } else if (item->rec.quantity < 2) {
        text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                  msg_format("Your %s is worthless to me.", item->name));
        str_copy(ask, "Shall I dispose of it for you? ");
    } else {
        text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                  msg_format("Your %s are worthless.", item->name));
        str_copy(ask, "Would you like to discard them? ");
    }
    if (confirm(ask, 1)) {
        if (price > 0)
            bottom_msg("Sold!");
        else if (item->rec.quantity < 2)
            bottom_msg("I will give it to one of my sons.");
        else
            bottom_msg("Very well.\tI'll take them.");
        item_remove(cur_node, item);
        amount = price;
        if (too_heavy(cur_node, amount, &room)) {
            bottom_msg("Overloaded.\tMoney will be put in pool.");
            cur_node->coins[0] += (unsigned short)room;
            price -= (unsigned short)room;
            pool[0] += price;
        } else
            cur_node->coins[0] += price;
    }
    text_clear(1, 0x15, 0x26, 0x16);
}

void far item_identify(struct Character far *hero, struct Item far *item,
                       char far *redraw)                         /* 280a:0e61 */
{
    long steel;
    long pooled_steel;
    char paid;

    if (item->rec.type == 0x49) {
        text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                  "Fool! You must remove them from the bundle first!");
    } else {
        paid = 0;
        item_name(hero, item, 0, 0, 0, 0);
        text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                  msg_format("For 20 steel pieces I'll identify your %s",
                             item->name));
        if (confirm("Is It a Deal? ", 1)) {
            pooled_steel = pool[0];
            if ((steel = cur_node->coins[0]) >= 20) {
                paid = 1;
                cur_node->coins[0] -= 20;
            } else if (pooled_steel + steel >= 20) {
                paid = 1;
                cur_node->coins[0] = 0;
                pool[0] -= 20 - steel;
            } else {
                bottom_msg("Not Enough Money");
                text_pause();
            }
        }
        if (paid) {
            if (!(item->rec.unid_mask & 7)) {
                text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                          msg_format("I can't tell anything new about your %s",
                                     item->name));
            } else {
                item->rec.unid_mask = item->rec.unid_mask & 0xf8;
                item_name(hero, item, 0, 0, 0, 0);
                text_wrap(1, 0x15, 0x26, 0x16, 0xb, 0, 1,
                          msg_format("It looks like some sort of %s", item->name));
                *redraw = 1;
            }
        }
        if (paid)
            text_pause();
        text_clear(1, 0x15, 0x26, 0x16);
    }
}
