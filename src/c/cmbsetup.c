/* DQK.EXE overlay 01 module 28d0: setting up a fight.
 * Loading the combat icons and picture, building the combat map -- in a
 * dungeon from the walls around the party's square, in the wilderness from
 * the terrain's river, woods and scatter -- readying each party member's
 * combat record, and placing every combatant in its side's formation.
 *
 * EXE 0x02bcf0..0x02dbf2, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 28d0, the only module of RTLink section 1 (payload
 * 0x2bcf0, link segment 28d0).  Fourteen zero bytes follow to the section's
 * end at 0x2dc00.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions
 * are defined in shipped order; bake consumes them in that order.
 * Section 2's 28d0 frame holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in, and a mod1d99_ callee is an
 * RTLink vector stub whose target is named beside it.
 *
 * combat_setup is reached through the vector stub 1d99:0fdb; every other
 * routine is near-called only, and scatter_tile, called ahead of its
 * definition, is the one whose static the bytes prove.
 *
 * _DATA is DGROUP 5d0a..5db3: a mask byte, the direction and formation tables, the
 * terrain table terrain_flags copies to its frame, and three literals.
 * _BSS is a95a..ab8b: the block and wall state of the dungeon map, the two
 * sides' anchors and formations, and the loop counters placing shares.
 * Nothing found in the image references unused, dummy_long or unused_byte;
 * they keep the shipped layout.  Turbo C lays _BSS out by the statics'
 * names, and these names give the shipped order.
 *
 * combatants, heroes and recs are indexed from 1; the shipped code adds
 * the address of element 0 (c3f1, b2f8, c526).  tile_info is indexed from
 * 1 the same way (6197).
 *
 * Directions run 0 (north) clockwise to 7 (north-west), as dir_dx and
 * dir_dy step them.  The dungeon map is 13 by 5 blocks of 6 by 5 cells
 * around the party's square; a formation is 6 rows of 11 cells.
 *
 * pic_load is called here with four arguments; charpic.c defines it with
 * three.
 */

#include "dqk.h"

extern void far pic_load(char far *name, unsigned char set, unsigned char start_slot,
                         char tile_count);                                /* 104c:0007 */

static unsigned char mask_5d0a = 2;
static unsigned char side_dirs[4][4] = { /* by quarter: 8, then three directions */
    { 8, 4, 6, 2 },
    { 8, 6, 4, 0 },
    { 8, 0, 6, 2 },
    { 8, 2, 0, 4 }
};
static unsigned char ring_dirs[4][4] = { /* by quarter and ring: 2 * base */
    { 0, 0, 2, 6 },
    { 2, 2, 0, 4 },
    { 4, 4, 2, 6 },
    { 6, 6, 4, 0 }
};
static unsigned char quarter_dir[4] = { 7, 2, 3, 6 };
static char corner_x[2][4] = {
    { 5, 4, 5, 6 },
    { 3, 8, 7, 2 }
};
static char corner_y[2][4] = {
    { 3, 2, 2, 3 },
    { 0, 2, 5, 3 }
};
static char spans[5][6][2] = {  /* by form_type and row: first, last column */
    { { 1, 0 }, { 1, 0 }, { 1, 0 }, { 2, 9 }, { 3, 10 }, { 4, 10 } },
    { { 0, 2 }, { 0, 3 }, { 1, 4 }, { 2, 5 }, { 3, 6 }, { 4, 7 } },
    { { 0, 6 }, { 0, 7 }, { 1, 8 }, { 1, 0 }, { 1, 0 }, { 1, 0 } },
    { { 3, 6 }, { 4, 7 }, { 5, 8 }, { 6, 9 }, { 7, 10 }, { 8, 10 } },
    { { 0, 6 }, { 0, 7 }, { 1, 8 }, { 2, 9 }, { 3, 10 }, { 4, 10 } }
};

static unsigned char num;
static char gx;
static char gy;
static char unused;
static char blk_x;
static char blk_y;
static unsigned char terrain;
static unsigned char form_type;
static char start_x[2];
static int east_wall;
static char start_y[2];
static long dummy_long;
static int west_wall;
static int active_side;
static int north_wall;
static int south_wall;
static char unused_byte;
static char side_facing[2];
static unsigned char forms_facing;
static unsigned char start_facing;
static unsigned char ring_widths[2];
static char formation_grid[2][4][6][11];
static unsigned char enemy_quarter;

