/* DQK.EXE overlay 06 module 25a4: the ECL script commands.  Starting a
 * script and loading its file, the script's view of game memory (reading
 * and writing its variables, the party member fields behind them, and the
 * packed and stored strings), the menus a script shows, comparing two
 * values or two strings, a gosub, a step forward, robbing and damaging the
 * party, the view refresh after a command, and 24 of the commands
 * ecl_dispatch sends here: print, new script, load files, the two menus,
 * treasure, damage, spell, experience, rob, the monster commands, input,
 * find item, who, add npc, destroy items, find name, journal entry and
 * shop; also the wandering monsters and the scroll a treasure or shop makes.
 *
 * EXE 0x0402c0..0x043a69, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 25a4, the first module of RTLink section 6 (payload
 * 0x402c0, link segment 25a4, parent section 3).  Its final retf is byte 8
 * of paragraph 291e, where move.c begins at 291e:0009; section 6's table
 * files all 290 fixup sites in the range under frame 25a4 and all 50 after
 * it under 291e or 2999.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Sections 4, 5, 7 and 8 hold other bytes at link segment 25a4.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, an ovlNN_ name which section and stored
 * segment, and a mod1d99_ callee is an RTLink vector stub whose target is
 * named beside it.  39 of these routines are the targets of section 6's
 * vectors, stubs 1d99:11a7..1d99:137d.  hero_write is static, near-called
 * by gb_write ahead of its definition; the four string compares are
 * far-called by compare_str ahead of theirs.
 *
 * The routines from ecl_init to view_update follow the order of DKK's
 * EclCore unit (eclcore.pas, dkk-disassembly), with party_hp_span added and
 * hero_write (DKK's specialWriteZ1) moved after scroll_make; the commands
 * do not follow the order of DKK's eclcmds.pas.
 *
 * _DATA is DGROUP 75aa..78c6: the wandering-monster and shop tables, then
 * 21 stored literals; -d stores "" as the NUL of "ECL1" and " " as the tail
 * of " dies. ".  Nothing outside this module reads them.  _BSS is c7c6..c7d7,
 * the three monster tables cmd_load_monster fills for monsters_place; their
 * names are chosen to fit Turbo C's name order.
 */

#include "dqk.h"

extern void far *far get_selected(void);                         /* 0bdb:02e1 */

static int wander_ids[7] = { 0x14, 0x14, 0x2a, 0x11, 0x3d, 0x15, 7 };   /* 75aa */
static int wander_icons[7] = { 0x14, 0x14, 0x2a, 0x11, 0x3d, 0x15, 7 }; /* 75b8 */
static int shop_first[9] = {    /* 75c6 */
    1, 0x20, 0x26, 0x2e, 0x36, 0x3e, 0x43, 0x48, 0x4d
};
static int shop_last[9] = {     /* 75d8 */
    0x1f, 0x25, 0x2d, 0x35, 0x3d, 0x42, 0x47, 0x4c, 0x79
};
static int shop_items[170] = {  /* 75ea: item ids, shop_first..shop_last from 1 */
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e,
    0x1f, 0x20, 0x21, 0x22, 0x23, 0x31, 0xdf, 0x24, 0x25, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x2b, 0x05, 0x09, 0x01, 0x0b, 0x0f,
    0x14, 0x3d, 0x56, 0x16, 0x19, 0x01, 0x0b, 0x0f, 0x14, 0x3b,
    0x50, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xfa, 0xfa, 0xff, 0xff,
    0xff, 0x2f, 0xd0, 0xee, 0xf0, 0xf1, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22,
    0x23, 0x31, 0xdf, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
    0x2b
};

static unsigned char mon_pic[6];                                 /* c7c6 */
static unsigned char mon_ids[6];                                 /* c7cc */
static unsigned char mon_count[6];                               /* c7d2 */

static void far hero_write(unsigned short addr, unsigned short val);
char far str_gt(char far *left_value, char far *right_value);
char far str_lt(char far *left_value, char far *right_value);
char far str_ge(char far *left_value, char far *right_value);
char far str_le(char far *left_value, char far *right_value);

/* The translated Pascal unit's empty initialization block (DKK eclmenu.pas). */
void far eclcmds_unit_init(void)                                   /* 25a4:0000 */
{
}

void far ecl_init(void)                                          /* 25a4:0001 */
{
    confirm_anim = 0;
    ecl_initial_redraw = 1;
    mem_fill(encounter_images_shown, 2, 0);
    next_monster_group = 8;
    ecl_ip = 0x8000;
    turn_done = 0;
    ecl_stack = 0;
    mem_fill(ecl_conditions, 4, 0);
    game_state[STATE_REST_ENCOUNTER_INTERVAL] = 0;
    game_state[STATE_REST_ENCOUNTER_CHANCE] = 0;
    ecl_load_args(1);
    ecl_before_move = (ecl_hi[1] << 8) | ecl_lo[1];
    ecl_load_args(1);
    ecl_after_move = (ecl_hi[1] << 8) | ecl_lo[1];
    ecl_load_args(1);
    ecl_before_camp = (ecl_hi[1] << 8) | ecl_lo[1];
    ecl_load_args(1);
    ecl_after_camp = (ecl_hi[1] << 8) | ecl_lo[1];
    ecl_load_args(1);
    ecl_main_entry = (ecl_hi[1] << 8) | ecl_lo[1];
    if (!game_loaded)
        for (tick_slot = 0; tick_slot < STATE_SCRIPT_LOCALS_COUNT; tick_slot++)
            game_state[tick_slot + STATE_SCRIPT_LOCALS] = 0;
}

void far ecl_load(unsigned char id)                              /* 25a4:0118 */
{
    int len;

    monster_unpack_in_progress = 0;
    set_data_disk(0x33);
    data_load("ECL1", 0L, id, &len, ecl_memory + 0x8000);
}

void far hero_load(char id, struct Character far * far *character_ref,
                   struct Item far * far *items,
                   struct Effect far * far *effect_ref)                  /* 25a4:0155 */
{
    *character_ref = 0;
    pool_alloc((void far * far *)character_ref, 0x19e, &npc_pool);
    mon_load(id, 0, *character_ref);
    *items = (*character_ref)->items;
    *effect_ref = (*character_ref)->effects;
}

unsigned char far monster_dist(char x, char y, unsigned char facing)
                                                                 /* 25a4:01c3 */
{
    unsigned char result;
    char blocked;
    unsigned char dist;

    dist = 0;
    blocked = 0;
    result = 0;
    if (game_state[STATE_DUNGEON] == 0) {
        result = 2;
        game_state[STATE_ENCOUNTER_DISTANCE_LIMIT] = 2;
    } else
        while (dist < 2 && !blocked) {
            if (wall_side(x, y, facing) == 0) {
                dist++;
                result = dist;
                switch (facing) {
                case 0: y--; break;
                case 2: x++; break;
                case 4: y++; break;
                case 6: x--; break;
                }
            } else
                blocked = 1;
        }
    return result;
}

