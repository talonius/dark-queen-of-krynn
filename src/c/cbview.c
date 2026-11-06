/* DQK.EXE overlay 00 module 2740: the combat map's view.
 * Each combatant's cells and slot, its cell and icon size, the seven-by-
 * seven window over the 50-by-25 combat map -- scrolling it to keep a cell
 * in view, drawing the terrain, the cursor and the icons in it -- probing
 * the cells a combatant would step into, placing a combatant, a combatant
 * falling, the terrain under the patches and the fallen, and the direction
 * from one cell to another.
 *
 * EXE 0x02a329..0x02bc17, one Turbo C large-model _TEXT segment starting at
 * byte 9 of paragraph 2740, the last module of RTLink section 0 (payload
 * 0x22400, link segment 1f4e; a root section, the parent of 1 and 2).  The
 * first nine bytes of paragraph 2740 are the tail of module 21e4; nine zero
 * bytes follow to the section's end at 0x2bc20.  Built with Turbo C 2.01,
 * tcc -ml -O -Z -d.  Functions are defined in shipped order; bake consumes
 * them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use.  The ovl00_2740_ names defined
 * here are the placeholders the landed callers already use, except
 * cbview_unit_init, which nothing calls.
 *
 * The module has no _DATA and no _BSS; every table it reads is extern.
 *
 * combatants, heroes, recs and tile_info are indexed from 1; the shipped
 * code adds the address of element 0 (c3f1, b2f8, c526, 6197).  grid_x,
 * grid_y and patch_dir are indexed from 1 at their own addresses.
 *
 * Directions run 0 (north) clockwise to 7 (north-west), as dir_dx and
 * dir_dy step them; 8 is no direction.  A view cell is 12 pixels on a side,
 * and 0x1f44 is the coordinate glyph_at and the clip rectangle take for the
 * view's top-left corner; icon_draw takes a view cell and adds it itself.
 */

#include "dqk.h"

void far cell_get(char x, char y, unsigned char far *who, unsigned char far *kind);
void far tile_update(char x, char y, unsigned char who);
char far in_combat_view(char x, char y);
char far combatant_visible(struct Character far *character, char whole);
char far combatant_x(struct Character far *character);
char far combatant_y(struct Character far *character);
unsigned char far combatant_slot(struct Character far *character);
void far view_clip(void);
char far combat_terrain_under(char x, char y);
void far cell_draw(char vx, char vy, char x, char y);

/* The translated Pascal unit's empty initialization block (DKK cbtmap.pas). */
void far cbview_unit_init(void)                                   /* 2740:0009 */
{
}

char far icon_cell_delta(unsigned char size, unsigned char cell_index,
                         char far *dx, char far *dy)             /* 2740:000a */
{
    char ok;

    ok = 0;
    if (size != 0) {
        *dx = icon_cells[size][cell_index].dx;
        *dy = icon_cells[size][cell_index].dy;
        if (*dx >= 0)
            ok = 1;
    }
    return ok;
}

void far grid_update(void)                                       /* 2740:0077 */
{
    unsigned char index;

    for (index = 1; index <= combatant_count; index++) {
        grid_x[index] = (combatants - 1)[index].x - cur_map->ox;
        grid_y[index] = (combatants - 1)[index].y - cur_map->oy;
    }
}

void far combat_palette_set(void)                                   /* 2740:00e4 */
{
    pal_select(pic_res);
}

void far cursor_draw(char x, char y)                             /* 2740:00f4 */
{
    char vx, vy, dx, dy;
    unsigned char index, who, kind;

    vx = x - cur_map->ox;
    vy = y - cur_map->oy;
    view_clip();
    for (index = 0; index <= 5; index++)
        if (icon_cell_delta(cur_map->cursor_size, index, &dx, &dy))
            if (in_combat_view(vx + dx, vy + dy)) {
                cell_draw(vx + dx, vy + dy, x + dx, y + dy);
                if (cur_map->cursor_on)
                    icon_draw(vx + dx, vy + dy, 0, 0, 0x1e);
            }
    cell_get(x, y, &who, &kind);
    if (who > 0)
        if (combatant_visible((heroes - 1)[who], 0))
            icon_draw(grid_x[who], grid_y[who], (heroes - 1)[who]->combat->direction, 0,
                      (heroes - 1)[who]->order_no);
    clip_reset();
}