static void cell_put(unsigned char dx, unsigned char dy, unsigned char tile)
{                                                                /* 28d0:0000 */
    char x;
    char y;

    x = blk_x * 6 + blk_y * 5 + dx + 0x15;
    y = blk_y * 5 + dy + 0xa;
    if (x >= 0 && x <= 0x31 && y >= 0 && y <= 0x18)
        cur_map->cells[y][x] = tile + 1;
}

static int wall_at(char x, char y, char dir)                     /* 28d0:0072 */
{
    if (x >= 0 && x <= geo.area->cols - 1 && y >= 0 && y <= geo.area->rows - 1) {
        if (wall_passable(x, y, dir)) {
            if (wall_side(x, y, dir))
                return 3;
            return 0;
        }
        return 1;
    }
    if (y == geo.y && (dir == 2 || dir == 6))
        return 0;
    return 1;
}

static int wall_between(char x, char y, unsigned char dir)       /* 28d0:00ef */
{
    short here;
    short there;
    unsigned char back;

    back = (dir + 4) % 8;
    here = wall_at(x, y, dir);
    there = wall_at(x + dir_dx[dir], y + dir_dy[dir], back);
    return here | there;
}

static void block_floor(void)                                    /* 28d0:014e */
{
    unsigned char col;
    unsigned char row;

    for (row = 2; row <= 4; row++)
        for (col = 0; col <= 5; col++)
            cell_put(col, row, 0x16);
    if (west_wall == 1) {
        for (row = 2; row <= 4; row++) {
            cell_put(row - 1, row, 4);
            cell_put(row, row, 3);
            cell_put(row + 1, row, 0xd);
        }
    } else if (west_wall == 3) {
        cell_put(1, 2, 8);
        cell_put(5, 4, 0);
    }
}

static void block_north(void)                                    /* 28d0:01fb */
{
    if (north_wall == 1) {
        cell_put(3, 0, 5);
        cell_put(4, 0, 5);
        cell_put(3, 1, 0xa);
        cell_put(4, 1, 0xa);
    } else {
        cell_put(3, 0, 0x16);
        cell_put(4, 0, 0x16);
        cell_put(3, 1, 0x16);
        cell_put(4, 1, 0x16);
    }
}

static void block_nw(char far *px, char far *py)                 /* 28d0:0285 */
{
    unsigned char scan_index;
    char upper_left_tile;
    char lower_left_tile;
    char upper_right_tile;
    char lower_right_tile;
    char open;

    if (!wall_between(*px, *py - 1, 6) && !wall_between(*px - 1, *py, 0))
        open = 1;
    else
        open = 0;
    upper_left_tile = 0;
    for (scan_index = 1; scan_index <= 4; scan_index++) {
        switch (scan_index) {
        case 1:
            switch (north_wall) {
            case 0:
                switch (west_wall) {
                case 0:
                    upper_left_tile = 0x16;
                    break;
                case 1:
                    if (open)
                        upper_left_tile = 0;
                    else
                        upper_left_tile = 0xd;
                    break;
                case 3:
                    upper_left_tile = 0xd;
                    break;
                }
                break;
            case 1:
            case 3:
                if (west_wall == 0) {
                    if (open)
                        upper_left_tile = 0xf;
                    else
                        upper_left_tile = 5;
                } else if (open)
                    upper_left_tile = 0x12;
                else
                    upper_left_tile = 2;
                break;
            }
            break;
        case 2:
            switch (north_wall) {
            case 0:
                upper_right_tile = 0x16;
                break;
            case 3:
                upper_right_tile = 0x11;
                break;
            case 1:
                upper_right_tile = 5;
                break;
            }
            break;
        case 3:
            switch (west_wall) {
            case 0:
                if (north_wall == 0)
                    lower_left_tile = 0x16;
                else if (open)
                    lower_left_tile = 0x10;
                else
                    lower_left_tile = 0xa;
                break;
            case 3:
                if (open)
                    lower_left_tile = 0x14;
                else
                    lower_left_tile = 7;
                break;
            case 1:
                if (open)
                    lower_left_tile = 1;
                else
                    lower_left_tile = 3;
                break;
            }
            break;
        case 4:
            switch (west_wall) {
            case 0:
            case 3:
                switch (north_wall) {
                case 0:
                    lower_right_tile = 0x16;
                    break;
                case 3:
                    lower_right_tile = 0x17;
                    break;
                case 1:
                    lower_right_tile = 0xa;
                    break;
                }
                break;
            case 1:
                switch (north_wall) {
                case 0:
                    lower_right_tile = 0xd;
                    break;
                case 3:
                    lower_right_tile = 0x15;
                    break;
                case 1:
                    lower_right_tile = 6;
                    break;
                }
                break;
            }
            break;
        }
    }
    cell_put(1, 0, upper_left_tile);
    cell_put(2, 0, upper_right_tile);
    cell_put(1, 1, lower_left_tile);
    cell_put(2, 1, lower_right_tile);
}

