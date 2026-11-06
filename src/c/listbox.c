/* DQK.EXE resident module 1b05: the scrolling list box.  list_box registers
 * a box with the dialog items (the list itself, and the up and down arrows
 * or a scroll bar), draws its visible window and marks the chosen entry;
 * the handlers move the choice on a key, a click, an arrow or the bar.
 * Entries come from the box's fetch callback: fetch(n, &key) returns entry
 * n, fetch(-1, &key) the one after the last fetched, and 0 past the end,
 * with key set for an entry that cannot be chosen.
 *
 * EXE 0x00d459..0x00e1b5, one Turbo C large-model _TEXT segment starting at
 * byte 9 of paragraph 1b05.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * It abuts textwin.c (1a62) and menu.c (1bdb).
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  A name another module
 * gives an address is reused from there; the rest are placeholders chosen
 * from use.  Nothing outside the module calls it but menu.c's list_menu,
 * which far-calls list_box; list_click, list_key, list_arrow and list_bar
 * are item handlers list_box registers.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is: list_click
 * and list_box far-call list_select, so it carries a prototype.  list_key
 * calls list_step near ahead of its definition, which only a static
 * function allows.
 *
 * A coordinate from 0x1f40 up is a text cell: 0x1f40 + 4 * n is column or
 * row n.  The field names of struct ListBox are menu.c's.
 */

#include <stdlib.h>
#include "dqk.h"

extern void far ui_mark(int id, int attr, ...);                  /* 1aa6:188f */

int far list_select(int entry_index, char redraw, struct ListBox far *box);
static char list_step(int step_delta, struct ListBox far *box);

static char item_fmt[] = "%s";                                   /* 1d08 */

static struct ListBox far *cur_list;                             /* a654 */

int far list_entries(struct ListBox far *box)                     /* 1b05:0009 */
{
    char far *entry_text;
    char key;
    register int entry_count;

    entry_count = 0;
    entry_text = box->fetch(0, &key);    /* -O pushes this 0 from si: mov ax,si */
    while (entry_text) {
        entry_text = box->fetch(-1, &key);
        entry_count++;
    }
    return entry_count;
}

int far list_scroll(register int top, register int cur, char redraw,
                    struct ListBox far *box)                     /* 1b05:0055 */
{
    int entry_count;
    int ofs;
    int index;
    int old_top;
    int old_cur;
    int col;
    int row;
    int height;
    int width;
    int scroll_delta;
    int return_code;
    char key;
    char far *new_p;
    char far *old_p;
    char far *entry_text;

    entry_count = list_entries(box);
    width = box->width;
    top = int_max(0, int_min(int_max(entry_count, width) - width, top));
    cur = int_max(0, int_min(entry_count - 1, cur));
    old_top = box->save_top;
    old_cur = box->top;
    old_p = box->fetch(old_cur, &key);
    new_p = box->fetch(cur, &key);
    if (key) {
        cur = old_cur;
        new_p = old_p;
        return_code = -1;
    } else
        return_code = 1;
    if (!redraw && old_top == top && old_cur == cur)
        return 0;
    col = box->col + 0x1f40;
    row = box->row + 0x1f40;
    height = box->height;
    if (top != old_top && abs(top - old_top) < width && !redraw) {
        if (top > old_top) {
            scroll_delta = top - old_top;
            if (old_cur != cur && old_top + scroll_delta <= old_cur && old_top + width > old_cur)
                status_printf((row - 0x1f40) >> 2, ((col - 0x1f40) >> 2) + (old_cur - old_top),
                              7, 8, item_fmt, old_p);
            win_scroll(col, row, width * 4 + col, height * 4 + row, scroll_delta * 4 + 0x1f40, 0x1f40,
                       is_xvga() || !pixel_shift() ? 0x17 : 8);
            if (old_cur != cur && cur >= top && top + width - scroll_delta > cur)
                status_printf((row - 0x1f40) >> 2, ((col - 0x1f40) >> 2) + (cur - top),
                              11, 8, item_fmt, new_p);
            ofs = (width - scroll_delta) * 4;
            index = old_top + width;
        } else {
            scroll_delta = old_top - top;
            if (old_cur != cur && old_cur >= old_top && old_top + width - scroll_delta > old_cur)
                status_printf((row - 0x1f40) >> 2, ((col - 0x1f40) >> 2) + (old_cur - old_top),
                              7, 8, item_fmt, old_p);
            win_scroll(col, row, width * 4 + col, height * 4 + row, 0x1f40 - scroll_delta * 4, 0x1f40,
                       is_xvga() || !pixel_shift() ? 0x17 : 8);
            if (old_cur != cur && top + scroll_delta <= cur && top + width > cur)
                status_printf((row - 0x1f40) >> 2, ((col - 0x1f40) >> 2) + (cur - top),
                              11, 8, item_fmt, new_p);
            ofs = 0;
            index = top;
            width = scroll_delta;
        }
    } else {
        ofs = 0;
        index = top;
    }
    entry_text = box->fetch(index, &key);
    do {
        text_clear((row - 0x1f40) >> 2, (col + ofs - 0x1f40) >> 2,
                   ((row - 0x1f40) >> 2) + height - 1, (col + ofs - 0x1f40) >> 2);
        if (cur != index)
            status_printf((row - 0x1f40) >> 2, (col + ofs - 0x1f40) >> 2,
                          key ? 12 : 7, 8, item_fmt, entry_text);
        else
            status_printf((row - 0x1f40) >> 2, (col + ofs - 0x1f40) >> 2,
                          11, 8, item_fmt, entry_text);
        ofs += 4;
        index++;
    } while (top + width > index && (entry_text = box->fetch(-1, &key)) != 0);
    text_clear((row - 0x1f40) >> 2, (col + ofs - 0x1f40) >> 2,
               ((row - 0x1f40) >> 2) + height - 1, ((col - 0x1f40) >> 2) + width - 1);
    box->top = cur;
    if (box->flags == 2)
        ui_mark(box->count + 2, 0x20, top);
    else
        box->save_top = top;
    return return_code;
}

