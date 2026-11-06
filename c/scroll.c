/* DQK.EXE resident module 21e9: the scroll bar, a dialog item of module
 * 2aa6's kind.  scroll_proc is its handler: it draws the bar (the track
 * glyph and an arrow at each end), hit-tests it, and on a press repeats an
 * arrow or a page step while the button is held, or drags the thumb.  The
 * item's val points at the position, width holds the count of positions
 * and len the track's length; flags bit 0x40 makes the bar horizontal.
 * thumb_pos maps a position to the thumb's pixel coordinate, thumb_move
 * erases and redraws the thumb, and scroll_fix clamps the position and
 * moves the thumb to it.  An arrow or page step calls the item's cb with
 * the item's index and the step (-1 or 1 for an arrow, -2 or 2 for a page),
 * a drag that moved the thumb with the index and 0.  Messages it does not
 * take go to dialog.c's default handler, ui_item_proc.
 *
 * EXE 0x014290..0x014f7d, one Turbo C large-model _TEXT segment starting at
 * paragraph 21e9.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * diskfile.c's last routine ends at 0x014290.  scroll_proc runs past
 * paragraph 22b7: its retf is 22b7:000c, and module 22b7 starts at
 * 22b7:000d, which three shipped far calls name.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  A name another module
 * gives an address is reused from there; the rest are placeholders chosen
 * from use.  Nothing outside the module calls its first five routines;
 * module 1b05 passes scroll_proc as an item handler at 0x00de47, 0x00deac
 * and 0x00df0b.  The module has no _DATA and no _BSS; it reads dialog.c's
 * dl_items.
 */

#include "dqk.h"

int far thumb_pos(struct Wid far *widget, int value)                      /* 21e9:0000 */
{
    long scaled_position;
    int lo;
    int x;
    int hi;
    int y;

    text_xy(widget->row, widget->col, &lo, &x);
    if (widget->flags & 0x40) {
        text_offset_xy(lo, x, 0, widget->len + 0x1f40, &y, &hi);
        lo = x;
    } else
        text_offset_xy(lo, x, widget->len + 0x1f40, 0, &hi, &y);
    if (widget->width > 1) {
        scaled_position = hi - lo;
        scaled_position *= value;
        scaled_position /= widget->width - 1;
        scaled_position += lo;
        return scaled_position;
    }
    return lo;
}

void far thumb_move(struct Wid far *widget, int from, int destination_position)         /* 21e9:00dc */
{
    int row;
    int col;
    int lim_row;
    int lim;

    if (destination_position == from)
        return;
    if (from >= 0) {
        if (widget->flags & 0x40) {
            row = widget->row;
            col = from;
            text_xy(0, widget->col, &lim_row, &lim);
            if (screen_cols() < 600) {
                if (col <= lim)
                    col = (col & ~7) + 4;
                else
                    col = ((col - 5) & ~7) + 4;
            } else {
                col &= ~1;
                if (col <= lim)
                    col = (col & ~0xf) + 8;
                else
                    col = ((col - 10) & ~0xf) + 8;
            }
            draw_glyph(row, col, 0, 0x3e);
        } else {
            row = from;
            text_offset_xy(from, 0x1f40, 0x1f44, 0, &row, &col);
            clip_set(from, 0x1f40, row, 0x1fe0);
            draw_glyph(widget->row, widget->col, widget->glyph >> 10, widget->glyph & 0x3ff);
            clip_reset();
        }
    }
    if (widget->flags & 0x40) {
        if (screen_cols() < 600)
            draw_glyph(widget->row, destination_position, 0, (destination_position & 7) + 0x36);
        else
            draw_glyph(widget->row, destination_position & ~1, 0, (destination_position >> 1 & 7) + 0x36);
    } else
        draw_glyph(destination_position, widget->col, widget->glyph ? widget->glyph >> 10 : 0, 0x13);
}

void far scroll_fix(struct Wid far *widget, register int old)         /* 21e9:027a */
{
    register int pos;

    pos = *widget->val;
    if (pos >= widget->width)
        *widget->val = pos = widget->width - 1;
    if (pos < 0)
        *widget->val = pos = 0;
    if (pos != old)
        thumb_move(widget, old >= 0 ? thumb_pos(widget, old) : -1, thumb_pos(widget, pos));
}