static void block_ne(char far *px, char far *py)                 /* 28d0:049e */
{
    unsigned char scan_index;
    char upper_left_tile;
    char lower_left_tile;
    char upper_right_tile;
    char lower_right_tile;
    char open;
    short upper_wall;
    short right;

    upper_wall = wall_between(*px, *py - 1, 2);
    right = wall_between(*px + 1, *py, 0);
    if (upper_wall == 0 && right == 0)
        open = 1;
    else
        open = 0;
    for (scan_index = 1; scan_index <= 4; scan_index++) {
        switch (scan_index) {
        case 1:
            switch (north_wall) {
            case 0:
                if (upper_wall == 1)
                    upper_left_tile = 4;
                else
                    upper_left_tile = 0x16;
                break;
            case 3:
                upper_left_tile = 0xf;
                break;
            case 1:
                upper_left_tile = 5;
                break;
            }
            break;
        case 2:
            if (north_wall == 0) {
                switch (upper_wall) {
                case 0:
                    upper_right_tile = 0x16;
                    break;
                case 3:
                    if (east_wall == 0 && right != 0)
                        upper_right_tile = 0x18;
                    else if (east_wall == 0)
                        upper_right_tile = 7;
                    else
                        upper_right_tile = 1;
                    break;
                case 1:
                    if (east_wall == 0) {
                        if (right != 0)
                            upper_right_tile = 0xb;
                        else
                            upper_right_tile = 7;
                    } else
                        upper_right_tile = 3;
                    break;
                }
            } else if (east_wall != 0)
                upper_right_tile = 9;
            else if (right != 0)
                upper_right_tile = 5;
            else if (open)
                upper_right_tile = 0x11;
            else
                upper_right_tile = 0x13;
            break;
        case 3:
            switch (north_wall) {
            case 0:
                lower_left_tile = 0x16;
                break;
            case 3:
                lower_left_tile = 0x10;
                break;
            case 1:
                lower_left_tile = 0xa;
                break;
            }
            break;
        case 4:
            if (north_wall == 0) {
                if (upper_wall == 0)
                    lower_right_tile = 0x16;
                else if (east_wall != 0)
                    lower_right_tile = 4;
                else if (right == 0)
                    lower_right_tile = 8;
                else
                    lower_right_tile = 0xc;
            } else if (east_wall != 0)
                lower_right_tile = 0xe;
            else if (right == 0)
                lower_right_tile = 0x17;
            else
                lower_right_tile = 0xa;
            break;
        }
    }
    cell_put(5, 0, upper_left_tile);
    cell_put(6, 0, upper_right_tile);
    cell_put(5, 1, lower_left_tile);
    cell_put(6, 1, lower_right_tile);
}

static void dungeon_map(void)                                    /* 28d0:06a7 */
{
    char x;
    char y;

    mem_fill(cur_map->cells, 0x4e2, 0x16);
    for (blk_y = -2; blk_y <= 2; blk_y++)
        for (blk_x = -6; blk_x <= 6; blk_x++) {
            x = geo.x + blk_x;
            y = geo.y + blk_y;
            west_wall = wall_between(x, y, 6);
            north_wall = wall_between(x, y, 0);
            east_wall = wall_between(x, y, 2);
            south_wall = wall_between(x, y, 4);
            block_floor();
            block_north();
            block_nw(&x, &y);
            block_ne(&x, &y);
        }
}