void far cell_redraw(char x, char y)                             /* 2740:025d */
{
    char vx, vy;
    unsigned char who, kind;

    vx = x - cur_map->ox;
    vy = y - cur_map->oy;
    view_clip();
    tile_update(vx, vy, 0);
    cell_get(x, y, &who, &kind);
    if (who > 0)
        if (combatant_visible((heroes - 1)[who], 0))
            icon_draw(grid_x[who], grid_y[who], (heroes - 1)[who]->combat->direction, 0,
                      (heroes - 1)[who]->order_no);
    clip_reset();
}

void far combat_grid_build(void)                                   /* 2740:0330 */
{
    unsigned char index, inner_index;
    char dx, dy;

    mem_zero(occupant_grid, sizeof occupant_grid);
    for (index = 1; index <= combatant_count; index++) {
        if ((combatants - 1)[index].size > 0) {
            for (inner_index = 0; inner_index <= 5; inner_index++)
                if (icon_cell_delta((combatants - 1)[index].size, inner_index, &dx, &dy))
                    occupant_grid[(combatants - 1)[index].y + dy][(combatants - 1)[index].x + dx] = index;
            grid_x[index] = (combatants - 1)[index].x - cur_map->ox;
            grid_y[index] = (combatants - 1)[index].y - cur_map->oy;
        }
    }
}

void far cell_get(char x, char y, unsigned char far *who,
                  unsigned char far *kind)                       /* 2740:0468 */
{
    if (x >= 0 && x <= 49 && y >= 0 && y <= 24) {
        *kind = cur_map->cells[y][x];
        *who = occupant_grid[y][x];
    } else {
        *who = 0;
        *kind = 0;
    }
}

void far tile_update(char x, char y, unsigned char who)          /* 2740:04d4 */
{
    char mx, my, dx, dy;
    unsigned char index, kind, size;

    if (who == 0) {
        mx = cur_map->ox + x;
        my = cur_map->oy + y;
        cell_get(mx, my, &who, &kind);
    }
    if (who > 0) {
        x = grid_x[who];
        y = grid_y[who];
        mx = cur_map->ox + x;
        my = cur_map->oy + y;
        size = (combatants - 1)[who].size;
        for (index = 0; index <= 5; index++)
            if (icon_cell_delta(size, index, &dx, &dy))
                if (in_combat_view(x + dx, y + dy))
                    cell_draw(x + dx, y + dy, mx + dx, my + dy);
    } else if (in_combat_view(x, y))
        cell_draw(x, y, mx, my);
}

char far in_combat_view(char x, char y)                         /* 2740:05f7 */
{
    char ok;

    if (x < 0 || x > 6 || y < 0 || y > 6)
        ok = 0;
    else
        ok = 1;
    return ok;
}

char far combatant_visible(struct Character far *character, char whole)         /* 2740:0626 */
{
    char seen;
    unsigned char index, slot;
    char dx, dy;

    seen = 1;
    slot = combatant_slot(character);
    if ((combatants - 1)[slot].size == 0)
        seen = 0;
    else
        for (index = 0; index <= 5; index++) {
            if (icon_cell_delta((combatants - 1)[slot].size, index, &dx, &dy)) {
                if (!in_combat_view(grid_x[slot] + dx, grid_y[slot] + dy)) {
                    seen = 0;
                    if (whole)
                        return seen;
                } else {
                    seen = 1;
                    if (!whole)
                        return seen;
                }
            }
        }
    return seen;
}

char far view_build(char x, char y, unsigned char margin)        /* 2740:06e6 */
{
    char moved;
    unsigned char view_margin;
    char index, inner_index, cx, cy, x_lo, x_hi, y_lo, y_hi;

    moved = 0;
    cx = cur_map->ox + 3;
    cy = cur_map->oy + 3;
    view_margin = margin;
    if (margin == 0xff)
        view_margin = 0;
    x_lo = cx - view_margin;
    x_hi = cx + view_margin;
    y_lo = cy - view_margin;
    y_hi = cy + view_margin;
    if (margin == 0xff || x < x_lo || x > x_hi || y < y_lo || y > y_hi) {
        if (x < x_lo)
            while (x < cx && cx > 3)
                cx--;
        else if (x > x_hi)
            while (x > cx && cx < 46)
                cx++;
        if (y < y_lo)
            while (y < cy && cy > 3)
                cy--;
        else if (y > y_hi)
            while (y > cy && cy < 21)
                cy++;
        cur_map->ox = cx - 3;
        cur_map->oy = cy - 3;
        cy = cur_map->oy;
        for (inner_index = 0; inner_index <= 6; inner_index++) {
            cx = cur_map->ox;
            for (index = 0; index <= 6; index++) {
                cell_draw(index, inner_index, cx, cy);
                cx++;
            }
            cy++;
        }
        grid_update();
        moved = 1;
    }
    return moved;
}

