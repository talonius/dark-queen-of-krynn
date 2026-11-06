/* DQK.EXE resident module 2aa6: dialogs ("DL" in its error texts).
 * A dialog is an array of 0x20-byte items in one far block; each item has
 * a handler that takes messages -- draw, hit test, press, activate, key,
 * set a field -- and falls back on ui_item_proc for the ones it does not
 * take.  ui_add builds items from a tag list, ui_poll runs one round of
 * mouse and keyboard input over them, and ui_mark and ui_send post a
 * message to one item or a range.
 *
 * EXE 0x01ce6e..0x01e815, one Turbo C large-model _TEXT segment starting at
 * byte e of paragraph 2aa6.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * It starts the byte after libedit.c's last retf (2aa6:000d) and ends at
 * sfxslots.c's first byte.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee names are
 * placeholders chosen from use; a modNNNN_ name says only which segment the
 * callee lives in.  Its own routines that menu.c calls keep menu.c's names
 * (ui_add, ui_poll, ui_mark, ui_set_hook, ui_run and three mod1aa6_
 * placeholders).
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is.  radio_proc
 * is static: group_proc near-calls it ahead of its definition.  ui_send is
 * declared without a prototype: its three callers pass three words, and
 * it reads five.
 *
 * -O cross-jumping keeps the LAST of two identical tails and points the
 * first at it.  Where the shipped code kept the first, the source says so:
 * ui_item_proc reaches its shared tails by goto, and ui_add puts three case
 * labels inside the item-type branches whose code they share.
 *
 * _BSS follows the statics' names (Turbo C's symbol-table order), not their
 * declaration order; these names reproduce the shipped dac0..dad1 layout,
 * chosen with goldbox.bss_order.  dl_items is public: module 21e9 reads it.
 */

#define DQK_DEFINES_DIALOG
#include "dqk.h"

int far button_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b);
int far group_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b);
static int radio_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b);
int far check_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b);
int far area_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b);
int far label_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b);
int far key_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b);
void far ui_send();
int far ui_poll(void);

static ItemProc dl_procs[7] = {                                  /* a1b6 */
    button_proc, group_proc, radio_proc, check_proc,
    area_proc, label_proc, key_proc
};

static int dl_used;                                              /* dac0 */
static char dl_drawn;                                            /* dac2 */
static long next_rep;                                            /* dac3 */
struct Wid far *dl_items;                                        /* dac7 */
static struct Wid far *item_pool;                                /* dacb */
static char dl_running;                                          /* dacf */
static int max_items;                                            /* dad0 */

char far key_match(int hot, int key)                             /* 2aa6:000e */
{
    if (hot == 0)
        return 0;
    if (hot < 0x20)
        return hot + 0x100 == key;
    switch (hot) {
    case '*':
        return 1;
    case '#':
        return key == '`' || key == 0x1b;
    case '@':
        return key == '\r' || key == '\n';
    }
    return to_lower(hot) == to_lower(key);
}

void far item_draw(struct Wid far *widget, int base)                  /* 2aa6:0095 */
{
    int x;
    int y;
    register int attr;
    register int scan_index;

    widget->flags |= 0x80;
    if (widget->flags & 2)
        return;
    if (widget->flags & 1)
        scan_index = 2;
    else {
        scan_index = (widget->flags & 8) ? 3 : 0;
        if (widget->flags & 4)
            scan_index++;
    }
    if (widget->glyph >= 0)
        draw_glyph(widget->row, widget->col, widget->glyph ? widget->glyph >> 10 : 0,
                   (widget->glyph ? widget->glyph & 0x3ff : base) + scan_index);
    if (widget->text == 0)
        return;
    if (widget->flags & 1) {
        if (widget->color) {
            if ((widget->color & 0xf0) == 0x90)
                attr = 0x98;
            else
                attr = (widget->color & 0xf0) | 7;
        } else
            attr = 0x17;
    } else {
        if (widget->color)
            attr = widget->color;
        else
            attr = 0xf0;
    }
    text_offset_xy(widget->row, widget->col, 0, 0x1f46, &x, &y);
    text_print_at(x, y, attr, widget->text);
}

