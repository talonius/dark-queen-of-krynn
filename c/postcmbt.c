/* DQK.EXE overlay 08 module 26b7: after a fight.  Settling the combat --
 * who fled, who fell, who is revived -- the experience from the monsters,
 * their money and the treasure's worth, the summary box, the treasure
 * menu with taking coins and items, and the combatants' clean-up.
 *
 * EXE 0x04a32b..0x04b855, one Turbo C large-model _TEXT segment starting at
 * 26b7:000b in RTLink section 8 (payload 0x491f0, link segment 25a4, parent
 * section 3), after money.c's final retf at 26b7:000a.  Its own final retf
 * is 26b7:1534, byte 4 of paragraph 280a, where the next module begins at
 * 280a:0005 (section 8's table files the last 26b7 fixup site at 0x04b852
 * and the first 280a site at 0x04b875).  Built with Turbo C 2.01, tcc -ml
 * -O -Z -d.  Functions are defined in shipped order; bake consumes them in
 * that order.  Code outside the section reaches two of them through vector
 * stubs: 1d99:1229 -> 135c, 1d99:1189 -> 151b.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, an ovlNN_ name which section and stored
 * segment, and a mod1d99_ callee is an RTLink vector stub whose target is
 * named beside it.
 *
 * The routines from loot_exp on follow the order of DKK's pcore.pas
 * (dkk-disassembly); loot_join is DKK's CharView.joinItem run over the
 * treasure, and drops_return and post_reset have no pcore counterpart.
 * loot_exp's pool[0] * 2 is Turbo C's inline long shift, shl ax,1;
 * rcl dx,1.
 *
 * _DATA is DGROUP 7fb2..8054: the code's twelve literals, ten distinct,
 * stored as nine -- -d stores the three "Take:" once and "Items Exit" as the
 * tail of "Money Items Exit".  _BSS is three chars, fled, killed and
 * has_items, at c7de..c7e0.
 */

#include "dqk.h"

static char fled;                                                /* c7de */
static char killed;                                              /* c7df */
static char has_items;                                           /* c7e0 */

void far post_reset(void);

void far loot_join(struct Item far *item)                        /* 26b7:000b */
{
    struct Item far *previous_item;
    struct Item far *pooled_item;
    struct Item far *next;
    struct Item far *dest;
    unsigned char count;

    dest = item;
    count = item->rec.quantity;
    pooled_item = pool_items;
    while (pooled_item) {
        next = pooled_item->next;
        if (pooled_item != dest && pooled_item->rec.quantity > 0 && pooled_item->rec.name_ids[0] == dest->rec.name_ids[0]
            && pooled_item->rec.name_ids[1] == dest->rec.name_ids[1] && pooled_item->rec.name_ids[2] == dest->rec.name_ids[2]
            && pooled_item->rec.type == dest->rec.type && pooled_item->rec.plus == dest->rec.plus
            && pooled_item->rec.save_bonus == dest->rec.save_bonus && pooled_item->rec.cursed == dest->rec.cursed
            && pooled_item->rec.weight == dest->rec.weight
            && pooled_item->rec.u.magic.charges == pooled_item->rec.u.magic.charges  /* p with p, as shipped */
            && pooled_item->rec.u.magic.charges < 2
            && pooled_item->rec.u.magic.effect == pooled_item->rec.u.magic.effect
            && pooled_item->rec.u.magic.special == pooled_item->rec.u.magic.special) {
            if (pooled_item->rec.quantity + count <= 0xff) {
                count += pooled_item->rec.quantity;
                previous_item = pool_items;
                if (pooled_item == previous_item)
                    pool_items = pooled_item->next;
                else
                    while (previous_item && previous_item->next != pooled_item)
                        previous_item = previous_item->next;
                previous_item->next = pooled_item->next;
                rec_free(&pooled_item);
            } else {
                dest->rec.quantity = 0xff;
                pooled_item->rec.quantity = pooled_item->rec.quantity - (0xff - count);
                count = pooled_item->rec.quantity;
                dest = pooled_item;
            }
        }
        pooled_item = next;
    }
    dest->rec.quantity = count;
}

