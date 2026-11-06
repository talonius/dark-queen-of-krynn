/* DQK.EXE overlay 03 module 1f4e: the party roster panel and the info line.
 * view_refresh does nothing while adventure_exit is set; otherwise it redraws
 * the view behind the panel: the big picture when game_state says one is up
 * and redraw_wanted asks for it, else the first-person view through 1d99:1111 in
 * view mode 4, and in view mode 3 unless game_state[STATE_WILDERNESS] is 1.
 * party_rows redraws the roster lines of the highlighted member and of
 * previous_highlighted_character, the member marked last time, and clears previous_highlighted_character;
 * party_display draws the whole roster, a name, AC and HP line per member
 * under a heading.  Both set the panel rectangle panel_x1..panel_y2, its
 * bottom row counting the members.  party_info_display writes the three info
 * rows, 0x0d to 0x0f, except in view mode 3: the time of day, the party's
 * position when game_state[STATE_MAP_RESTRICTED] is 0, the compass letter's glyph, and
 * "camping" or "searching".  party_key_select stores cur_node in previous_highlighted_character
 * whatever the key, and steps cur_node to the next member (keys 0x84 and
 * 0x85) or the previous one (0x87 and 0x88), wrapping at the ends.
 *
 * EXE 0x030ed0..0x031635, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 1f4e in RTLink section 3 (payload 0x30ed0, link
 * segment 1f4e): section 3's first module.  Its last 5 bytes lie in
 * paragraph 1fc4, whose module starts at 1fc4:0005.  Built with Turbo C
 * 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped order; bake
 * consumes them in that order.  Section 0's 1f4e link segment holds other
 * bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the
 * shipped call, or the DGROUP offset of an extern variable.  Callee and
 * variable names are placeholders chosen from use; an ovlNN_ name says
 * which section and stored segment the callee lives in, and a mod1d99_
 * callee is an RTLink vector stub whose target is named beside it.  camp.c
 * names 0065, 023b, 03f8 and 067c; gamemain.c names 023b and 03f8, and
 * party.c 023b and 067c.  Every routine here is also the target of a
 * section 3 vector stub: 0000 of 1d99:0f59, 0065 of 1d99:0e4b, 023b of
 * 1d99:0e73, 03f8 of 1d99:0f63, 067c of 1d99:0f95.
 *
 * _DATA is ten literals, DGROUP 5e56..5e98.  No _BSS is declared: nothing
 * found references DGROUP acac..acaf, the gap between aiturn.c's _BSS and
 * glbdata.c's, and the panel rectangle panel_x1..panel_y2, which only this
 * module addresses, has unreferenced bytes inside it (ae1c..ae1f) and
 * after it, so it is declared extern.
 */

#include "dqk.h"

void far view_refresh(void)                                      /* 1f4e:0000 */
{
    if (!adventure_exit) {
        if (game_state[STATE_DUNGEON] == 0 && game_state[STATE_WILDERNESS] == 1 && redraw_wanted) {
            big_open(game_state[STATE_WILDERNESS_PICTURE_ID]);
            big_redraw();
            redraw_wanted = 0;
        } else if (view_mode == 4 || (view_mode == 3 && game_state[STATE_WILDERNESS] != 1)) {
            view_draw(geo.x, geo.y, geo.facing);
            redraw_wanted = 0;
        }
    }
}

void far party_rows(struct Character far *highlighted_character, char hilite)            /* 1f4e:0065 */
{
    struct Character far *character_cursor;
    unsigned char col;
    unsigned char row;
    unsigned char band;

    if ((view_mode == 3 && game_state[STATE_WILDERNESS] == 1) || picture_hides_roster)
        return;
    if (previous_highlighted_character == 0)
        previous_highlighted_character = highlighted_character;
    if (view_mode == 0)
        col = 1;
    else
        col = 0x11;
    panel_x1 = col;
    panel_y1 = 4;
    panel_x2 = 0x26;
    panel_y2 = 3;
    row = 4;
    for (character_cursor = node_list; character_cursor != 0; character_cursor = character_cursor->next) {
        panel_y2++;
        if (character_cursor == previous_highlighted_character || character_cursor == highlighted_character) {
            if (hilite && character_cursor == highlighted_character)
                status_printf(col, row, 0xb, 0, "%s", character_cursor->name);
            else
                character_name_show(character_cursor, col, row, 0);
            if (character_cursor->ac[0] <= 50 && character_cursor->ac[0] >= 0)      /* >= 0 is compiled, as shipped */
                band = 1;
            else if (character_cursor->ac[0] <= 60 && character_cursor->ac[0] >= 51)
                band = 2;
            else if (character_cursor->ac[0] <= 69 && character_cursor->ac[0] >= 61)
                band = 1;
            else
                band = 0;
            show_ac(character_cursor, band + 0x20, row);
            if (character_cursor->hp > 99)
                band = 0;
            else if (character_cursor->hp > 9)
                band = 1;
            else
                band = 2;
            show_hp(character_cursor, band + 0x24, row, 0, 0);
        }
        row++;
    }
    if (row < 0xc)
        text_clear(col, row, 0x26, row);
    previous_highlighted_character = 0;
}

