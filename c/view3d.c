/* DQK.EXE overlay 11 module 2f0d: the first-person view and the overhead map.
 * The geo area (an 8-byte header and four planes of cols * rows cells,
 * read from the GEO library) and the tests on its cells; the wall sets,
 * three slots of tile libraries; the view three rows deep, drawn from the
 * far row in, each wall as a piece placed at pos_x[k], pos_y[k]; the
 * overhead map; and the frame panels with a "back" tile.
 *
 * EXE 0x0608ed..0x061cfe, one Turbo C large-model _TEXT segment starting at
 * byte d of paragraph 2f0d in RTLink section 11 (payload 0x5cbb0, link
 * segment 2b3a, no parent section).  The first d bytes of paragraph 2f0d
 * are the tail of module 2b3a, and the last e bytes of the range lie in
 * paragraph 304e, whose module starts at 304e:000e.  Built with Turbo C
 * 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped order; bake
 * consumes them in that order.  Sections 9's and 10's 2b3a link segments
 * hold other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.  Ten of these
 * routines have RTLink vectors: 1d99:13e1 -> 01cc, 1d99:116b -> 01de,
 * 1d99:10fd -> 04a4, 1d99:10f3 -> 0628, 1d99:1387 -> 075b, 1d99:148b ->
 * 0807, 1d99:1111 -> 085a, 1d99:1143 -> 118a, 1d99:13b9 -> 12e3,
 * 1d99:1139 -> 13ef.
 *
 * _DATA is DGROUP 9114..9198: cur_geo, five tables of ten and five
 * literals.  _BSS is c9f0..ca0d, the one record geo.
 */

#define DQK_DEFINES_VIEW3D
#include "dqk.h"

extern void far huge_move(void far *src, void far *dst, unsigned n);
                                                                 /* 00d7:08c2 */

static int cur_geo = -1;                /* 9114  set loaded into slot 2   */
static unsigned char piece_first[10] =  /* 9116  running sum of cols*rows;
                                           no code found reads these three */
    { 0, 2, 6, 0xa, 0x16, 0x26, 0x36, 0x6e, 0x84, 0x9a };
static unsigned char piece_cols[10] =                            /* 9120 */
    { 1, 1, 1, 3, 2, 2, 7, 2, 2, 1 };
static unsigned char piece_rows[10] =                            /* 912a */
    { 2, 4, 4, 4, 8, 8, 8, 0xb, 0xb, 2 };
static int pos_x[10] = { 5, 4, 6, 4, 2, 7, 2, 0, 9, 5 };         /* 9134 */
static int pos_y[10] = { 4, 3, 3, 3, 1, 1, 1, 0, 0, 4 };         /* 9148 */

struct Geo geo;                                                  /* c9f0 */

unsigned char far wall_side(char x, char y, unsigned char dir);
unsigned char far area_cell_flags(char x, char y);
void far wall_set_load(unsigned char area_id, unsigned char slot);
static void far back_load(char area_id);

void far map_draw(char x, char y, unsigned char dir)             /* 2f0d:000d */
{
    char px;
    char py;
    char inner_index;
    char index;
    char x0;
    char y0;
    char cx;
    char cy;
    short glyph;

    glyph_library_open('2', "TopView", 0xc);
    screen_draw_begin(1);
    x0 = x - 5;
    x0 = int_min(geo.area->cols - 11, x0);
    x0 = int_max(0, x0);
    px = x - x0;
    y0 = y - 5;
    y0 = int_min(geo.area->rows - 11, y0);
    y0 = int_max(0, y0);
    py = y - y0;
    for (index = 0; index <= 10; index++) {
        cy = index + y0;
        for (inner_index = 0; inner_index <= 10; inner_index++) {
            cx = inner_index + x0;
            glyph = 0x100;
            if (wall_side(cx, cy, 0) > 0)
                glyph++;
            if (wall_side(cx, cy, 2) > 0)
                glyph += 2;
            if (wall_side(cx, cy, 4) > 0)
                glyph += 4;
            if (wall_side(cx, cy, 6) > 0)
                glyph += 8;
            draw_glyph((index + 3) * 4 + 0x1f40, (inner_index + 3) * 4 + 0x1f40, 0xc,
                       glyph - 0xff);
        }
    }
    draw_glyph((py + 3) * 4 + 0x1f40, (px + 3) * 4 + 0x1f40, 0xc,
               dir / 2 + 0x11);
    slot_close(0xc);
    screen_flush();
}

