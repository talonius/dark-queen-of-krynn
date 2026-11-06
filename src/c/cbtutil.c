/* DQK.EXE overlay 00 module 20cb: combat services around the icons.
 * The four facings of a moving icon, flying an icon along a line over the
 * combat map, flashing a hit or spell on a combatant, the two sides'
 * counts, the targets in range and the distance to one, ending a turn and
 * guarding, redrawing the combat view, the ranged-weapon tests, bandaging
 * the dying, scattering an icon around the view's centre, and a missile's
 * facing for its direction.
 *
 * EXE 0x023bd5..0x024d63, one Turbo C large-model _TEXT segment starting at
 * byte 5 of paragraph 20cb, in RTLink section 0 (payload 0x22400, link
 * segment 1f4e; a root section, the parent of 1 and 2).  The first five
 * bytes of paragraph 20cb are the tail of module 1f4e; the lone retf at
 * 21e4:0003 after the last routine is nothing's target and is left to
 * module 21e4.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are
 * defined in shipped order; bake consumes them in that order.  Section 3's
 * 1f4e link segment holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.  The ovl00_20cb_
 * names defined here are the placeholders the landed callers already use.
 *
 * _DATA is sine and two literals, DGROUP 5976..5a3e.  The module has no
 * _BSS.
 *
 * heroes, near_ids and steps are indexed from 1; the shipped code adds the
 * address of element 0 (b2f8, bce7, c2f3).
 */

#include <stdlib.h>
#include "dqk.h"

static char sine[180] = {       /* sin of 2-degree steps, times 100       */
       0,    3,    7,   10,   14,   17,   21,   24,   28,   31,
      34,   37,   41,   44,   47,   50,   53,   56,   59,   62,
      64,   67,   69,   72,   74,   77,   79,   81,   83,   85,
      87,   88,   90,   91,   93,   94,   95,   96,   97,   98,
      98,   99,   99,  100,  100,  100,  100,  100,   99,   99,
      98,   98,   97,   96,   95,   94,   93,   91,   90,   88,
      87,   85,   83,   81,   79,   77,   74,   72,   69,   67,
      64,   62,   59,   56,   53,   50,   47,   44,   41,   37,
      34,   31,   28,   24,   21,   17,   14,   10,    7,    3,
       0,   -3,   -7,  -10,  -14,  -17,  -21,  -24,  -28,  -31,
     -34,  -37,  -41,  -44,  -47,  -50,  -53,  -56,  -59,  -62,
     -64,  -67,  -69,  -72,  -74,  -77,  -79,  -81,  -83,  -85,
     -87,  -88,  -90,  -91,  -93,  -94,  -95,  -96,  -97,  -98,
     -98,  -99,  -99, -100, -100, -100, -100, -100,  -99,  -99,
     -98,  -98,  -97,  -96,  -95,  -94,  -93,  -91,  -90,  -88,
     -87,  -85,  -83,  -81,  -79,  -77,  -74,  -72,  -69,  -67,
     -64,  -62,  -59,  -56,  -53,  -50,  -47,  -44,  -41,  -37,
     -34,  -31,  -28,  -24,  -21,  -17,  -14,  -10,   -7,   -3
};

void far icon_facing(unsigned char cell, unsigned char half, unsigned char row,
                     char mirror)                                /* 20cb:0005 */
{
    res_copy(icon_res, half * 0x26 + cell, 0, icon_res, row + 0x99);
    if (mirror)
        res_glyph_mirror(icon_res, row + 0x99);
}

void far icon_facings(unsigned char cell)                        /* 20cb:0053 */
{
    icon_facing(cell, 0, 0, 0);
    icon_facing(cell, 0, 1, 1);
    icon_facing(cell, 1, 2, 1);
    icon_facing(cell, 1, 3, 0);
}

int far view_row(int y)                                          /* 20cb:00a0 */
{
    return y * 4 + 0x1f44;
}