void far combat_view_scroll(char x, char y, unsigned char margin,
                         unsigned char dir)                      /* 2740:0846 */
{
    struct Character far *character;
    unsigned char index;
    char tx, ty;

    screen_draw_begin(1);
    tx = x + dir_dx[dir];
    ty = y + dir_dy[dir];
    if (view_build(tx, ty, margin)) {
        view_clip();
        for (index = 1; index <= combatant_count; index++) {
            character = (heroes - 1)[index];
            if (character != 0)
                if (character->enabled != 0 || character->status == 10)
                    if ((combatants - 1)[index].size > 0)
                        if (combatant_visible(character, 0))
                            icon_draw(grid_x[index], grid_y[index], character->combat->direction, 0,
                                      character->order_no);
        }
        clip_reset();
    }
    cell_redraw(x, y);
    if (!in_combat_view(tx - cur_map->ox, ty - cur_map->oy)) {
        if (tx > 49)
            tx = 49;
        if (tx < 0)
            tx = 0;
        if (ty > 24)
            ty = 24;
        if (ty < 0)
            ty = 0;
    }
    cursor_draw(tx, ty);
    screen_flush();
}

void far combatant_facing_set(struct Character far *character, unsigned char dir, char frame,
                         char erase)                             /* 2740:09ab */
{
    unsigned char slot;
    long value;

    slot = combatant_slot(character);
    if (!combatant_visible(character, 1) && move_flag)
        combat_view_scroll(combatant_x(character), combatant_y(character), 3, 8);
    if (character->combat->direction / 4 != dir / 4 || frame != 0 || erase != 0)
        if (move_flag)
            tile_update(0, 0, slot);
    if (find_effect(character, 0xd5, &value)) {
        character->combat->direction = 2;
        dir = 2;
    } else
        character->combat->direction = dir;
    if (erase == 0)
        if (combatant_visible(character, 0))
            if (move_flag) {
                view_clip();
                icon_draw(grid_x[slot], grid_y[slot], dir, frame, character->order_no);
                clip_reset();
                screen_flush();
            }
}

char far combatant_x(struct Character far *character)                         /* 2740:0aea */
{
    char x;

    x = (combatants - 1)[combatant_slot(character)].x;
    return x;
}

char far combatant_y(struct Character far *character)                         /* 2740:0b19 */
{
    char y;

    y = (combatants - 1)[combatant_slot(character)].y;
    return y;
}

unsigned char far combatant_size(struct Character far *character)             /* 2740:0b49 */
{
    unsigned char size;

    size = (combatants - 1)[combatant_slot(character)].size;
    return size;
}

unsigned char far combatant_slot(struct Character far *character)             /* 2740:0b79 */
{
    unsigned char slot;
    char found;
    unsigned char index;

    found = 0;
    slot = 0;
    index = 0;
    do {
        index++;
        if ((heroes - 1)[index] == character)
            found = 1;
    } while (!found && index != combatant_count);
    if (found)
        slot = index;
    return slot;
}

void far combat_direction_probe(struct Character far *character, unsigned char dir, unsigned char far *who,
                         unsigned char far *cell, unsigned char far *kind,
                         char far *special_terrain)                            /* 2740:0bd1 */
{
    unsigned char index, slot, occupant_slot, terrain, best;
    char nx, ny, x, y, dx, dy;

    *special_terrain = 0;
    *who = 0;
    *cell = 23;
    best = 1;
    *kind = 0;
    slot = combatant_slot(character);
    x = (combatants - 1)[slot].x;
    y = (combatants - 1)[slot].y;
    for (index = 0; index <= 5; index++) {
        if (icon_cell_delta((combatants - 1)[slot].size, index, &dx, &dy)) {
            nx = x + dx + dir_dx[dir];
            ny = y + dy + dir_dy[dir];
            cell_get(nx, ny, &occupant_slot, &terrain);
            if (occupant_slot == slot)
                occupant_slot = 0;
            if (occupant_slot > 0)
                *who = occupant_slot;
            if (terrain == 0x1b || terrain == 0x1c || terrain == 0x1d)
                *kind = terrain;
            else if (terrain == 0x46)
                *special_terrain = 1;
            if (terrain == 0 || *cell == 0)
                *cell = 0;
            else if ((tile_info - 1)[terrain].cost >= best) {
                best = (tile_info - 1)[terrain].cost;
                *cell = terrain;
            }
        }
    }
}