void far loot_exp(long far *share)                               /* 26b7:01fb */
{
    struct Character far *character_cursor;
    short index;
    long total;
    struct Item far *item;
    struct Item far *loot_item;
    unsigned char eligible_member_count;

    total = 0;
    character_cursor = node_list;
    eligible_member_count = 0;
    while (character_cursor) {
        loot_item = character_cursor->items;
        if (character_cursor->side == 1 && character_cursor->status != 0 && character_cursor->status != 3 && character_cursor->status != 9) {
            killed = 1;
            if (character_cursor->combat->not_pc == 1) {
                for (index = 0; index <= 2; index++)
                    pool[index] += character_cursor->coins[index];
                total += character_cursor->xp_award;
                if (game_state[STATE_NO_ITEM_LOOT] != 1 && loot_item && loot_item->rec.type != 0x69) {
                    item = pool_items;
                    pool_items = loot_item;
                    for (;;) {
                        eligible_member_count++;
                        loot_item->rec.equipped = 0;
                        if (!loot_item->next)
                            break;
                        if (pool_items->rec.quantity > 0 && loot_item->rec.unid_mask)
                            loot_join(pool_items);
                        loot_item = loot_item->next;
                    }
                    loot_item->next = item;
                    character_cursor->items = 0;
                }
            }
        }
        character_cursor = character_cursor->next;
    }
    total += pool[0] * 2;
    total += pool[1] * 250;
    total += pool[2] * 2200;
    item = pool_items;
    while (item && item != dropped_item_boundary) {
        if (item->rec.plus > 0)
            total += item->rec.plus * 400;
        item = item->next;
    }
    if (game_state[STATE_PARTY_COUNT] <= down_count)
        down_count--;
    *share = total / (game_state[STATE_PARTY_COUNT] - down_count);
}

void far exp_award(long exp)                                     /* 26b7:0427 */
{
    struct Character far *character_cursor;
    long share;

    if (game_state[STATE_NO_XP])
        return;
    character_cursor = node_list;
    while (character_cursor) {
        if (character_cursor->enabled && character_cursor->status != 1) {
            share = exp;
            switch (character_cursor->cls) {
            case 0:
                if (character_cursor->stats[2].cur > 0xe)
                    share = exp / 10 + exp;
                break;
            case 2:
                if (character_cursor->stats[0].cur > 0xf)
                    share = exp / 10 + exp;
                break;
            case 3:
                if (character_cursor->stats[0].cur > 0xf && character_cursor->stats[2].cur > 0xf)
                    share = exp / 10 + exp;
                break;
            case 4:
                if (character_cursor->stats[0].cur > 0xf && character_cursor->stats[1].cur > 0xf
                    && character_cursor->stats[2].cur > 0xf)
                    share = exp / 10 + exp;
                break;
            case 5:
                if (character_cursor->stats[1].cur > 0xf)
                    share = exp / 10 + exp;
                break;
            case 6:
                if (character_cursor->stats[3].cur > 0xf)
                    share = exp / 10 + exp;
                break;
            case 8: case 10: case 11: case 12: case 13: case 14: case 16:
                share = exp / 2;
                break;
            case 9: case 15:
                share = exp / 3;
                break;
            }
            character_cursor->xp += share;
            character_cursor->level_up = can_level_up(character_cursor);
        }
        character_cursor = character_cursor->next;
    }
}