void far area_alloc(void)                                        /* 2f0d:01cc */
{
    geo.area = mem_alloc(sizeof(struct Area));
}

void far view_attrs(char backdrop_attribute, char wall_attribute, char view_attribute_5, char view_attribute_6)              /* 2f0d:01de */
{
    geo.attr_1c = backdrop_attribute;
    geo.attr_1d = wall_attribute;
    geo.attr_5 = view_attribute_5;
    geo.attr_6 = view_attribute_6;
}

/* v1.0 has no view_glyph_cell wrapper (v1.1 2f0d:01fb) */

void far view_frame(void)                                        /* 2f0d:021c */
{
    unsigned char col;
    unsigned char row;
    unsigned char tile_flags;

    cell_fill(2, 0x10, 0xb, 0x2c, geo.attr_1c);
    cell_fill(2, 0x3c, 0xb, 2, geo.attr_5);
    cell_fill(2, 0x3e, 0xb, 0x2a, geo.attr_6);
    col = 2;
    row = 2;
    tile_flags = area_cell_flags(geo.x, geo.y);
    if ((geo.flag_4 & 0x80) || tile_flags != 0) {
        if (!(geo.flag_4 & 0x80))
            tile_flags = geo.area->tile_7;
        else
            switch (tile_flags) {
            case 0:
                tile_flags = geo.area->tile_5;
                break;
            case 1:
                tile_flags = geo.area->tile_6;
                break;
            case 2:
                tile_flags = geo.area->tile_7;
                break;
            case 3:
                tile_flags = 0xff;
                break;
            }
    } else
        tile_flags = 0x22;
    if (tile_flags >= 0x20 && tile_flags != 0xff && (game_state[STATE_CLOCK_HOUR] < 6 || game_state[STATE_CLOCK_HOUR] >= 0x14))
        tile_flags++;
    if (tile_flags != 0xff) {
        back_load(tile_flags);
        glyph_cell(col, row, 1, 0, back_res);
    }
}

void far piece_glyph(int col, register int row, unsigned char cell,
                     unsigned char scan_index)                            /* 2f0d:0346 */
{
    register int set;

    set = 0;
    while (cell > 5) {
        cell -= 5;
        set++;
    }
    if (++scan_index == 10) {
        scan_index = 0;
        if (cell > 2)
            cell = 1;
    }
    if (scan_index < 8)
        row += 4;
    cell--;
    if (cell > 1) {
        if (set == 2)
            glyph_draw_at(row, col, db_buffer(), scan_index + 1);
        else
            glyph_put(col, row, scan_index + 1, 0, wall_set_res[set]);
        scan_index = cell * 9 + scan_index + 2;
    } else
        scan_index = cell * 10 + scan_index + 1;
    if (set == 2)
        glyph_draw_at(row, col, db_buffer(), scan_index);
    else
        glyph_put(col, row, scan_index, 0, wall_set_res[set]);
}

void far piece_at(char col, char row, unsigned char cell, unsigned char piece_kind)
                                                                 /* 2f0d:0435 */
{
    piece_glyph(col * 4 + 0x1f4c, row * 4 + 0x1f4c, cell, piece_kind);
}

char far in_area(char x, char y)                                 /* 2f0d:045e */
{
    char result;

    if (x > geo.area->cols - 1 || x < 0 || y > geo.area->rows - 1 || y < 0)
        result = 0;
    else
        result = 1;
    return result;
}