void far monster_show(unsigned char sprite, unsigned char picture,
                      unsigned char dist, char far *shown)       /* 25a4:0255 */
{
    char size;

    size = dist + 1;
    if (!shown[1] && sprite != 0xff) {
        screen_flush_mode_set(1);
        if (!shown[0]) {
            if (geo.show_map) {
                geo.show_map = 0;
                redraw_wanted = 1;
                view_refresh();
            }
            if (game_state[STATE_DUNGEON]) {
                pic_open("SPRIT", sprite, 1, &pic);
                shown[0] = 1;
                encounter_sprite_shown = 1;
            }
        } else {
            redraw_wanted = 1;
            view_refresh();
        }
        if (view_mode == 4)
            pic_show(size);
        screen_flush_mode_set(0);
    }
    if (!shown[1] && picture != 0xff && (dist == 0 || sprite == 0xff)
            && view_mode == 4) {
        confirm_anim = 1;
        pic_open("PIC", picture, 0, &pic);
        shown[1] = 1;
        pic_frame(3, 3, 1, 0);
    }
}

char far gb_char(unsigned char encoded_char)                                /* 25a4:0354 */
{
    char decoded_char;

    if (encoded_char > 0x1f)
        decoded_char = encoded_char;
    else
        decoded_char = encoded_char + 0x40;
    return decoded_char;
}

char far gb_zone(unsigned short addr)                            /* 25a4:0377 */
{
    if (addr >= 1 && addr <= 0x400)
        return 0;
    if (addr >= 0x401 && addr <= 0x800)
        return 1;
    if (addr >= 0x8000 && addr <= 0xaee2)
        return 2;
    return 4;
}

unsigned char far hero_index(struct Character far *character)                 /* 25a4:03b3 */
{
    unsigned char result;
    unsigned char idx;
    struct Character far *character_cursor;
    char found;

    character_cursor = node_list;
    found = 0;
    idx = 0;
    while (character_cursor && !found) {
        if (character_cursor == character)
            found = 1;
        else {
            idx++;
            character_cursor = character_cursor->next;
        }
    }
    result = idx;
    return result;
}

unsigned far hero_read(unsigned short addr, char size)           /* 25a4:040d */
{
    unsigned short value;       /* size is passed and never read */

    value = 0;
    addr = addr - 0x401;
    if (addr >= 0x1e && addr <= 0xaa)
        value = cur_node->spells[addr - 0x1e];
    else
        switch (addr) {
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
        case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b:
            value = ((unsigned char far *)&cur_node->stats[(addr - 0x10) / 2])[addr & 1];
            break;
        case 0x1c: value = cur_node->str_ex; break;
        case 0x1d: value = cur_node->str_ex_base; break;
        case 0xad: value = cur_node->race; break;
        case 0xae: value = cur_node->cls; break;
        case 0xb0:
        case 0xb1: value = cur_node->age; break;
        case 0xb2: value = cur_node->hp_max; break;
        case 0xc8: value = cur_node->knight_order; break;
        case 0xc9: value = cur_node->deity; break;
        case 0xca: value = cur_node->robe_order; break;
        case 0x137: value = cur_node->move_base; break;
        case 0x13b: case 0x13c: case 0x13d: case 0x13e:
        case 0x13f: case 0x140: case 0x141: case 0x142:
            value = cur_node->thief[addr - 0x13b];
            break;
        case 0x14b: case 0x14c: case 0x14d: case 0x14e: case 0x14f: case 0x150:
            value = cur_node->coins[(addr - 0x14b) / 2];
            break;
        case 0x151: case 0x152: case 0x153: case 0x154: case 0x155: case 0x156:
        case 0x157:
            value = cur_node->level[addr - 0x151];
            break;
        case 0x158: case 0x159: case 0x15a: case 0x15b: case 0x15c: case 0x15d:
        case 0x15e:
            value = cur_node->old_level[addr - 0x158];
            break;
        case 0x166: value = cur_node->gender; break;
        case 0x167: value = cur_node->alignment; break;
        case 0x170: value = cur_node->ac_base; break;
        case 0x1eb: value = cur_node->spell_resist; break;
        case 0x1ed: value = cur_node->status; break;
        case 0x1ef: value = cur_node->side; break;
        case 0x1fc: value = cur_node->hp; break;
        }
    return value;
}

void far gb_write(unsigned short addr, unsigned short val, unsigned char kind)
                                                                 /* 25a4:06d2 */
{
    char zone;
    char found;
    char size;

    if (kind > 1)
        size = 2;
    else
        size = 1;
    zone = gb_zone(addr);
    switch (zone) {
    case 0:
        switch (addr - 1) {
        case STATE_MAP_RESTRICTED - 1:
            if (geo.show_map && val) {
                geo.show_map = 0;
                view_refresh();
            }
            break;
        case STATE_X - 1:
            geo.x = val;
            script_view_dirty = 1;
            break;
        case STATE_Y - 1:
            geo.y = val;
            script_view_dirty = 1;
            break;
        case STATE_FACING - 1:
            do {
                found = 1;
                switch (val) {
                case 0: geo.facing = 0; break;
                case 1: geo.facing = 2; break;
                case 2: geo.facing = 4; break;
                case 3: geo.facing = 6; break;
                default:
                    found = 0;
                    val -= 4;
                }
            } while (found != 1);
            script_view_dirty = 1;
            break;
        case STATE_DUNGEON - 1:
            if (game_state[STATE_DUNGEON] != val) {
                geo.show_map = 0;
                saved_view_mode = view_mode;
                if (val == 0)
                    view_mode = 3;
                else
                    view_mode = 4;
            }
            break;
        case STATE_REALM - 1:
            if (game_state[STATE_REALM] != val && val < 7) {
                frame_select(val);
                game_state[STATE_REALM] = val;
            }
            break;
        }
        if (size == 1)
            game_state[addr] = val;
        else {
            game_state[addr] = val;
            game_state[addr + 1] = val >> 8;
        }
        break;
    case 1:
        hero_write(addr, val);
        break;
    case 2:
        if (size == 1)
            ecl_memory[addr] = val;
        else {
            ecl_memory[addr] = val;
            ecl_memory[addr + 1] = val >> 8;
        }
        break;
    }
}

unsigned far gb_read(unsigned short addr, unsigned char kind)    /* 25a4:0897 */
{
    unsigned char zone;
    unsigned short value;
    char size;
    struct Character far *character_cursor;
    struct Effect far *got;

    if (kind > 1)
        size = 2;
    else
        size = 1;
    zone = gb_zone(addr);
    value = 0;
    switch (zone) {
    case 0:
        switch (addr - 1) {
        case STATE_DETECT_MAGIC - 1:
            character_cursor = node_list;
            while (character_cursor && !find_effect(character_cursor, 5, &got))
                character_cursor = character_cursor->next;
            if (!character_cursor)
                value = 0;
            else
                value = 1;
            break;
        case STATE_X - 1: value = geo.x; break;
        case STATE_Y - 1: value = geo.y; break;
        case STATE_FACING - 1:
            if (game_state[STATE_DUNGEON] == 0)
                value = geo.facing;
            else
                value = (unsigned)geo.facing / 2;
            break;
        case STATE_FACING_OCTANT - 1: value = geo.facing; break;
        case STATE_WALL_AHEAD - 1: value = (unsigned char)geo.byte_3; break;
        case STATE_AREA_CELL_ATTR - 1: value = (unsigned char)geo.flag_4; break;
        case STATE_AREA_CELL_FLAGS - 1: value = area_cell_flags(geo.x, geo.y); break;
        default:
            if (size == 1)
                value = game_state[addr];
            else
                value = game_state[addr] + (game_state[addr + 1] << 8);
        }
        break;
    case 1:
        value = hero_read(addr, size);
        break;
    case 2:
        if (size == 1)
            value = ecl_memory[addr];
        else
            value = ecl_memory[addr] + (ecl_memory[addr + 1] << 8);
        break;
    }
    return value;
}

