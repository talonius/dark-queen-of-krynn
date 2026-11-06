/* DQK.EXE overlay 03 module 2033: the ECL script interpreter.
 * ecl_load_args reads a command's operands from the script at ecl_memory
 * (a type byte, a low byte and, for types 1-3 and 0x81, a high byte) and
 * ecl_arg takes one operand's value.  ecl_dispatch switches on the opcode
 * byte ecl_op, which run_script reads at ecl_ip: 42 of its 66 opcodes
 * go to 32 handlers here, the other 24 to 22 vector stubs into overlay 6.
 * run_script runs commands from an entry point until ecl_done, adventure_exit
 * or adventure_restart is set, or esc_get reports Esc while demo_active is set;
 * run_new_script reruns the entry points while ecl_new is set; and
 * game_loop is the loop game_main calls: it runs the entry at
 * ecl_main_entry, then reads choices from 1d99:1369 and runs the entries at
 * ecl_before_move and ecl_after_move around each, until adventure_exit is set.
 *
 * EXE 0x031d24..0x03316e, one Turbo C large-model _TEXT segment starting at
 * byte 4 of paragraph 2033 in RTLink section 3 (payload 0x30ed0, link
 * segment 1f4e).  The first 4 bytes of paragraph 2033 are the tail of
 * gamemain.c (module 1fe9), and the last 14 bytes of the range lie in
 * paragraph 2177, whose module starts at 2177:000e (near-called from
 * 0x033648).  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are
 * defined in shipped order; bake consumes them in that order.  Section 0's
 * 1f4e link segment holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.  Five handlers are
 * static, near-called by ecl_dispatch ahead of their definitions, and so
 * is run_script, near-called by enter_camp ahead of its own.
 *
 * _DATA is two literals, DGROUP 5ed4..5eea; "" is stored on "PIC"'s
 * terminator.  _BSS is acbe..acc5.
 */

#define DQK_DEFINES_ECLRUN
#include "dqk.h"

char ecl_done;                                                   /* acbe */
char ecl_new;                                                    /* acbf */
static struct Character far *saved_node;                              /* acc0 */
static char pending_load;                                        /* acc4 */
static char restore_node;                                        /* acc5 */

static void far cmd_press_key(void);
static void far cmd_yes_no(void);
static void far cmd_redraw_exit(void);
static void far cmd_sound(void);
static void far cmd_nop(void);
static void far run_script(unsigned entry);

void far pending_load_set(char load_id)                                 /* 2033:0004 */
{
    pending_load = load_id;
}

char far pending_load_get(void)                                   /* 2033:000f */
{
    return pending_load;
}

void far ecl_load_args(unsigned char count)                      /* 2033:0013 */
{
    register int index;
    unsigned char n_str;
    unsigned short value;
    unsigned short len;

    n_str = 0;
    for (index = 1; index <= count; index++) {
        ecl_type[index] = ecl_memory[ecl_ip + 1];
        ecl_lo[index] = ecl_memory[ecl_ip + 2];
        ecl_ip += 2;
        switch (ecl_type[index]) {
        case 1:
        case 2:
        case 3:
            ecl_ip++;
            ecl_hi[index] = ecl_memory[ecl_ip];
            break;
        case 0x80:
            n_str++;
            len = ecl_lo[index];
            if (len > 0)
                gb_unpack(len, n_str);
            else
                ecl_str[n_str][0] = 0;
            break;
        case 0x81:
            n_str++;
            ecl_ip++;
            ecl_hi[index] = ecl_memory[ecl_ip];
            value = (ecl_hi[index] << 8) | ecl_lo[index];
            gb_fetch_str(n_str, value);
            break;
        }
    }
    ecl_ip++;
}