int far view_col(int x)                                          /* 20cb:00af */
{
    return x * 4 + 0x1f44;
}

void far tile_blit(int x, int y, unsigned char glyph, char mode,
                   struct Res far *resource)                            /* 20cb:00be */
{
    glyph_cell(x, y, glyph, mode, resource);
}

void far icon_fly(int x0, int y0, int x1, int y1, unsigned char frames,
                  unsigned char pace)                            /* 20cb:00df */
{
    unsigned char path[0x94];
    struct Line walk;
    unsigned char path_idx;
    unsigned char path_count;
    unsigned char frame_idx;
    char last_pass;
    char done;
    int cam_x;
    int cam_y;
    int acc_x;
    int acc_y;
    int diff_x;
    int diff_y;
    int step_x;
    int step_y;
    int over_x;
    int over_y;
    register int px;
    register int py;

    if (x0 < 0)
        x0 = 0;
    else if (x0 > 0x31)
        x0 = 0x31;
    if (y0 < 0)
        y0 = 0;
    else if (y0 > 0x18)
        y0 = 0x18;
    if (x1 < 0)
        x1 = 0;
    else if (x1 > 0x31)
        x1 = 0x31;
    if (y1 < 0)
        y1 = 0;
    else if (y1 > 0x18)
        y1 = 0x18;
    mem_fill(path, sizeof path, 8);
    mem_fill(&walk, sizeof walk, 0);
    path_idx = 0;
    frame_idx = 0;
    walk.x0 = x0 * 3;
    walk.y0 = y0 * 3;
    walk.x1 = x1 * 3;
    walk.y1 = y1 * 3;
    line_init(&walk);
    do {
        done = !line_step(&walk);
        path[path_idx] = walk.dir;
        path_idx++;
    } while (!done);
    if (path_idx < 4)
        return;
    path_count = path_idx - 2;
    diff_x = x1 - x0;
    diff_y = y1 - y0;
    if (!in_combat_view(x0 - cur_map->ox, y0 - cur_map->oy)
        || !in_combat_view(x1 - cur_map->ox, y1 - cur_map->oy)) {
        if (abs(diff_x) <= 6 && abs(diff_y) <= 6) {
            last_pass = 1;
            cam_x = x0 + diff_x / 2;
            cam_y = y0 + diff_y / 2;
        } else {
            last_pass = 0;
            cam_x = cur_map->ox + 3;
            cam_y = cur_map->oy + 3;
        }
    } else {
        last_pass = 1;
        cam_x = cur_map->ox + 3;
        cam_y = cur_map->oy + 3;
    }
    combat_view_scroll(cam_x, cam_y, 0xff, 8);
    path_idx = 0;
    acc_x = 0;
    acc_y = 0;
    do {
        px = (x0 - cur_map->ox) * 3 + acc_x;
        py = (y0 - cur_map->oy) * 3 + acc_y;
        done = 0;
        do {
            step_x = dir_dx[path[path_idx]];
            step_y = dir_dy[path[path_idx]];
            px += step_x;
            py += step_y;
            if (pace > 0 || px % 3 == 0 || py % 3 == 0) {
                screen_flush_mode_set(2);
                rows_save(view_row(py), view_col(px), 12, 12);
                tile_blit(px, py, frame_idx + 0x99, 5, icon_res);
                delay(pace * 2);
                rows_restore(view_row(py), view_col(px), 12, 12);
                frame_idx++;
                if (frame_idx >= frames)
                    frame_idx = 0;
                screen_flush_mode_set(0);
            }
            path_idx++;
            if (px < 0 || px > 0x12 || py < 0 || py > 0x12)
                done = 1;
            if (!done && path_idx < path_count) {
                acc_x += step_x;
                acc_y += step_y;
                if (abs(acc_x) == 3) {
                    x0 += int_sign(acc_x);
                    cam_x += int_sign(acc_x);
                    acc_x = 0;
                }
                if (abs(acc_y) == 3) {
                    y0 += int_sign(acc_y);
                    cam_y += int_sign(acc_y);
                    acc_y = 0;
                }
            }
        } while (path_idx < path_count && !done);
        if (path_idx < path_count) {
            over_x = 0;
            over_y = 0;
            acc_y = 0;
            acc_x = 0;
            x0 = x1;
            y0 = y1;
            if (x1 + 3 > 0x31)
                over_x = x1 - 0x31;
            else if (x1 < 3)
                over_x = 3 - x1;
            if (y1 + 3 > 0x18)
                over_y = y1 - 0x18;
            else if (y1 < 3)
                over_y = 3 - y1;
            cam_x = x1 + over_x;
            cam_y = y1 + over_y;
            combat_view_scroll(cam_x, cam_y, 0xff, 8);
            px = (x1 - cur_map->ox) * 3;
            py = (y1 - cur_map->oy) * 3;
            path_idx = path_count;
            done = 0;
            do {
                step_x = -dir_dx[path[path_idx]];
                step_y = -dir_dy[path[path_idx]];
                px += step_x;
                py += step_y;
                if (px > 0x12)
                    x0 = cur_map->ox + 6;
                else if (px < 0)
                    x0 = cur_map->ox;
                if (py > 0x12)
                    y0 = cur_map->oy + 6;
                else if (py < 0)
                    y0 = cur_map->oy;
                if (px < 0 || px > 0x12 || py < 0 || py > 0x12)
                    done = 1;
                if (!done) {
                    acc_x += step_x;
                    acc_y += step_y;
                    if (abs(acc_x) == 3) {
                        x0 += int_sign(acc_x);
                        cam_x += int_sign(acc_x);
                        acc_x = 0;
                    }
                    if (abs(acc_y) == 3) {
                        y0 += int_sign(acc_y);
                        cam_y += int_sign(acc_y);
                        acc_y = 0;
                    }
                    path_idx--;
                }
            } while (!done);
        } else {
            last_pass = 1;
            if (!in_combat_view(x1 - cur_map->ox, y1 - cur_map->oy))
                combat_view_scroll(x1, y1, 3, 8);
            px = (x1 - cur_map->ox) * 3;
            py = (y1 - cur_map->oy) * 3;
            rows_save(view_row(py), view_col(px), 12, 12);
            tile_blit(px, py, frame_idx + 0x99, 5, icon_res);
            if (pace > 0) {
                screen_flush();
                delay(pace * 2);
                rows_restore(view_row(py), view_col(px), 12, 12);
            }
        }
    } while (!last_pass);
    screen_flush();
}