int far ui_item_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b)   /* 2aa6:01c7 */
{
    struct Wid far *widget_cursor;
    int x;
    int y;
    char old;
    char far *text;
    char was;
    char button_released;
    char hit;

    widget_cursor = widget;
    switch (msg) {
    case 1:
        widget_cursor->flags |= 0x80;
        break;
    case 3:
        was = 1;
        (*widget_cursor->fn)(widget_cursor, 0x13);
        (*widget_cursor->fn)(widget_cursor, 1);
        (*widget_cursor->fn)(widget_cursor, 7);
        do {
            button_released = inp_clicked(&x, &y);
            hit = (*widget_cursor->fn)(widget_cursor, 2, x, y) ? 1 : 0;
            if (was != hit) {
                was = hit;
                (*widget_cursor->fn)(widget_cursor, was ? 0x13 : 0x1b);
                (*widget_cursor->fn)(widget_cursor, 1);
            }
            if (was && !button_released)
                (*widget_cursor->fn)(widget_cursor, 7);
        } while (!button_released);
        (*widget_cursor->fn)(widget_cursor, 0x1b);
        return was;
    case 5:
        if (widget_cursor->flags & 3)
            break;
        return key_match(widget_cursor->key1, message_arg_a) || key_match(widget_cursor->key2, message_arg_a);
    case 4:
        if (widget_cursor->cb == 0)
            break;
        (*widget_cursor->cb)(widget_cursor - dl_items);
        break;
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
    case 0x16:
        old = widget_cursor->flags;
        widget_cursor->flags |= 1 << (msg - 0x10);
    changed:                            /* 2aa6:03d8, reached from 0x18.. */
        if (widget_cursor->flags == old)
            break;
    dirty:                              /* 2aa6:03e7 */
        widget_cursor->flags &= 0x7f;
        break;
    case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d:
    case 0x1e:
        old = widget_cursor->flags;
        widget_cursor->flags &= ~(1 << (msg - 0x18));
        goto changed;
    case 0x20:
        widget_cursor->key1 = message_arg_a;
        break;
    case 0x21:
        widget_cursor->key2 = message_arg_a;
        break;
    case 0x22:
        widget_cursor->cb = *(ItemCb far *)&message_arg_a;
        break;
    case 0x23:
        widget_cursor->val = *(int far * far *)&message_arg_a;
        break;
    case 0x24:
        widget_cursor->width = message_arg_a;
        break;
    case 0x25:
        if (widget_cursor->glyph == message_arg_a)
            break;
        widget_cursor->glyph = message_arg_a;
        goto dirty;
    case 0x26:
        if (widget_cursor->color == message_arg_a)
            break;
        widget_cursor->color = message_arg_a;
        goto dirty;
    case 0x27:
        text = *(char far * far *)&message_arg_a;
        if (widget_cursor->text == text)
            break;
        widget_cursor->text = text;
        goto dirty;
    case 0x28:
        if (widget_cursor->row == message_arg_a && widget_cursor->col == message_arg_b)
            break;
        widget_cursor->row = message_arg_a;
        widget_cursor->col = message_arg_b;
        goto dirty;
    case 0x29:
        widget_cursor->cursor = message_arg_a;
        break;
    case 0x2a:
        widget_cursor->len = message_arg_a;
        break;
    case 0x2b:
        widget_cursor->fn = dl_procs[message_arg_a - 1];
        break;
    case 0x2c:
        widget_cursor->fn = *(ItemProc far *)&message_arg_a;
        break;
    }
    return 0;
}