unsigned far ecl_arg(char idx)                                   /* 2033:0112 */
{
    unsigned char lo;
    unsigned char hi;

    lo = ecl_lo[idx];
    hi = ecl_hi[idx];
    switch (ecl_type[idx]) {
    case 0:
        return lo;
    case 1:
    case 3:
    case 0x80:
        return gb_read((hi << 8) + lo, ecl_type[idx]);
    case 2:
    case 0x81:
        return (hi << 8) | lo;
    }
    return 0;
}

void far cmd_exit(void)                                          /* 2033:01ae */
{
    struct EclFrame far *next;

    if (restore_node) {
        cur_node = saved_node;
        restore_node = 0;
    }
    mem_fill(encounter_images_shown, 2, 0);
    confirm_anim = 0;
    ecl_done = 1;
    ecl_ip++;
    while (ecl_stack) {
        next = ecl_stack->next;
        pool_free(ecl_stack, 6, &ecl_frame_pool);
        ecl_stack = next;
    }
    text_row = 0x11;
    text_col = 1;
    script_box_clear_marker = 0;
}

void far cmd_goto(void)                                          /* 2033:023e */
{
    ecl_load_args(1);
    ecl_ip = (ecl_hi[1] << 8) | ecl_lo[1];
}

void far cmd_gosub(void)                                         /* 2033:025b */
{
    ecl_load_args(1);
    ecl_gosub(1);
}

void far cmd_compare(void)                                       /* 2033:026d */
{
    unsigned short first_operand;
    unsigned short second_operand;

    ecl_load_args(2);
    if (ecl_type[1] >= 0x80 || ecl_type[2] >= 0x80)
        compare_str(ecl_str[1], ecl_str[2]);
    else {
        first_operand = ecl_arg(1);
        second_operand = ecl_arg(2);
        compare_int(first_operand, second_operand);
    }
}

void far cmd_arith(void)                                         /* 2033:02c2 */
{
    unsigned short first_operand;
    unsigned short second_operand;
    unsigned short addr;
    unsigned short unused;
    unsigned short result;

    unused;                     /* keeps its frame word, as shipped */
    ecl_load_args(3);
    first_operand = ecl_arg(1);
    second_operand = ecl_arg(2);
    addr = (ecl_hi[3] << 8) | ecl_lo[3];
    switch (ecl_op) {
    case 4:
        result = first_operand + second_operand;
        break;
    case 5:
        result = second_operand - first_operand;
        break;
    case 6:
        result = first_operand / second_operand;
        game_state[STATE_DIVISION_REMAINDER] = first_operand % second_operand;
        break;
    case 7:
        result = first_operand * second_operand;
        break;
    }
    gb_write(addr, result, ecl_type[3]);
}

void far cmd_random(void)                                        /* 2033:036a */
{
    unsigned char max;
    unsigned char random_value;
    unsigned short addr;

    ecl_load_args(2);
    max = ecl_arg(1);
    if (max < 0xff)
        max++;
    addr = (ecl_hi[2] << 8) | ecl_lo[2];
    random_value = random_n(max);
    gb_write(addr, random_value, ecl_type[2]);
}

void far cmd_save(void)                                          /* 2033:03c8 */
{
    unsigned short val;
    unsigned short addr;

    ecl_load_args(2);
    addr = (ecl_hi[2] << 8) | ecl_lo[2];
    if (ecl_type[1] < 0x80) {
        val = ecl_arg(1);
        gb_write(addr, val, ecl_type[2]);
    } else
        gb_write_str(addr, ecl_str[1]);
}

void far cmd_load_char(void)                                     /* 2033:0422 */
{
    struct Character far *character;
    unsigned char character_id;
    unsigned char scan_index;

    restore_node = 1;
    ecl_load_args(1);
    scan_index = 0;
    character_id = ecl_arg(1);
    character = node_list;
    character_id = character_id & 0x7f;
    if (character_id > 0)
        while (scan_index < character_id && character) {
            character = character->next;
            scan_index++;
        }
    if (character) {
        cur_node = character;
        character_select_failed = 0;
    } else
        character_select_failed = 1;
}