void far hero_fall(struct Character far *character)                           /* 2740:0d55 */
{
    unsigned char slot, scan_index, frame;
    char dx, dy, cell_x, cell_y;

    if (view_mode != 5) {
        sfx_play(6);
        text_pause();
        return;
    }
    for (scan_index = 1; scan_index < 9 && (recs - 1)[scan_index].hero != character; scan_index++)
        ;
    if (scan_index < 9)
        return;
    slot = combatant_slot(character);
    cell_x = combatant_x(character);
    cell_y = combatant_y(character);
    if (!combatant_visible(character, 1))
        combat_view_scroll(cell_x, cell_y, 3, 8);
    tile_update(0, 0, slot);
    sfx_play(6);
    view_clip();
    for (frame = 0; frame <= 8; frame++) {
        for (scan_index = 0; scan_index <= 5; scan_index++)
            if (icon_cell_delta((combatants - 1)[slot].size, scan_index, &dx, &dy))
                if (in_combat_view(grid_x[slot] + dx, grid_y[slot] + dy))
                    icon_draw(grid_x[slot] + dx, grid_y[slot] + dy, 3, frame & 1, 0x18);
        screen_flush();
        delay(10);
    }
    clip_reset();
    if (character->combat->not_pc == 0 && character->status != 8) {
        frame = combat_terrain_under(cell_x, cell_y);
        rec_count++;
        (recs - 1)[rec_count].terrain = frame;
        if (cur_map->cells[cell_y][cell_x] != 0x1d && cur_map->cells[cell_y][cell_x] != 0x1c
            && cur_map->cells[cell_y][cell_x] != 0x46) {
            if (character->status == 7)
                cur_map->cells[cell_y][cell_x] = 0x1e;
            else
                cur_map->cells[cell_y][cell_x] = 0x1f;
        }
        (recs - 1)[rec_count].hero = character;
        (recs - 1)[rec_count].x = cell_x;
        (recs - 1)[rec_count].y = cell_y;
    }
    text_pause();
    tile_update(0, 0, slot);
    (combatants - 1)[combatant_slot(character)].size = 0;
    combat_grid_build();
    combat_view_scroll(cur_map->ox + 3, cur_map->oy + 3, 3, 8);
    character->combat->initiative = 0;
    character->combat->movement = 0;
    character->combat->guarding = 0;
}

char far hero_place(struct Character far *character, unsigned char x, unsigned char y,
                    char moving)                                 /* 2740:10bd */
{
    char ok;
    unsigned char slot, index, who, terrain;
    char found;
    unsigned char kind;
    char special_terrain;

    if (view_mode == 5) {
        ok = 0;
        slot = combatant_slot(character);
        (combatants - 1)[slot].size = character->icon_dim & 0x7f;
        (combatants - 1)[slot].x = x;
        (combatants - 1)[slot].y = y;
        combat_direction_probe(character, 8, &who, &terrain, &kind, &special_terrain);
        if (who != 0 || terrain == 0 || (tile_info - 1)[terrain].cost == 0xff)
            (combatants - 1)[slot].size = 0;
        else {
            ok = 1;
            if (moving)
                if (character->combat->not_pc == 0) {
                    for (index = 1; index <= rec_count; index++)
                        if ((recs - 1)[index].hero == character) {
                            if ((recs - 1)[index].terrain != 0x1f)
                                terrain = (recs - 1)[index].terrain;
                            mem_fill(&(recs - 1)[index], sizeof(struct Rec), 0);
                        }
                    found = 0;
                    for (index = 1; index <= rec_count; index++)
                        if ((recs - 1)[index].hero != 0 && (recs - 1)[index].x == x
                            && (recs - 1)[index].y == y)
                            found = 1;
                    if (!found)
                        cur_map->cells[y][x] = terrain;
                }
            combat_grid_build();
        }
    } else
        ok = 1;
    return ok;
}