int far button_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b)    /* 2aa6:053b */
{
    int x1;
    int y1;
    int x2;
    int y2;
    register int glyph_variant;

    switch (msg) {
    case 1:
        widget->flags |= 0x80;
        if (widget->flags & 2)
            break;
        glyph_variant = (widget->flags & 9) ? 1 : 0;
        if (widget->glyph >= 0)
            draw_glyph(widget->row, widget->col, widget->glyph ? widget->glyph >> 10 : 0,
                       (widget->glyph ? widget->glyph & 0x3ff : 0xe) + glyph_variant);
        else if (widget->glyph < -1) {
            x2 = -widget->glyph;
            y2 = x2 >> 10;
            x2 &= 0x3ff;
            if (glyph_variant)
                glyph_variant = 4;
            draw_glyph(widget->row, widget->col, y2, x2 + glyph_variant);
            for (y1 = 0; y1 < widget->width; y1++)
                draw_glyph(widget->row, widget->col + y1 * 4, y2, x2 + glyph_variant + (y1 & 1) + 1);
            draw_glyph(widget->row, widget->col + (y1 - 1) * 4, y2, x2 + glyph_variant + 3);
            if (glyph_variant)
                glyph_variant = 1;
        }
        if (widget->text == 0)
            break;
        if ((widget->flags & 0x40) || widget->glyph < 0)
            y1 = widget->col;
        else {
            glyph_bounds(widget->row, widget->col, widget->glyph ? widget->glyph >> 10 : 0,
                         widget->glyph ? widget->glyph & 0x3ff : 0xe, &x1, &y1, &x2, &y2);
            text_xy_to(0x1f40, 0x1f40, x1, y1, &x1, &y1);
            text_xy_to(x1 + 0x1f40, y1 + 0x1f40, x2, y2, &x2, &y2);
            text_offset_xy(0, y1 + 0x1f40, 0, (y2 >> 1) + 0x1f40 - str_len(widget->text) * 2,
                         &x1, &y1);
        }
        text_xy(widget->row, y1, &x1, &y1);
        if (widget->flags & 1) {
            if (widget->color) {
                if (widget->len)
                    y2 = widget->len;
                else if ((widget->color & 0xf0) == 0x90)
                    y2 = 0x98;
                else
                    y2 = (widget->color & 0xf0) | 7;
            } else
                y2 = 0x17;
        } else {
            if (widget->color)
                y2 = widget->color;
            else
                y2 = 0xf0;
        }
        if (glyph_variant && screen_rows() >= 300)
            glyph_variant = 2;
        text_print_at(x1 + glyph_variant, y1, y2, widget->text);
        if (!(widget->flags & 0x20))
            break;
        if (widget->flags & 1)
            break;
        text_print_at(x1 + glyph_variant, y1, (widget->color & 0xf0) | 4, "%c", *widget->text);
        break;
    case 2:
        if (widget->flags & 3)
            break;
        if (widget->glyph >= 0)
            glyph_bounds(widget->row, widget->col, widget->glyph ? widget->glyph >> 10 : 0,
                         widget->glyph ? widget->glyph & 0x3ff : 0xe, &x1, &y1, &x2, &y2);
        else if (widget->glyph == -1) {
            text_xy(widget->row, widget->col, &x1, &y1);
            text_offset_xy(x1, y1, 0x1f44, str_len(widget->text) * 4 + 0x1f40, &x2, &y2);
        } else {
            text_xy(widget->row - 1, widget->col - 2, &x1, &y1);
            text_xy(widget->row + 5, widget->col + widget->width * 4 + 2, &x2, &y2);
        }
        return x1 <= message_arg_a && message_arg_a < x2 && y1 <= message_arg_b && message_arg_b < y2;
    case 3:
        if (widget->flags & 4)
            next_rep = timer_ticks();
        if ((x1 = ui_item_proc(widget, msg, message_arg_a, message_arg_b)) == 0)
            next_rep = 0;
        return x1;
    case 7:
        if (next_rep == 0)
            break;
        if (timer_ticks() < next_rep)
            break;
        next_rep = timer_ticks() + 12;
        if (widget->cb)
            (*widget->cb)(widget - dl_items);
        break;
    case 4:
        if (next_rep) {
            next_rep = 0;
            break;
        }
    default:
        return ui_item_proc(widget, msg, message_arg_a, message_arg_b);
    }
    return 0;
}

int far group_proc(struct Wid far *widget, int msg, register int message_arg_a, int message_arg_b) /* 2aa6:0aad */
{
    register int index;

    switch (msg) {
    case 0:
        if (widget->val)
            index = *widget->val;
        else
            index = 1;
        group_proc(widget, 4, index, 0);
        break;
    case 4:
        if (widget->width != message_arg_a) {
            if (widget->width)
                radio_proc(widget + widget->width, 0x1a, 0, 0);
            widget->width = message_arg_a;
            if (widget->val)
                *widget->val = message_arg_a;
        }
        if (message_arg_a)
            radio_proc(widget + message_arg_a, 0x12, 0, 0);
        if (widget->cb == 0)
            break;
        (*widget->cb)(widget - dl_items, message_arg_a);
        break;
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
    case 0x16: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c:
    case 0x1d: case 0x1e:
        for (index = 1; index <= widget->len; index++)
            radio_proc(widget + index, msg, 0, 0);
    default:
        return ui_item_proc(widget, msg, message_arg_a, message_arg_b);
    }
    return 0;
}