void far show_picture(unsigned char id)                          /* 2033:04a0 */
{
    timer_ticks();
    if (id != 0xff) {
        encounter_images_shown[1] = 1;
        confirm_anim = 1;
        if (id >= 0xf0) {
            big_open(id);
            big_redraw();
            if (view_mode == 3)
                cursor_show();
            else
                picture_hides_roster = 1;
            redraw_wanted = 0;
        } else {
            pic_open("PIC", id, 0, &pic);
            pic_frame(3, 3, 1, 0);
        }
    } else {
        if ((saved_view_mode != 4 || view_mode == 4) && (confirm_anim || encounter_sprite_shown)) {
            flag_c3a4 = 0;
            redraw_wanted = 1;
            view_refresh();
            confirm_anim = 0;
            encounter_sprite_shown = 0;
        }
        encounter_images_shown[0] = 0;
        encounter_images_shown[1] = 0;
    }
}

void far cmd_picture(void)                                       /* 2033:0552 */
{
    ecl_load_args(1);
    show_picture(ecl_arg(1));
}

void far cmd_return(void)                                        /* 2033:0569 */
{
    struct EclFrame far *next;

    ecl_ip++;
    if (ecl_stack) {
        ecl_ip = ecl_stack->ip;
        next = ecl_stack->next;
        pool_free(ecl_stack, 6, &ecl_frame_pool);
        ecl_stack = next;
    } else
        cmd_exit();
}

void far cmd_compare_and(void)                                   /* 2033:05be */
{
    char eq1;
    char eq2;

    mem_zero(ecl_conditions, 4);
    ecl_load_args(4);
    if (ecl_type[1] >= 0x80) {
        eq1 = ecl_str[1] == ecl_str[2];     /* the addresses, as shipped */
        if (ecl_type[3] >= 0x80)
            eq2 = ecl_str[3] == ecl_str[4];
        else
            eq2 = ecl_arg(3) == ecl_arg(4);
    } else {
        eq1 = ecl_arg(1) == ecl_arg(2);
        if (ecl_type[3] >= 0x80)
            eq2 = ecl_str[1] == ecl_str[2]; /* 1 and 2, not 3 and 4 */
        else
            eq2 = ecl_arg(3) == ecl_arg(4);
    }
    if (eq1 && eq2)
        ecl_conditions[0] = 1;
    else
        ecl_conditions[1] = 1;
}

void far cmd_if(void)                                            /* 2033:06a6 */
{
    ecl_ip++;
    if (!ecl_conditions[ecl_op - 0x16])
        ecl_skip();
}

void far cmd_and_or(void)                                        /* 2033:06be */
{
    unsigned short first_operand;
    unsigned short second_operand;
    unsigned short addr;
    unsigned short unused;
    unsigned char bitwise_result;

    unused;                     /* keeps its frame word, as shipped */
    ecl_load_args(3);
    first_operand = ecl_arg(1);
    second_operand = ecl_arg(2);
    addr = (ecl_hi[3] << 8) | ecl_lo[3];
    if (ecl_op == 0x2f)
        bitwise_result = first_operand & second_operand;
    else
        bitwise_result = first_operand | second_operand;
    compare_int(0, bitwise_result);
    gb_write(addr, bitwise_result, ecl_type[3]);
}

void far cmd_get_table(void)                                     /* 2033:073a */
{
    unsigned short base;
    unsigned short dest;
    unsigned short addr;
    unsigned short val;
    unsigned char idx;

    ecl_load_args(3);
    base = (ecl_hi[1] << 8) | ecl_lo[1];
    idx = ecl_arg(2);
    dest = (ecl_hi[3] << 8) | ecl_lo[3];
    addr = idx + base;
    val = gb_read(addr, ecl_type[3]);
    gb_write(dest, val, ecl_type[3]);
}