static unsigned char terrain_flags(void)                         /* 28d0:0783 */
{
    unsigned char flags[32] = {
        0x00, 0x18, 0x11, 0x15, 0x01, 0x01, 0x60, 0x14,
        0x08, 0x01, 0x00, 0x21, 0x71, 0x09, 0x06, 0x04,
        0x01, 0x09, 0x09, 0x08, 0x59, 0x00, 0x11, 0x11,
        0x00, 0x00, 0x01, 0x11, 0x00, 0x00, 0x20, 0x20
    };

    return flags[terrain];
}

static void bank_put(char x, char y, unsigned char far *run)     /* 28d0:07ae */
{
    cur_map->cells[y][x] = 0x3a;
    if (x < 0x31) {
        cur_map->cells[y][x + 1] = fx_roll_dice(1, 2) + 0x3a;
        if (x + 1 < 0x31)
            cur_map->cells[y][x + 2] = fx_roll_dice(1, 2) + 0x3a;
        if (x + 2 < 0x31)
            cur_map->cells[y][x + 3] = 0x3d;
    }
    *run = 1;
}

static void wild_river(void)                                     /* 28d0:0860 */
{
    unsigned char run;
    char col;
    char row;

    if (!game_state[STATE_WILDERNESS_RIVER])
        return;
    col = 0x22 - fx_roll_dice(5, 4);
    while ((col + 2) % 7 > 0)
        col--;
    run = 1;
    for (row = 0; row <= 0x18; row++) {
        if (col <= 0x31) {
            cur_map->cells[row][col] = 0x34;
            if (col < 0x31) {
                if (fx_roll_dice(1, 0x14) <= run)
                    bank_put(col + 1, row, &run);
                else {
                    run++;
                    cur_map->cells[row][col + 1] = fx_roll_dice(1, 2) + 0x34;
                    if (col + 1 < 0x31)
                        cur_map->cells[row][col + 2] = 0x37;
                    if (col + 2 < 0x31)
                        cur_map->cells[row][col + 3] = 0x37;
                    if (col + 3 < 0x31)
                        cur_map->cells[row][col + 4] = fx_roll_dice(1, 2) + 0x37;
                }
            }
            if (col + 4 < 0x31)
                cur_map->cells[row][col + 5] = 0x34;
            col++;
        }
    }
}

static void wild_woods(void)                                     /* 28d0:09e7 */
{
    char col;
    char row;
    char chance;

    if (terrain_flags() & 0x80)
        return;
    chance = 5;
    if ((char)(terrain_flags() & mask_5d0a))
        chance -= 3;
    if (terrain_flags() & 4)
        chance -= 3;
    if (terrain_flags() & 0x40)
        chance += 2;
    if (terrain_flags() & 8)
        chance += 5;
    if (chance < 0)
        chance = 1;
    for (col = 0; col <= 0x31; col++)
        for (row = 1; row <= 0x18; row++)
            if ((tile_info - 1)[cur_map->cells[row][col]].code == 0x25
                && (tile_info - 1)[cur_map->cells[row - 1][col]].code == 0x25
                && fx_roll_dice(1, 100) <= chance) {
                if (fx_roll_dice(1, 100) <= chance)
                    cur_map->cells[row][col] = fx_roll_dice(1, 2) + 0x29;
                else {
                    cur_map->cells[row - 1][col] = fx_roll_dice(1, 5) + 0x1f;
                    cur_map->cells[row][col] = fx_roll_dice(1, 5) + 0x24;
                }
            }
}

static void scatter_tile(char x, char y, unsigned char tile_weight_a, unsigned char tile_weight_b,
                         unsigned char tile_weight_c, unsigned char tile_weight_d,
                         unsigned char tile_weight_e);