void far hit_flash(struct Character far *character, char effect, char far *msg) /* 20cb:0731 */
{
    char gx;
    char gy;
    unsigned char cell;
    unsigned char frame;
    unsigned char rep;
    unsigned char reps;

    if (view_mode == 5) {
        if (effect)
            cell = 0x17;
        else
            cell = 0x19;
        icon_facings(cell);
        if (!combatant_visible(character, 1))
            combat_view_scroll(combatant_x(character), combatant_y(character), 3, 8);
        if (effect)
            sfx_play(5);
        else
            sfx_play(4);
        hero_text(character, msg, 10, 0);
        if (effect)
            reps = game_state[STATE_SPEED];
        else
            reps = 0;
        gx = grid_x[combatant_slot(character)] * 3;
        gy = grid_y[combatant_slot(character)] * 3;
        for (rep = 0; rep <= reps; rep++)
            for (frame = 0; frame <= 3; frame++) {
                screen_flush_mode_set(2);
                rows_save(view_row(gy), view_col(gx), 12, 12);
                tile_blit(gx, gy, frame + 0x99, 5, icon_res);
                delay(70);
                rows_restore(view_row(gy), view_col(gx), 12, 12);
                screen_flush_mode_set(0);
            }
        screen_flush();
        if (reps == 0)
            text_pause();
    } else
        hero_text(character, msg, 10, 1);
}