/* An arrow (glyph >= 0) is drawn at row, col; a page step (glyph < 0)
 * passes the two ends of the track region it covers instead. */
char far arrow_track(struct Wid far *widget, register int row, int col,
                     register int glyph, int step)               /* 21e9:02f3 */
{
    long next;
    int pos;
    int x;
    int y;
    int top;
    int left;
    int bottom;
    int right;
    char was;
    char hit;

    was = 1;
    if (glyph >= 0) {
        draw_glyph(row, col, 0, glyph + 1);
        glyph_bounds(row, col, 0, glyph, &top, &left, &bottom, &right);
    } else if (widget->flags & 0x40) {
        text_xy(widget->row, row, &top, &left);
        text_xy(widget->row + 6, col, &bottom, &right);
    } else {
        text_xy(row, widget->col, &top, &left);
        text_xy(col, widget->col + 6, &bottom, &right);
    }
    next = timer_ticks();
    x = top;
    y = left;
    do {
        hit = top <= x && x < bottom && left <= y && y < right ? 1 : 0;
        if (was != hit) {
            was = hit;
            if (glyph >= 0)
                draw_glyph(row, col, 0, (was ? 1 : 0) + glyph);
        }
        if (was && timer_ticks() >= next) {
            next = timer_ticks() + 7;
            pos = *widget->val;
            if (widget->cb)
                (*widget->cb)(widget - dl_items, step);
            scroll_fix(widget, pos);
            if (glyph < 0) {
                pos = thumb_pos(widget, *widget->val);
                if (widget->flags & 0x40) {
                    if (step < 0)
                        right = pos;
                    else
                        text_offset_xy(0, pos, 0, 0x1f44, &pos, &left);
                } else {
                    if (step < 0)
                        bottom = pos;
                    else
                        text_offset_xy(pos, 0, 0x1f44, 0, &top, &pos);
                }
            }
        }
    } while (!inp_clicked(&x, &y));
    if (was && glyph >= 0)
        draw_glyph(row, col, 0, glyph);
    return was;
}

char far thumb_drag(struct Wid far *widget, int off)                  /* 21e9:0570 */
{
    long scaled_position;
    int top;
    int left;
    int bottom;
    int right;
    int pos;
    int skip;
    int lo;
    int hi;
    int x;
    int y;
    char button_released;
    register int last;
    register int orig;

    if (widget->flags & 0x40) {
        text_xy(widget->row - 8, widget->col - 0x14, &top, &left);
        text_xy(widget->row + 0xe, widget->col + widget->len + 0x14, &bottom, &right);
        text_xy(0, widget->col, &skip, &lo);
        text_xy(0, widget->col + widget->len, &skip, &hi);
        text_xy(widget->row, 0, &skip, &pos);    /* pos is set again before any read */
    } else {
        text_xy(widget->row - 0x14, widget->col - 6, &top, &left);
        text_xy(widget->row + widget->len + 0x14, widget->col + 0xa, &bottom, &right);
        text_xy(widget->row, 0, &lo, &skip);
        text_xy(widget->row + widget->len, 0, &hi, &skip);
    }
    orig = last = thumb_pos(widget, *widget->val);
    do {
        button_released = inp_clicked(&x, &y);
        if (x < top || x >= bottom || y < left || y >= right)
            pos = orig;
        else {
            pos = (widget->flags & 0x40) ? y : x;
            pos = int_max(lo, int_min(hi, pos + off));
        }
        if (pos != last) {
            thumb_move(widget, last, pos);
            last = pos;
        }
    } while (!button_released);
    if (pos != orig) {
        scaled_position = widget->width - 1;
        scaled_position *= pos - lo;
        scaled_position += (hi - lo) >> 1;
        scaled_position /= hi - lo;
        *widget->val = scaled_position;
        if (widget->cb)
            (*widget->cb)(widget - dl_items, 0);
        if ((last = thumb_pos(widget, *widget->val)) != pos)
            thumb_move(widget, pos, last);
        return 1;
    }
    return 0;
}

