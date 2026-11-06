/* DQK.EXE overlay 04 module 25de: allocating the game's buffers and
 * starting a new game.
 * game_setup reads the configuration, allocates the shared buffers and has
 * module 25ca size the slot pools; game_init loads the font, sets up the
 * palette, clears the party, combat and view state, and reads ITEMS.DAT
 * into item_table and item.dat into item_templates; game_reset clears part of
 * that state again.  arena_alloc hands out storage from one block that
 * db_buffer returns, and never frees it.
 *
 * EXE 0x037cb6..0x0383cb, one Turbo C large-model _TEXT segment starting at
 * byte 6 of paragraph 25de in RTLink section 4 (payload 0x37910, link
 * segment 25a4).  The first 6 bytes of paragraph 25de are the tail of
 * module 25ca, and the last 11 bytes of the range lie in paragraph 264f;
 * the uncalled retf after them, 264f:000b, is left to module 264f.  Built
 * with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped
 * order; bake consumes them in that order.  Sections 5 to 8 hold other
 * bytes at the 25a4 link segment.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use.  game_setup, game_init and
 * game_reset are reached through the vector stubs 1d99:119d, 1d99:1193
 * and 1d99:1161, from gamemain.c.  (v1.0 has no game_done, the empty
 * routine v1.1 added after game_setup, and game_init calls sfx_init after
 * the palette set-up rather than after the loading message.)
 *
 * _DATA is DGROUP 6d78..6dc0: color_table, alloc_next and five literals.
 * _BSS is empty: every other variable this module writes is declared extern.
 */

#include "dqk.h"

extern void far pal_glyph(int slot, int n, char fade);           /* 169b:0f27 */

void far *far arena_alloc(int byte_count);