void far list_arrow(int id)                                      /* 1b05:0509 */
{
    struct ListBox far *box;
    register int scroll_step;

    box = cur_list;
    scroll_step = box->width - 1;
    switch (id - box->count) {
    case 2:
        list_scroll(box->save_top - scroll_step, box->top, 0, box);
        break;
    case 3:
        list_scroll(box->save_top + scroll_step, box->top, 0, box);
        break;
    }
}

void far list_click(int id, int x, int y)                        /* 1b05:0579 */
{
    int out_row;
    int out_col;
    struct ListBox far *box;
    register int entry_index;
    register int return_code;

    box = cur_list;
    text_xy_to(box->col + 0x1f40, box->row + 0x1f40, x, y, &out_row, &out_col);
    entry_index = box->save_top + (out_row >> 2);
    return_code = list_select(entry_index, 1, box);
    if (return_code == 1 && box->done) {
        if (box->done())
            ui_mark(id, 0x14);
    } else
        while (!mouse_clicked(&x, &y)) {
            text_xy_to(box->col + 0x1f40, box->row + 0x1f40, x, y, &out_row, &out_col);
            entry_index = box->save_top + (out_row >> 2);
            list_select(entry_index, 0, box);
        }
}

int far list_key(int unused, register int key)                  /* 1b05:0664 */
{
    struct ListBox far *box;
    int first;
    register int scroll_step;

    box = cur_list;
    scroll_step = box->width - 1;
    while (key_poll()) {
        if (key == 0x152)
            sound_toggle();
        else if (key == 0x153)
            music_toggle();
        key = key_get();
    }
    switch (key) {              /* 2e 3b 07 at 1b05:06b0: Turbo C's sparse-switch value search */
    case 0x152:
        sound_toggle();
        return 0;
    case 0x153:
        music_toggle();
        return 0;
    case 0x107:
    case 0x108:
        first = box->save_top;
        if (list_step(-1, box) && first == 0)
            list_step(list_entries(box) - 1, box);
        break;
    case 0x104:
    case 0x105:
        if (list_step(1, box))
            list_step(-box->top, box);
        break;
    case 0x101:
        if (box->top == box->save_top)
            scroll_step = box->width - 1;
        else
            scroll_step = box->top - box->save_top;
        list_step(-scroll_step, box);
        break;
    case 0x103:
        if (box->width + box->save_top - 1 == box->top)
            scroll_step = box->width - 1;
        else
            scroll_step = box->width + box->save_top - 1 - box->top;
        list_step(scroll_step, box);
        break;
    default:
        return 0;
    }
    return 1;
}