void far cmd_save_table(void)                                    /* 2033:07a9 */
{
    unsigned short val;
    unsigned short base;
    unsigned short idx;
    unsigned short unused;
    unsigned short addr;

    unused;                     /* keeps its frame word, as shipped */
    ecl_load_args(3);
    val = ecl_arg(1);
    base = (ecl_hi[2] << 8) | ecl_lo[2];
    idx = ecl_arg(3);
    addr = base + idx;
    gb_write(addr, val, ecl_type[1]);
}

void far cmd_clear_monsters(void)                                /* 2033:07fe */
{
    ecl_ip++;
    post_reset();
    scripted_monsters_queued = 0;
    encounter_npc_count = 0;
}

void far cmd_check_party(void)                                   /* 2033:0812 */
{
    ecl_load_args(6);
}

void far cmd_combat(void)                                        /* 2033:081b */
{
    unsigned short monster_distance;
    char combat_requested;
    char saved;

    picture_hides_roster = 0;
    saved = exploration_menu_context;
    exploration_menu_context = 0;
    ecl_ip++;
    combat_requested = 1;
    if (!scripted_monsters_queued) {
        if (game_state[STATE_VAULT_SERVICE] == 1) {
            game_state[STATE_VAULT_SERVICE] = 0;
            vault_menu();
        } else if (game_state[STATE_TEMPLE_SERVICE] == 1) {
            temple_shop();
            game_state[STATE_TEMPLE_SERVICE] = 0;
            if (!game_state[STATE_DUNGEON])
                combat_requested = 0;
        } else
            post_combat();
    } else {
        pic_release();
        monster_distance = monster_dist(geo.x, geo.y, geo.facing);
        if (game_state[STATE_ENCOUNTER_DISTANCE] > monster_distance)
            game_state[STATE_ENCOUNTER_DISTANCE] = monster_distance;
        mode_change(4);
        box_draw();
        status_printf(0, 0x18, 7, 0, "A battle begins...");
        monsters_place();
        wander_load();
        combat_main();
        post_combat();
        if (!game_state[STATE_DUNGEON] && game_state[STATE_WILDERNESS] == 1)
            big_open(game_state[STATE_WILDERNESS_PICTURE_ID]);
    }
    if (!game_state[STATE_DUNGEON] && game_state[STATE_WILDERNESS] == 1)
        view_mode = 3;
    else
        view_mode = 4;
    game_state[STATE_SEARCH_FLAGS] = game_state[STATE_SEARCH_FLAGS] & 1;
    mem_fill(encounter_images_shown, 2, 0);
    confirm_anim = 0;
    if (combat_requested) {
        view_redraw();
        if (!game_state[STATE_DUNGEON] && !adventure_exit && game_state[STATE_WILDERNESS] == 1)
            cursor_show();
    }
    exploration_menu_context = saved;
}

void far cmd_on_goto(void)                                       /* 2033:0991 */
{
    unsigned char sel;
    unsigned char argument_count;
    unsigned char scan_index;
    char done;

    done = 0;
    ecl_load_args(2);
    sel = ecl_arg(1);
    argument_count = ecl_arg(2);
    ecl_ip--;
    ecl_load_args(argument_count);
    scan_index = 0;
    while (scan_index < argument_count && !done) {
        if (sel == scan_index) {
            if (ecl_op == 0x25)
                ecl_ip = (ecl_hi[scan_index + 1] << 8) | ecl_lo[scan_index + 1];
            else
                ecl_gosub(scan_index + 1);
            done = 1;
        } else
            scan_index++;
    }
}

void far cmd_delay(void)                                         /* 2033:0a26 */
{
    ecl_ip++;
    delay(600);
}

void far cmd_clock(void)                                         /* 2033:0a35 */
{
    unsigned char elapsed_units;
    short unit;

    ecl_load_args(2);
    elapsed_units = ecl_arg(1);
    unit = ecl_arg(2);
    clock_advance(elapsed_units, unit);
}

void far cmd_print_return(void)                                  /* 2033:0a68 */
{
    ecl_ip++;
    if (print_return_flag) {
        text_col = 1;
        text_row++;
        print_return_flag = 0;
    } else {
        text_col = 1;
        text_row++;
    }
}