static int radio_proc(struct Wid far *widget, int msg, register int message_arg_a, int message_arg_b) /* 2aa6:0c21 */
{
    struct Wid far *group_widget;
    int x1;
    int y1;
    int x2;
    int y2;
    register int index;

    switch (msg) {
    case 1:
        item_draw(widget, 0x10);
        break;
    case 2:
        if (widget->flags & 3)
            break;
        if (widget->glyph) {
            glyph_bounds(widget->row, widget->col, widget->glyph >> 10, widget->glyph & 0x3ff,
                         &x1, &y1, &x2, &y2);
            if (x1 <= message_arg_a && message_arg_a < x2 && y1 <= message_arg_b && message_arg_b < y2)
                return 1;
        }
        text_xy(widget->row, widget->col, &x1, &y1);
        if (message_arg_a < x1)
            break;
        if (message_arg_b < y1)
            break;
        index = widget->text ? str_len(widget->text) : 0;
        text_offset_xy(x1, y1, 0x1f44, (widget->len + index) * 4 + 0x1f48, &x1, &y1);
        return message_arg_a < x1 && message_arg_b < y1;
    case 4:
        for (index = 0, group_widget = widget; group_widget->fn != dl_procs[1]; index++, group_widget--)
            ;
        group_proc(group_widget, 4, index, 0);
        if (widget->cb)
            (*widget->cb)(widget - dl_items);
        break;
    default:
        return ui_item_proc(widget, msg, message_arg_a, message_arg_b);
    }
    return 0;
}

int far check_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b)     /* 2aa6:0de5 */
{
    switch (msg) {      /* 2e 3b 07 at 0df1: Turbo C's sparse-switch value search */
    case 0:
        if (widget->val == 0)
            break;
        check_proc(widget, *widget->val ? 0x12 : 0x1a, 0, 0);
        break;
    case 1:
        item_draw(widget, 0x15);
        break;
    case 2:
        return radio_proc(widget, 2, message_arg_a, message_arg_b);
    case 4:
        widget->flags ^= 4;
        widget->flags &= 0x7f;
        if (widget->val)
            *widget->val = (widget->flags & 4) ? 1 : 0;
        if (widget->cb == 0)
            break;
        (*widget->cb)(widget - dl_items);
        break;
    case 0x12:
        widget->flags |= 4;
        widget->flags &= 0x7f;
        if (widget->val)
            *widget->val = 1;
        break;
    case 0x1a:
        widget->flags &= 0x7b;
        if (widget->val)
            *widget->val = 0;
        break;
    default:
        return ui_item_proc(widget, msg, message_arg_a, message_arg_b);
    }
    return 0;
}

int far area_proc(struct Wid far *widget, int msg, register int message_arg_a, int message_arg_b) /* 2aa6:0f37 */
{
    int x;
    int y;
    int callback_result;
    char was;
    char hit;
    register int cur;

    switch (msg) {
    case 2:
        if (widget->flags & 3)
            break;
        text_xy_to(widget->row, widget->col, message_arg_a, message_arg_b, &x, &y);
        if (x < 0 || x >= widget->len || y < 0 || y >= widget->width)
            break;
        if ((widget->flags & 0x40) && widget->cb && !(*widget->cb)(-2, message_arg_a, message_arg_b))
            break;
        return 1;
    case 3:
        if (!(widget->flags & 0x40))
            return 1;
        was = 0;
        x = message_arg_a;
        y = message_arg_b;
        cur = -1;
        do {
            if (widget->cb) {
                callback_result = (*widget->cb)(-2, x, y);
                if (cur < 0)
                    cur = callback_result;
                hit = callback_result == cur ? 1 : 0;
            } else
                hit = area_proc(widget, 2, x, y) ? 1 : 0;
            if (was != hit) {
                if (widget->cb)
                    (*widget->cb)(-3, cur, was);
                was = hit;
            }
        } while (!inp_clicked(&x, &y));
        if (was && widget->cb)
            (*widget->cb)(-4, cur);
        return was;
    case 4:
        if (widget->cb)
            (*widget->cb)(widget - dl_items, message_arg_a, message_arg_b);
        break;
    default:
        return ui_item_proc(widget, msg, message_arg_a, message_arg_b);
    }
    return 0;
}

