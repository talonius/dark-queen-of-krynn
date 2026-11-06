/* DQK.EXE overlay 02 module 2a89: lines over the map and closing in on a cell.
 * Stepping a line cell by cell, testing whether a line stays clear of the
 * terrain and within a cost, whether a cell lies ahead of another in a
 * given direction, and planning, for each combatant, the cheapest clear
 * approach to a target's cells.
 *
 * EXE 0x02f9af..0x0304af, one Turbo C large-model _TEXT segment starting at
 * byte f of paragraph 2a89, the last module of RTLink section 2 (payload
 * 0x2de10, link segment 28d0).  The first 15 bytes of paragraph 2a89 are
 * the tail of module 28d0; one zero byte follows to the section's end at
 * 0x304b0.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are
 * defined in shipped order; bake consumes them in that order.  Section 1's
 * 28d0 frame holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; an ovlNN_ name says which section
 * and stored segment the callee lives in.
 *
 * _DATA is dir_of, DGROUP 6170..6178.  The module has no _BSS.
 *
 * tile_info, steps and combatants are indexed from 1.  The shipped code
 * adds the address of element 0 (6197, c2f3, c3f1), which falls on other
 * variables; (steps - 1)[i] folds to that displacement, where
 * steps[i - 1] computes i - 1 at run time.
 *
 * Directions run 0 (north) clockwise to 7 (north-west), as dir_dx and
 * dir_dy step them; 8 is no direction.  A line's steps count 2 for a
 * straight step and 3 for a diagonal one.
 */

#include <stdlib.h>
#include "dqk.h"

char dir_of[3][3] = {           /* [sy + 1][sx + 1]                       */
    { 7, 0, 1 },
    { 6, 8, 2 },
    { 5, 4, 3 }
};

/* The translated Pascal unit's empty initialization block (DKK pathing.pas). */
void far approach_unit_init(void)                                   /* 2a89:000f */
{
}

int far int_sign(int value)                                          /* 2a89:0010 */
{
    int result;

    if (value < 0)
        result = -1;
    else if (value > 0)
        result = 1;
    else
        result = 0;
    return result;
}

void far steps_sort(void)                                        /* 2a89:0031 */
{
    unsigned char step_index;
    unsigned char comparison_index;
    unsigned char dir_j;
    unsigned char dir_i;
    struct Step tmp;

    if (step_count > 1) {
        for (step_index = 1; step_index <= step_count - 1; step_index++) {
            for (comparison_index = step_index + 1; comparison_index <= step_count; comparison_index++) {
                dir_i = (steps - 1)[step_index].dir;
                dir_j = (steps - 1)[comparison_index].dir;
                if ((steps - 1)[comparison_index].cost < (steps - 1)[step_index].cost
                    || (steps - 1)[comparison_index].cost == (steps - 1)[step_index].cost && dir_j < dir_i
                       && dir_j % 2 <= dir_i % 2) {
                    tmp = (steps - 1)[step_index];
                    (steps - 1)[step_index] = (steps - 1)[comparison_index];
                    (steps - 1)[comparison_index] = tmp;
                }
            }
        }
    }
}

void far line_init(struct Line far *line)                           /* 2a89:01a3 */
{
    line->x = line->x0;
    line->y = line->y0;
    line->dx = abs(line->x1 - line->x0);
    line->dy = abs(line->y1 - line->y0);
    line->sx = int_sign(line->x1 - line->x0);
    line->sy = int_sign(line->y1 - line->y0);
    line->err = 0;
    line->steps = 0;
}

char far line_step(struct Line far *line)                           /* 2a89:020f */
{
    char moved;
    char xs;
    char ys;

    moved = 0;
    xs = 1;
    ys = 1;
    if (line->dx >= line->dy) {
        if (line->x != line->x1) {
            line->x += line->sx;
            xs = line->sx + 1;
            line->err += line->dy;
            line->err += line->dy;
            line->steps++;
            line->steps++;
            if (line->err >= line->dx) {
                line->err -= line->dx;
                line->err -= line->dx;
                line->y += line->sy;
                ys = line->sy + 1;
                line->steps++;
            }
            moved = 1;
        }
    } else if (line->y != line->y1) {
        line->y += line->sy;
        ys = line->sy + 1;
        line->err += line->dx;
        line->err += line->dx;
        line->steps++;
        line->steps++;
        if (line->err >= line->dy) {
            line->err -= line->dy;
            line->err -= line->dy;
            line->x += line->sx;
            xs = line->sx + 1;
            line->steps++;
        }
        moved = 1;
    }
    line->dir = dir_of[ys][xs];
    return moved;
}