int far enemy_team(struct Character far *character)                           /* 20cb:08fd */
{
    short side;

    if (character->side == 0)
        side = 1;
    else
        side = 0;
    return side;
}

void far combat_sides_count(void)                                   /* 20cb:0920 */
{
    struct Character far *character;

    side_count[0] = 0;
    side_count[1] = 0;
    for (character = node_list; character != 0; character = character->next)
        if (character->enabled != 0 && character->status != 9)
            side_count[character->side]++;
    if (encounter_mode == 0x20 && side_count[1] != 0)
        side_count[1]--;
}

unsigned char far combat_targets_near(struct Character far *character, unsigned char range,
                                  char same)                     /* 20cb:0980 */
{
    unsigned char target_count_result;
    long effect;
    char found;
    short side;
    unsigned char count;
    unsigned char matching;
    unsigned char step_index;

    plan_steps(combatant_x(character), combatant_y(character), range, 0xff,
                 combatant_size(character));
    count = step_count;
    matching = 0;
    if (same)
        side = character->side;
    else
        side = enemy_team(character);
    if (count > 0) {
        for (step_index = 1; step_index <= count; step_index++)
            if ((heroes - 1)[(steps - 1)[step_index].who]->side == side) {
                matching++;
                mem_move5(&(steps - 1)[step_index], &(steps - 1)[matching], 3);
            }
        count = matching;
        step_count = matching;
    }
    target_count_result = count;
    for (step_index = 1; step_index <= count; step_index++)
        (near_ids - 1)[step_index] = (steps - 1)[step_index].who;
    if (find_effect(character, 0x8c, &effect)) {
        step_index = 1;
        found = 0;
        while (step_index <= count && !found) {
            if ((heroes - 1)[(near_ids - 1)[step_index]] == taunting_character) {
                matching = (near_ids - 1)[1];
                (near_ids - 1)[1] = (near_ids - 1)[step_index];
                (near_ids - 1)[step_index] = matching;
                found = 1;
            }
            step_index++;
        }
    }
    return target_count_result;
}

unsigned char far get_distance(struct Character far *character, struct Character far *character_cursor) /* 20cb:0b26 */
{
    unsigned char dist;
    struct Step saved[255];
    unsigned char step_index;

    mem_move5(steps - 1, saved, 4); /* 4 of the list's 765 bytes, as shipped */
    cur_map->dist_mode = 1;
    plan_steps(combatant_x(character), combatant_y(character), 0xff, 0xff,
                 combatant_size(character));
    cur_map->dist_mode = 0;
    step_index = 1;
    while ((heroes - 1)[(steps - 1)[step_index].who] != character_cursor && step_index < step_count)
        step_index++;
    dist = (steps - 1)[step_index].cost / 2;
    mem_move5(saved, steps - 1, 4);
    return dist;
}

char far combat_actions_end(struct Character far *character)                     /* 20cb:0c11 */
{
    char result;

    character->combat->initiative = 0;
    character->combat->spell_cast = 0;
    character->combat->guarding = 0;
    character->combat->movement = 0;
    result = 1;
    return result;
}

char far combat_guard(struct Character far *character)                     /* 20cb:0c52 */
{
    char result;

    result = combat_actions_end(character);
    if (character->combat->panicked == 0) {
        character->combat->guarding = 1;
        bottom_msg("Guarding");
    }
    return result;
}

void far combat_view_show(void)                                   /* 20cb:0c94 */
{
    frame_right_panel();
    combat_palette_set();
    combat_view_scroll(cur_map->ox + 3, cur_map->oy + 3, 0xff, 8);
}

char far has_ranged_weapon(struct Character far *character)                     /* 20cb:0cbf */
{
    char result;

    if (character->equipped[0] != 0 && item_table[character->equipped[0]->rec.type].range > 1)
        result = 1;
    else
        result = 0;
    return result;
}

char far has_throwable_weapon(struct Character far *character)                     /* 20cb:0cfe */
{
    char result;

    result = has_ranged_weapon(character)
        && (char)((item_table[character->equipped[0]->rec.type].flags & 0x14) == 0x14)
        || character->equipped[0]->rec.type == 0x43;
    return result;
}