int far label_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b)     /* 2aa6:1119 */
{
    switch (msg) {
    case 1:
        widget->flags |= 0x80;
        if (!(widget->flags & 2))
            text_print_at(widget->row, widget->col, widget->color ? widget->color : 0xf0, widget->text, widget->len);
        break;
    default:
        return ui_item_proc(widget, msg, message_arg_a, message_arg_b);
    }
    return 0;
}

int far key_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b)       /* 2aa6:1183 */
{
    if (msg == 5 && widget->cb && !(widget->flags & 3))
        return (*widget->cb)(widget - dl_items, message_arg_a);
    if (msg == 4)
        return 0;
    return ui_item_proc(widget, msg, message_arg_a, message_arg_b);
}

void far dl_init(int max)                                        /* 2aa6:11eb */
{
    max_items = max;
    if ((item_pool = mem_alloc(max * sizeof(struct Wid))) == 0) {
        con_printf("Insufficient memory in DLInit\n");
        dos_call(0x4c01);
    }
    dl_running = dl_drawn = 0;
    next_rep = 0;
}

/* Empty mirror of dl_init, called first on the exit path. */
void far dl_done(void)                                            /* 2aa6:1236 */
{
}

ItemProc far ui_set_hook(int kind, ItemProc handler)                  /* 2aa6:1237 */
{
    ItemProc old;

    old = dl_procs[kind - 1];
    if (handler)
        dl_procs[kind - 1] = handler;
    return old;
}

void far dl_begin(void)                                      /* 2aa6:1277 */
{
    dl_items = item_pool;
    dl_used = 0;
    dl_running = 1;
    dl_drawn = 0;
}

void far ui_add(int option_code, ...)                                     /* 2aa6:1294 */
{
    struct Wid far *widget;
    int far *argument_cursor;

    argument_cursor = &option_code;
    widget = dl_items + dl_used - 1;
    for (;;) {
        switch (option_code = *argument_cursor++) {
        case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
            if (dl_used >= max_items) {
                error_box("To many dialog items (%d/%d)", dl_used, max_items);
                return;
            }
            dl_used++;
            widget++;
            mem_fill(widget, sizeof(struct Wid), 0);
            if (option_code == 8) {
                widget->fn = *((ItemProc far *)argument_cursor)++;
                break;
            }
            widget->fn = dl_procs[option_code - 1];
            if (option_code == 2) {
        case 0x2a:
                widget->len = *argument_cursor++;
                break;
            }
            if (option_code == 7) {
        case 0x22:
                widget->cb = *((ItemCb far *)argument_cursor)++;
                break;
            }
            widget->row = *argument_cursor++;
            widget->col = *argument_cursor++;
            if (option_code == 5) {
                widget->len = *argument_cursor++;
        case 0x24:
                widget->width = *argument_cursor++;
            } else
                widget->text = *((char far * far *)argument_cursor)++;
            break;
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
        case 0x16:
            widget->flags |= 1 << (option_code - 0x10);
            break;
        case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d:
        case 0x1e:
            widget->flags &= ~(1 << (option_code - 0x10));    /* -0x10 as shipped: clears nothing */
            break;
        case 0x20:
            widget->key1 = *argument_cursor++;
            break;
        case 0x21:
            widget->key2 = *argument_cursor++;
            break;
        case 0x23:
            widget->val = *((int far * far *)argument_cursor)++;
            break;
        case 0x25:
            widget->glyph = *argument_cursor++;
            break;
        case 0x26:
            widget->color = *argument_cursor++;
            break;
        case 0x27:
            widget->text = *((char far * far *)argument_cursor)++;
            break;
        case 0x28:
            widget->row = *argument_cursor++;
            widget->col = *argument_cursor++;
            break;
        case 0x29:
            widget->cursor = *argument_cursor++;
            break;
        case 0:
            return;
        }
    }
}

void far dl_draw(char all)                                  /* 2aa6:151f */
{
    struct Wid far *widget;
    register int index;

    if (!dl_drawn) {
        ui_send(0, dl_used - 1, 0);
        dl_drawn = 1;
    }
    for (index = 0, widget = dl_items; index < dl_used; index++, widget++)
        if (all || !(widget->flags & 0x80))
            (*widget->fn)(widget, 1);
}