void far cmd_clear_box(void)                                     /* 2033:0a8d */
{
    ecl_ip++;
    picture_hides_roster = 0;
    frame_main();
    party_display(cur_node);
    party_info_display();
    noncombat_redraw_pending = 0;
    script_box_clear_marker = 1;
}

void far cmd_call(void)                                          /* 2033:0aba */
{
    unsigned short addr;

    ecl_load_args(1);
    addr = (ecl_hi[1] << 8) | ecl_lo[1];
    switch (addr) {
    case 1:
        view_update();
        break;
    case 2:
        step_forward();
        break;
    case 3:
        pic_frame(3, 3, 1, anim_frame);
        if (anim_first)
            moon_display_update(1);
        anim_frame++;
        if (anim_frame > anim_last)
            anim_frame = anim_first;
        text_pause();
        break;
    }
}

void far enter_camp(void)                                        /* 2033:0b3b */
{
    char return_code;

    return_code = 0;
    run_script(ecl_before_camp);
    camp_menu(&return_code);
    if (return_code) {
        view_redraw();
        run_script(ecl_after_camp);
        while (ecl_new) {
            ecl_new = 0;
            run_script(ecl_main_entry);
        }
    }
    redraw_wanted = 1;
    view_refresh();
    if (!game_state[STATE_DUNGEON] || game_state[STATE_WILDERNESS] == 1)
        cursor_show();
    game_saved = 0;
}

void far cmd_program(void)                                       /* 2033:0baf */
{
    unsigned char sel;
    unsigned short saved_instruction_pointer;
    char saved;

    saved = exploration_menu_context;
    exploration_menu_context = 0;
    if (restore_node) {
        cur_node = saved_node;
        restore_node = 0;
    }
    ecl_load_args(1);
    sel = ecl_arg(1);
    switch (sel) {
    case 0:
        party_menu(0);
        if (view_kind != 9 && !game_state[STATE_DUNGEON] && game_state[STATE_WILDERNESS] == 1 && !adventure_restart) {
            view_redraw();
            cursor_show();
        }
        break;
    case 1:
        ending_show();
        break;
    case 2:
        game_exit();
        break;
    case 3:
        fx_drain(cur_node, 1);
        break;
    case 9:
        saved_instruction_pointer = ecl_ip;
        enter_camp();
        ecl_ip = saved_instruction_pointer;
        if (game_state[STATE_WILDERNESS] == 1)
            cmd_exit();
        break;
    }
    script_view_dirty = 1;
    exploration_menu_context = saved;
}

void far cmd_dump(void)                                          /* 2033:0c9b */
{
    ecl_ip++;
    hero_drop(0, 1);
    saved_node = cur_node;
    party_display(cur_node);
}

void far cmd_find_special(void)                                  /* 2033:0cc5 */
{
    struct Effect far *out;
    char effect_id;

    mem_fill(ecl_conditions, 6, 0);
    ecl_load_args(1);
    effect_id = ecl_arg(1);
    if (find_effect(cur_node, effect_id, &out))
        ecl_conditions[0] = 1;
    else
        ecl_conditions[1] = 1;
}

