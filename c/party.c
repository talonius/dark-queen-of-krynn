/* DQK.EXE overlay 07 module 289c: the party menu.  Creating, deleting,
 * modifying and training characters, a knight's petition to a new order,
 * viewing, adding and removing party members, loading and saving the game,
 * and leaving the menu to adventure or to DOS.
 *
 * EXE 0x04773d..0x048c33, one Turbo C large-model _TEXT segment starting at
 * byte 0xd of paragraph 289c, the last module of RTLink section 7 (payload
 * 0x447b0, link segment 25a4, parent section 3).  The previous module's
 * final retf is 289c:000c; 13 zero bytes follow this module's last retf to
 * the section's end.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions
 * are defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, and a mod1d99_ callee is an RTLink vector
 * stub whose target is named beside it.  ovl03_ callees are in section 3,
 * this section's parent, and are called directly.
 *
 * _DATA is DGROUP 7d98..7f53: knight_order's two requirement rows, then 25
 * stored literals; -d stores "" on the NUL of "Woof".  _BSS is three ints at
 * c7d8..c7dd, written here and read by nothing found.  The menu's enable
 * flags and the texts it shows from DGROUP 3cd2..3ece are extern.
 */

#include "dqk.h"

extern void far ui_mark(int id, int attr, ...);                  /* 1aa6:188f */

static int last_pick;                                            /* c7d8 */
static int pick_key;                                             /* c7da */
static int pick_first;                                           /* c7dc */

void far party_add(void);
static void party_delete(void);
void far party_drop(void);
static void knight_order(void);

char far party_key(int id, short key)                            /* 289c:000d */
{
    while (key_poll()) {
        if (key == 0x152)
            sound_toggle();
        else if (key == 0x153)
            music_toggle();
        key = key_get();
    }
    if (key == 0x152)
        sound_toggle();
    else if (key == 0x153)
        music_toggle();
    else if (cur_node) {
        if (key == 'J' || key == 'j') {
            if (str_cmp(msg_text(2), "Woof"))
                free_train = 0;
            else {
                free_train = !free_train;
                if (free_train)
                    error_box("training on");
                else
                    error_box("training off");
                if (!game_state[STATE_TRAINING_CLASSES])
                    party_items[3] = 0;
                ui_mark(id, 0x14);
                return 1;
            }
        } else if (key == 0x107 || key == 0x105 || key == 0x108 || key == 0x104) {
            previous_nonrose_knight = cur_node->level[1] &&
                        (cur_node->level[1] <= 0 || cur_node->knight_order != 3);
            if (key == 0x107 || key == 0x108)
                key = 0x87;
            else
                key = 0x85;
            party_key_select(key);
            party_display(cur_node);
            training_selection_changed = (cur_node->level[1] &&
                         (cur_node->level[1] <= 0 || cur_node->knight_order != 3)) ^ previous_nonrose_knight &&
                        game_state[STATE_TRAINING_CLASSES] > 0;
            return 1;
        }
    }
    return 0;
}

int far party_dialog(void)                                       /* 289c:01a7 */
{
    register int index;
    register int pick;

    frame_invalidate();
    dl_begin();
    ui_add(1, 0x1f7b, 0x1f94, "Create Character", 0x20, 'C', 0x24, 0x12, 0x14, 0x15,
           1, 0x1f82, 0x1f94, "Delete Character", 0x20, 'D', 0x24, 0x12, 0x14, 0x15,
           1, 0x1f89, 0x1f44, "Modify Character", 0x20, 'M', 0x24, 0x12, 0x14, 0x15,
           1, 0x1f90, 0x1f44, "Train Character", 0x20, 'T', 0x24, 0x12, 0x14, 0x15,
           0);
    ui_add(1, 0x1f97, 0x1f44, "Knight new Order", 0x20, 'K', 0x24, 0x12, 0x14, 0x15,
           1, 0x1f9e, 0x1f44, "View Character", 0x20, 'V', 0x24, 0x12, 0x14, 0x15,
           1, 0x1f7b, 0x1f44, "Add Character", 0x20, 'A', 0x24, 0x12, 0x14, 0x15,
           1, 0x1f82, 0x1f44, "Remove Character", 0x20, 'R', 0x24, 0x12, 0x14, 0x15,
           0);
    ui_add(1, 0x1f89, 0x1f94, "Load Saved Game", 0x20, 'L', 0x24, 0x12, 0x14, 0x15,
           1, 0x1f90, 0x1f94, "Save Current Game", 0x20, 'S', 0x24, 0x12, 0x14, 0x15,
           1, 0x1f97, 0x1f94, "Begin Adventuring", 0x20, 'B', 0x24, 0x12, 0x14, 0x15,
           1, 0x1f9e, 0x1f94, "Exit From Game", 0x20, 'E', 0x24, 0x12, 0x14, 0x15,
           0);
    ui_add(7, party_key, 0x14, 0);
    frame_row = 0x1f50;
    frame_col = 0x1f44;
    set_menu_kind(9);
    menu_frame(frame_row, frame_col);
    for (index = 0; index < 12; index++)
        if (party_items[index])
            ui_mark(index, 0x18);
        else
            ui_mark(index, 0x10);
    dl_draw(1);
    screen_flush_mode_set(0);
    screen_flush();
    pick = ui_run(0);
    if (get_picked()) {
        pick = 5;
        set_picked(0);
    }
    last_pick = pick;
    dl_end();
    return pick;
}