void far combat_results(void)                                    /* 26b7:0593 */
{
    struct Character far *character_cursor;
    char done;
    unsigned char index;
    char alive;

    down_count = 0;
    adventure_exit = 1;
    fled = 0;
    character_cursor = node_list;
    while (character_cursor && character_cursor->combat->not_pc != 1) {
        if (character_cursor->status == 3)
            fled = 1;
        character_cursor = character_cursor->next;
    }
    character_cursor = node_list;
    alive = 0;
    while (character_cursor && !alive) {
        if (character_cursor->enabled || character_cursor->status == 4 || character_cursor->status == 3 || character_cursor->status == 5)
            alive = 1;
        character_cursor = character_cursor->next;
    }
    if (game_state[STATE_NONLETHAL_COMBAT] && alive)
        adventure_exit = 0;
    combat_won = 0;
    character_cursor = node_list;
    if (!demo_active) {
        while (character_cursor && character_cursor->combat->not_pc != 1) {
            if ((character_cursor->status == 3 || character_cursor->status == 1 || character_cursor->status == 0)
                && character_cursor->side == 0 && !(character_cursor->npc & 0x80))
                adventure_exit = 0;
            if (character_cursor->status == 1 || character_cursor->status == 0) {
                combat_won = 1;
                fled = 0;
            }
            if (!character_cursor->enabled || character_cursor->status == 1)
                down_count++;
            for (index = 0; index < 0x12; index++)
                fx_remove(character_cursor, fx_clear[index], 0);
            character_cursor = character_cursor->next;
        }
        if (combat_won) {
            loot_exp(&exp_share);
            exp_award(exp_share);
        }
        character_cursor = node_list;
        if (!adventure_exit) {
            while (character_cursor && character_cursor->combat->not_pc != 1) {
                if (!fled) {
                    switch (character_cursor->status) {
                    case 3:
                        character_cursor->status = 0;
                        character_cursor->enabled = 1;
                        break;
                    case 5:
                        if (game_state[STATE_NONLETHAL_COMBAT]) {
                            character_cursor->status = 0;
                            character_cursor->enabled = 1;
                            character_cursor->hp = 1;
                        } else
                            character_cursor->status = 4;
                        break;
                    case 4:
                        if (character_cursor->hp > 0) {
                            character_cursor->status = 0;
                            character_cursor->enabled = 1;
                        } else if (game_state[STATE_NONLETHAL_COMBAT]) {
                            character_cursor->status = 0;
                            character_cursor->enabled = 1;
                            character_cursor->hp = 1;
                        }
                        break;
                    }
                    character_cursor = character_cursor->next;
                } else if (character_cursor->status == 3) {
                    character_cursor->status = 0;
                    character_cursor->enabled = 1;
                    character_cursor = character_cursor->next;
                } else {
                    cur_node = character_cursor;
                    character_cursor = character_cursor->next;
                    hero_drop(0, 1);
                }
            }
        } else {
            character_cursor = node_list;
            done = 0;
            while (character_cursor && !done) {
                if (character_cursor->combat->not_pc != 1) {
                    cur_node = character_cursor;
                    hero_drop(0, 1);
                } else
                    done = 1;
                character_cursor = character_cursor->next;
            }
            game_state[STATE_PARTY_COUNT] = 0;
        }
    } else {
        character_cursor = node_list;
        while (character_cursor) {
            if (character_cursor->enabled && character_cursor->status == 0 && character_cursor->side != 1) {
                combat_won = 1;
                loot_exp(&exp_share);
                exp_award(exp_share);
            }
            character_cursor = character_cursor->next;
        }
        character_cursor = node_list;
        while (character_cursor) {
            if (character_cursor->status == 0 || character_cursor->status == 1)
                character_cursor->enabled = 1;
            if (character_cursor->status == 5)
                character_cursor->status = 4;
            character_cursor = character_cursor->next;
        }
    }
}

void far combat_summary(long exp)                                /* 26b7:091f */
{
    char num[12];
    char msg[82];
    struct Item far *item;
    struct Item far *next_item;

    frame_border();
    if (killed) {
        if (fled) {
            status_printf(1, 3, 7, 0, fled_text);
            exp = 0;
            item = pool_items;
            while (item) {
                next_item = item->next;
                rec_free(&item);
                item = next_item;
            }
            mem_zero(pool, sizeof pool);
        } else if (!combat_won && game_state[STATE_NONLETHAL_COMBAT]) {
            status_printf(1, 3, 7, 0, lost_text);
            exp = 0;
        } else
            status_printf(1, 3, 7, 0, "The party has won.");
    }
    str_format(num, "%l", exp);
    str_copy(msg, msg_format("%s%ld", receives_text, exp));
    if (!game_state[STATE_NO_XP]) {
        status_printf(1, 5, 7, 0, msg);
        status_printf(1, 7, 7, 0, xp_text);
    }
    has_coins = 0;
    has_items = 0;
    pool_check(&has_coins, &has_items);
    if (has_coins || has_items)
        status_printf(1, 10, 7, 0, treasure_text);
    else
        status_printf(1, 10, 7, 0, no_treasure_text);
    menu_run(empty_text, continue_items, 1, 0);
}

void far loot_pick(char far *key, struct Item far * far *pick,
                   struct Item far * far *cur, int far *top)     /* 26b7:0b04 */
{
    struct Item far *head;
    char redraw;
    struct Item far *item;

    redraw = 1;
    item = pool_items;
    while (item) {
        item_name(0, item, 0, 0, 0, 0);
        item = item->next;
    }
    head = pool_items;
    hotkey_reset(1);
    *key = list_menu(items_title, take_items, 1, 1, 0x26, 0x16, head, 1, 0,
                     &redraw, top, cur);
    *pick = *cur;
}