void far ecl_dispatch(void)                                      /* 2033:0d1e */
{
    switch (ecl_op) {
    case 0x00: cmd_exit(); break;
    case 0x01: cmd_goto(); break;
    case 0x02: cmd_gosub(); break;
    case 0x03: cmd_compare(); break;
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07: cmd_arith(); break;
    case 0x08: cmd_random(); break;
    case 0x09: cmd_save(); break;
    case 0x0a: cmd_load_char(); break;
    case 0x0b: cmd_load_monster(); break;
    case 0x0c: cmd_setup_monster(); break;
    case 0x0d: cmd_approach(); break;
    case 0x0e: cmd_picture(); break;
    case 0x0f: cmd_input_number(); break;
    case 0x10: cmd_input_string(); break;
    case 0x11:
    case 0x12: cmd_print(); break;
    case 0x13: cmd_return(); break;
    case 0x14: cmd_compare_and(); break;
    case 0x15: cmd_vert_menu(); break;
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b: cmd_if(); break;
    case 0x1c: cmd_clear_monsters(); break;
    case 0x1d: cmd_press_key(); break;
    case 0x1e: cmd_check_party(); break;
    case 0x1f: cmd_journal(); break;
    case 0x20: cmd_new_ecl(); break;
    case 0x21: cmd_load_files(); break;
    case 0x22: cmd_yes_no(); break;
    case 0x23: cmd_redraw_exit(); break;
    case 0x24: cmd_combat(); break;
    case 0x25:
    case 0x26: cmd_on_goto(); break;
    case 0x27: cmd_treasure(); break;
    case 0x28: cmd_rob(); break;
    case 0x29: cmd_nop(); break;
    case 0x2a: cmd_get_table(); break;
    case 0x2b: cmd_horiz_menu(0); break;
    case 0x2c: cmd_sound(); break;
    case 0x2d: cmd_call(); break;
    case 0x2e: cmd_damage(); break;
    case 0x2f:
    case 0x30: cmd_and_or(); break;
    case 0x31: cmd_horiz_menu(1); break;
    case 0x32: cmd_find_item(); break;
    case 0x33: cmd_print_return(); break;
    case 0x34: cmd_clock(); break;
    case 0x35: cmd_save_table(); break;
    case 0x36: cmd_add_npc(); break;
    case 0x37: cmd_find_name(); break;
    case 0x38: cmd_program(); break;
    case 0x39: cmd_who(); break;
    case 0x3a: cmd_delay(); break;
    case 0x3b: cmd_spell(); break;
    case 0x3c: cmd_shop(); break;
    case 0x3d: cmd_clear_box(); break;
    case 0x3e: cmd_dump(); break;
    case 0x3f: cmd_find_special(); break;
    case 0x40: cmd_destroy_items(); break;
    case 0x41: cmd_award_xp(); break;
    }
}

static void far run_script(unsigned entry)                       /* 2033:0f46 */
{
    int key;

    tick_slot = 1;
    ecl_ip = entry;
    ecl_done = 0;
    esc_set(0);
    while (!ecl_done && !adventure_exit && (!demo_active || !esc_get()) && !adventure_restart) {
        prev_op = ecl_op;
        ecl_op = ecl_memory[ecl_ip];
        ecl_dispatch();
        if (demo_active && key_poll()) {
            key = key_get();
            if (key == 0x152)
                sound_toggle();
            if (key == 0x153)
                music_toggle();
            if (demo_active && (key == 0x1b || key == 0x60))
                esc_set(1);
        }
    }
    ecl_done = 0;
}

void far run_new_script(void)                                    /* 2033:0ff4 */
{
    do {
        pic_release();
        str_copy(pic_saved, "");
        view_kind_saved = 0xff;
        ecl_new = 0;
        geo.flag_4 = area_cell_at(geo.x, geo.y);
        game_state[STATE_MAP_EXIT] = 0;
        saved_node = cur_node;
        run_script(ecl_main_entry);
        if (!ecl_new)
            game_state[STATE_SCRIPT_ID] = ecl_id;
        if (!ecl_new) {
            if (saved_view_mode != 4 || view_mode == 4 || (saved_view_mode == 4 && view_mode == 4))
                view_refresh();
            ecl_new = 0;
            run_script(ecl_before_move);
            if (!ecl_new) {
                run_script(ecl_after_move);
                if (!ecl_new) {
                    cur_node = saved_node;
                    party_display(cur_node);
                }
            }
        }
    } while (ecl_new);
    saved_view_mode = view_mode;
}