static char color_table[16] = {                                  /* 6d78 */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x17, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
static char far *alloc_next = 0;                                 /* 6d88 */

static void item_data_load(void);

char far item_table_read(int file_handle)                                 /* 25de:0006 */
{
    register int size = 0x800;

    return dos_io(0x3f00, item_table, file_handle, size) == size;
}

void far game_setup(void)                                        /* 25de:0033 */
{
    config_load();
    db_init(0x9400L);
    dl_init(40);
    game_state = mem_alloc(STATE_BYTE_COUNT);
    game_state = game_state - STATE_FIRST_BYTE;
    ecl_memory = mem_alloc(0x2ee3);
    ecl_memory = ecl_memory + 0x8000;
    item_table = mem_alloc(0x810);
    item_templates = mem_alloc(0x10ef);
    party_pool_init(8);
    item_pool_init(440);
    item_pool_select(200);
    ecl_stack_pool_init(50);
    effect_pool_init(400);
    area_alloc();
    sfx_setup();
    target_scores = arena_alloc(0xab2);
    cur_map = arena_alloc(0x4eb);
    npc_pool_init(50);
    cloud_pool_init(70);
    combat_pool_init(59);
    drop_pool_init(20);
}

void far game_init(void)                                         /* 25de:0131 */
{
    unsigned char index;

    video_check();
    font_load('1', "game");
    ticks_start();
    text_print_at(0x1fa0, 0x1f40, 10, "Loading...Please Wait");
    glyph_library_open('1', "always", 0);
    if (is_xvga() || !pixel_shift()) {
        pal_init();
        glyph_palette_load(slot_base(0), 0);
        pal_commit();
        color_map_set(color_table);
    } else
        pal_glyph(0, 0, 0);
    sfx_init();
    res_reset();
    button_hook_install();
    misc_unit_init();
    debug_password_init();
    mem_zero(combatants, sizeof(combatants));
    combatant_count = 0;
    long_bd94 = 0;
    mem_zero(wall_set_res, 4);      /* slot 0 only */
    mem_fill(&pic_res, sizeof(pic_res), 0);
    mem_fill(game_state + STATE_FIRST_BYTE, STATE_BYTE_COUNT, 0);
    game_state[STATE_DIFFICULTY] = 3;
    game_state[STATE_DUNGEON] = 1;
    game_state[STATE_SCRIPT_ID] = 0;
    vault_slot = 90;
    game_state[STATE_MOON_PHASE_1] = 2;
    game_state[STATE_MOON_PHASE_2] = 2;
    game_state[STATE_MOON_PHASE_3] = 2;
    game_state[STATE_SCRIPT_TEXT_COLOR_MODE] = 0;
    mem_fill(ecl_memory + 0x8000, 0x2ee3, 0);
    mem_zero(&back_res, 8);     /* and the far pointer at bd28 */
    word_bd1e = 0;
    word_b2eb = 0;
    res_alloc(&icon_res, 157);
    mem_zero(arr_c6ba, 36);
    mem_zero(anim_state, 31);
    for (index = 1; index <= 2; index++) {
        portrait_slots[index].flag = 0xff;
        portrait_slots[index].value = 0;
    }
    mem_fill(&pic, sizeof(pic), 0);
    big_res = 0;
    pool_items = 0;
    mem_fill(&geo, 5, 0);
    geo.x = 7;
    geo.y = 13;
    geo.facing = 0;
    mem_zero(&geo.pic_set, 4);
    pictures_enabled = 1;
    text_slow = 1;
    game_loaded = 0;
    encounter_ticks = 0;
    node_list = 0;
    cur_node = 0;
    selected_target = 0;
    previous_highlighted_character = 0;
    ecl_ip = 0x8000;
    game_state[STATE_SPEED] = 4;
    demo_active = 0;
    geo.show_map = 0;
    game_state[STATE_PARTY_COUNT] = 0;
    menu_top_entry = 1;
    resting = 0;
    saved_search_flag = 0;
    script_box_clear_marker = 0;
    print_return_flag = 0;
    confirm_anim = 0;
    adventure_exit = 0;
    encounter_sprite_shown = 0;
    monster_unpack_in_progress = 0;
    pic_name[0] = 0;
    pic_saved[0] = 0;
    view_kind = 0xff;
    view_kind_saved = 0xff;
    game_saved = 0;
    horizon_color_select = 0xe8;
    wall_color_select = 0xdb;
    encounter_menu_active = 0;
    order_mode = 0;
    move_flag = 1;
    view_flag = 0xff;
    ui_mode = 1;
    view_mode = 4;
    saved_view_mode = 0;
    item_handler_active = 0;
    no_study = 0;
    flag_c3a4 = 0;
    scripted_monsters_queued = 0;
    picture_hides_roster = 0;
    geo.long_7[0] = 0;
    geo.long_7[2] = 0;
    geo.long_7[1] = 0;
    byte_bd1d = 0;
    move_menu_active = 0;
    aim_cursor_active = 0;
    exploration_menu_context = 0;
    picker_direct_select = 0;
    year_rolled = 0;
    file_open('1', "ITEMS.DAT", 0, item_table_read);
    spellfx_init();
    fx_install();
    input_done();
    set_step_mode(0);
    item_data_load();
}

void far game_reset(void)                                        /* 25de:04ec */
{
    mem_fill(game_state + STATE_FIRST_BYTE, STATE_BYTE_COUNT, 0);
    game_state[STATE_DIFFICULTY] = 3;
    game_state[STATE_DUNGEON] = 1;
    game_state[STATE_SCRIPT_ID] = 0;
    vault_slot = 90;
    game_state[STATE_SCRIPT_TEXT_COLOR_MODE] = 0;
    mem_fill(ecl_memory + 0x8000, 0x2ee3, 0);
    mem_fill(&geo, 5, 0);
    geo.x = 7;
    geo.y = 13;
    geo.facing = 2;
    party_click_enabled = 1;
    mem_zero(&geo.pic_set, 4);
    text_slow = 1;
    game_loaded = 0;
    encounter_ticks = 0;
    node_list = 0;
    cur_node = 0;
    previous_highlighted_character = 0;
    ecl_ip = 0x8000;
    game_state[STATE_SPEED] = 4;
    geo.show_map = 0;
    game_state[STATE_PARTY_COUNT] = 0;
    menu_top_entry = 1;
    resting = 0;
    saved_search_flag = 0;
    script_box_clear_marker = 0;
    print_return_flag = 0;
    confirm_anim = 0;
    adventure_exit = 0;
    encounter_sprite_shown = 0;
    monster_unpack_in_progress = 0;
    pic_name[0] = 0;
    pic_saved[0] = 0;
    view_kind = 0xff;
    view_kind_saved = 0xff;
    game_saved = 0;
    horizon_color_select = 0xe8;
    wall_color_select = 0xdb;
    encounter_menu_active = 0;
    move_flag = 1;
    view_flag = 0xff;
    box_draw();
    ui_mode = 1;
    view_mode = 4;
    saved_view_mode = 0;
    item_handler_active = 0;
    byte_bd1d = 0;
    move_menu_active = 0;
    aim_cursor_active = 0;
    exploration_menu_context = 0;
    picker_direct_select = 0;
    order_mode = 0;
    picture_hides_roster = 0;
    year_rolled = 0;
    scripted_monsters_queued = 0;
}

char far item_data_read(int file_handle)                                  /* 25de:066c */
{
    mem_fill(item_templates, 0x10ef, 0xff);
    return dos_io(0x3f00, item_templates + 1, file_handle, 0x10de) == 0x10de;
}

static void item_data_load(void)                                 /* 25de:06b2 */
{
    char name[22];

    str_copy(name, "item.dat");
    file_open('1', name, 0, item_data_read);
}

void far *far arena_alloc(int byte_count)                             /* 25de:06e6 */
{
    char far *allocation;

    if (alloc_next == 0)
        alloc_next = db_buffer();
    allocation = alloc_next;
    alloc_next += byte_count;
    return allocation;
}