char far wall_passable(char x, char y, unsigned char dir)      /* 2f0d:04a4 */
{
    char passable;
    char result;

    if (!in_area(x, y) && (ecl_id == 0 || ecl_id == 0x50))
        result = 0;
    else {
        if (x > geo.area->cols - 1)
            x = 0;
        else if (x < 0)
            x = geo.area->cols - 1;
        if (y > geo.area->rows - 1)
            y = 0;
        else if (y < 0)
            y = geo.area->rows - 1;
        passable = 1;
        if (wall_side(x, y, dir) > 0)
            switch (dir) {
            case 6:
                passable = (geo.area->flags[geo.area->cols * y + x] & 0x40) >> 6;
                break;
            case 4:
                passable = (geo.area->flags[geo.area->cols * y + x] & 0x10) >> 4;
                break;
            case 2:
                passable = (geo.area->flags[geo.area->cols * y + x] & 4) >> 2;
                break;
            case 0:
                passable = geo.area->flags[geo.area->cols * y + x] & 1;
                break;
            }
        result = passable;
    }
    return result;
}

unsigned char far wall_side(char x, char y, unsigned char dir)     /* 2f0d:0628 */
{
    unsigned char result;

    if (x > geo.area->cols - 1)
        x = 0;
    else if (x < 0)
        x = geo.area->cols - 1;
    if (y > geo.area->rows - 1)
        y = 0;
    else if (y < 0)
        y = geo.area->rows - 1;
    switch (dir) {
    case 0:
        result = (geo.area->walls[geo.area->cols * y + x] & 0xf0) >> 4;
        break;
    case 2:
        result = geo.area->walls[geo.area->cols * y + x] & 0xf;
        break;
    case 4:
        result = (geo.area->walls2[geo.area->cols * y + x] & 0xf0) >> 4;
        break;
    case 6:
        result = geo.area->walls2[geo.area->cols * y + x] & 0xf;
        break;
    }
    return result;
}

char far area_cell_at(char x, char y)                         /* 2f0d:075b */
{
    char result;

    if (!in_area(x, y) && (ecl_id == 0 || ecl_id == 0x50))
        result = 0;
    else {
        if (x > geo.area->cols - 1)
            x = 0;
        else if (x < 0)
            x = geo.area->cols - 1;
        if (y > geo.area->rows - 1)
            y = 0;
        if (y < 0)
            y = geo.area->rows - 1;
        result = geo.area->cells[geo.area->cols * y + x];
    }
    return result;
}

unsigned char far area_cell_flags(char x, char y)                /* 2f0d:0807 */
{
    register int cell_flags;

    if (!in_area(x, y))
        return 0;
    cell_flags = geo.area->flags[geo.area->cols * y + x];
    return (cell_flags & 8) >> 2 | (cell_flags & 2) >> 1;
}