void far loot_items(void)                                        /* 26b7:0bbe */
{
    struct Item far *pick;
    struct Item far *item;
    char key;
    char done;
    struct Item far *cur;
    int top;
    char failed;

    pick = 0;
    cur = pool_items;
    top = 0;
    frame_border();
    ui_mode = 1;
    do {
        loot_pick(&key, &pick, &cur, &top);
        if (key)
            done = 1;
        else {
            done = 0;
            item_give(pick, &failed);
            if (!failed) {
                item = pool_items;
                if (view_mode == 10) {
                    if (pick->rec.type == 0x49)
                        pool_set_records(pool_records() - (pick->rec.quantity + 1));
                    else
                        pool_set_records(pool_records() - 1);
                }
                if (item == pick)
                    pool_items = pick->next;
                else {
                    while (item->next != pick)
                        item = item->next;
                    item->next = pick->next;
                }
                pick->next = 0;
                if (pick)
                    rec_free(&pick);
                if (!pool_items)
                    done = 1;
            }
        }
    } while (!done);
    view_redraw();
}

void far loot_take(char far *coins, char far *items)             /* 26b7:0d01 */
{
    unsigned char key;
    char done;

    ui_mode = 1;
    done = 0;
    if (*coins) {
        if (*items) {
            do {
                if (*coins && *items) {
                    hotkey_reset(2);
                    key = node_prompt("Take:", "Money Items Exit", 1, 1);
                } else if (*coins) {
                    hotkey_reset(1);
                    key = node_prompt("Take:", "Money Exit", 1, 1);
                    if (!menu_escaped && key == 1)
                        key = 2;
                } else {
                    hotkey_reset(1);
                    key = node_prompt("Take:", "Items Exit", 1, 1);
                    if (!menu_escaped && key < 2)
                        key++;
                }
                if (menu_escaped) {
                    switch (key) {
                    case 0x1b:
                        done = 1;
                        break;
                    default:
                        party_rows(cur_node, 0);
                        party_key_select(key);
                        party_rows(cur_node, 1);
                    }
                } else {
                    switch (key) {
                    case 0:
                        take_menu();
                        view_redraw();
                        break;
                    case 1:
                        loot_items();
                        break;
                    case 2:
                        done = 1;
                        break;
                    default:
                        party_rows(cur_node, 0);
                        party_key_select(key);
                        party_rows(cur_node, 1);
                    }
                }
                pool_check(coins, items);
                if (!*items)
                    done = 1;
                party_display(cur_node);
            } while (!done);
        } else {
            take_menu();
            view_redraw();
        }
    } else
        loot_items();
}

void far loot_menu(void)                                         /* 26b7:0e93 */
{
    unsigned char spell_slot;
    unsigned char count;
    unsigned char key;
    char done;
    char changed;
    unsigned char spell;
    char detect;
    char saved;

    done = 0;
    view_redraw();
    saved = 1;
    do {
        mem_fill(hotkeys, sizeof hotkeys, 0xff);
        pool_check(&has_coins, &has_items);
        spell_slot = 0;
        detect = 0;
        if (has_items)
            while (spell_slot <= 0x88 && !detect) {
                if ((cur_node->spells[spell_slot] == 5 || cur_node->spells[spell_slot] == 0xb
                     || cur_node->spells[spell_slot] == 0x4d || cur_node->spells[spell_slot] == 0x3c)
                    && cur_node->enabled) {
                    detect = 1;
                    spell = cur_node->spells[spell_slot];
                }
                spell_slot++;
            }
        count = 0;
        for (spell_slot = 0; spell_slot <= 5; spell_slot++) {
            switch (spell_slot) {
            case 1:
                if (has_coins || has_items)
                    hotkey_add(1, &count);
                break;
            case 3:
                if (has_coins)
                    hotkey_add(3, &count);
                break;
            case 4:
                if (detect)
                    hotkey_add(4, &count);
                break;
            default:
                hotkey_add(spell_slot, &count);
            }
        }
        ui_mode = saved;
        set_menu_kind(0xc);
        key = node_prompt(empty_text, treasure_items, 1, 1);
        saved = ui_mode;
        if (menu_escaped) {
            switch (key) {
            case 0x1b:
                key = 5;
                break;
            case 0x84:
            case 0x88:
                party_rows(cur_node, 0);
                party_key_select(key);
                party_rows(cur_node, 1);
                key = 0xff;
                break;
            }
        }
        if (!get_aborted()) {
            switch (key) {
            case 0:
                char_view(&changed);
                break;
            case 1:
                loot_take(&has_coins, &has_items);
                break;
            case 2:
                pool_money();
                break;
            case 3:
                share_money();
                break;
            case 4:
                spell_cast(spell, 0, 0, &changed);
                break;
            case 5:
                pool_check(&has_coins, &has_items);
                if (has_coins || has_items) {
                    text_wrap(1, 0x11, 0x26, 0x16, 7, 0, 1, left_text);
                    text_wrap(1, 0x11, 0x26, 0x16, 0xb, 0, 0, claim_text);
                    if (confirm(empty_text, 0))
                        text_clear(1, 0x11, 0x26, 0x16);
                    else
                        done = 1;
                } else
                    done = 1;
                break;
            }
        } else
            set_aborted(0);
    } while (!done);
    sfx_play(0xff);
}