static void wild_scatter(void)                                   /* 28d0:0b79 */
{
    char col;
    char row;
    int scatter_chance;

    scatter_chance = 0x32;
    if (terrain_flags() & 0x10)
        scatter_chance += 10;
    if (terrain_flags() & 0x20)
        scatter_chance += 0x1e;
    if (terrain_flags() & 0x40)
        scatter_chance += 0x14;
    if (terrain_flags() & 4)
        scatter_chance -= 10;
    if ((char)(terrain_flags() & mask_5d0a))
        scatter_chance -= 0x14;
    if (terrain_flags() & 0x80)
        scatter_chance -= 0x32;
    for (col = 0; col <= 0x31; col++)
        for (row = 0; row <= 0x18; row++)
            if ((tile_info - 1)[cur_map->cells[row][col]].code == 0x25) {
                if (scatter_chance >= -0x1e && scatter_chance <= 9)
                    scatter_tile(col, row, 0, 0, 0, 0x1e, 0xf);
                if (scatter_chance >= 10 && scatter_chance <= 0x1d)
                    scatter_tile(col, row, 0, 1, 5, 0x14, 0xa);
                if (scatter_chance >= 0x1e && scatter_chance <= 0x45)
                    scatter_tile(col, row, 0, 2, 5, 10, 5);
                if (scatter_chance >= 0x46 && scatter_chance <= 0x59)
                    scatter_tile(col, row, 10, 2, 10, 10, 1);
                if (scatter_chance >= 0x5a && scatter_chance <= 0x6e)
                    scatter_tile(col, row, 0xf, 5, 0xf, 10, 1);
            }
}

static void scatter_tile(char x, char y, unsigned char tile_weight_a, unsigned char tile_weight_b,
                         unsigned char tile_weight_c, unsigned char tile_weight_d,
                         unsigned char tile_weight_e)                       /* 28d0:0ce3 */
{
    unsigned char roll;

    roll = fx_roll_dice(1, 100);
    if (roll <= tile_weight_a)
        cur_map->cells[y][x] = fx_roll_dice(1, 2) + 0x31;
    else if (roll <= tile_weight_a + tile_weight_b + 1)
        cur_map->cells[y][x] = 0x2c;
    else if (roll <= tile_weight_a + tile_weight_b + 1)       /* the bound above again: 28d0:0d61 */
        cur_map->cells[y][x] = fx_roll_dice(1, 5) + 0x2c;
    else if (roll <= tile_weight_a + tile_weight_b + tile_weight_c + tile_weight_d)
        cur_map->cells[y][x] = 0x44;
    else if (roll <= tile_weight_a + tile_weight_b + tile_weight_c + tile_weight_d + tile_weight_e)
        cur_map->cells[y][x] = fx_roll_dice(1, 6) + 0x3d;
}

static void wild_map(void)                                       /* 28d0:0e3b */
{
    mem_fill(cur_map->cells, 0x4e2, 0x45);
    terrain = game_state[STATE_WILDERNESS_TERRAIN];
    wild_river();
    wild_woods();
    wild_scatter();
}

static void map_build(void)                                      /* 28d0:0e70 */
{
    struct Res far *res;
    short index;

    res = 0;
    set_open_mode('2');
    tile_open_file("COMSPR", 0xff, 1, 0, &res);
    res_palette_load(res);
    for (index = 0xf; index <= 0x25; index++) {
        res_copy(res, index - 0xf, 1, icon_res, index);
        res_copy(res, index + 0x71, 1, icon_res, index + 0x26);
    }
    res_free(&res);
    if (game_state[STATE_DUNGEON])
        pic_load("DungCom1", geo.pic_set, 0, 0x19);
    else
        pic_load("WildCom1", geo.pic_set, 0, 0x27);
    cur_map->cursor_on = 0;
    cur_map->cursor_size = 1;
    cur_map->dist_mode = 0;
    if (game_state[STATE_DUNGEON])
        dungeon_map();
    else
        wild_map();
}