void far view_draw(char x, char y, unsigned char dir)            /* 2f0d:085a */
{
    unsigned char left;
    unsigned char right;
    unsigned char back;
    int y0;
    int dx;
    int dy;
    int cx;
    int cy;
    unsigned char index;
    unsigned char view_depth;
    unsigned char front_wall;
    unsigned char side_wall;
    unsigned char prev;
    register int off;
    register int x0;

    mode_change(0);
    pal_select(0);                      /* v1.0 only */
    moon_display_update(0);
    if (geo.show_map != 0)
        map_draw(x, y, dir);
    else {
        if (wall_set_res[0] == 0) {
            if (geo.area->set_2 != 0xff)
                if (geo.area->set_2 != cur_geo) {
                    wall_set_load(geo.area->set_2, 2);
                    huge_move(slot_base(*(int far *)wall_set_res[2]), db_buffer(),
                              (unsigned)slot_used(*(int far *)wall_set_res[2]));
                    res_free(&wall_set_res[2]);
                    cur_geo = geo.area->set_2;
                } else if (is_xvga() || !pixel_shift())
                    glyph_palette_load(db_buffer(), 0);
            wall_set_load(geo.area->set_0, 0);
            if (geo.area->set_1 != 0xff)
                wall_set_load(geo.area->set_1, 1);
        }
        screen_draw_begin(1);
        flag_c3a4 = 0;
        view_frame();
        clip_set(0x1f4c, 0x1f4c, 0x1f78, 0x1f78);
        left = (dir + 6) % 8;
        back = (dir + 4) % 8;
        right = (dir + 2) % 8;
        x0 = x;
        y0 = y;
        dx = dir_dx[dir];
        dy = dir_dy[dir];
        view_depth = 2;
        x0 += view_depth * dx;
        y0 += view_depth * dy;
        do {
            switch (view_depth) {
            case 2:
                cx = x0;
                cy = y0;
                index = 0;
                off = 0;
                prev = 0;
                while (index < 4) {
                    front_wall = wall_side(cx, cy, dir);
                    if (!in_area(cx, cy) && !wall_side(cx, cy, right))
                        prev = 0;
                    if (front_wall != 0) {
                        if (prev > 0)
                            piece_at(pos_x[9] + off + 1, pos_y[9], prev, 9);
                        prev = front_wall;
                        if (index < 3)
                            piece_at(pos_x[0] + off, pos_y[0], front_wall, 0);
                    } else {
                        if (prev > 0
                            && wall_side(cx - dir_dx[left], cy - dir_dy[left], left))
                            piece_at(pos_x[9] + off + 1, pos_y[9], prev, 9);
                        prev = 0;
                    }
                    index++;
                    off -= 2;
                    cx = cx + dir_dx[left];
                    cy = cy + dir_dy[left];
                }
                cx = x0;
                cy = y0;
                index = 0;
                off = 0;
                prev = 0;
                while (index < 4) {
                    front_wall = wall_side(cx, cy, dir);
                    if (!in_area(cx, cy) && !wall_side(cx, cy, left))
                        prev = 0;
                    if (front_wall != 0) {
                        if (prev > 0)
                            piece_at(pos_x[9] + off - 1, pos_y[9], prev, 9);
                        prev = front_wall;
                        if (index != 0 && index < 3)
                            piece_at(pos_x[0] + off, pos_y[0], front_wall, 0);
                    } else {
                        if (prev > 0
                            && wall_side(cx - dir_dx[right], cy - dir_dy[right], right))
                            piece_at(pos_x[9] + off - 1, pos_y[9], prev, 9);
                        prev = 0;
                    }
                    index++;
                    off += 2;
                    cx = cx + dir_dx[right];
                    cy = cy + dir_dy[right];
                }
                cx = x0;
                cy = y0;
                index = 0;
                off = 0;
                while (index < 3) {
                    side_wall = wall_side(cx, cy, left);
                    if (side_wall != 0)
                        if (index == 0)
                            piece_at(pos_x[1] + off, pos_y[1], side_wall, 1);
                        else
                            piece_at(pos_x[1] + off - 1, pos_y[1], side_wall, 1);
                    index++;
                    off -= 2;
                    cx = cx + dir_dx[left];
                    cy = cy + dir_dy[left];
                }
                cx = x0;
                cy = y0;
                index = 0;
                off = 0;
                while (index < 3) {
                    side_wall = wall_side(cx, cy, right);
                    if (side_wall != 0)
                        if (index == 0)
                            piece_at(pos_x[2] + off, pos_y[2], side_wall, 2);
                        else
                            piece_at(pos_x[2] + off + 1, pos_y[2], side_wall, 2);
                    index++;
                    off += 2;
                    cx = cx + dir_dx[right];
                    cy = cy + dir_dy[right];
                }
                break;
            case 1:
                cx = x0 + dir_dx[left];
                cx = cx + dir_dx[left];
                cy = y0 + dir_dy[left];
                cy = cy + dir_dy[left];
                index = 0;
                off = -6;
                while (index < 3) {
                    front_wall = wall_side(cx, cy, dir);
                    if (front_wall != 0)
                        piece_at(pos_x[3] + off, pos_y[3], front_wall, 3);
                    side_wall = wall_side(cx, cy, left);
                    if (index != 0 && side_wall != 0)
                        piece_at(pos_x[4] + off, pos_y[4], side_wall, 4);
                    index++;
                    off += 3;
                    cx = cx + dir_dx[right];
                    cy = cy + dir_dy[right];
                }
                cx = x0 + dir_dx[right];
                cx = cx + dir_dx[right];
                cy = y0 + dir_dy[right];
                cy = cy + dir_dy[right];
                index = 0;
                off = 6;
                while (index < 3) {
                    front_wall = wall_side(cx, cy, dir);
                    if (index < 2 && front_wall != 0)
                        piece_at(pos_x[3] + off, pos_y[3], front_wall, 3);
                    side_wall = wall_side(cx, cy, right);
                    if (index != 0 && side_wall != 0)
                        piece_at(pos_x[5] + off, pos_y[5], side_wall, 5);
                    index++;
                    off -= 3;
                    cx = cx + dir_dx[left];
                    cy = cy + dir_dy[left];
                }
                break;
            case 0:
                cx = x0 + dir_dx[left];
                cy = y0 + dir_dy[left];
                index = 0;
                off = -7;
                while (index < 2) {
                    front_wall = wall_side(cx, cy, dir);
                    if (front_wall != 0)
                        piece_at(pos_x[6] + off, pos_y[6], front_wall, 6);
                    side_wall = wall_side(cx, cy, left);
                    if (index != 0 && side_wall != 0)
                        piece_at(pos_x[7] + off, pos_y[7], side_wall, 7);
                    index++;
                    off += 7;
                    cx = cx + dir_dx[right];
                    cy = cy + dir_dy[right];
                }
                cx = x0 + dir_dx[right];
                cy = y0 + dir_dy[right];
                index = 0;
                off = 7;
                while (index < 2) {
                    front_wall = wall_side(cx, cy, dir);
                    if (index < 1 && front_wall != 0)
                        piece_at(pos_x[6] + off, pos_y[6], front_wall, 6);
                    side_wall = wall_side(cx, cy, right);
                    if (index != 0 && side_wall != 0)
                        piece_at(pos_x[8] + off, pos_y[8], side_wall, 8);
                    index++;
                    off -= 7;
                    cx = cx + dir_dx[left];
                    cy = cy + dir_dy[left];
                }
            }
            x0 = x0 + dir_dx[back];
            y0 = y0 + dir_dy[back];
            view_depth--;
        } while (view_depth != 0xff);
        clip_reset();
        screen_flush();
    }
}