void far party_display(struct Character far *highlighted_character)                    /* 1f4e:023b */
{
    struct Character far *character_cursor;
    unsigned char col;
    unsigned char row;
    unsigned char band;
    unsigned char armor_class_value;

    if ((view_mode == 3 && game_state[STATE_WILDERNESS] == 1) || picture_hides_roster)
        return;
    if (view_mode == 0)
        col = 1;
    else
        col = 0x11;
    panel_x1 = col;
    panel_y1 = 4;
    panel_x2 = 0x26;
    panel_y2 = 3;
    row = 2;
    status_printf(col, row, 0xc, 0, "Name");
    status_printf(0x21, row, 0xc, 0, "AC\tHP");
    row += 2;
    for (character_cursor = node_list; character_cursor != 0; character_cursor = character_cursor->next) {
        panel_y2++;
        text_clear(col, row, 0x26, row);
        if (character_cursor == highlighted_character)
            status_printf(col, row, 0xb, 0, "%s", character_cursor->name);
        else
            character_name_show(character_cursor, col, row, 0);
        armor_class_value = character_cursor->ac[0];
        if (armor_class_value < 0 || armor_class_value > 69)                /* v < 0 is compiled, as shipped */
            band = 0;
        else if (armor_class_value > 50 && armor_class_value <= 60)
            band = 2;
        else
            band = 1;
        show_ac(character_cursor, band + 0x20, row);
        armor_class_value = character_cursor->hp;
        if (armor_class_value > 99)
            band = 0;
        else if (armor_class_value > 9)
            band = 1;
        else
            band = 2;
        show_hp(character_cursor, band + 0x24, row, 0, 0);
        row++;
    }
    if (row < 0xc)
        text_clear(col, row, 0x26, row);
}

void far party_info_display(void)                                   /* 1f4e:03f8 */
{
    unsigned char x;
    unsigned char y;
    char mins[4];
    char pos[42];
    char info[42];

    if (view_mode == 3)
        return;
    info[0] = 0;
    pos[0] = 0;
    x = geo.x;
    y = geo.y;
    two_digits(game_state[STATE_CLOCK_TEN_MINUTES] * 10 + game_state[STATE_CLOCK_MINUTE], mins);
    text_clear(0x1a, 0x0d, 0x26, 0x0f);
    if (game_state[STATE_CLOCK_HOUR] / 12 == 0) {
        if (game_state[STATE_CLOCK_HOUR] == 0)
            str_copy(info, msg_format("12:%s AM", mins));
        else
            str_copy(info, msg_format("%d:%s AM", game_state[STATE_CLOCK_HOUR], mins));
    } else if (game_state[STATE_CLOCK_HOUR] == 12)
        str_copy(info, msg_format("%d:%s PM", game_state[STATE_CLOCK_HOUR], mins));
    else
        str_copy(info, msg_format("%d:%s PM", game_state[STATE_CLOCK_HOUR] - 12, mins));
    status_printf(0x1a, 0x0f, 7, 0, info);
    if (game_state[STATE_MAP_RESTRICTED] == 0) {
        str_copy(pos, msg_format("%d,%d", x, y));
        status_printf(0x1a, 0x0d, 7, 0, pos);
    }
    switch (dir_names[geo.facing][0]) {
    case 'E':
        draw_glyph(0x1f40, 0x1f40, 1, 0x1c);
        break;
    case 'N':
        draw_glyph(0x1f40, 0x1f40, 1, 0x19);
        break;
    case 'S':
        draw_glyph(0x1f40, 0x1f40, 1, 0x1a);
        break;
    case 'W':
        draw_glyph(0x1f40, 0x1f40, 1, 0x1b);
        break;
    }
    str_cat(pos, " ");                  /* pos is not shown again, as shipped */
    if (view_mode == 2)
        status_printf(0x1a, 0x0e, 7, 0, "camping");
    else if (game_state[STATE_SEARCH_FLAGS] & 1)
        status_printf(0x1a, 0x0e, 7, 0, "searching");
}

void far party_key_select(unsigned char key)                      /* 1f4e:067c */
{
    struct Character far *character_cursor;
    register int key_code;

    previous_highlighted_character = cur_node;
    key_code = key;
    character_cursor = node_list;
    if (character_cursor == 0)
        return;
    if (key_code == 0x87 || key_code == 0x85 || key_code == 0x84 || key_code == 0x88) {
        switch (key_code) {
        case 0x87:
        case 0x88:
            if (character_cursor == cur_node)
                while (character_cursor->next != 0)
                    character_cursor = character_cursor->next;
            else
                while (character_cursor != 0 && character_cursor->next != cur_node)
                    character_cursor = character_cursor->next;
            break;
        case 0x84:
        case 0x85:
            if (cur_node->next != 0)
                character_cursor = cur_node->next;
            break;
        }
        cur_node = character_cursor;
    }
}
