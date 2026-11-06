/* DQK.EXE overlay 06 module 291e: moving the party.
 * The check that the square ahead is still inside the area (clamping the
 * party to its edge and flagging a map exit when it is not), the dungeon
 * step with its delay, wall and event tests and the clock, a step on the
 * wilderness map, the list of the quest items the party holds, the
 * adventure menu that drives them, and resolving a queued move.
 *
 * EXE 0x043a69..0x04421f, one Turbo C large-model _TEXT segment starting at
 * byte 9 of paragraph 291e in RTLink section 6 (payload 0x402c0, link
 * segment 25a4, parent section 3).  The first 9 bytes of paragraph 291e are
 * the tail of section 6's module 25a4, and the last f bytes of the range lie
 * in paragraph 2999, whose module starts at 2999:000f (section 6's table
 * files each site in the range under frame 291e).  Built with Turbo C 2.01,
 * tcc -ml -O -Z -d.  Functions are defined in shipped order; bake consumes
 * them in that order.  Sections 4, 5, 7 and 8 hold other bytes at link
 * segment 25a4.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.  Section 3 reaches
 * two of these routines through vector stubs: 1d99:135f -> 0753 (called
 * at 0x3301a) and 1d99:1369 -> 0365 (at 0x32f13 and 0x32fa0).
 *
 * The routines follow the order of DKK's movement.pas (dkk-disassembly):
 * moveForward (v1.0's checkMapExit), movePartyForward3D,
 * movePartyWilderness, mapMainMenu and resolveMovement, with quest_list,
 * for which movement.pas has no counterpart, before the menu that calls it.
 * quest_list hands list_free the walk pointer, which the walk leaves at 0,
 * rather than the list's head, so the list it built is not freed, as
 * shipped (side comment there).
 *
 * _DATA is DGROUP 78c8..7b95: quest_names, whose 40 far pointers are
 * relocated, then its 40 literals and "Empty!"; nothing outside this
 * module reads the table.  The module declares no _BSS.
 */

#include "dqk.h"

extern void far status_msg(char far *fmt, char foreground, char background);     /* 0a62:0904 */

static char far *quest_names[40] = {                                    /* 78c8 */
    "Crylokon", "Golden Key", "Rusty Key", "Ring of Deception",
    "Iron Dragon Scale", "Book of Amrocar", "A Piece of the Oracle",
    "Two pieces of the Oracle", "Three pieces of the Oracle",
    "Four pieces of the Oracle", "The Oracle of Tengur", "Circlet of Gold",
    "Heart Shaped Key", "Clay Pass", "Keyhole Pass", "Lantern Pass",
    "Sword Pass", "Quill Pass", "Swamp Pass", "Baton Pass", "Incense Pass",
    "Ziggurat Pass", "Davik Release", "Prison Keys", "Signet Ring",
    "Odd Contraption", "Thenol Army Uniforms", "Thenol Palace Uniforms",
    "Big Grey Cat", "Mislaxa Talisman", "Item 30", "Item 31", "Item 32",
    "Item 33", "Item 34", "Item 35", "Item 36", "Item 37", "Item 38",
    "Item 39"
};

/* The translated Pascal unit's empty initialization block (DKK explore.pas). */
void far move_unit_init(void)                                         /* 291e:0009 */
{
}

void far map_exit_check(void)                                    /* 291e:000a */
{
    short nx;
    short ny;
    unsigned short face;

    nx = geo.x;
    ny = geo.y;
    face = geo.facing;
    game_state[STATE_MAP_EXIT] = 0;
    if (wall_passable(nx, ny, face)) {
        nx += dir_dx[face];
        ny += dir_dy[face];
        if (geo.area->cols - 1 < nx) {
            geo.x = geo.area->cols - 1;
            game_state[STATE_MAP_EXIT] = 1;
        }
        if (nx < 0) {
            geo.x = 0;
            game_state[STATE_MAP_EXIT] = 1;
        }
        if (geo.area->rows - 1 < ny) {
            geo.y = geo.area->rows - 1;
            game_state[STATE_MAP_EXIT] = 1;
        }
        if (ny < 0) {
            geo.y = 0;
            game_state[STATE_MAP_EXIT] = 1;
        }
    }
}