static void far back_load(char area_id)                                /* 2f0d:1152 */
{
    set_open_mode('2');
    tile_open_file("back1", area_id, 0, geo.attr_1c, &back_res);
    res_palette_load(back_res);
}

void far back_free(void)                                         /* 2f0d:118a */
{
    res_free(&back_res);
    word_b2eb = 0;
}

void far wall_set_load(unsigned char area_id, unsigned char slot)      /* 2f0d:119d */
{
    short index;
    char name[8];
    unsigned char map[0x100];

    set_open_mode('2');
    str_format(name, "8x8d%c%d", area_id >= 10 ? 'c' : 'b',
               is_xvga() || !pixel_shift() ? slot + 1 : 1);
    tile_open_file(name, area_id, 0, 0, &wall_set_res[slot]);
    if (slot != 0 && tile_opened() && (is_xvga() || !pixel_shift())) {
        for (index = 0; index < 0x100; index++)
            map[index] = index;
        for (index = 0; index < 0x25; index++)
            map[index + 0x20] = slot * 0x25 + index + 0x20;
        for (index = 0; index < 0x30; index++)
            res_glyph_recolor(map, index, wall_set_res[slot]);
    }
    res_palette_load(wall_set_res[slot]);
}

void far geo_load(char area_id)                                        /* 2f0d:12e3 */
{
    int len;

    set_data_disk('3');
    data_load("GEO1", 0L, area_id, &len, (char far *)geo.area);
    if (geo.area->cols * geo.area->rows * 4 + 8 != len) {
        error_box("Unable to load geo in Load3DMap.");
        game_exit();
    }
    mem_move(geo.area->walls + len * 3, geo.area->flags,
             len = geo.area->cols * geo.area->rows);
    mem_move(geo.area->walls + len * 2, geo.area->cells, len);
    mem_move(geo.area->walls + len, geo.area->walls2, len);
    res_free(&wall_set_res[0]);
    res_free(&wall_set_res[1]);
    game_state[STATE_LOADED_AREA_ID] = area_id;
}

void far wall_sets_free(char reset)                              /* 2f0d:13ef */
{
    register int index;

    for (index = 0; index < 2; index++)  /* v1.0 frees sets 0 and 1 only */
        res_free(&wall_set_res[index]);
    if (reset)
        cur_geo = -1;
}