void far gb_write_str(unsigned short addr, char far *text)          /* 25a4:0a59 */
{
    char zone;
    unsigned char len;

    zone = gb_zone(addr);
    len = str_length(text);
    switch (zone) {
    case 0:
        mem_move5(text, game_state + addr, len);
        game_state[len + addr] = 0;
        break;
    case 1:
        if (addr == 0x401)
            str_copy(cur_node->name, text);
        break;
    }
}

void far gb_unpack(unsigned char len, unsigned char string_slot)           /* 25a4:0ae2 */
{
    unsigned char index;
    unsigned char cur;
    unsigned char prev;
    unsigned char packed_char;
    unsigned char phase;
    char buf[2];

    ecl_str[string_slot][0] = 0;
    index = 0;
    phase = 1;
    buf[1] = 0;
    cur = 0;
    while (index < len) {
        if (phase < 4) {
            ecl_ip++;
            prev = cur;
            cur = ecl_memory[ecl_ip];
            index++;
        }
        switch (phase) {
        case 1: packed_char = (cur >> 2) & 0x3f; break;
        case 2: packed_char = ((prev << 4) + (cur >> 4)) & 0x3f; break;
        case 3: packed_char = ((prev << 2) + (cur >> 6)) & 0x3f; break;
        case 4: packed_char = cur & 0x3f;
        }
        if (phase < 4)
            phase++;
        else
            phase = 1;
        if (packed_char != 0) {
            buf[0] = gb_char(packed_char);
            str_cat(ecl_str[string_slot], buf);
        }
    }
    if (phase == 4) {
        packed_char = cur & 0x3f;
        if (packed_char != 0) {
            buf[0] = gb_char(packed_char);
            str_cat(ecl_str[string_slot], buf);
        }
    }
}

void far gb_fetch_str(unsigned char string_slot, unsigned short addr)      /* 25a4:0c18 */
{
    char zone;
    unsigned char index;

    zone = gb_zone(addr);
    ecl_str[string_slot][0] = 0;
    index = 0;
    switch (zone) {
    case 0:
        while (game_state[index + addr] != 0) {
            str_copy(ecl_str[string_slot],
                     msg_format("%s%c", ecl_str[string_slot], game_state[index + addr]));
            index++;
        }
        break;
    case 1:
        if (addr == 0x401)
            str_copy(ecl_str[string_slot], cur_node->name);
        break;
    case 2:
        while (ecl_memory[index + addr] != 0) {
            str_copy(ecl_str[string_slot],
                     msg_format("%s%c", ecl_str[string_slot], ecl_memory[index + addr]));
            index++;
        }
        break;
    }
}

void far menu_opts(char far *menu, char far *opts, unsigned char far *count)
                                                                 /* 25a4:0d51 */
{
    unsigned char index;

    *opts = 0;
    *count = 0;
    for (index = 0; index < str_length(menu); index++) {
        if ((menu[index] >= 'A' && menu[index] <= 'Z')
                || (menu[index] >= '0' && menu[index] <= '9'))
            menu[index] = menu[index] + 0x20;
        if (menu[index] == '~') {
            str_delete(menu, index + 1, 1);
            str_format(opts, "%s%c", opts, menu[index]);
            (*count)++;
        }
    }
}

unsigned char far menu_horiz(char far *title, char far *menu, char enter_ok,
                             char anim)                          /* 25a4:0e3f */
{
    char opts[41];
    unsigned char menu_choice;
    unsigned char count;
    unsigned char result;
    char pause;

    if (str_same(press_text, menu))
        pause = 1;
    else
        pause = 0;
    menu_opts(menu, opts, &count);
    if (pause) {
        press_return_wait(anim);
        result = 0;
    } else {
        hotkey_reset(count - 1);
        do {
            menu_choice = menu_run(title, menu, 1, anim);
            if (menu_escaped && menu_choice != 0x1b) {
                if (view_mode != 3 && game_state[STATE_DUNGEON] == 1) {
                    party_rows(cur_node, 0);
                    party_key_select(menu_choice);
                    party_rows(cur_node, 1);
                }
                menu_choice = 0xff;
            }
        } while (!(menu_choice < count || (menu_choice == 0x1b && menu_escaped && enter_ok)));
        if (menu_choice == 0x1b)
            result = 0;
        else
            result = menu_choice;
    }
    return result;
}

void far menu_vert(char far *title, char far *menu, char row, char col,
                   char row2, char col2, struct Node far *list, char list_mode,
                   char far *redraw, int far *top,
                   struct Node far * far *out)                   /* 25a4:0f4a */
{
    char opts[41];
    char text[51];
    unsigned char count;

    str_copy(text, menu);
    menu_opts(text, opts, &count);
    hotkey_reset(0);
    list_menu(title, vmenu_text, row, col, row2, col2, list, list_mode, 0, redraw, top,
              out);
}

void far compare_str(char far *left_value, char far *right_value)                  /* 25a4:0fc6 */
{
    mem_fill(ecl_conditions, 6, 0);
    if (str_cmp(left_value, right_value) == 0)
        ecl_conditions[0] = 1;
    if (str_cmp(left_value, right_value) != 0)
        ecl_conditions[1] = 1;
    if (str_lt(left_value, right_value))
        ecl_conditions[2] = 1;
    if (str_gt(left_value, right_value))
        ecl_conditions[3] = 1;
    if (str_le(left_value, right_value))
        ecl_conditions[4] = 1;
    if (str_ge(left_value, right_value))
        ecl_conditions[5] = 1;
}

void far compare_int(unsigned short left_value, unsigned short right_value)         /* 25a4:1086 */
{
    mem_fill(ecl_conditions, 6, 0);
    if (left_value == right_value)
        ecl_conditions[0] = 1;
    if (left_value != right_value)
        ecl_conditions[1] = 1;
    if (left_value < right_value)
        ecl_conditions[2] = 1;
    if (left_value > right_value)
        ecl_conditions[3] = 1;
    if (left_value <= right_value)
        ecl_conditions[4] = 1;
    if (left_value >= right_value)
        ecl_conditions[5] = 1;
}

void far ecl_gosub(unsigned char argument_slot)                              /* 25a4:10ec */
{
    struct EclFrame far *prev;

    if (ecl_stack == 0) {
        pool_alloc((void far * far *)&ecl_stack, 6, &ecl_frame_pool);
        ecl_stack->next = 0;
    } else {
        prev = ecl_stack;
        pool_alloc((void far * far *)&ecl_stack, 6, &ecl_frame_pool);
        ecl_stack->next = prev;
    }
    ecl_stack->ip = ecl_ip;
    ecl_ip = (ecl_hi[argument_slot] << 8) | ecl_lo[argument_slot];
}