void far game_loop(void)                                   /* 2033:10cd */
{
    unsigned char choice;
    struct Character far *character;
    unsigned char menu;

    do {
        adventure_restart = 0;
        saved_node = cur_node;
        redraw_wanted = 1;
        restore_node = 0;
        view_mode = 4;
        ecl_new = 0;
        pending_load_set(0);
        if (!game_state[STATE_SCRIPT_ID]) {
            if (demo_active)
                ecl_id = 7;
            else {
                ecl_id = 4;
                party_display(cur_node);
            }
        } else
            ecl_id = game_state[STATE_SCRIPT_ID];
        if (!game_state[STATE_DUNGEON])
            view_mode = 3;
        ecl_load(ecl_id);
        ecl_init();
        run_script(ecl_main_entry);
        if (demo_active) {
            character = node_list;
            while (character) {
                cur_node = character;
                character = character->next;
                hero_drop(1, 1);
            }
            return;
        }
        if (!ecl_new)
            game_state[STATE_SCRIPT_ID] = ecl_id;
        else
            run_new_script();
        if (view_mode != 3 && game_loaded) {
            if (noncombat_redraw_pending == 1)
                view_redraw();
            redraw_wanted = 1;
            view_refresh();
        }
        game_loaded = 0;
        exploration_menu_context = 0;
        menu = 1;
        do {
            ui_mode = menu;
            choice = adv_menu();
            menu = ui_mode;
            saved_node = cur_node;
            if (!ecl_new)
                game_state[STATE_SCRIPT_ID] = ecl_id;
            while ((game_state[STATE_SEARCH_FLAGS] > 1 || choice == 4) && !adventure_exit && !adventure_restart) {
                if (choice == 4)
                    enter_camp();
                else {
                    saved_search_flag = game_state[STATE_SEARCH_FLAGS] & 1;
                    game_state[STATE_SEARCH_FLAGS] = 1;
                    redraw_wanted = 1;
                    view_refresh();
                    run_script(ecl_after_move);
                    if (ecl_new)
                        run_new_script();
                    game_state[STATE_SEARCH_FLAGS] = saved_search_flag;
                }
                if (!adventure_exit && !adventure_restart) {
                    ui_mode = menu;
                    choice = adv_menu();
                    menu = ui_mode;
                    saved_node = cur_node;
                }
            }
            if (!adventure_exit && !adventure_restart)
                run_script(ecl_before_move);
            if (ecl_new)
                run_new_script();
            else if (!adventure_exit && !adventure_restart) {
                game_state[STATE_PREVIOUS_X] = geo.x;
                game_state[STATE_PREVIOUS_Y] = geo.y;
                move_resolve();
                view_refresh();
                confirm_anim = 0;
                run_script(ecl_after_move);
                if (ecl_new)
                    run_new_script();
            }
        } while (!adventure_exit && !adventure_restart);
    } while (!adventure_exit);
    adventure_exit = 0;
    character = node_list;
    while (character) {
        cur_node = character;
        character = character->next;
        hero_drop(0, 1);
    }
    if (pending_load_get())
        game_load_menu();
}

static void far cmd_press_key(void)                              /* 2033:1383 */
{
    char buf[42];

    ecl_ip++;
    str_copy(buf, press_text);
    menu_horiz("", buf, 1, confirm_anim);
    box_draw();
}

static void far cmd_yes_no(void)                                 /* 2033:13c4 */
{
    ecl_ip++;
    mem_zero(ecl_conditions, 4);
    if (confirm(empty_text, 1))
        ecl_conditions[0] = 1;
    else
        ecl_conditions[1] = 1;
    box_draw();
}

static void far cmd_redraw_exit(void)                            /* 2033:1402 */
{
    text_clear(1, 0x11, 0x26, 0x16);
    view_update();
    cmd_exit();
}

static void far cmd_sound(void)                                  /* 2033:1420 */
{
    unsigned char sound_id;

    ecl_load_args(1);
    sound_id = ecl_arg(1);
    sfx_play(sound_id);
}

static void far cmd_nop(void)                                    /* 2033:1449 */
{
    ecl_ip++;
}