void far step_dungeon(void)                                      /* 291e:00c6 */
{
    sfx_play(0xb);
    delay(50);
    geo.x += dir_dx[geo.facing];
    geo.y += dir_dy[geo.facing];
    if (geo.x < 0)
        geo.x = geo.area->cols - 1;
    if (geo.area->cols - 1 < geo.x)
        geo.x = 0;
    if (geo.y < 0)
        geo.y = geo.area->rows - 1;
    if (geo.y > geo.area->rows - 1)
        geo.y = 0;
    geo.byte_3 = wall_side(geo.x, geo.y, geo.facing);
    geo.flag_4 = area_cell_at(geo.x, geo.y);
    if ((game_state[STATE_SEARCH_FLAGS] & 1) > 0)
        clock_advance(1, 2);
    else
        clock_advance(1, 1);
}

void far step_wild(void)                                         /* 291e:01a7 */
{
    char nx;
    char ny;

    cursor_hide();
    game_state[STATE_PREVIOUS_X] = game_state[STATE_WILDERNESS_X];
    game_state[STATE_PREVIOUS_Y] = game_state[STATE_WILDERNESS_Y];
    nx = game_state[STATE_WILDERNESS_X] + dir_dx[geo.facing];
    ny = game_state[STATE_WILDERNESS_Y] + dir_dy[geo.facing];
    if (nx < 0)
        nx = 0;
    if (nx > 0x25)
        nx = 0x25;
    if (ny < 0)
        ny = 0;
    if (ny > 0xe)
        ny = 0xe;
    game_state[STATE_WILDERNESS_X] = nx;
    game_state[STATE_WILDERNESS_Y] = ny;
    cursor_show();
    clock_advance(0xc, 3);
}

void far quest_list(void)                                        /* 291e:0241 */
{
    struct Node far *node;
    struct Node far *list;
    char unused_list_arg_a;
    char redraw;
    char unused_list_arg_b;
    short top;
    register int count;
    register int index;

    unused_list_arg_a = 1;
    unused_list_arg_b = 2;
    redraw = 1;
    node = 0;
    list = 0;
    top = 0;
    count = 0;
    for (index = STATE_QUEST_ITEMS - 1; index <= STATE_QUEST_ITEMS + STATE_QUEST_ITEMS_COUNT - 2; index++)
        if (game_state[index + 1] != 0)
            count++;
    if (count != 0) {
        list_alloc(count, &list);
        node = list;
        for (index = STATE_QUEST_ITEMS - 1; node != 0; index++) {
            if (game_state[index + 1] != 0) {
                str_copy(node->text, quest_names[index - (STATE_QUEST_ITEMS - 1)]);
                node->key = 0;
                node = node->next;
            }
        }
        frame_border();
        hotkey_reset(1);
        list_menu(empty_text, exit_items, 1, 1, 0x26, 0x16, list, unused_list_arg_a, unused_list_arg_b,
                  &redraw, &top, &list);
        list_free(&node);       /* node, 0 after the walk: frees nothing */
    } else
        bottom_msg("Empty!");
}