void far view_center(struct Character far *character, unsigned char margin, char cursor) /* 2740:12f5 */
{
    cur_map->cursor_on = cursor;
    cur_map->cursor_size = (combatants - 1)[combatant_slot(character)].size;
    if (move_flag)
        combat_view_scroll(combatant_x(character), combatant_y(character), margin, 8);
    cur_map->cursor_on = 0;
    cur_map->cursor_size = 1;
}

void far view_clip(void)                                         /* 2740:1367 */
{
    clip_set(0x1f44, 0x1f44, 0x1f98, 0x1f98);
}

char far combat_terrain_under(char x, char y)                         /* 2740:137a */
{
    char under;
    unsigned char terrain, index;
    struct Cloud far *cloud;

    terrain = cur_map->cells[y][x];
    if (terrain == 0x1d || terrain == 0x1c || terrain == 0x1b || terrain == 0x46) {
        for (cloud = cloud_list; cloud != 0; cloud = cloud->next)
            for (index = 1; index <= 9; index++)
                if (cloud->on[index] != 0 && cloud->x + dir_dx[patch_dir[index]] == x
                    && cloud->y + dir_dy[patch_dir[index]] == y)
                    terrain = cloud->tile[index];
    } else if (terrain == 0x1f) {
        for (index = 1; index <= rec_count; index++)
            if ((recs - 1)[index].x == x && (recs - 1)[index].y == y)
                terrain = (recs - 1)[index].terrain;
    }
    under = terrain;
    return under;
}

unsigned char far dir_between(int x1, int y1, int x2, int y2)    /* 2740:14e0 */
{
    unsigned char dir, direction;
    char found;
    int adx, ady;

    adx = char_abs(x2 - x1);
    ady = char_abs(y2 - y1);
    direction = 0;
    found = 0;
    while (!found) {
        switch (direction) {
        case 1:
            found = y2 < y1 && x2 > x1 && adx * 618 / 256 >= ady && adx * 106 / 256 <= ady;
            break;
        case 3:
            found = y2 > y1 && x2 > x1 && adx * 618 / 256 >= ady && adx * 106 / 256 <= ady;
            break;
        case 5:
            found = y2 > y1 && x2 < x1 && adx * 618 / 256 >= ady && adx * 106 / 256 <= ady;
            break;
        case 7:
            found = y2 < y1 && x2 < x1 && adx * 618 / 256 >= ady && adx * 106 / 256 <= ady;
            break;
        case 0:
            found = y2 <= y1 && adx * 618 / 256 <= ady;
            break;
        case 2:
            found = x2 >= x1 && adx * 106 / 256 >= ady;
            break;
        case 4:
            found = y2 >= y1 && adx * 618 / 256 <= ady;
            break;
        case 6:
            found = x2 <= x1 && adx * 106 / 256 >= ady;
            break;
        }
        if (!found)
            direction++;
    }
    dir = direction;
    return dir;
}

unsigned char far combatant_direction(struct Character far *from, struct Character far *target) /* 2740:16dd */
{
    unsigned char dir;

    dir = dir_between(combatant_x(from), combatant_y(from),
                      combatant_x(target), combatant_y(target));
    return dir;
}

void far cell_draw(char vx, char vy, char x, char y)             /* 2740:172c */
{
    unsigned char terrain, base, top, index;
    int row, col;

    col = (vx + vx + vx) * 4 + 0x1f44;  /* add,add,shl,shl; vx * 12 is a mul */
    row = (vy + vy + vy) * 4 + 0x1f44;
    terrain = cur_map->cells[y][x];
    if (terrain > 0x4c || terrain == 0)
        terrain = 0x15;
    base = (tile_info - 1)[terrain].code;
    if (terrain == 0x1d || terrain == 0x1c || terrain == 0x1b || terrain == 0x1f || terrain == 0x1e || terrain == 0x46) {
        terrain = combat_terrain_under(x, y);
        top = (tile_info - 1)[terrain].code;
        switch (terrain) {
        case 0x1e:
        case 0x1f:
            for (index = 1; index <= rec_count; index++)
                if ((recs - 1)[index].x == x && (recs - 1)[index].y == y)
                    terrain = (recs - 1)[index].terrain;
        }
        if (terrain > 0x4c || terrain == 0)
            terrain = 0x15;
        top = (tile_info - 1)[terrain].code;
        glyph_at(col, row, top + 1, 0, pic_res);
        glyph_at(col, row, base + 0xf, 1, icon_res);
    } else
        glyph_at(col, row, base + 1, 0, pic_res);
}
