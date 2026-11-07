;  globals.asm -- the game's communal variables, for the RTLink/Plus build.
;
;  Turbo C 2.01 emits an uninitialised global (`int x;` at file scope) as a
;  communal variable, which the linker, not the compiler, allocates.  In the
;  shipped DQK.EXE these 218 sit in DGROUP's _BSS at acd8..c6f7, after
;  config.c's _BSS and before copyprot.c's, with effects.c's own _BSS at
;  beec..c27f in the middle of them.  The reconstructed C declares every one of
;  them extern in dqk.h (the byte-exact build resolved them by address), so no
;  object defines them; this file does, in their shipped order and spacing,
;  so variables that were neighbours in the shipped image are neighbours here.
;  Sizes are the distance to the next variable, so a variable owns any unnamed
;  bytes after it.  effects.c's own _BSS, which sat at beec..c27f, is not
;  reserved here: it is effects.c's to place.  The declaration beside each
;  name is dqk.h's.
;
;  Generated from src/generated/manifest.json (each variable's DGROUP offset)
;  and dqk.h.  Assembled with TASM 3.1 /ml; zeroed at start-up with the rest of
;  _BSS by start.asm.

                .8086

DGROUP          group   _BSS
_BSS            segment word public 'BSS'

                public  _sound_card
_sound_card     db          8 dup (?)       ; acd8  extern int sound_card;
                public  _save_drive
_save_drive     db          2 dup (?)       ; ace0  extern int save_drive;
                public  _save_path
_save_path      db        133 dup (?)       ; ace2  extern char save_path[];
                public  _cur_node
_cur_node       db          4 dup (?)       ; ad67  extern struct Character far *cur_node;
                public  _next_monster_group
_next_monster_group db          1 dup (?)       ; ad6b  extern unsigned char next_monster_group;
                public  _node_count
_node_count     db          2 dup (?)       ; ad6c  extern int node_count;
                public  _ice_explosion_count
_ice_explosion_count db          1 dup (?)       ; ad6e  extern unsigned char ice_explosion_count;
                public  _meteor_explosion_count
_meteor_explosion_count db          1 dup (?)       ; ad6f  extern unsigned char meteor_explosion_count;
                public  _ice_explosion_queue
_ice_explosion_queue db         84 dup (?)       ; ad70  extern struct Character far *ice_explosion_queue[];
                public  _meteor_explosion_queue
_meteor_explosion_queue db         86 dup (?)       ; adc4  extern struct Character far *meteor_explosion_queue[];
                public  _panel_x1
_panel_x1       db          1 dup (?)       ; ae1a  extern unsigned char panel_x1;
                public  _panel_y1
_panel_y1       db          5 dup (?)       ; ae1b  extern unsigned char panel_y1;
                public  _panel_x2
_panel_x2       db          1 dup (?)       ; ae20  extern unsigned char panel_x2;
                public  _panel_y2
_panel_y2       db          5 dup (?)       ; ae21  extern unsigned char panel_y2;
                public  _tick_slot
_tick_slot      db          1 dup (?)       ; ae26  extern char tick_slot;
                public  _redraw_wanted
_redraw_wanted  db          1 dup (?)       ; ae27  extern char redraw_wanted;
                public  _debug_password
_debug_password db         22 dup (?)       ; ae28  extern char debug_password[];
                public  _item_table
_item_table     db          4 dup (?)       ; ae3e  extern struct ItemInfo far *item_table;
                public  _view_kind
_view_kind      db          1 dup (?)       ; ae42  extern char view_kind;
                public  _view_flag
_view_flag      db          5 dup (?)       ; ae43  extern char view_flag;
                public  _turn_done
_turn_done      db          3 dup (?)       ; ae48  extern char turn_done;
                public  _view_kind_saved
_view_kind_saved db          1 dup (?)       ; ae4b  extern char view_kind_saved;
                public  _combat_round
_combat_round   db          1 dup (?)       ; ae4c  extern unsigned char combat_round;
                public  _text_col
_text_col       db          1 dup (?)       ; ae4d  extern unsigned char text_col;
                public  _text_row
_text_row       db          1 dup (?)       ; ae4e  extern unsigned char text_row;
                public  _exp_share
_exp_share      db          6 dup (?)       ; ae4f  extern long exp_share;
                public  _damage
_damage         db          2 dup (?)       ; ae55  extern int damage;
                public  _view_mode
_view_mode      db          2 dup (?)       ; ae57  extern int view_mode;
                public  _demo_active
_demo_active    db          1 dup (?)       ; ae59  extern char demo_active;
                public  _pooled
_pooled         db          1 dup (?)       ; ae5a  extern char pooled;
                public  _game_saved
_game_saved     db          1 dup (?)       ; ae5b  extern char game_saved;
                public  _ecl_memory
_ecl_memory     db          5 dup (?)       ; ae5c  extern unsigned char far *ecl_memory;
                public  _move_rate
_move_rate      db          2 dup (?)       ; ae61  extern unsigned char move_rate;
                public  _ecl_id
_ecl_id         db          1 dup (?)       ; ae63  extern char ecl_id;
                public  _confirm_anim
_confirm_anim   db          1 dup (?)       ; ae64  extern char confirm_anim;
                public  _text_slow
_text_slow      db          1 dup (?)       ; ae65  extern char text_slow;
                public  _refused
_refused        db          1 dup (?)       ; ae66  extern char refused;
                public  _saved_view_mode
_saved_view_mode db          2 dup (?)       ; ae67  extern int saved_view_mode;
                public  _prev_op
_prev_op        db          1 dup (?)       ; ae69  extern unsigned char prev_op;
                public  _ecl_hi
_ecl_hi         db         66 dup (?)       ; ae6a  extern unsigned char ecl_hi[];
                public  _grid_x
_grid_x         db         52 dup (?)       ; aeac  extern char grid_x[];
                public  _grid_y
_grid_y         db         53 dup (?)       ; aee0  extern char grid_y[];
                public  _hotkeys
_hotkeys        db         40 dup (?)       ; af15  extern struct HotKey hotkeys[20];
                public  _exploration_menu_context
_exploration_menu_context db          1 dup (?)       ; af3d  extern char exploration_menu_context;
                public  _same_team_attack_approved
_same_team_attack_approved db          1 dup (?)       ; af3e  extern char same_team_attack_approved;
                public  _combat_won
_combat_won     db          1 dup (?)       ; af3f  extern char combat_won;
                public  _resting
_resting        db          1 dup (?)       ; af40  extern char resting;
                public  _ecl_after_camp
_ecl_after_camp db          2 dup (?)       ; af41  extern unsigned ecl_after_camp;
                public  _ecl_main_entry
_ecl_main_entry db          2 dup (?)       ; af43  extern unsigned ecl_main_entry;
                public  _spell_fn
_spell_fn       db        348 dup (?)       ; af45  extern void (far *spell_fn[])(void);
                public  _ice_storm_handler
_ice_storm_handler db        220 dup (?)       ; b0a1  extern void (far *ice_storm_handler)(void);
                public  _icon_res
_icon_res       db          4 dup (?)       ; b17d  extern struct Res far *icon_res;
                public  _effect_id
_effect_id      db          1 dup (?)       ; b181  extern char effect_id;
                public  _save_succeeded
_save_succeeded db          1 dup (?)       ; b182  extern char save_succeeded;
                public  _training_done
_training_done  db          1 dup (?)       ; b183  extern char training_done;
                public  _fx_lock
_fx_lock        db          1 dup (?)       ; b184  extern char fx_lock;
                public  _ecl_lo
_ecl_lo         db         65 dup (?)       ; b185  extern unsigned char ecl_lo[];
                public  _attack_rate
_attack_rate    db          1 dup (?)       ; b1c6  extern unsigned char attack_rate;
                public  _ecl_op
_ecl_op         db          1 dup (?)       ; b1c7  extern unsigned char ecl_op;
                public  _ecl_before_move
_ecl_before_move db          2 dup (?)       ; b1c8  extern unsigned ecl_before_move;
                public  _encounter_picture
_encounter_picture db          1 dup (?)       ; b1ca  extern unsigned char encounter_picture;
                public  _magic_on
_magic_on       db          1 dup (?)       ; b1cb  extern char magic_on;
                public  _rest_time
_rest_time      db         14 dup (?)       ; b1cc  extern unsigned rest_time[7];
                public  _down_count
_down_count     db          1 dup (?)       ; b1da  extern unsigned char down_count;
                public  _pool
_pool           db         12 dup (?)       ; b1db  extern void far pool_alloc(void far * far *out, int unused, struct Pool far *pool);
                public  _pool_items
_pool_items     db          4 dup (?)       ; b1e7  extern struct Item far *pool_items;
                public  _menu_top_entry
_menu_top_entry db          2 dup (?)       ; b1eb  extern int menu_top_entry;
                public  _game_loaded
_game_loaded    db          3 dup (?)       ; b1ed  extern char game_loaded;
                public  _anim_state
_anim_state     db         27 dup (?)       ; b1f0  extern char anim_state[];
                public  _byte_b20b
_byte_b20b      db          1 dup (?)       ; b20b  extern char byte_b20b;
                public  _anim_first
_anim_first     db          1 dup (?)       ; b20c  extern char anim_first;
                public  _anim_last
_anim_last      db          1 dup (?)       ; b20d  extern unsigned char anim_last;
                public  _anim_frame
_anim_frame     db          1 dup (?)       ; b20e  extern unsigned char anim_frame;
                public  _game_state
_game_state     db          6 dup (?)       ; b20f  extern unsigned char far *game_state;
                public  _ecl_type
_ecl_type       db         65 dup (?)       ; b215  extern unsigned char ecl_type[];
                public  _picture_hides_roster
_picture_hides_roster db          1 dup (?)       ; b256  extern char picture_hides_roster;
                public  _death_explosion_active
_death_explosion_active db          1 dup (?)       ; b257  extern char death_explosion_active;
                public  _dropped_item_pool
_dropped_item_pool db         61 dup (?)       ; b258  extern struct Pool dropped_item_pool;
                public  _portrait_slots
_portrait_slots db          3 dup (?)       ; b295  extern struct Slot portrait_slots[];
                public  _ecl_before_camp
_ecl_before_camp db         12 dup (?)       ; b298  extern unsigned ecl_before_camp;
                public  _party_pool
_party_pool     db         67 dup (?)       ; b2a4  extern struct Pool party_pool;
                public  _save_bonus
_save_bonus     db          1 dup (?)       ; b2e7  extern unsigned char save_bonus;
                public  _ecl_after_move
_ecl_after_move db          2 dup (?)       ; b2e8  extern unsigned ecl_after_move;
                public  _current_spell
_current_spell  db          1 dup (?)       ; b2ea  extern unsigned char current_spell;
                public  _word_b2eb
_word_b2eb      db          4 dup (?)       ; b2eb  extern int word_b2eb;
                public  _attacks_used
_attacks_used   db          6 dup (?)       ; b2ef  extern unsigned char attacks_used[];
                public  _ecl_conditions
_ecl_conditions db          6 dup (?)       ; b2f5  extern char ecl_conditions[4];
                public  _script_text_cleared
_script_text_cleared db          1 dup (?)       ; b2fb  extern char script_text_cleared;
                public  _heroes
_heroes         db        208 dup (?)       ; b2fc  extern struct Character far *heroes[];
                public  _ecl_stack
_ecl_stack      db          4 dup (?)       ; b3cc  extern struct EclFrame far *ecl_stack;
                public  _item_cur
_item_cur       db          4 dup (?)       ; b3d0  extern struct Item far *item_cur;
                public  _cloud_pool
_cloud_pool     db         64 dup (?)       ; b3d4  extern struct Pool cloud_pool;
                public  _picker_direct_select
_picker_direct_select db          1 dup (?)       ; b414  extern char picker_direct_select;
                public  _drops
_drops          db          4 dup (?)       ; b415  extern struct Drop far *drops;
                public  _encounter_sprite_shown
_encounter_sprite_shown db          1 dup (?)       ; b419  extern char encounter_sprite_shown;
                public  _ecl_frame_pool
_ecl_frame_pool db         64 dup (?)       ; b41a  extern struct Pool ecl_frame_pool;
                public  _combat_round_limit
_combat_round_limit db          1 dup (?)       ; b45a  extern unsigned char combat_round_limit;
                public  _taunting_character
_taunting_character db          4 dup (?)       ; b45b  extern struct Character far *taunting_character;
                public  _ecl_str
_ecl_str        db        840 dup (?)       ; b45f  extern char ecl_str[][40];
                public  _adventure_exit
_adventure_exit db          1 dup (?)       ; b7a7  extern char adventure_exit;
                public  _big_res
_big_res        db          4 dup (?)       ; b7a8  extern struct Res far *big_res;
                public  _file_kind
_file_kind      db          2 dup (?)       ; b7ac  extern int file_kind;
                public  _cur_item
_cur_item       db          4 dup (?)       ; b7ae  extern struct Item far *cur_item;
                public  _npc_pool
_npc_pool       db         64 dup (?)       ; b7b2  extern struct Pool npc_pool;
                public  _side_count
_side_count     db          2 dup (?)       ; b7f2  extern unsigned char side_count[];
                public  _damage_dice_count
_damage_dice_count db          1 dup (?)       ; b7f4  extern unsigned char damage_dice_count;
                public  _movement_stops_attack_rate
_movement_stops_attack_rate db          1 dup (?)       ; b7f5  extern char movement_stops_attack_rate;
                public  _monster_unpack_in_progress
_monster_unpack_in_progress db          2 dup (?)       ; b7f6  extern char monster_unpack_in_progress;
                public  _script_view_dirty
_script_view_dirty db          1 dup (?)       ; b7f8  extern char script_view_dirty;
                public  _ecl_ip
_ecl_ip         db          2 dup (?)       ; b7f9  extern unsigned ecl_ip;
                public  _no_study
_no_study       db          1 dup (?)       ; b7fb  extern char no_study;
                public  _item_cast_active
_item_cast_active db          1 dup (?)       ; b7fc  extern char item_cast_active;
                public  _occupant_grid
_occupant_grid  db       1250 dup (?)       ; b7fd  extern char occupant_grid[25][50];
                public  _hits_left
_hits_left      db          2 dup (?)       ; bcdf  extern char hits_left[];
                public  _heal_ticks
_heal_ticks     db          2 dup (?)       ; bce1  extern unsigned heal_ticks;
                public  _fx_target
_fx_target      db          4 dup (?)       ; bce3  extern struct Character far *fx_target;
                public  _year_rolled
_year_rolled    db          1 dup (?)       ; bce7  extern char year_rolled;
                public  _near_ids
_near_ids       db         52 dup (?)       ; bce8  extern unsigned char near_ids[];
                public  _can_reach
_can_reach      db          1 dup (?)       ; bd1c  extern char can_reach;
                public  _byte_bd1d
_byte_bd1d      db          1 dup (?)       ; bd1d  extern char byte_bd1d;
                public  _word_bd1e
_word_bd1e      db          6 dup (?)       ; bd1e  extern int word_bd1e;
                public  _back_res
_back_res       db          8 dup (?)       ; bd24  extern struct Res far *back_res;
                public  _view_cx
_view_cx        db          1 dup (?)       ; bd2c  extern char view_cx;
                public  _order_idx
_order_idx      db          1 dup (?)       ; bd2d  extern unsigned char order_idx;
                public  _combat_action_done
_combat_action_done db          1 dup (?)       ; bd2e  extern char combat_action_done;
                public  _saved_search_flag
_saved_search_flag db          1 dup (?)       ; bd2f  extern char saved_search_flag;
                public  _frame_col
_frame_col      db          2 dup (?)       ; bd30  extern int frame_col;
                public  _frame_row
_frame_row      db          2 dup (?)       ; bd32  extern int frame_row;
                public  _pic_name
_pic_name       db         21 dup (?)       ; bd34  extern char pic_name[];
                public  _view_cy
_view_cy        db          2 dup (?)       ; bd49  extern char view_cy;
                public  _ecl_initial_redraw
_ecl_initial_redraw db          1 dup (?)       ; bd4b  extern char ecl_initial_redraw;
                public  _death_explosion_count
_death_explosion_count db          1 dup (?)       ; bd4c  extern unsigned char death_explosion_count;
                public  _pictures_enabled
_pictures_enabled db          1 dup (?)       ; bd4d  extern char pictures_enabled;
                public  _scripted_monsters_queued
_scripted_monsters_queued db          2 dup (?)       ; bd4e  extern char scripted_monsters_queued;
                public  _party_click_enabled
_party_click_enabled db          1 dup (?)       ; bd50  extern char party_click_enabled;
                public  _effect_pool
_effect_pool    db         64 dup (?)       ; bd51  extern struct Pool effect_pool;
                public  _item_handler_active
_item_handler_active db          1 dup (?)       ; bd91  extern char item_handler_active;
                public  _free_train
_free_train     db          1 dup (?)       ; bd92  extern char free_train;
                public  _skip_startup_party_menu
_skip_startup_party_menu db          1 dup (?)       ; bd93  extern char skip_startup_party_menu;
                public  _long_bd94
_long_bd94      db          5 dup (?)       ; bd94  extern long long_bd94;
                public  _combat_order
_combat_order   db        292 dup (?)       ; bd99  extern struct Character far *combat_order[0x49];
                public  _element_mask
_element_mask   db          2 dup (?)       ; bebd  extern int element_mask;
                public  _copyprot_markers
_copyprot_markers db          6 dup (?)       ; bebf  extern char copyprot_markers[];
                public  _attack_roll
_attack_roll    db          1 dup (?)       ; bec5  extern char attack_roll;
                public  _pic_saved
_pic_saved      db         21 dup (?)       ; bec6  extern char pic_saved[];
                public  _cur_map
_cur_map        db          5 dup (?)       ; bedb  extern struct Map far *cur_map;
                public  _script_box_clear_marker
_script_box_clear_marker db          8 dup (?)       ; bee0  extern char script_box_clear_marker;
                public  _encounter_sprite
_encounter_sprite db          1 dup (?)       ; bee8  extern unsigned char encounter_sprite;
                public  _shop_temple_frame
_shop_temple_frame db          1 dup (?)       ; bee9  extern char shop_temple_frame;
                public  _noncombat_redraw_pending
_noncombat_redraw_pending db          1 dup (?)       ; beea  extern char noncombat_redraw_pending;
                public  _disk_number
_disk_number    db          1 dup (?)       ; beeb  extern char disk_number;
                public  _mirror_image_bypass
_mirror_image_bypass db          1 dup (?)       ; c280  extern char mirror_image_bypass;
                public  _bonus_slots
_bonus_slots    db          1 dup (?)       ; c281  extern unsigned char bonus_slots;
                public  _death_explosion_queue
_death_explosion_queue db         84 dup (?)       ; c282  extern struct Character far *death_explosion_queue[];
                public  _cloud_list
_cloud_list     db          4 dup (?)       ; c2d6  extern struct Cloud far *cloud_list;
                public  _previous_nonrose_knight
_previous_nonrose_knight db          1 dup (?)       ; c2da  extern char previous_nonrose_knight;
                public  _character_select_failed
_character_select_failed db          1 dup (?)       ; c2db  extern char character_select_failed;
                public  _adventure_restart
_adventure_restart db          1 dup (?)       ; c2dc  extern char adventure_restart;
                public  _step_count
_step_count     db          1 dup (?)       ; c2dd  extern unsigned char step_count;
                public  _morale
_morale         db          3 dup (?)       ; c2de  extern unsigned char morale;
                public  _encounter_menu_active
_encounter_menu_active db          1 dup (?)       ; c2e1  extern char encounter_menu_active;
                public  _treasure_has_items
_treasure_has_items db          1 dup (?)       ; c2e2  extern char treasure_has_items;
                public  _dropped_item_boundary
_dropped_item_boundary db          4 dup (?)       ; c2e3  extern struct Item far *dropped_item_boundary;
                public  _encounter_mode
_encounter_mode db          1 dup (?)       ; c2e7  extern unsigned char encounter_mode;
                public  _encounter_ticks
_encounter_ticks db          3 dup (?)       ; c2e8  extern unsigned encounter_ticks;
                public  _current_save_type
_current_save_type db          2 dup (?)       ; c2eb  extern int current_save_type;
                public  _ai_level
_ai_level       db          1 dup (?)       ; c2ed  extern unsigned char ai_level;
                public  _pic
_pic            db          1 dup (?)       ; c2ee  extern struct Pic pic;
                public  _anim_speed
_anim_speed     db          5 dup (?)       ; c2ef  extern unsigned char anim_speed;
                public  _pick_special
_pick_special   db          1 dup (?)       ; c2f4  extern char pick_special;
                public  _menu_escaped
_menu_escaped   db          1 dup (?)       ; c2f5  extern char menu_escaped;
                public  _steps
_steps          db        156 dup (?)       ; c2f6  extern struct Step steps[];
                public  _pic_disk
_pic_disk       db          1 dup (?)       ; c392  extern unsigned char pic_disk;
                public  _wall_set_res
_wall_set_res   db         16 dup (?)       ; c393  extern struct Res far *wall_set_res[3];
                public  _inventory_redraw
_inventory_redraw db          1 dup (?)       ; c3a3  extern char inventory_redraw;
                public  _flag_c3a4
_flag_c3a4      db          1 dup (?)       ; c3a4  extern char flag_c3a4;
                public  _picker_start_character
_picker_start_character db          4 dup (?)       ; c3a5  extern struct Character far *picker_start_character;
                public  _item_record_pool
_item_record_pool db         64 dup (?)       ; c3a9  extern struct Pool item_record_pool;
                public  _has_coins
_has_coins      db          1 dup (?)       ; c3e9  extern void far pool_check(char far *has_coins, char far *has_items);
                public  _vault_slot
_vault_slot     db          1 dup (?)       ; c3ea  extern char vault_slot;
                public  _selected_target
_selected_target db          4 dup (?)       ; c3eb  extern struct Character far *selected_target;
                public  _combatant_count
_combatant_count db          1 dup (?)       ; c3ef  extern unsigned char combatant_count;
                public  _aim_cursor_active
_aim_cursor_active db          1 dup (?)       ; c3f0  extern char aim_cursor_active;
                public  _move_menu_active
_move_menu_active db          3 dup (?)       ; c3f1  extern char move_menu_active;
                public  _anim_off
_anim_off       db          1 dup (?)       ; c3f4  extern char anim_off;
                public  _move_flag
_move_flag      db          1 dup (?)       ; c3f5  extern char move_flag;
                public  _wall_color_select
_wall_color_select db          1 dup (?)       ; c3f6  extern char wall_color_select;
                public  _combatants
_combatants     db        312 dup (?)       ; c3f7  extern struct Combatant combatants[52];
                public  _recs
_recs           db         73 dup (?)       ; c52f  extern void far mon_load(unsigned char id, char recs, struct Character far *hero);
                public  _node_pool
_node_pool      db          4 dup (?)       ; c578  extern struct Pool far *node_pool;
                public  _target_scores
_target_scores  db         17 dup (?)       ; c57c  extern int (far *target_scores)[37];
                public  _combat_pool
_combat_pool    db         65 dup (?)       ; c58d  extern struct Pool combat_pool;
                public  _menu_timeout
_menu_timeout   db          4 dup (?)       ; c5ce  extern long menu_timeout;
                public  _timeout_key
_timeout_key    db          1 dup (?)       ; c5d2  extern char timeout_key;
                public  _rec_count
_rec_count      db          1 dup (?)       ; c5d3  extern unsigned char rec_count;
                public  _pic_res
_pic_res        db          4 dup (?)       ; c5d4  extern struct Res far *pic_res;
                public  _print_return_flag
_print_return_flag db          1 dup (?)       ; c5d8  extern char print_return_flag;
                public  _view_update_pending
_view_update_pending db          1 dup (?)       ; c5d9  extern char view_update_pending;
                public  _study_wait
_study_wait     db          5 dup (?)       ; c5da  extern unsigned char study_wait[];
                public  _targets
_targets        db          4 dup (?)       ; c5df  extern struct Character far *targets[];
                public  _spell_target
_spell_target   db        212 dup (?)       ; c5e3  extern struct Character far *spell_target;
                public  _panel_dirty
_panel_dirty    db          1 dup (?)       ; c6b7  extern char panel_dirty;
                public  _morale_base
_morale_base    db          1 dup (?)       ; c6b8  extern unsigned char morale_base;
                public  _crit_hit
_crit_hit       db          1 dup (?)       ; c6b9  extern char crit_hit;
                public  _arr_c6ba
_arr_c6ba       db         36 dup (?)       ; c6ba  extern char arr_c6ba[];
                public  _item_templates
_item_templates db          4 dup (?)       ; c6de  extern struct ItemRec far *item_templates;
                public  _order_mode
_order_mode     db          1 dup (?)       ; c6e2  extern char order_mode;
                public  _horizon_color_select
_horizon_color_select db          1 dup (?)       ; c6e3  extern char horizon_color_select;
                public  _ui_mode
_ui_mode        db          1 dup (?)       ; c6e4  extern char ui_mode;
                public  _node_list
_node_list      db          4 dup (?)       ; c6e5  extern struct Character far *node_list;
                public  _previous_highlighted_character
_previous_highlighted_character db          4 dup (?)       ; c6e9  extern struct Character far *previous_highlighted_character;
                public  _hit_proc
_hit_proc       db          4 dup (?)       ; c6ed  extern void (far *hit_proc)(unsigned char spell, char mode, char far *result);
                public  _encounter_npc_count
_encounter_npc_count db          1 dup (?)       ; c6f1  extern unsigned char encounter_npc_count;
                public  _ice_explosion_active
_ice_explosion_active db          1 dup (?)       ; c6f2  extern char ice_explosion_active;
                public  _meteor_explosion_active
_meteor_explosion_active db          1 dup (?)       ; c6f3  extern char meteor_explosion_active;
                public  _target_count
_target_count   db          1 dup (?)       ; c6f4  extern unsigned char target_count;
                public  _training_selection_changed
_training_selection_changed db          1 dup (?)       ; c6f5  extern char training_selection_changed;
                public  _encounter_images_shown
_encounter_images_shown db          2 dup (?)       ; c6f6  extern char encounter_images_shown[];
_BSS            ends
                end