void far step_forward(void)                                      /* 25a4:1186 */
{
    switch (geo.facing) {
    case 0:
        if (geo.y > 0)
            geo.y--;
        else
            geo.y = geo.area->rows - 1;
        break;
    case 2:
        if (geo.area->cols - 1 > geo.x)
            geo.x++;
        else
            geo.x = 0;
        break;
    case 4:
        if (geo.y < geo.area->rows - 1)
            geo.y++;
        else
            geo.y = 0;
        break;
    case 6:
        if (geo.x > 0)
            geo.x--;
        else
            geo.x = geo.area->cols - 1;
        break;
    }
    geo.flag_4 = area_cell_at(geo.x, geo.y);
    geo.byte_3 = wall_side(geo.x, geo.y, geo.facing);
    script_view_dirty = 1;
}

void far rob_money(int pct, struct Character far *character)                  /* 25a4:124f */
{
    character->coins[0] = character->coins[0] * (long)pct / 100;
    character->coins[1] = character->coins[1] * (long)pct / 100;
    character->coins[2] = character->coins[2] * (long)pct / 100;
}

void far rob_items(unsigned char chance, struct Character far *character)     /* 25a4:12d1 */
{
    unsigned char roll;
    struct Item far *item;
    struct Item far *next;

    item = character->items;
    while (item) {
        if (item->rec.weight > 0xff) {
            if (chance > 0x5a)
                chance = chance - 0x5a;
            else
                chance = 0;
        } else if (item->rec.weight > 0x18) {
            if (chance > 0x32)
                chance = chance - 0x32;
            else
                chance = 0;
        }
        roll = fx_roll_dice(1, 100);
        next = item->next;
        if (roll <= chance) {
            if (item->rec.equipped != 0)
                item_equip_toggle(item);
            item_remove(character, item);
        }
        item = next;
    }
}

void far ecl_skip(void)                                          /* 25a4:1385 */
{
    ecl_op = ecl_memory[ecl_ip];
    switch (ecl_op) {
    case 0x01: case 0x02: case 0x0a: case 0x0e: case 0x11: case 0x12:
    case 0x1f: case 0x20: case 0x2c: case 0x2d: case 0x32: case 0x38:
    case 0x39: case 0x3c: case 0x3f: case 0x40:
        ecl_load_args(1);
        break;
    case 0x03: case 0x08: case 0x09: case 0x0f: case 0x10: case 0x21:
    case 0x34: case 0x36: case 0x37: case 0x41:
        ecl_load_args(2);
        break;
    case 0x04: case 0x05: case 0x06: case 0x07: case 0x0b: case 0x28:
    case 0x2a: case 0x2f: case 0x30: case 0x35: case 0x3b:
        ecl_load_args(3);
        break;
    case 0x0c: case 0x14:
        ecl_load_args(4);
        break;
    case 0x2e:
        ecl_load_args(5);
        break;
    case 0x1e:
        ecl_load_args(6);
        break;
    case 0x25: case 0x26: case 0x2b: case 0x31:
        ecl_load_args(2);
        ecl_ip--;
        ecl_load_args(ecl_arg(2));
        break;
    case 0x15:
        ecl_load_args(3);
        ecl_ip--;
        ecl_load_args(ecl_arg(3));
        break;
    case 0x27:
        ecl_load_args(4);
        ecl_ip--;
        ecl_load_args(ecl_arg(4));
        break;
    default:
        ecl_ip++;
    }
}

void far party_hp_span(unsigned char far *hi, unsigned char far *lo)
                                                                 /* 25a4:14d3 */
{
    struct Character far *character_cursor;
    unsigned char movement_rate;

    character_cursor = node_list;
    *hi = character_cursor->movement;
    *lo = character_cursor->movement;
    while (character_cursor) {
        movement_rate = calc_move(character_cursor) / 2;
        if (*hi < movement_rate)
            *hi = movement_rate;
        if (*lo > movement_rate)
            *lo = movement_rate;
        character_cursor = character_cursor->next;
    }
}

void far inflict_damage(short dmg, struct Character far *character)           /* 25a4:1550 */
{
    char msg[82];
    char num[5];
    char pause;

    if (character->enabled == 0)
        return;
    pause = 0;
    int_to_str(dmg, num);
    if (character->hp + 10 < dmg)
        str_format(msg, "  %s%s", character->name, " dies. ");
    else
        str_copy(msg, msg_format("  %s%s%s%s", character->name, " is hit FOR ", num,
                                 " points of Damage."));
    if (text_row > 0x16) {
        text_row = 0x11;
        pause = 1;
        continue_prompt(0xb, 0, continue_items);
    } else
        pause = 0;
    text_col = 0x26;
    text_wrap(1, 0x11, 0x26, 0x16, 0xb, 0, pause, msg);
    hero_damage(character, dmg);
    if (character->status == 5)
        character->status = 4;
    if (view_mode != 3)
        text_clear(0x11, 1, 0x26, 0xa);
    party_display(cur_node);
}

void far view_update(void)                                       /* 25a4:1685 */
{
    geo.flag_4 = area_cell_at(geo.x, geo.y);
    if (confirm_anim || encounter_sprite_shown || ecl_initial_redraw || script_view_dirty || view_update_pending) {
        mem_fill(encounter_images_shown, 2, 0);
        redraw_wanted = 1;
        view_refresh();
        party_info_display();
        view_update_pending = 0;
        ecl_initial_redraw = 0;
        script_view_dirty = 0;
        confirm_anim = 0;
        encounter_sprite_shown = 0;
        geo.byte_3 = wall_side(geo.x, geo.y, geo.facing);
    }
}

void far wander_load(void)                                       /* 25a4:170e */
{
    unsigned char id;
    unsigned char count;
    unsigned char icon;
    unsigned char copies;
    struct Character far *tail;
    struct Character far *clone;
    struct Character far *wanderer;
    struct Character far *saved;
    struct Item far *items;
    struct Item far *prev_item;
    struct Item far *items_head;
    struct Effect far *effect;
    struct Effect far *prev_fx;
    struct Effect far *fx_head;

    saved = cur_node;
    copies = 1;
    tail = 0;
    clone = 0;
    id = wander_ids[game_state[STATE_REALM]];
    hero_load(id, &wanderer, &items, &effect);
    wanderer->status = 9;
    wanderer->npc = wanderer->npc | 0x32;
    wanderer->side = game_state[STATE_REALM] == 5;
    items_head = items;
    fx_head = effect;
    count = 10;
    icon = wander_icons[game_state[STATE_REALM]];
    tail = node_list;
    icon_load("CPIC", icon, next_monster_group);
    wanderer->order_no = next_monster_group;
    while (tail->next != 0)
        tail = tail->next;
    tail->next = wanderer;
    tail = tail->next;
    tail->next = 0;
    encounter_npc_count++;
    copies++;
    while (copies <= count && encounter_npc_count < 0x32) {
        pool_alloc((void far * far *)&clone, 0x19e, &npc_pool);
        tail->next = clone;
        tail = tail->next;
        *tail = *wanderer;
        tail->next = 0;
        tail->effects = 0;
        tail->items = 0;
        copies++;
        encounter_npc_count++;
        while (items != 0) {
            if (tail->items == 0) {
                pool_alloc((void far * far *)&tail->items, 0x3d, &item_record_pool);
                *tail->items = *items;
                tail->items->next = 0;
            } else {
                prev_item = tail->items;
                pool_alloc((void far * far *)&tail->items, 0x3d, &item_record_pool);
                *tail->items = *items;
                tail->items->next = prev_item;
            }
            items = items->next;
        }
        items = items_head;
        while (effect != 0) {
            if (tail->effects == 0) {
                pool_alloc((void far * far *)&tail->effects, 9, &effect_pool);
                *tail->effects = *effect;
                tail->effects->next = 0;
            } else {
                prev_fx = tail->effects;
                pool_alloc((void far * far *)&tail->effects, 9, &effect_pool);
                *tail->effects = *effect;
                tail->effects->next = prev_fx;
            }
            effect = effect->next;
        }
        effect = fx_head;
    }
    next_monster_group++;
    cur_node = saved;
}