void far party_menu(char in_game)                                /* 289c:046f */
{
    struct Character far *character_cursor;
    short count;
    char redraw;
    register int pick;

    pick_key = 0x87;
    saved_view_mode = view_mode;
    view_mode = 0;
    training_selection_changed = 1;
    pick = 1;
    pick_first = 0;
    mode_change(6);
    for (;;) {
        screen_flush_mode_set(1);
        if (pick <= 11)
            frame_gen();
        else
            screen_draw_begin(1);
        if (cur_node) {
            party_display(cur_node);
            party_items[1] = 1;
            party_items[2] = 1;
            party_items[3] = game_state[STATE_TRAINING_CLASSES] > 0 || free_train;
            party_items[4] = party_items[3];
            party_items[5] = 1;
            party_items[6] = 1;
            count = 0;
            for (character_cursor = node_list; character_cursor; character_cursor = character_cursor->next)
                if (!(character_cursor->npc & 0x80))
                    count++;
            if (count > 5)
                party_items[6] = 0;
            party_items[7] = 1;
            party_items[8] = 1;
            party_items[9] = 1;
            party_items[10] = 1;
        } else {
            party_items[1] = 1;
            party_items[2] = 0;
            party_items[3] = 0;
            party_items[4] = 0;
            party_items[5] = 0;
            party_items[6] = 1;
            party_items[7] = 0;
            party_items[8] = 1;
            party_items[9] = 0;
            party_items[10] = 0;
        }
        pick = party_dialog();
        if (pick != 7 && pick <= 11)
            frame_border();
        if (pick == 1)
            party_display(cur_node);
        switch (pick) {
        case 0:
            if (party_items[0])
                hero_create();
            break;
        case 1:
            if (party_items[1])
                party_delete();
            break;
        case 2:
            if (party_items[2]) {
                modify_hero();
                game_saved = 0;
            }
            break;
        case 3:
            if (party_items[3]) {
                train_hero();
                game_saved = 0;
            }
            break;
        case 4:
            if (party_items[4])
                knight_order();
            break;
        case 5:
            if (party_items[5])
                char_view(&redraw);
            break;
        case 6:
            if (party_items[6]) {
                party_add();
                game_saved = 0;
            }
            break;
        case 7:
            if (party_items[7] && cur_node) {
                if (!(cur_node->npc & 0x80)) {
                    character_file_save(cur_node, "");
                    hero_drop(0, 1);
                } else {
                    frame_border();
                    party_drop();
                }
                if (!cur_node) {
                    if (in_game) {
                        game_reset();
                        view_mode = 0;
                    } else
                        adventure_exit = 1;
                }
                game_saved = 0;
            }
            break;
        case 8:
            if (party_items[8] &&
                (!cur_node || game_saved || confirm("Game not saved. Load anyway? ", 1))) {
                if (in_game) {
                    while (cur_node)
                        hero_drop(0, 1);
                    game_load_menu();
                    if (!cur_node) {
                        game_reset();
                        saved_view_mode = view_mode;
                        view_mode = 0;
                    }
                } else {
                    pending_load_set(1);
                    adventure_exit = 1;
                    return;
                }
            }
            break;
        case 9:
            if (party_items[9] && node_list)
                game_save_menu();
            break;
        case 10:
            if (party_items[10] && (node_list || demo_active)) {
                if (!(view_mode = saved_view_mode))
                    view_mode = 4;
                if (!game_loaded && view_kind != 9) {
                    if (view_mode == 3 && game_state[STATE_WILDERNESS] == 1)
                        frame_lower_panel();
                    else
                        frame_main();
                    party_display(cur_node);
                } else
                    switch (view_mode) {
                    case 0:
                    case 4:
                        frame_main();
                        party_display(cur_node);
                        break;
                    case 3:
                        frame_lower_panel();
                        break;
                    }
                box_draw();
                game_state[STATE_TRAINING_CLASSES] = 0;
                game_saved = 0;
                return;
            }
            break;
        case 11:
            if (party_items[11] && confirm(exit_text, 0)) {
                if (node_list && !game_saved && !confirm(exit_unsaved_text, 0))
                    game_save_menu();
                else
                    game_exit();
            }
            break;
        }
        if (!pick == 10 && pick <= 11)  /* 289c:086b, always false as shipped */
            frame_border();
    }
}