static void party_ready(void)                                    /* 28d0:0f7b */
{
    struct Character far *character;
    unsigned char count;
    unsigned char mor;
    long spare;

    character = node_list;
    count = 0;
    while (character != 0) {
        recalc_stats(character);
        count++;
        pool_alloc((void far * far *)&character->combat, sizeof(struct Combat), &combat_pool);
        mem_fill(character->combat, sizeof(struct Combat), 0);
        if (game_state[STATE_PARTY_COUNT] < count)
            character->combat->not_pc = 1;
        if (find_effect(character, 0xd5, &spare))
            character->combat->direction = 2;
        else
            character->combat->direction = quarter_dir[geo.facing / 2];
        if (character->side == 1) {
            if (find_effect(character, 0xd5, &spare))
                character->combat->direction = 2;
            else
                character->combat->direction = (character->combat->direction + 4) % 8;
        }
        mor = character->npc & 0x7f;
        if (character->side == 0 && character->combat->not_pc == 1 && (mor == 0 || mor > 0x66))
            character->npc = game_state[STATE_PARTY_COUNT] + 0x80;
        character->combat->byte_17 = 1;
        if (game_state[STATE_UNDERWATER] && character->side == 0)
            fx_add(character, 0x95, 0, 0, 0);
        character = character->next;
    }
}

static char outside(char x, char y)                              /* 28d0:1117 */
{
    if (x >= 0 && x < 11 || y >= 0 && y < 6)
        return 0;
    return 1;
}

static char spot_try(unsigned char who, char x, char y, char bx, char by,
                     unsigned char ring)                         /* 28d0:113a */
{
    char blocked;
    unsigned char cell;
    unsigned char kind;
    char special_terrain;

    if (x < 0 || y < 0 || x > 10 || y > 5)
        return 0;
    if (formation_grid[active_side][ring][y][x] == 0)
        return 0;
    (combatants - 1)[who].x = bx * 6 + x + by * 5 + 0x16;
    (combatants - 1)[who].y = by * 5 + y + 0xa;
    combat_direction_probe((heroes - 1)[who], 8, &blocked, &cell, &kind, &special_terrain);
    if (!blocked && cell > 0 && (tile_info - 1)[cell].cost < 0xff) {
        formation_grid[active_side][ring][y][x] = 0;
        return 1;
    }
    return 0;
}

static char place_one(unsigned char who)                         /* 28d0:128e */
{
    char placed;
    char first;
    char fail;
    char off;
    char blocked;
    char result;
    unsigned char state;
    unsigned char dir;
    unsigned char base;
    unsigned char scan_index;
    char ddx;
    char ddy;
    char px;
    char py;
    unsigned char placement_offset;
    unsigned char step;
    char ox;
    char oy;
    unsigned char tries;
    unsigned char ring;
    char x0;
    char y0;
    char x;
    char y;

    first = 1;
    fail = 0;
    state = 1;
    placement_offset = 0;
    ring = 0;
    ox = start_x[active_side];
    oy = start_y[active_side];
    do {
        base = ring_dirs[side_facing[active_side]][ring] / 2;
        switch (state) {
        case 1:
            ddx = dir_dx[quarter_dir[(base + 2) % 4]];
            ddy = dir_dy[quarter_dir[(base + 2) % 4]];
            x0 = corner_x[ring > 0][base] + ddx * placement_offset;
            y0 = corner_y[ring > 0][base] + ddy * placement_offset;
            x = x0;
            y = y0;
            step = 1;
            state = 2;
            tries = 1;
            break;
        case 2:
            ddx = dir_dx[quarter_dir[(base + 1) % 4]];
            ddy = dir_dy[quarter_dir[(base + 1) % 4]];
            x = ddx * step + x0;
            y = ddy * step + y0;
            state = 3;
            tries++;
            break;
        case 3:
            ddx = dir_dx[quarter_dir[(base + 3) % 4]];
            ddy = dir_dy[quarter_dir[(base + 3) % 4]];
            x = ddx * step + x0;
            y = ddy * step + y0;
            state = 2;
            step++;
            tries++;
            break;
        }
        if (x < 0 || y < 0 || x > 10 || y > 5)
            off = 1;
        else
            off = 0;
        if (state > 1 && (off && !outside(x, y)
                          || first && ring_widths[active_side] <= tries
                          || !first && tries > 0xb)) {
            placement_offset++;
            if (active_side == 0 && pool_lowbit(side_facing[0]) && ring == 0 && placement_offset == 1) {
                px = start_x[active_side] + geo.x;
                py = start_y[active_side] + geo.y;
                blocked = 0;
                for (scan_index = 1; scan_index <= 3; scan_index++) {
                    dir = side_dirs[side_facing[active_side]][scan_index];
                    if (view_mode == 3 || wall_between(px, py, dir) != 1)
                        blocked = 1;
                }
                if (blocked)
                    placement_offset++;
            }
            state = 1;
            first = 0;
        }
        if (off && outside(x, y)) {
            placed = 0;
            state = 0;
            while (ring < 3 && state != 1) {
                ring++;
                px = start_x[active_side] + geo.x;
                py = start_y[active_side] + geo.y;
                dir = side_dirs[side_facing[active_side]][ring];
                if (view_mode == 3 || wall_between(px, py, dir) != 1) {
                    ox = start_x[active_side] + dir_dx[dir];
                    oy = start_y[active_side] + dir_dy[dir];
                    placement_offset = 0;
                    state = 1;
                }
            }
            if (state != 1)
                fail = 1;
        }
        if (!off)
            placed = spot_try(who, x, y, ox, oy, ring);
    } while (!placed && !fail);
    if (fail)
        result = 0;
    else
        result = 1;
    return result;
}