char far spell_fits(unsigned char spell, unsigned char kind)     /* 25a4:1a4e */
{
    char result;

    if ((scroll_spell_kinds - 1)[spell] == 3 || (scroll_spell_kinds - 1)[spell] == kind)
        result = 1;
    else
        result = 0;
    return result;
}

void far scroll_make(struct ItemRec far *rec)                        /* 25a4:1a83 */
{
    unsigned char spell;
    char ok;
    unsigned char scroll_variant;

    mem_zero(rec, 0x11);
    rec->type = 0x27;
    rec->name_ids[1] = 0x27;
    rec->name_ids[0] = 0x66;
    rec->plus = 1;
    rec->weight = 1;
    rec->value = 3000;
    rec->cursed = 0;
    rec->quantity = 0;
    scroll_variant = fx_roll_dice(1, 2);
    rec->name_ids[2] = 0x8a - scroll_variant;
    rec->unid_mask = 1 << (6 - scroll_variant);
    for (tick_slot = 1; tick_slot <= 3; tick_slot++) {
        ok = 0;
        do {
            spell = fx_roll_dice(1, 0x7e);
            if ((spell_info - 1)[spell].cast_class == 2 && spell_fits(spell, scroll_variant))
                ok = 1;
        } while (!ok);
        (rec->u.spells - 1)[tick_slot] = spell;
    }
}

static void far hero_write(unsigned short addr, unsigned short val)
                                                                 /* 25a4:1b5e */
{
    unsigned char idx;

    addr = addr - 0x401;
    if (addr >= 0x1e && addr <= 0xaa) {
        if (val == 0xff)
            val = 0;
        if (val == 0 && cur_node->spells[addr - 0x1e] != 0
                && !(cur_node->spells[addr - 0x1e] & 0x80))
            cur_node->spells[addr - 0x1e] |= 0x80;
        else
            cur_node->spells[addr - 0x1e] = val;
    } else
        switch (addr) {
        case 0xb0: cur_node->age = val; break;
        case 0x137: cur_node->move_base = val; break;
        case 0x14b: case 0x14c: case 0x14d: case 0x14e: case 0x14f: case 0x150:
            idx = (addr - 0x14b) / 2;
            cur_node->coins[idx] = val;
            break;
        case 0x167: cur_node->alignment = val; break;
        case 0x170: cur_node->ac_base = val; break;
        case 0x1eb: cur_node->spell_resist = val; break;
        case 0x1ed:
            cur_node->status = val;
            cur_node->enabled = val == 0;
            break;
        case 0x1ef: cur_node->side = val; break;
        case 0x1fc: cur_node->hp = val; break;
        }
}

void far cmd_print(void)                                         /* 25a4:1cd3 */
{
    unsigned char color;

    color = game_state[STATE_SCRIPT_TEXT_COLOR_MODE];
    if (color)
        color = 3;
    else
        color = 7;
    ecl_load_args(1);
    script_text_cleared = 0;
    text_slow = 1;
    if (ecl_type[1] < 0x80)
        str_copy(ecl_str[1], int_str(ecl_arg(1)));
    if (ecl_op == 0x11)
        text_wrap(1, 0x11, 0x26, 0x16, color, 0, 0, ecl_str[1]);
    else {
        text_row = 0x11;
        text_col = 1;
        text_wrap(1, 0x11, 0x26, 0x16, color, 0, 1, ecl_str[1]);
    }
    text_slow = 0;
}

void far cmd_new_ecl(void)                                       /* 25a4:1d8d */
{
    unsigned char id;

    ecl_load_args(1);
    game_state[STATE_SCRIPT_ID] = ecl_id;
    id = ecl_arg(1);
    ecl_id = id;
    ecl_load(id);
    game_loaded = 0;
    ecl_init();
    ecl_done = 1;
    ecl_new = 1;
    mem_fill(encounter_images_shown, 2, 0);
}

void far cmd_load_files(void)                                    /* 25a4:1dec */
{
    unsigned char area;
    unsigned char pics;

    ecl_load_args(2);
    area = ecl_arg(1);
    pics = ecl_arg(2);
    game_state[STATE_AREA_ID] = area;
    geo_load(area);
    game_state[STATE_MOVE_BLOCK] = 0;
    if (pics != 0xff && pics != geo.pic_set) {
        res_free(&big_res);
        view_flag = 0;
        geo.pic_set = pics;
    }
    if (saved_view_mode == 3 && view_mode != 3 && noncombat_redraw_pending) {
        frame_main();
        party_display(cur_node);
        party_info_display();
        noncombat_redraw_pending = 0;
    }
}

void far cmd_vert_menu(void)                                     /* 25a4:1e91 */
{
    unsigned char argument_count;
    unsigned char index;
    char text[82];
    struct Node far *list;
    struct Node far *head;
    int top;
    char redraw;
    unsigned short addr;
    unsigned char kind;

    ui_mode = 1;
    script_text_cleared = 0;
    redraw = 1;
    index = 1;
    ecl_load_args(3);
    addr = (ecl_hi[1] << 8) | ecl_lo[1];
    str_copy(text, ecl_str[1]);
    argument_count = ecl_arg(3);
    ecl_ip--;
    ecl_load_args(argument_count);
    list_alloc(argument_count, &list);
    head = list;
    text_col = 1;
    text_row = 0x11;
    text_wrap(1, 0x11, 0x26, 0x16, 7, 0, 1, text);
    while (list != 0) {
        kind = ecl_type[1];
        str_copy(list->text, ecl_str[index]);
        list->key = 0;
        list = list->next;
        index++;
    }
    list = head;
    top = 0;
    menu_vert("", "", 1, text_row + 1, 0x26, 0x16, list, 0, &redraw, &top,
              &head);
    gb_write(addr, top, kind);
    list_free(&list);
    text_clear(1, 0x11, 0x26, 0x16);
}

void far cmd_horiz_menu(char prompt)                             /* 25a4:1ff7 */
{
    unsigned char argument_count;
    unsigned char sel;
    unsigned char index;
    char text[82];
    unsigned short addr;
    unsigned char kind;

    if (prompt)
        text_wrap(1, 0x11, 0x26, 0x16, 7, 0, 0, "what do you do?");
    ui_mode = 1;
    str_copy(text, "");
    ecl_load_args(2);
    addr = (ecl_hi[1] << 8) | ecl_lo[1];
    kind = ecl_type[1];
    argument_count = ecl_arg(2);
    ecl_ip--;
    ecl_load_args(argument_count);
    for (index = 1; index <= argument_count - 1; index++)
        str_copy(text, msg_format("%s~%s ", text, ecl_str[index]));
    str_copy(text, msg_format("%s~%s", text, ecl_str[argument_count]));
    sel = menu_horiz("", text, 0, confirm_anim);
    gb_write(addr, sel, kind);
    box_draw();
}