char far line_clear(char x, char y, char far *tx, char far *ty,
                    unsigned short far *cost)                    /* 2a89:0328 */
{
    char ok;
    struct Line path;
    struct Line rise;
    char done;
    unsigned short limit;

    limit = *cost * 2 + 1;
    path.x0 = x;
    path.y0 = y;
    path.x1 = *tx;
    path.y1 = *ty;
    line_init(&path);
    rise.x0 = 0;
    rise.y0 = (tile_info - 1)[cur_map->cells[y][x]].height;
    if (path.dx > path.dy)
        rise.x1 = path.dx;
    else
        rise.x1 = path.dy;
    rise.y1 = (tile_info - 1)[cur_map->cells[y][x]].height;   /* start cell again: 2a89:03af */
    line_init(&rise);
    done = 0;
    do {
        if (!cur_map->dist_mode
            && (tile_info - 1)[cur_map->cells[path.y][path.x]].block > rise.y
            || path.steps > limit) {
            *tx = path.x;
            *ty = path.y;
            *cost = path.steps;
            ok = 0;
            return ok;
        }
        line_step(&rise);
        done = !line_step(&path);
    } while (!done);
    *cost = path.steps;
    ok = 1;
    return ok;
}

char far dir_ok(char x, char y, char x2, char y2, unsigned char dir)
{                                                                /* 2a89:048f */
    char ok;
    char nx;
    char ny;

    if (x < 0 || x > 0x31 || y < 0 || y > 0x18
        || x2 < 0 || x2 > 0x31 || y2 < 0 || y2 > 0x18) {
        ok = 0;
    } else {
        if (dir == 0xff)
            dir = 8;
        nx = dir_dx[dir] + x;
        ny = dir_dy[dir] + y;
        if (x == x2 && y == y2)
            ok = 1;
        else if (nx == x2 && ny == y2)
            ok = 1;
        else switch (dir) {
        case 0:
            ok = x2 >= nx && nx - x2 + ny >= y2
                 || x2 <= nx && x2 - nx + ny >= y2;
            break;
        case 1:
            ok = x2 >= nx && nx - x2 + ny >= y2
                 || nx + ny - y2 <= x2 && y2 <= ny;
            break;
        case 2:
            ok = nx + ny - y2 <= x2 && y2 <= ny
                 || nx + y2 - ny <= x2 && y2 >= ny;
            break;
        case 3:
            ok = nx + y2 - ny <= x2 && y2 >= ny
                 || x2 >= nx && x2 - nx + ny <= y2;
            break;
        case 4:
            ok = x2 >= nx && x2 - nx + ny <= y2
                 || x2 <= nx && nx - x2 + ny <= y2;
            break;
        case 5:
            ok = x2 <= nx && nx - x2 + ny <= y2
                 || nx + ny - y2 >= x2 && y2 >= ny;
            break;
        case 6:
            ok = nx + ny - y2 >= x2 && y2 >= ny
                 || nx + y2 - ny >= x2 && y2 <= ny;
            break;
        case 7:
            ok = nx + y2 - ny >= x2 && y2 <= ny
                 || x2 <= nx && x2 - nx + ny >= y2;
            break;
        case 8:
            ok = 1;
            break;
        }
    }
    return ok;
}

void far plan_steps(char x, char y, unsigned short budget, unsigned char dir,
                    unsigned char size)                          /* 2a89:07d7 */
{
    unsigned char step_index;
    unsigned char initial_direction;
    unsigned char index;
    unsigned char inner_index;
    unsigned char best_i;
    unsigned char best_j;
    char found;
    char tx;
    char ty;
    char dx;
    char dy;
    char to_x[6];
    char to_y[6];
    char from_x[6];
    char from_y[6];
    unsigned short cost;
    unsigned short best;

    for (index = 0; index <= 5; index++) {
        if (icon_cell_delta(size, index, &dx, &dy)) {
            to_x[index] = x + dx;
            to_y[index] = y + dy;
        } else
            to_x[index] = -1;
    }
    step_count = 0;
    for (step_index = 1; step_index <= combatant_count; step_index++) {
        if ((combatants - 1)[step_index].size > 0) {
            found = 0;
            best = 0xff;
            for (inner_index = 0; inner_index <= 5; inner_index++) {
                if (icon_cell_delta((combatants - 1)[step_index].size, inner_index, &dx, &dy)) {
                    from_x[inner_index] = (combatants - 1)[step_index].x + dx;
                    from_y[inner_index] = (combatants - 1)[step_index].y + dy;
                } else
                    from_x[inner_index] = -1;
            }
            for (inner_index = 0; inner_index <= 5; inner_index++) {
                if (from_x[inner_index] >= 0) {
                    for (index = 0; index <= 5; index++) {
                        if (to_x[index] >= 0
                            && dir_ok(to_x[index], to_y[index], from_x[inner_index], from_y[inner_index], dir)) {
                            tx = from_x[inner_index];
                            ty = from_y[inner_index];
                            cost = budget;
                            if (line_clear(to_x[index], to_y[index], &tx, &ty, &cost)) {
                                found = 1;
                                if (cost < best) {
                                    best = cost;
                                    best_i = index;
                                    best_j = inner_index;
                                }
                            }
                        }
                    }
                }
            }
            if (found) {
                step_count++;
                (steps - 1)[step_count].who = step_index;
                (steps - 1)[step_count].cost = best;
                initial_direction = 0;
                if (dir < 8)
                    initial_direction = dir;
                else
                    while (!dir_ok(to_x[best_i], to_y[best_i],
                                   from_x[best_j], from_y[best_j], initial_direction))
                        initial_direction++;
                (steps - 1)[step_count].dir = initial_direction;
            }
        }
    }
    steps_sort();
}