char far in_party(char far *name)                                /* 289c:088f */
{
    struct Character far *character;

    for (character = node_list; character; character = character->next)
        if (str_same(character->name, name))
            return 1;
    return 0;
}

void far party_add(void)                                         /* 289c:08dc */
{
    struct Node far *list;
    struct Node far *files;
    struct Node far *sel;
    struct Node far *file;
    struct Character far *hero;
    struct Character far *character;
    int index;
    char redraw;
    unsigned char players;
    unsigned char rangers;
    unsigned char choice;

    text_clear(1, 1, 0x26, 0x16);
    hotkey_reset(2);
    choice = menu_run(add_where_text, source_items, 0, 0);
    if (menu_escaped && choice == 0x1b)
        choice = 2;
    switch (choice) {
    case 1:
        file_kind = 4;
        break;
    case 2:
        return;
    default:
        file_kind = 1;
        break;
    }
    character_files_scan(&list, &files, 0);
    if (!list)
        return;
    sel = list;
    index = 0;
    do {
        for (sel = list; sel; sel = sel->next)
            if (in_party(sel->text))
                str_copy(sel->text, msg_format("* %s", sel->text));
        redraw = 1;
        hotkey_reset(1);
        choice = list_menu(add_text, add_items, 1, 2, 0x26, 0x16, list, 1, 0,
                           &redraw, &index, &sel);
        if (menu_escaped && choice == 0x1b)
            choice = 1;
        if (choice == 0 && sel->text[0] != '*') {
            pool_alloc(&hero, 0x19e, &party_pool);
            file = list_nth(index, files);
            character_file_load(file->text, hero, 0, 1);
            str_copy(sel->text, msg_format("* %s", sel->text));
            players = 0;
            if (!node_list) {
                game_state[STATE_PARTY_COUNT] = 0;
                party_character_append(hero);
                character_icon_load(1);
            } else {
                character = node_list;
                rangers = 0;
                /* the names' addresses are compared, never their text */
                while (character && character->name != hero->name) {
                    if (character->npc < 0x80)
                        players++;
                    if (character->level[4] > 0)
                        rangers++;
                    character = character->next;
                }
                if (!character &&
                    ((!(hero->npc & 0x80) && players < 6) ||
                     ((hero->npc & 0x80) && game_state[STATE_PARTY_COUNT] < 8)) &&
                    (!hero->level[4] || rangers < 3)) {
                    party_character_append(hero);
                    character_icon_load(1);
                    if (!(hero->npc & 0x80))
                        players++;
                } else {
                    if (hero->level[4] > 0 && rangers > 2)
                        bottom_msg("too many rangers in party");
                    pool_free(hero, 0x19e, &party_pool);
                }
            }
        }
    } while (choice != 1 && players <= 5 && game_state[STATE_PARTY_COUNT] <= 7);
    character_file_lists_free(list, files);
}

static void party_delete(void)                                   /* 289c:0c23 */
{
    struct Node far *list;
    struct Node far *files;
    struct Node far *sel;
    struct Node far *file;
    struct Node far *previous_entry;
    char prompt[42];
    int index;
    char redraw;
    char choice;

    text_clear(1, 1, 0x26, 0x16);
    file_kind = 1;
    character_files_scan(&list, &files, 1);
    if (!list)
        return;
    sel = list;
    index = 0;
    while (sel) {
        if (in_party(sel->text))
            str_copy(sel->text, msg_format("* %s", sel->text));
        sel = sel->next;
    }
    do {
        redraw = 1;
        hotkey_reset(1);
        choice = list_menu(delete_text, delete_items, 1, 2, 0x26, 0x16, list, 1, 0,
                           &redraw, &index, &sel);
        if (menu_escaped && choice == 0x1b)
            choice = 1;
        if (choice == 0) {
            str_copy(prompt, msg_format("Delete %s forever? ",
                                        sel->text + (sel->text[0] == '*' ? 2 : 0)));
            if (confirm(prompt, 0) && confirm("Are you sure? ", 0)) {
                file = list_nth(index, files);
                archive_open('S', file->text, 3, 0);
                if (list == sel)
                    list = list->next;
                else {
                    for (previous_entry = list; previous_entry->next != sel; previous_entry = previous_entry->next)
                        ;
                    previous_entry->next = previous_entry->next->next;
                }
                pool_free(sel, 0x28, node_pool);
                if (files == file)
                    files = files->next;
                else {
                    for (previous_entry = files; previous_entry->next != file; previous_entry = previous_entry->next)
                        ;
                    previous_entry->next = previous_entry->next->next;
                }
                pool_free(file, 0x28, node_pool);
            }
        }
    } while (choice != 1 && list);
    character_file_lists_free(list, files);
}