char far bandage_dying(char apply)                             /* 20cb:0d63 */
{
    char result;
    struct Character far *character;
    char found;

    found = 0;
    for (character = node_list; character != 0; character = character->next)
        if (character->combat->not_pc == 0 && character->side == 0 && character->status == 5) {
            found = 1;
            if (apply) {
                character->status = 4;
                character->combat->dying_turns = 0;
                hero_text(character, "is bandaged", 10, 1);
                apply = 0;
            }
        }
    result = found;
    return result;
}

void far icon_scatter(unsigned char radius, unsigned char cell,
                      unsigned char sound)                       /* 20cb:0def */
{
    unsigned char pick;
    unsigned char max_ring;
    unsigned char ring;
    int point;
    char saved_x;
    char saved_y;
    char draw_x;
    char draw_y;
    char off_y;
    char off_x;
    register int dist;
    register int angle;

    combat_view_scroll(view_cx, view_cy, 0xff, 8);
    saved_x = view_cx;
    saved_y = view_cy;
    sfx_play(sound);
    icon_facing(cell, 0, 0, 0);
    icon_facing(cell, 0, 1, 1);
    icon_facing(cell, 1, 2, 0);
    icon_facing(cell, 1, 3, 1);
    off_x = 0;
    off_y = 0;
    if (saved_x < 3)
        off_x = (saved_x - 3) * 3;
    else if (saved_x > 0x2e)
        off_x = (saved_x - 0x2e) * 3;
    if (saved_y < 3)
        off_y = (saved_y - 3) * 3;
    else if (saved_y > 0x15)
        off_y = (saved_y - 0x15) * 3;
    max_ring = radius * 3 + 1;
    for (ring = 1; ring <= max_ring; ring++) {
        dist = ring;
        for (point = ring * 2 - 2; point >= 0; point--) {
            angle = random_int(180);
            draw_x = (sine[(angle + 45) % 180] * dist + 50) / 100 + off_x;
            draw_y = (sine[angle] * dist + 50) / 100 + off_y;
            if (draw_x >= -9 && draw_x <= 9 && draw_y >= -9 && draw_y <= 9) {
                pick = random_n(4);
                tile_blit(draw_x + 9, draw_y + 9, pick + 0x99, 4, icon_res);
            }
        }
        screen_flush();
    }
    combat_view_scroll(view_cx, view_cy, 0xff, 8);
}

void far missile_facing(unsigned char dir, unsigned char cell)   /* 20cb:0fc8 */
{
    if (pool_lowbit(dir)) {
        if (dir == 3 || dir == 5) {
            if (cell == 0x14)
                icon_facing(cell, 1, 0, dir == 5);
            else
                icon_facing(cell + 1, 1, 0, dir == 5);
        } else if (cell == 0x14)
            icon_facing(cell, 0, 0, dir == 7);
        else
            icon_facing(cell + 1, 0, 0, dir == 7);
    } else if (cell == 0x14)
        icon_facing(cell, dir / 4, 0, 0);
    else
        icon_facing(dir % 4 + cell, dir / 4, 0, 0);
}

char far ranged_ammo_find(struct Character far *character, struct Item far * far *sel) /* 20cb:10c3 */
{
    char result;
    struct Item far *item;
    unsigned char flags;

    result = 0;
    if (game_state[STATE_UNDERWATER])
        return 0;
    item = character->equipped[0];
    *sel = 0;
    flags = 0;
    if (item != 0) {
        flags = item_table[item->rec.type].flags;
        if (flags & 0x10)
            *sel = item;
        if (flags & 8) {
            if (flags & 1)
                *sel = character->equipped[11];
            if (flags & 0x80)
                *sel = character->equipped[12];
        }
    }
    if (*sel != 0 || flags == 10 || flags == 12)
        result = 1;
    return result;
}