void far cmd_treasure(void)                                      /* 25a4:2127 */
{
    unsigned char argument_count;
    unsigned char index;
    unsigned char id;
    struct ItemRec rec;
    struct Item item;
    struct Item far *prev;

    ecl_load_args(4);
    pool[0] += ecl_arg(1);
    pool[1] += ecl_arg(2);
    pool[2] += ecl_arg(3);
    argument_count = ecl_arg(4);
    ecl_ip--;
    ecl_load_args(argument_count);
    for (index = 1; index <= argument_count; index++) {
        id = ecl_arg(index);
        if (id != 0xff && item_templates[id].type == 0xff)
            continue;
        if (id == 0xff) {
            scroll_make(&rec);
            mem_move5(&rec, &item.rec, 0x11);
            item.more = 0;
        } else
            mem_move5(&item_templates[id], &item.rec, 0x11);
        item.next = 0;
        item.in_use = 0;
        if (pool_items == 0) {
            pool_alloc((void far * far *)&pool_items, 0x3d, &item_record_pool);
            *pool_items = item;
        } else {
            prev = pool_items;
            pool_alloc((void far * far *)&pool_items, 0x3d, &item_record_pool);
            *pool_items = item;
            pool_items->next = prev;
        }
    }
}

void far cmd_damage(void)                                        /* 25a4:22ac */
{
    unsigned char attack;
    unsigned char dice;
    unsigned char sides;
    unsigned char index;
    unsigned char save_mod;
    unsigned char attack_var;
    unsigned char bonus;
    unsigned char pos;
    unsigned char save_type;
    unsigned char inner;
    unsigned short amount;
    struct Character far *cursor;
    struct Character far *scan;
    struct Character far *saved;
    char all;
    char half;

    saved = cur_node;
    all = 0;
    cursor = node_list;
    ecl_load_args(5);
    attack = ecl_arg(1);
    dice = ecl_arg(2);
    sides = ecl_arg(3);
    bonus = ecl_arg(4);
    attack_var = ecl_arg(5);
    amount = fx_roll_dice(dice, sides) + bonus;
    half = attack & 0x10;
    if (attack & 0x40)
        all = 1;
    else
        pos = fx_roll_dice(1, game_state[STATE_PARTY_COUNT]);
    if (attack & 0x80) {
        save_mod = attack & 0xf;
        save_type = attack_var & 7;
        if (all) {
            while (cursor) {
                if (attack & 0x20)
                    inflict_damage(amount, cursor);
                else if (!fx_saving_throw(cursor, save_type, save_mod))
                    inflict_damage(amount >> 1, cursor); /* halved on 0, as shipped */
                else if (half)
                    inflict_damage(amount, cursor);
                cursor = cursor->next;
            }
        } else if (attack_var & 0x80) {
            if (save_type == 0
                    || !fx_saving_throw(cur_node, save_type - 1, save_mod))
                inflict_damage(amount, cur_node);
            else if (half)
                inflict_damage(amount >> 1, cur_node);
        } else {
            for (index = 2; index <= pos; index++)
                cursor = cursor->next;
            if (!fx_saving_throw(cursor, save_type, save_mod))
                inflict_damage(amount >> 1, cursor);       /* as above */
            else if (half)
                inflict_damage(amount, cursor);
        }
    } else
        for (index = 1; index <= attack; index++) {
            cursor = node_list;
            pos = fx_roll_dice(1, game_state[STATE_PARTY_COUNT]);
            for (inner = 1; inner < pos; inner++)
                cursor = cursor->next;
            if (fx_attack_roll_no_attacker(cursor, attack_var))
                inflict_damage(amount, cursor);
            amount = fx_roll_dice(dice, sides) + bonus;
        }
    scan = node_list;
    adventure_exit = 1;
    while (scan) {
        if (scan->enabled)
            adventure_exit = 0;
        scan = scan->next;
    }
    if (adventure_exit) {
        frame_border();
        text_col = 2;
        text_row = 2;
        text_wrap(2, 5, 0x25, 0x16, 7, 0, 1,
                  "The Dark Queen sneers with pleasure");
        text_wrap(2, 6, 0x25, 0x16, 7, 0, 1, "at your meaningless demise.");
        text_wrap(2, 7, 0x25, 0x16, 7, 0, 1,
                  "Nothing can stop her now... NOTHING!");
        delay(3000);
    }
    cur_node = saved;
    continue_prompt(0xb, 0, continue_items);
}

void far cmd_spell(void)                                         /* 25a4:260f */
{
    unsigned char spell;
    unsigned char index;
    unsigned char count;
    struct Character far *cursor;
    char found;
    char limit;
    unsigned short index_at;
    unsigned short count_at;

    limit = 0;
    found = 0;
    ecl_load_args(3);
    spell = ecl_arg(1);
    index_at = (ecl_hi[2] << 8) | ecl_lo[2];
    count_at = (ecl_hi[3] << 8) | ecl_lo[3];
    index = 1;
    count = 0;
    cursor = node_list;
    while (cursor && !found) {
        index = 0;
        if (cursor->enabled != 0)
            do {
                if (cursor->spells[index] == spell)
                    found = 1;
                else if (index < 0x88)
                    index++;
                else
                    limit = 1;
            } while (!limit && !found);
        limit = 0;
        cursor = cursor->next;
        if (cursor && !found)
            count++;
    }
    if (!found)
        index = 0xff;
    gb_write(index_at, index, ecl_type[2]);
    gb_write(count_at, count, ecl_type[3]);
}

void far cmd_award_xp(void)                                      /* 25a4:2719 */
{
    long share;
    long total;
    char whole;
    struct Character far *cursor;
    short idx;
    unsigned char classes;
    long amount;

    ecl_load_args(2);
    amount = ecl_arg(2);
    total = amount * 100;
    whole = ecl_arg(1);
    text_row = 0x11;
    text_col = 1;
    if (whole) {
        text_wrap(1, 0x11, 0x26, 0x16, 7, 0, 1,
                  "Congratulations the party gains experience!");
        text_pause();
        cursor = node_list;
        while (cursor) {
            if (cursor->enabled) {
                classes = 0;
                for (idx = 0; idx <= 6; idx++)
                    if (cursor->level[idx] > 0)
                        classes++;
                share = total / classes;
                cursor->xp = cursor->xp + share;
                cursor->level_up = can_level_up(cursor);
            }
            cursor = cursor->next;
        }
        party_display(cur_node);
    } else {
        cursor = cur_node;
        if (cursor->enabled) {
            text_wrap(1, 0x11, 0x26, 0x16, 7, 0, 1,
                      msg_format("Congratulations %s gains experience!",
                                 cursor->name));
            text_pause();
            classes = 0;
            for (idx = 0; idx <= 6; idx++)
                if (cursor->level[idx] > 0)
                    classes++;
            share = total / classes;
            cursor->xp = cursor->xp + share;
            cursor->level_up = can_level_up(cursor);
        }
    }
}