void far party_drop(void)                                        /* 289c:0ee9 */
{
    char prompt[42];

    if (cur_node) {
        str_copy(prompt, msg_format("Drop %s forever? ", cur_node->name));
        if (confirm(prompt, 0) && confirm("Are you sure? ", 0)) {
            if (!cur_node->enabled)
                bottom_msg(msg_format("You dump %s out back.", cur_node->name));
            else
                bottom_msg(msg_format("%s bids you farewell.", cur_node->name));
            hero_drop(0, 1);
        } else
            bottom_msg(msg_format("%s breathes a sigh of relief.", cur_node->name));
    }
    party_display(cur_node);
}

void far knight_advance(struct Character far *character)                      /* 289c:0fe0 */
{
    char loss;
    unsigned char old;
    char adjust;

    character->knight_order++;
    while (level_xp_threshold(character, character->level[1], 1) > character->xp) {
        old = character->level[1];
        character->level[1]--;
        if (old > 9) {
            loss = 2;
            adjust = 0;
        } else {
            loss = character->hp_base / (old + 1);
            adjust = con_hp_adj(cur_node, 1);
        }
        character->hp_base -= loss;
        character->hp_max -= loss + adjust;
        if (character->hp >= character->hp_max)
            character->hp = character->hp_max;
    }
    recalc_from_levels(character);
    character->level_up = can_level_up(character);
}

static void knight_order(void)                                   /* 289c:10c2 */
{
    int sword_need[6] = { 12, 9, 13, 9, 10, 3 };
    int rose_need[6] = { 15, 10, 13, 12, 15, 3 };
    char order[8];
    unsigned char stat_index;
    char ok;
    char choice;
    char col;
    struct Character far *character;

    character = cur_node;
    frame_border();
    if (!character->level[1] || (character->level[1] > 0 && character->knight_order == 3))
        return;
    col = 0x13 - str_length(character->name) / 2;
    status_printf(col, 3, 7, 0, character->name);
    status_printf(7, 5, 7, 0, petition_text);
    switch (character->knight_order) {
    case 1:
        str_copy(order, sword_text);
        break;
    case 2:
        str_copy(order, rose_text);
        break;
    }
    status_printf(10, 7, 7, 0, msg_format("%s%s", knight_text, order));
    ok = 0;
    switch (character->knight_order) {
    case 1:
        col = 0x13 - (str_length(character->name) + 0x15) / 2;
        if (character->level[1] < 3 || character->xp < xp_by_level[4][3])
            status_printf(col, 9, 7, 0, "%s%s", character->name, inexperienced_text);
        else {
            col = 0x13 - (str_length(character->name) + 0x12) / 2;
            ok = 1;
            for (stat_index = 0; stat_index <= 5; stat_index++)
                if (sword_need[stat_index] > character->stats[stat_index].base)
                    ok = 0;
            if (!ok)
                status_printf(col, 9, 7, 0, "%s%s", character->name, unqualified_text);
            else {
                col = 0x13 - (str_length(character->name) + 0xd) / 2;
                status_printf(col, 9, 7, 0, "%s%s", character->name, may_become_text);
                status_printf(10, 0xb, 7, 0, "%s%s", knight_text, sword_text);
            }
        }
        break;
    case 2:
        col = 0x14 - (str_length(character->name) + 0x15) / 2;
        if (character->level[1] < 4 || character->xp < xp_by_level[5][4])
            status_printf(col, 9, 7, 0, "%s%s", character->name, inexperienced_text);
        else {
            col = 0x14 - (str_length(character->name) + 0x12) / 2;
            ok = 1;
            for (stat_index = 0; stat_index <= 5; stat_index++)
                if (rose_need[stat_index] > character->stats[stat_index].base)
                    ok = 0;
            if (!ok)
                status_printf(col, 9, 7, 0, "%s%s", character->name, unqualified_text);
            else {
                col = 0x13 - (str_length(character->name) + 0xd) / 2;
                status_printf(col, 9, 7, 0, "%s%s", character->name, may_become_text);
                status_printf(10, 0xb, 7, 0, msg_format("%s%s", knight_text, rose_text));
            }
        }
        break;
    }
    if (ok) {
        hotkey_reset(1);
        choice = menu_run(empty_text, change_items, 0, 0);
        if (!choice)
            knight_advance(character);
    }
}