int far ui_run(int (far *idle)(void))                            /* 2aa6:1586 */
{
    register int return_code;

    if (!dl_running)
        error_box("not in dialog in DLDialog");
    else if (!dl_drawn) {
        ui_send(0, dl_used - 1, 0);
        dl_drawn = 1;
    }
    while ((return_code = ui_poll()) < 0 && (idle == 0 || (return_code = (*idle)()) < 0))
        ;
    return return_code;
}

int far ui_poll(void)                                            /* 2aa6:15e3 */
{
    struct Wid far *widget;
    int x;
    int y;
    register int index;
    register int pointer_value;

    set_text_mode(1);
    dl_draw(0);
    pointer_value = inp_press(7, &x, &y);
    for (index = 0, widget = dl_items; index < dl_used && !(*widget->fn)(widget, 2, x, y); index++, widget++)
        ;
    if (pointer_value) {
        if (index < dl_used) {
            if ((*widget->fn)(widget, 3, x, y)) {
                (*widget->fn)(widget, 4, x, y);
                if (widget->flags & 0x10)
                    return index;
                widget->flags &= 0x7f;
            }
        } else
            beep();
    }
    if (index < dl_used) {
        if (widget->cursor)
            pointer_value = widget->cursor;
        else
            pointer_value = prompt_icon < 0 ? 1 : prompt_frame;
    } else
        pointer_value = prompt_icon < 0 ? 2 : prompt_frame + 1;
    if (pointer_value >= 0)
        pointer_glyph_load(prompt_icon < 0 ? 0 : prompt_icon, pointer_value);
    else
        pointer_shape_set(0);
    if (!kbd_waiting())
        return -1;
    pointer_value = kbd_read();
    for (index = 0, widget = dl_items; index < dl_used && !(*widget->fn)(widget, 5, pointer_value); index++, widget++)
        ;
    if (index < dl_used) {
        (*widget->fn)(widget, 0x13);
        (*widget->fn)(widget, 1);
        (*widget->fn)(widget, 4, -1);
        (*widget->fn)(widget, 0x1b);
        if (widget->flags & 0x10)
            return index;
    } else
        beep();
    return -1;
}

int far ui_track(void)                                           /* 2aa6:17b9 */
{
    struct Wid far *widget;
    int x;
    int y;
    register int index;
    register int pointer_value;

    inp_pos(&x, &y);
    for (index = 0, widget = dl_items; index < dl_used && !(*widget->fn)(widget, 2, x, y); index++, widget++)
        ;
    if (index < dl_used) {
        if (widget->cursor)
            pointer_value = widget->cursor;
        else
            pointer_value = prompt_icon < 0 ? 1 : prompt_frame;
    } else {
        pointer_value = prompt_icon < 0 ? 2 : prompt_frame + 1;
        index = -1;
    }
    if (pointer_value >= 0)
        pointer_glyph_load(prompt_icon < 0 ? 0 : prompt_icon, pointer_value);
    else
        pointer_shape_set(0);
    return index;
}

void far dl_end(void)                                      /* 2aa6:1875 */
{
    ui_send(0, dl_used - 1, 6);
    dl_running = 0;
}

void far ui_mark(int id, int msg, int message_arg_a, int message_arg_b)                  /* 2aa6:188f */
{
    if (!dl_running)
        return;
    if (id < 0 || id >= dl_used)
        error_box("DLItem index %d/%d invalid", id, dl_used);
    else
        (*dl_items[id].fn)(&dl_items[id], msg, message_arg_a, message_arg_b);
}

void far ui_send(int first, int last, int msg, int message_arg_a, int message_arg_b)     /* 2aa6:18eb */
{
    if (!dl_running)
        return;
    for (; first <= last; first++)
        (*dl_items[first].fn)(&dl_items[first], msg, message_arg_a, message_arg_b);
}

int far ui_query(int id, int what)                               /* 2aa6:1931 */
{
    switch (what) {
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
    case 0x16:
        return (char)(dl_items[id].flags & (1 << (what - 0x10)) ? 1 : 0);
    case 0x24:
        return dl_items[id].width;
    }
    return 0;
}
