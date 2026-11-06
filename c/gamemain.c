/* DQK.EXE overlay 03 module 1fe9: the title menu and the game's main.
 * The two insert-disk handlers game_main installs with disk_setup, one on
 * the status line and one in the text window; drawing the title and
 * version lines; and game_main, which start_main reaches through the
 * vector stub 1d99:114d with argc and argv.  game_main starts the system,
 * makes argv the message table, and loops: it shows the Play Demo Quit
 * menu when demo_active is set (it sets it before the first pass, and a
 * Demo choice sets it again), so after Play the next pass plays again
 * without the menu; Quit shuts down and exits to DOS.  At start-up it
 * calls 1d99:1157 unless message 2 (argv[2] when given) is "Woof".  On a
 * pass with demo_active and skip_startup_party_menu both 0, unless argv[3] equals
 * debug_password, it calls game_exit when 1d99:117f returns 0 or copyprot_markers[1..3]
 * holds a 0; demo_active is tested again after 1d99:1175.
 *
 * EXE 0x031886..0x031d24, one Turbo C large-model _TEXT segment starting at
 * byte 6 of paragraph 1fe9 in RTLink section 3 (payload 0x30ed0, link
 * segment 1f4e).  The first 6 bytes of paragraph 1fe9 are the tail of
 * module 1fc4, and the last 4 bytes of the range lie in paragraph 2033,
 * whose module starts at 2033:0004 (far-called from 0x031ba7).  Built with
 * Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped order;
 * bake consumes them in that order.  Section 0's 1f4e link segment holds
 * other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.  game_main passes
 * disk_msg_text and disk_msg_status to disk_setup as handlers, and
 * near-calls title_draw.
 *
 * _DATA is two literals, DGROUP 5eca..5ed2.  This module has no _BSS.
 */

#include "dqk.h"

void far disk_msg_status(char kind, char disk, char far *text)   /* 1fe9:0006 */
{
    char buf[40];

    box_draw();
    if (kind) {
        if (disk == 'A')
            disk = '1';
        else if (disk == 'B')
            disk = '2';
        else if (disk == 'C')
            disk = '3';
        str_format(buf, text, disk);
        status_printf(0, 0x18, 0x0f, 8, buf);
    }
    screen_flush();
}

void far disk_msg_text(char kind, char disk, char far *text)     /* 1fe9:0073 */
{
    if (disk == 'A')
        disk = '1';
    else if (disk == 'B')
        disk = '2';
    else if (disk == 'C')
        disk = '3';
    prompt_attr_set(0x0f);
    win_text_printf(text, disk);
}

void far title_draw(void)                                        /* 1fe9:00b6 */
{
    status_printf(8, 5, 7, 0, title_text);
    status_printf(4, 6, 7, 0, version_text);
}

void far game_main(int argc, char far *far *argv)                /* 1fe9:00ef */
{
    char choice;

    game_setup();
    sys_init(argc, argv, 0x90, 500);
    if (pointer_mode_set(-1) == 0)
        pointer_hide();
    msg_set_table(argc, argv);
    disk_setup(disk_msg_text, 1, "Pod", 'S');
    game_init();
    disk_number = 0;
    prompt_window_push(0x1fa0, 0);
    if (str_cmp(msg_text(2), "Woof"))
        title_show();
    frame_select(0);
    prompt_window_pop();
    pal_select(0);
    disk_setup(disk_msg_status, 1, "Pod", 'S');
    post_reset();
    menu_timeout = 7000;
    timeout_key = 1;
    hotkey_reset(2);
    pointer_show();
    frame_border();
    title_draw();
    choice = menu_run(empty_text, main_menu_text, 0, 0);
    menu_timeout = 0;
    timeout_key = 1;
    skip_startup_party_menu = 0;
    if (choice == 1)
        demo_active = 1;
    while (choice != 2) {
        if (demo_active)
            game_state[STATE_SPEED] = 5;
        else
            disk_number = 1;
        if (!demo_active && !skip_startup_party_menu) {
            sfx_play(0xff);
            party_menu(1);
            if (str_cmp(msg_text(3), debug_password) && !demo_active) {
                if (!journal_check())
                    game_exit();
                for (tick_slot = 1; tick_slot < 4; tick_slot++)
                    if (!copyprot_markers[tick_slot])
                        game_exit();
            }
            if (view_mode == 3 && game_state[STATE_WILDERNESS] == 1)
                frame_lower_panel();
            else {
                frame_main();
                party_display(cur_node);
                party_info_display();
            }
        }
        view_attrs(geo.attr_1c, geo.attr_1d, geo.attr_5, geo.attr_6);
        game_loop();
        if (pending_load_get())
            do {
                view_mode = saved_view_mode;
                pending_load_set(0);
                sfx_play(0xff);
                party_menu(1);
                if (view_mode == 3 && game_state[STATE_WILDERNESS] == 1)
                    frame_lower_panel();
                else {
                    frame_main();
                    party_display(cur_node);
                    party_info_display();
                }
                view_attrs(geo.attr_1c, geo.attr_1d, geo.attr_5, geo.attr_6);
                game_loop();
            } while (pending_load_get());
        game_reset();
        if (demo_active) {
            if (!esc_get()) {
                prompt_window_push(0x1fa0, 0);
                disk_setup(disk_msg_text, 1, "Pod", 'S');
                title_show();
                frame_select(0);
                prompt_window_pop();
                disk_setup(disk_msg_status, 1, "Pod", 'S');
            } else
                frame_select(0);
            input_pending_clear();
            menu_timeout = 1200;
            timeout_key = 1;
            frame_border();
            title_draw();
            hotkey_reset(2);
            choice = menu_run(empty_text, main_menu_text, 0, 0);
            menu_timeout = 0;
            timeout_key = 0x1b;
            demo_active = choice == 1;
        }
    }
    dl_done();
    game_sound_stop();
    sys_shutdown();
    dos_call(0x4c00);
}