void far cmd_rob(void)                                           /* 25a4:28f8 */
{
    unsigned char whole;
    unsigned char percent;
    unsigned char chance;
    struct Character far *cursor;
    int keep;

    ecl_load_args(3);
    whole = ecl_arg(1);
    percent = ecl_arg(2);
    keep = 100 - percent;
    chance = ecl_arg(3);
    if (whole == 0) {
        rob_money(keep, cur_node);
        rob_items(chance, cur_node);
    } else {
        cursor = node_list;
        while (cursor) {
            rob_money(keep, cursor);
            rob_items(chance, cursor);
            cursor = cursor->next;
        }
    }
}

void far cmd_load_monster(void)                                  /* 25a4:29a3 */
{
    ecl_load_args(3);
    if (next_monster_group < 0xe) {
        mon_ids[next_monster_group - 8] = ecl_arg(1);
        mon_count[next_monster_group - 8] = ecl_arg(2);
        mon_pic[next_monster_group - 8] = ecl_arg(3);
        next_monster_group++;
        scripted_monsters_queued = 1;
    }
}

void far monster_add(unsigned char id, unsigned char count, unsigned char icon,
                     unsigned char idx)                          /* 25a4:29fc */
{
    unsigned char copies;
    struct Character far *tail;
    struct Character far *clone;
    struct Character far *monster;
    struct Character far *saved;
    struct Item far *items;
    struct Item far *prev_item;
    struct Item far *items_head;
    struct Effect far *effect;
    struct Effect far *prev_fx;
    struct Effect far *fx_head;

    saved = cur_node;
    copies = 1;
    if (encounter_npc_count < 0x28) {
        tail = 0;
        clone = 0;
        hero_load(id, &monster, &items, &effect);
        if (monster->side == 1 && game_state[STATE_DIFFICULTY] > 0 && game_state[STATE_DIFFICULTY] < 6) {
            monster->hp = monster->hp * (game_state[STATE_DIFFICULTY] + 1) / 4;
            monster->hp_max = monster->hp_max * (game_state[STATE_DIFFICULTY] + 1) / 4;
            if (monster->hp == 0 || monster->hp_max == 0) {
                monster->hp = 1;
                monster->hp_max = 1;
            }
        }
        if (monster->level_undead != 0)
            ai_level++;
        items_head = items;
        fx_head = effect;
        if (count <= 0)
            count = 1;
        tail = node_list;
        icon_load("CPIC", icon, idx);
        monster->order_no = idx;
        while (tail->next != 0)
            tail = tail->next;
        tail->next = monster;
        tail = tail->next;
        tail->next = 0;
        encounter_npc_count++;
        copies++;
        while (copies <= count && encounter_npc_count < 0x28) {
            pool_alloc((void far * far *)&clone, 0x19e, &npc_pool);
            tail->next = clone;
            tail = tail->next;
            *tail = *monster;
            tail->next = 0;
            tail->effects = 0;
            tail->items = 0;
            copies++;
            encounter_npc_count++;
            while (items != 0) {
                if (tail->items == 0) {
                    pool_alloc((void far * far *)&tail->items, 0x3d,
                               &item_record_pool);
                    *tail->items = *items;
                    tail->items->next = 0;
                } else {
                    prev_item = tail->items;
                    pool_alloc((void far * far *)&tail->items, 0x3d,
                               &item_record_pool);
                    *tail->items = *items;
                    tail->items->next = prev_item;
                }
                items = items->next;
            }
            items = items_head;
            while (effect != 0) {
                if (tail->effects == 0) {
                    pool_alloc((void far * far *)&tail->effects, 9, &effect_pool);
                    *tail->effects = *effect;
                    tail->effects->next = 0;
                } else {
                    prev_fx = tail->effects;
                    pool_alloc((void far * far *)&tail->effects, 9, &effect_pool);
                    *tail->effects = *effect;
                    tail->effects->next = prev_fx;
                }
                effect = effect->next;
            }
            effect = fx_head;
        }
        cur_node = saved;
    }
}

void far monsters_place(void)                                    /* 25a4:2d82 */
{
    int index;

    for (index = 0; index + 8 < next_monster_group; index++)
        monster_add(mon_ids[index], mon_count[index], mon_pic[index], index + 8);
}

void far cmd_setup_monster(void)                                 /* 25a4:2db1 */
{
    ecl_load_args(4);
    encounter_sprite = ecl_arg(1);
    game_state[STATE_ENCOUNTER_DISTANCE_LIMIT] = ecl_arg(2);
    encounter_picture = ecl_arg(3);
    encounter_mode = ecl_arg(4);
    game_state[STATE_ENCOUNTER_DISTANCE] = monster_dist(geo.x, geo.y, geo.facing);
    if (game_state[STATE_ENCOUNTER_DISTANCE_LIMIT] < game_state[STATE_ENCOUNTER_DISTANCE])
        game_state[STATE_ENCOUNTER_DISTANCE] = game_state[STATE_ENCOUNTER_DISTANCE_LIMIT];
    if (game_state[STATE_ENCOUNTER_DISTANCE_LIMIT] == 0x10)
        game_state[STATE_ENCOUNTER_DISTANCE_LIMIT] = game_state[STATE_ENCOUNTER_DISTANCE] = 2;
    monster_show(encounter_sprite, encounter_picture, game_state[STATE_ENCOUNTER_DISTANCE], encounter_images_shown);
}

void far cmd_approach(void)                                      /* 25a4:2e4f */
{
    if (game_state[STATE_ENCOUNTER_DISTANCE] > 0) {
        game_state[STATE_ENCOUNTER_DISTANCE]--;
        monster_show(encounter_sprite, encounter_picture, game_state[STATE_ENCOUNTER_DISTANCE], encounter_images_shown);
    }
    ecl_ip++;
}

void far cmd_input_number(void)                                  /* 25a4:2e7c */
{
    unsigned short val;
    unsigned short addr;

    ecl_load_args(2);
    addr = (ecl_hi[2] << 8) | ecl_lo[2];
    val = input_number("", 7, 0);
    gb_write(addr, val, ecl_type[2]);
}

void far cmd_input_string(void)                                  /* 25a4:2ec8 */
{
    unsigned short addr;
    char text[82];

    ecl_load_args(2);
    addr = (ecl_hi[2] << 8) | ecl_lo[2];
    str_copy(text, input_text("", 7, 0, 0x28));
    if (text[0] == 0)
        str_copy(text, " ");
    gb_write_str(addr, text);
}

void far cmd_find_item(void)                                     /* 25a4:2f3b */
{
    unsigned char type;
    struct Character far *hero;
    struct Item far *item;
    char found;

    ecl_load_args(1);
    found = 0;
    type = ecl_arg(1);
    mem_fill(ecl_conditions, 6, 0);
    ecl_conditions[1] = 1;
    hero = node_list;
    while (hero && !found) {
        item = hero->items;
        while (item && !found) {
            if (item->rec.type == type) {
                ecl_conditions[0] = 1;
                ecl_conditions[1] = 0;
                found = 1;
            }
            item = item->next;
        }
        hero = hero->next;
    }
}