void far fight_cleanup(void)                                    /* 26b7:11b0 */
{
    struct Character far *character_cursor;
    struct Character far *next;

    game_state[STATE_DISABLED_ENEMY_COUNT] = 0;
    character_cursor = node_list;
    while (character_cursor) {
        if (character_cursor->combat->not_pc == 1 || character_cursor->side == 1) {
            killed = 1;
            if (!character_cursor->enabled)
                game_state[STATE_DISABLED_ENEMY_COUNT]++;
            next = character_cursor->next;
            cur_node = character_cursor;
            hero_drop(character_cursor->combat->not_pc == 1, 1);
            character_cursor = next;
        } else {
            if (character_cursor->combat) {
                pool_free(character_cursor->combat, 0x18, &combat_pool);
                character_cursor->combat = 0;
            }
            character_cursor = character_cursor->next;
        }
    }
    cur_node = node_list;
}

void far drops_return(void)                                      /* 26b7:1297 */
{
    struct Item far *item;
    struct Drop far *dropped_item;
    struct Drop far *old;
    char failed;

    dropped_item = drops;
    while (dropped_item) {
        pool_alloc(&item, sizeof(struct Item), &item_record_pool);
        cur_node = dropped_item->owner;
        *item = dropped_item->item;
        item->next = 0;
        item_give(item, &failed);
        pool_free(item, sizeof(struct Item), &item_record_pool);
        old = dropped_item;
        dropped_item = dropped_item->next;
        pool_free(old, sizeof(struct Drop), &dropped_item_pool);
    }
    drops = 0;
}

void far post_combat(void)                                       /* 26b7:135c */
{
    struct Item far *item;
    struct Item far *released_item;
    struct Character far *character;

    drops_return();
    game_state[STATE_INITIATIVE_SIDE_MASK] = 0;
    mem_zero(ecl_conditions, sizeof ecl_conditions);
    killed = 0;
    if (!demo_active)
        combat_results();
    view_mode = 6;
    fight_cleanup();
    if (!demo_active) {
        character = node_list;
        ecl_conditions[1] = 1;
        while (character) {
            recalc_stats(character);
            character = character->next;
        }
        if (!adventure_exit) {
            if (fled) {
                item = pool_items;
                while (item) {
                    released_item = item;
                    item = item->next;
                    rec_free(&released_item);
                }
                pool_items = 0;
            } else {
                ecl_conditions[0] = 1;
                ecl_conditions[1] = 0;
            }
            if (!demo_active) {
                combat_summary(exp_share);
                if (has_coins || has_items)
                    loot_menu();
            }
            pool_clear();
        } else {
            frame_border();
            text_col = 2;
            text_row = 6;
            text_wrap(2, 5, 0x25, 0x16, 7, 0, 1,
                      "The Dark Queen sneers with pleasure");
            text_wrap(2, 6, 0x25, 0x16, 7, 0, 1, "at your meaningless demise.");
            text_wrap(2, 7, 0x25, 0x16, 7, 0, 1,
                      "Nothing can stop her now... NOTHING!");
            continue_prompt(0, 0, continue_items);
        }
        text_slow = 1;
        game_state[STATE_NONPARTY_HIT_BONUS] = 0;
        game_state[STATE_PARTY_HIT_BONUS] = 0;
        game_state[STATE_PARTY_MOVE_BONUS] = 0;
        game_state[STATE_NO_ITEM_LOOT] = 0;
        game_state[STATE_NONLETHAL_COMBAT] = 0;
        game_state[STATE_NO_XP] = 0;
    }
    post_reset();
}

void far post_reset(void)                                        /* 26b7:151b */
{
    ai_level = 0;
    encounter_npc_count = 0;
    scripted_monsters_queued = 0;
    next_monster_group = 8;
    pool_clear();
}