char far adv_menu(void)                                          /* 291e:0365 */
{
    char return_code;
    char done;
    unsigned char menu_choice;
    unsigned char count;
    char view;
    char saved;
    unsigned char scan_index;
    unsigned char y;
    unsigned char x;

    game_state[STATE_MOVE_BLOCK] = 0;
    saved = ui_mode;
    switch (view_mode) {
    case 4:
        done = 0;
        menu_choice = 0xff;
        do {
            count = 0;
            for (scan_index = 1; scan_index <= 7; scan_index++)
                hotkey_add(scan_index, &count);
            ui_mode = saved;
            menu_choice = node_dir_menu(empty_text, explore_items, 1, 0);
            saved = ui_mode;
            exploration_menu_context = 1;
            if (menu_escaped == 0) {
                switch (menu_choice) {
                case 0:
                    box_draw();
                    exploration_menu_context = 1;
                    break;
                case 1:
                    if (game_state[STATE_MAP_RESTRICTED] == 0 ||
                        str_cmp(msg_text(3), debug_password) == 0) {
                        geo.show_map = !geo.show_map;
                        view_draw(geo.x, geo.y, geo.facing);
                    } else
                        status_msg(not_here_text, 0xb, 0);
                    break;
                case 2:
                    if (cur_node->status == 0)
                        cast_menu();
                    break;
                case 3:
                    char_view(&view);
                    break;
                case 4:
                    done = 1;
                    break;
                case 5:
                    game_state[STATE_SEARCH_FLAGS] = game_state[STATE_SEARCH_FLAGS] ^ 1;
                    party_info_display();
                    break;
                case 6:
                    game_state[STATE_SEARCH_FLAGS] = game_state[STATE_SEARCH_FLAGS] | 2;
                    party_info_display();
                    clock_advance(1, 2);
                    ecl_ip = ecl_after_move;
                    done = 1;
                    break;
                case 7:
                    quest_list();
                    view_redraw();
                    break;
                default:
                    party_rows(cur_node, 0);
                    party_key_select(menu_choice);
                    party_rows(cur_node, 1);
                    party_info_display();
                }
            } else {
                switch (menu_choice) {
                case 0x88:
                    map_exit_check();
                    done = 1;
                    party_info_display();
                    menu_choice = 0xff;
                    break;
                case 0x84:
                    geo.facing = (geo.facing + 4) % 8;
                    geo.byte_3 = wall_side(geo.x, geo.y, geo.facing);
                    view_draw(geo.x, geo.y, geo.facing);
                    party_info_display();
                    menu_choice = 0xff;
                    break;
                case 0x86:
                    geo.facing = (geo.facing + 6) % 8;
                    geo.byte_3 = wall_side(geo.x, geo.y, geo.facing);
                    view_draw(geo.x, geo.y, geo.facing);
                    party_info_display();
                    menu_choice = 0xff;
                    break;
                case 0x82:
                    geo.facing = (geo.facing + 2) % 8;
                    geo.byte_3 = wall_side(geo.x, geo.y, geo.facing);
                    view_draw(geo.x, geo.y, geo.facing);
                    party_info_display();
                    menu_choice = 0xff;
                    break;
                case 0x1b:
                    exploration_menu_context = 0;
                    break;
                default:
                    party_key_select(menu_choice);
                    party_display(cur_node);
                }
            }
        } while (done == 0);
        break;
    case 3:
        exploration_menu_context = 1;
        if (game_state[STATE_WILDERNESS] != 0) {
            done = 0;
            do {
                mem_fill(hotkeys, sizeof hotkeys, 0xff);
                count = 0;
                mem_fill(hotkeys, sizeof hotkeys, 0xff);
                hotkey_add(4, &count);
                ui_mode = saved;
                x = game_state[STATE_WILDERNESS_X];
                y = game_state[STATE_WILDERNESS_Y];
                menu_choice = grid_small(empty_text, explore_items, 1, 0, y, x);
                saved = ui_mode;
                if (menu_escaped == 0) {
                    if (menu_choice == 4)
                        done = 1;
                } else {
                    done = 1;
                    switch (menu_choice) {
                    case 0x88: geo.facing = 0; break;
                    case 0x81: geo.facing = 1; break;
                    case 0x82: geo.facing = 2; break;
                    case 0x83: geo.facing = 3; break;
                    case 0x84: geo.facing = 4; break;
                    case 0x85: geo.facing = 5; break;
                    case 0x86: geo.facing = 6; break;
                    case 0x87: geo.facing = 7; break;
                    default: done = 0;
                    }
                    party_info_display();
                }
            } while (done == 0);
        }
        break;
    }
    if (script_text_cleared == 0) {
        text_clear(1, 0x11, 0x26, 0x16);
        script_text_cleared = 1;
    }
    return_code = menu_choice;
    return return_code;
}

void far move_resolve(void)                                      /* 291e:0753 */
{
    turn_done = 0;
    switch (view_mode) {
    case 4:
        if (game_state[STATE_MOVE_BLOCK] < 0xff) {
            redraw_wanted = 1;
            if (wall_passable(geo.x, geo.y, geo.facing) == 1) {
                turn_done = 1;
                step_dungeon();
            }
            party_info_display();
        } else
            game_state[STATE_MOVE_BLOCK] = 0;
        break;
    case 3:
        if (game_state[STATE_MOVE_BLOCK] < 0xff) {
            step_wild();
            party_info_display();
        }
        break;
    default:
        game_state[STATE_MOVE_BLOCK] = 0;
    }
    pic_release();
}