void far cmd_who(void)                                           /* 25a4:2fe0 */
{
    char text[81];
    char saved;

    saved = exploration_menu_context;
    exploration_menu_context = 0;
    character_select_failed = 0;
    ecl_load_args(1);
    text_clear(1, 0x11, 0x26, 0x16);
    str_copy(text, ecl_str[1]);
    if (str_same(text, "who will attempt to disarm the trap?"))
        str_copy(text, "Who will try to disarm?");
    pick_hero(text, 0, 0, &cur_node);
    exploration_menu_context = saved;
    cur_node = get_selected();
}

void far cmd_add_npc(void)                                       /* 25a4:307b */
{
    unsigned char id;
    unsigned char morale;

    ecl_load_args(2);
    id = ecl_arg(1);
    mon_add(id);
    morale = ecl_arg(2);
    morale = morale / 2;
    morale = morale | 0x80;
    cur_node->npc = morale;
    recalc_stats(cur_node);
    party_display(cur_node);
}

void far cmd_destroy_items(void)                                 /* 25a4:30ec */
{
    unsigned char type;
    struct Character far *hero;
    struct Character far *saved;
    struct Item far *item;
    struct Item far *next;
    unsigned char all;
    char found;

    ecl_load_args(1);
    type = ecl_arg(1);
    all = type & 0x80;
    type = type & 0x7f;
    saved = cur_node;
    hero = node_list;
    while (hero) {
        found = 0;
        item = hero->items;
        while (item) {
            next = item->next;
            if (item->rec.type == type && (!all || item->rec.plus != 0)) {
                cur_node = hero;
                if (item->rec.equipped != 0)
                    item_equip_toggle(item);
                item_remove(hero, item);
                found = 1;
            }
            item = next;
        }
        if (found)
            recalc_stats(hero);
        hero = hero->next;
    }
    cur_node = saved;
}

void far cmd_find_name(void)                                     /* 25a4:31e8 */
{
    char name[41];
    struct Character far *hero;
    unsigned short addr;
    char found;
    unsigned char idx;

    ecl_load_args(2);
    str_copy(name, ecl_str[1]);
    addr = (ecl_hi[2] << 8) | ecl_lo[2];
    hero = node_list;
    found = 0;
    idx = 0;
    while (hero && !found) {
        if (str_same(hero->name, name) && (hero->npc & 0x80))
            found = 1;
        else {
            hero = hero->next;
            idx++;
        }
    }
    if (!found)
        idx = 0xff;
    else if (hero->enabled == 0)
        idx = idx | 0x80;
    gb_write(addr, idx, ecl_type[1]);
}

void far cmd_journal(void)                                       /* 25a4:32b2 */
{
    char text[41];
    unsigned short entry;

    ecl_load_args(1);
    entry = ecl_arg(1);
    text_row = 0x11;
    text_col = 1;
    str_copy(text, msg_format("This is recorded as journal entry %s.",
                              byte_str(entry)));
    /* v1.0: the entry is printed at full speed (text_slow is not set) */
    text_wrap(1, 0x11, 0x26, 0x16, 7, 0, 1, text);
    str_copy(text, press_text);
    menu_horiz("", text, 1, confirm_anim);
    box_draw();
}

void far cmd_shop(void)                                          /* 25a4:335e */
{
    struct ItemRec rec;
    unsigned char sel;
    unsigned char id;
    struct Item item;
    struct Item far *prev;
    unsigned short first;
    unsigned short last;
    unsigned char index;

    ecl_load_args(1);
    post_reset();
    sel = ecl_arg(1);
    if (sel > 9 || sel < 1)
        sel = 1;
    first = shop_first[sel - 1];
    last = shop_last[sel - 1];
    for (index = first; index <= last; index++) {
        id = shop_items[index - 1];
        if (id == 0xff)
            scroll_make(&rec);
        else
            rec = item_templates[id];
        if (rec.type == 0xff)
            continue;
        mem_move5(&rec, &item.rec, 0x11);
        item.rec.unid_mask = item.rec.unid_mask & 0xf8;
        item.more = 0;
        item.next = 0;
        item.in_use = 0;
        if (pool_items == 0) {
            pool_alloc((void far * far *)&pool_items, 0x3d, &item_record_pool);
            *pool_items = item;
        } else {
            prev = pool_items;
            pool_alloc((void far * far *)&pool_items, 0x3d, &item_record_pool);
            *pool_items = item;
            pool_items->next = prev;
        }
    }
    shop_menu();
    post_reset();
}

char far str_gt(char far *left_value, char far *right_value)                        /* 25a4:34d1 */
{
    char result;
    char done;
    int index;
    int compare_length;

    if (str_length(left_value) < str_length(right_value))
        compare_length = str_length(left_value);
    else
        compare_length = str_length(right_value);
    if (compare_length == 0) {
        done = 1;
        if (str_length(left_value) == 0)
            result = 0;
        else
            result = 1;
    } else
        done = 0;
    for (index = 0; !done && index < compare_length; index++)
        if (left_value[index] != right_value[index]) {
            done = 1;
            if (left_value[index] > right_value[index])
                result = 1;
            else
                result = 0;
        }
    if (!done) {
        if (str_length(left_value) > str_length(right_value))
            result = 1;
        else
            result = 0;
    }
    return result;
}

char far str_lt(char far *left_value, char far *right_value)                        /* 25a4:35b4 */
{
    char result;
    char done;
    int index;
    int compare_length;

    if (str_length(left_value) < str_length(right_value))
        compare_length = str_length(left_value);
    else
        compare_length = str_length(right_value);
    if (compare_length == 0) {
        done = 1;
        if (str_length(right_value) == 0)
            result = 0;
        else
            result = 1;
    } else
        done = 0;
    for (index = 0; !done && index < compare_length; index++)
        if (left_value[index] != right_value[index]) {
            done = 1;
            if (left_value[index] < right_value[index])
                result = 1;
            else
                result = 0;
        }
    if (!done) {
        if (str_length(left_value) < str_length(right_value))
            result = 1;
        else
            result = 0;
    }
    return result;
}

char far str_ge(char far *left_value, char far *right_value)                        /* 25a4:3697 */
{
    char result;
    char done;
    int index;
    int compare_length;

    if (str_length(left_value) >= str_length(right_value)) {
        compare_length = str_length(right_value);
        done = 0;
    } else {
        done = 1;
        result = 0;
    }
    if (compare_length == 0) {               /* n is unset after the else: shipped */
        done = 1;
        result = 1;
    }
    for (index = 0; !done && index < compare_length; index++)
        if (left_value[index] != right_value[index]) {
            done = 1;
            result = 0;
        }
    if (!done)
        result = 1;
    return result;
}

char far str_le(char far *left_value, char far *right_value)                        /* 25a4:3720 */
{
    char result;
    char done;
    int index;
    int compare_length;

    if (str_length(right_value) >= str_length(left_value)) {
        compare_length = str_length(left_value);
        done = 0;
    } else {
        done = 1;
        result = 0;
    }
    if (compare_length == 0) {               /* n is unset after the else: shipped */
        done = 1;
        result = 1;
    }
    for (index = 0; !done && index < compare_length; index++)
        if (left_value[index] != right_value[index]) {
            done = 1;
            result = 0;
        }
    if (!done)
        result = 1;
    return result;
}