static void formation_set(void)                                  /* 28d0:16b3 */
{
    unsigned char ring;
    unsigned char gap;
    short side_index;

    if (!game_state[STATE_ENCOUNTER_DISTANCE] && geo.facing != start_facing)
        gap = 1;
    else
        gap = game_state[STATE_ENCOUNTER_DISTANCE];
    if (geo.facing == forms_facing)
        return;
    forms_facing = geo.facing;
    side_facing[0] = geo.facing / 2;
    start_x[1] = dir_dx[geo.facing] * gap + start_x[0];
    start_y[1] = dir_dy[geo.facing] * gap + start_y[0];
    side_facing[1] = (geo.facing + 4) % 8 / 2;
    for (side_index = 0; side_index <= 1; side_index++)
        for (ring = 0; ring <= 3; ring++) {
            if (ring == 1)
                form_type = 4;
            else
                form_type = side_facing[side_index];
            for (gy = 0; gy <= 5; gy++)
                for (gx = 0; gx <= 10; gx++)
                    if (spans[form_type][gy][0] > gx || spans[form_type][gy][1] < gx)
                        formation_grid[side_index][ring][gy][gx] = 0;
                    else
                        formation_grid[side_index][ring][gy][gx] = 1;
        }
}

static void place_all(void)                                      /* 28d0:1864 */
{
    struct Character far *prev;
    struct Character far *character;
    unsigned char kept;
    unsigned char dropped;
    unsigned char sides;
    unsigned char dir;
    char ok;
    long spare;
    int index;

    index = 0;
    if (encounter_mode == 0x20) {
        cur_map->cells[16][29] = 0x47;
        cur_map->cells[15][33] = 0x48;
        cur_map->cells[14][32] = 0x49;
        cur_map->cells[15][31] = 0x4a;
        cur_map->cells[15][30] = 0x4b;
        cur_map->cells[15][32] = 0x4c;
    }
    start_facing = geo.facing;
    forms_facing = 0xff;
    enemy_quarter = 0;
    combat_sides_count();
    dropped = 0;
    kept = 0;
    for (num = 1; num <= 0x34; num++)
        (combatants - 1)[num].size = 0;
    mem_fill(occupant_grid, 0x4e2, 0);
    start_x[0] = 0;
    start_y[0] = 0;
    ring_widths[0] = (side_count[0] + 1) / 2;
    sides = 0;
    if (encounter_mode & 0xf) {
        if (encounter_mode & 1)
            sides++;
        if (encounter_mode & 2)
            sides++;
        if (encounter_mode & 4)
            sides++;
        if (encounter_mode & 8)
            sides++;
    } else
        sides++;
    sides = sides * 2;
    ring_widths[1] = (side_count[1] + sides - 1) / sides;
    formation_set();
    num = 1;
    combatant_count = 1;
    character = prev = node_list;
    while (character != 0) {
        input_press_discard();
        (heroes - 1)[num] = character;
        active_side = character->side;
        (combatants - 1)[num].byte_4 = num;
        (combatants - 1)[num].size = character->icon_dim & 7;
        if (encounter_mode & 0xf) {
            if (character->side == 0)
                geo.facing = start_facing;
            else {
                do
                    enemy_quarter = (enemy_quarter + 1) & 3;
                while (!(encounter_mode & 1 << enemy_quarter));
                geo.facing = enemy_quarter * 2;
            }
            formation_set();
        }
        dir = geo.facing;
        if (character->side == 1)
            dir = (dir + 4) % 8;
        if (find_effect(character, 0xd5, &spare))
            character->combat->direction = 2;
        else
            character->combat->direction = quarter_dir[dir / 2];
        if (encounter_mode == 0x20 && character->side == 1) {
            switch (index) {
            case 0:
                (combatants - 1)[num].x = 0x1e;
                (combatants - 1)[num].y = 0x10;
                break;
            case 1:
                (combatants - 1)[num].x = 0x1d;
                (combatants - 1)[num].y = 0x10;
                break;
            case 2:
                (combatants - 1)[num].x = 0x1e;
                (combatants - 1)[num].y = 0xe;
                break;
            case 3:
                (combatants - 1)[num].x = 0x1f;
                (combatants - 1)[num].y = 0xe;
                break;
            case 4:
                (combatants - 1)[num].x = 0x21;
                (combatants - 1)[num].y = 0xf;
                break;
            case 5:
                (combatants - 1)[num].x = 0x20;
                (combatants - 1)[num].y = 0xe;
                break;
            }
            index++;
            ok = 1;
        } else
            ok = place_one(num);
        if (ok) {
            prev = character;
            if (!character->enabled) {
                (combatants - 1)[num].size = 0;
                if (!character->combat->not_pc && character->status != 8) {
                    gx = (combatants - 1)[num].x;
                    gy = (combatants - 1)[num].y;
                    rec_count++;
                    (recs - 1)[rec_count].terrain = combat_terrain_under(gx, gy);
                    cur_map->cells[gy][gx] = character->status == 7 ? 0x1e : 0x1f;
                    (recs - 1)[rec_count].hero = character;
                    (recs - 1)[rec_count].x = gx;
                    (recs - 1)[rec_count].y = gy;
                }
            }
            if (character->status == 9) {
                character->enabled = 0;
                (combatants - 1)[num].size = 0;
            }
            combat_grid_build();
            num++;
            combatant_count++;
            kept++;
        } else {
            (combatants - 1)[num].size = 0;
            if (character->combat->not_pc == 1) {
                dropped++;
                (heroes - 1)[num] = 0;
                cur_node = character;
                hero_drop(1, 0);
                character = prev;
            } else {
                prev = character;
                num++;
                combatant_count++;
            }
        }
        character = character->next;
    }
    prev->next = 0;
    geo.facing = start_facing;
}

void far combat_setup(void)                                      /* 28d0:1dd7 */
{
    struct Character far *character;

    flag_c3a4 = 0;
    text_slow = 0;
    view_flag = 0xff;
    magic_on = 0;
    combat_round = 0;
    combat_round_limit = 0xf;
    attack_roll = 0;
    rec_count = 0;
    cloud_list = 0;
    taunting_character = 0;
    dropped_item_boundary = 0;
    mem_zero(recs, sizeof recs);
    game_state[STATE_ALLY_ATTACK_CONFIRMED] = 0;
    map_build();
    input_press_discard();
    party_ready();
    input_press_discard();
    place_all();
    input_press_discard();
    cur_map->ox = combatant_x(node_list) - 3;
    cur_map->oy = combatant_y(node_list) - 3;
    combat_view_show();
    for (character = node_list; character != 0; character = character->next) {
        character->combat->turn_attempts = 0;
        fx_trigger_group(8, &character);
    }
    hostile_hp_percent_update();
    move_menu_active = 0;
    aim_cursor_active = 0;
    view_mode = 5;
    exploration_menu_context = 0;
    character = node_list;
    current_spell = 0;
}