void far list_bar(int id, int step)                              /* 1b05:07e1 */
{
    struct ListBox far *box;
    register int scroll_step;

    box = cur_list;
    scroll_step = box->width - 1;
    switch (step) {
    case -2:
        list_scroll(box->save_top - scroll_step, box->top, 1, box);
        break;
    case -1:
        list_scroll(box->save_top - 1, box->top, 1, box);
        break;
    default:
        list_scroll(box->save_top, box->top, 1, box);
        break;
    case 1:
        list_scroll(box->save_top + 1, box->top, 1, box);
        break;
    case 2:
        list_scroll(box->save_top + scroll_step, box->top, 1, box);
        break;
    }
}

void far list_box(struct ListBox far *box, char redraw)          /* 1b05:08b3 */
{
    int col;
    int height;
    int top;
    int cur;
    int entry_count;
    register int width;
    register int row;

    cur_list = box;
    col = box->col + 0x1f40;
    row = box->row + 0x1f40;
    width = box->width;
    top = box->save_top;
    height = box->height;
    cur = box->top;
    entry_count = list_entries(box);
    ui_add(5, col, row, width * 4, height * 4, 0x22, list_click, 7, list_key, 0);
    row += height * 4;
    switch (box->flags) {
    case 1:
        ui_add(1, col + 1, row, 0, 0, 0x22, list_arrow, 0x25, 0xc, 0x12,
               1, width * 4 + col - 4, row, 0, 0, 0x22, list_arrow, 0x25, 0xe, 0x12, 0);
        break;
    case 2:
        if (width < 7)
            ui_add(8, scroll_proc, 0x28, col + 5, row, 0x2a, 0x10, 0x24, entry_count - (width - 1),
                   0x23, &box->save_top, 0x25, 0x416, 0x22, list_bar, 0);
        else if (width >= 7 && width <= 12)
            ui_add(8, scroll_proc, 0x28, col + 5, row, 0x2a, 0x1a, 0x24, entry_count - (width - 1),
                   0x23, &box->save_top, 0x25, 0x415, 0x22, list_bar, 0);
        else if (width > 12)
            ui_add(8, scroll_proc, 0x28, col + 5, row, 0x2a, 0x37, 0x24, entry_count - (width - 1),
                   0x23, &box->save_top, 0x25, 0x414, 0x22, list_bar, 0);
        break;
    }
    list_scroll(top, cur, redraw, box);
    list_select(cur, !redraw, box);
}

int far list_select(register int entry_index, char redraw, struct ListBox far *box) /* 1b05:0b03 */
{
    int top;
    int width;
    int col;
    int row;
    int x;
    int return_code;
    int count;
    char key;
    char far *entry_text;
    register int cur;

    cur = box->top;
    top = box->save_top;
    width = box->width;
    col = box->col + 0x1f40;
    row = box->row + 0x1f40;
    count = list_entries(box);
    entry_index = int_max(0, int_min(count - 1, entry_index));
    if (entry_index < top)
        return_code = list_scroll(entry_index, entry_index, 0, box);
    else if (top + width <= entry_index)
        return_code = list_scroll(entry_index - width + 1, entry_index, 0, box);
    else {
        entry_text = box->fetch(entry_index, &key);
        if (key)
            return -1;
        return_code = entry_index == cur;
        if (!redraw && return_code)
            return 1;
        if (cur >= top && cur - top < width && cur < count) {
            x = (cur - top) * 4 + col;
            status_printf((row - 0x1f40) >> 2, (x - 0x1f40) >> 2, 7, 8, item_fmt,
                          box->fetch(cur, &key));
        }
        x = (entry_index - top) * 4 + col;
        status_printf((row - 0x1f40) >> 2, (x - 0x1f40) >> 2, 11, 8, item_fmt, entry_text);
        box->top = entry_index;
    }
    if (!return_code && box->extra)
        box->extra();
    return return_code;
}

static char list_step(register int step_delta, struct ListBox far *box)   /* 1b05:0ca2 */
{
    int old;
    register int entry_index;

    old = box->top;
    entry_index = box->top + step_delta;
    entry_index = int_max(0, int_min(list_entries(box) - 1, entry_index));
    step_delta = entry_index - box->top;
    if (list_select(box->top + step_delta, 0, box) < 0) {
        if (step_delta > 0)
            step_delta++;
        else
            step_delta--;
        if (list_select(box->top + step_delta, 0, box) < 0) {
            if (step_delta > 0)
                step_delta -= 2;
            else
                step_delta += 2;
            list_select(box->top + step_delta, 0, box);
        }
    }
    return box->top == old;
}