int far scroll_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b)    /* 21e9:082d */
{
    struct Wid far *widget_cursor;
    int relative_row;
    int relative_col;
    int thumb_boundary;
    char horiz;
    register int thumb_position;

    widget_cursor = widget;
    horiz = (widget_cursor->flags & 0x40) ? 1 : 0;
    switch (msg) {      /* 2e 3b 07 at 085a: Turbo C's sparse-switch value search */
    case 0:
        if (*widget_cursor->val < 0)
            *widget_cursor->val = 0;
        else if (*widget_cursor->val >= widget_cursor->width)
            *widget_cursor->val = widget_cursor->width;         /* scroll_fix clamps to width - 1 */
        break;
    case 1:
        widget_cursor->flags |= 0x80;
        draw_glyph(widget_cursor->row, widget_cursor->col, widget_cursor->glyph >> 10, widget_cursor->glyph & 0x3ff);
        if (horiz) {
            draw_glyph(widget_cursor->row, widget_cursor->col - 6, 0, 0x17);
            draw_glyph(widget_cursor->row, widget_cursor->col + widget_cursor->len + 2, 0, 0x15);
        } else {
            draw_glyph(widget_cursor->row - 5, widget_cursor->col, 0, 0xc);
            draw_glyph(widget_cursor->row + widget_cursor->len + 5, widget_cursor->col, 0, 0xe);
        }
        scroll_fix(widget_cursor, -1);
        break;
    case 2:
        text_xy_to(widget_cursor->row, widget_cursor->col, message_arg_a, message_arg_b, &relative_row, &relative_col);
        if (horiz)
            return relative_row >= 0 && relative_row < 4 && relative_col >= -6 && widget_cursor->len + 10 > relative_col;
        return relative_row >= -6 && widget_cursor->len + 10 > relative_row && relative_col >= 0 && relative_col < 4;
    case 3:
        text_xy_to(widget_cursor->row, widget_cursor->col, message_arg_a, message_arg_b, &relative_row, &relative_col);
        if (horiz) {
            if (relative_col < 0)
                return arrow_track(widget_cursor, widget_cursor->row, widget_cursor->col - 6, 0x17, -1);
            if (widget_cursor->len + 4 < relative_col)
                return arrow_track(widget_cursor, widget_cursor->row, widget_cursor->col + widget_cursor->len + 2, 0x15, 1);
            thumb_position = thumb_pos(widget_cursor, *widget_cursor->val);
            text_offset_xy(0, thumb_position, 0, 0x1f44, &relative_row, &thumb_boundary);
            message_arg_a = message_arg_b;
            relative_row = widget_cursor->col;
        } else {
            if (relative_row < 0)
                return arrow_track(widget_cursor, widget_cursor->row - 5, widget_cursor->col, 0xc, -1);
            if (widget_cursor->len + 4 < relative_row)
                return arrow_track(widget_cursor, widget_cursor->row + widget_cursor->len + 5, widget_cursor->col, 0xe, 1);
            thumb_position = thumb_pos(widget_cursor, *widget_cursor->val);
            text_offset_xy(thumb_position, 0, 0x1f44, 0, &thumb_boundary, &relative_col);
            relative_row = widget_cursor->row;
        }
        if (message_arg_a < thumb_position)
            return arrow_track(widget_cursor, relative_row, thumb_position, -1, -2);
        if (message_arg_a >= thumb_boundary)
            return arrow_track(widget_cursor, thumb_boundary, widget_cursor->len + relative_row + 4, -1, 2);
        return thumb_drag(widget_cursor, thumb_position - message_arg_a);
    case 4:
    case 5:
        break;
    case 0x20:
        relative_row = *widget_cursor->val;
        if (message_arg_a == relative_row)
            break;
        *widget_cursor->val = message_arg_a;
    moved:                              /* 21e9:0c0d; 0x23 jumps here from 0c4d */
        scroll_fix(widget_cursor, relative_row);
        break;
    case 0x23:
        relative_row = *widget_cursor->val;
        widget_cursor->val = *(int far * far *)&message_arg_a;
        if (*widget_cursor->val == relative_row)
            break;
        goto moved;
    case 0x24:
        if (message_arg_a < 0)
            message_arg_a = 0;
        if (widget_cursor->width == message_arg_a)
            break;
        relative_row = thumb_pos(widget_cursor, *widget_cursor->val);
        widget_cursor->width = message_arg_a;
        if (*widget_cursor->val >= message_arg_a)
            *widget_cursor->val = message_arg_a - 1;
        thumb_move(widget_cursor, relative_row, thumb_pos(widget_cursor, *widget_cursor->val));
        break;
    default:
        return ui_item_proc(widget_cursor, msg, message_arg_a, message_arg_b);
    }
    return 0;
}
