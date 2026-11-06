/* DQK.EXE overlay 06 module 2999: the cursor mark drawn over the party's cell.
 *
 * EXE 0x04421f..0x0442b2, one Turbo C large-model _TEXT segment starting at
 * byte f of paragraph 2999, in RTLink section 6 (payload 0x402c0, link
 * segment 25a4).  Its first 15 bytes of paragraph 2999 are the tail of module
 * 291e.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined in
 * shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  Callee names are placeholders chosen from call shapes: rows_save and
 * rows_restore are twins taking (row, col, rows, cols) in cells.
 *
 * Cell coordinates are biased by 0x1f40, the form fill_rect and draw_glyph
 * take them in; game_state holds the party's zero-based column and row
 * at +25 and +26.
 */

#include "dqk.h"

/* The translated Pascal unit's empty initialization block (DKK mapcurs.pas). */
void far cursor_unit_init(void)                                         /* 2999:000f */
{
}

void far cursor_stub2(void)                                         /* 2999:0010 */
{
}

void far cursor_show(void)                                          /* 2999:0011 */
{
    rows_save((game_state[STATE_WILDERNESS_Y] + 1) * 4 + 0x1f40,
              (game_state[STATE_WILDERNESS_X] + 1) * 4 + 0x1f40, 4, 4);
    draw_glyph((game_state[STATE_WILDERNESS_Y] + 1) * 4 + 0x1f40,
               (game_state[STATE_WILDERNESS_X] + 1) * 4 + 0x1f40, 0, 0x1d);
}

void far cursor_hide(void)                                          /* 2999:0072 */
{
    rows_restore((game_state[STATE_WILDERNESS_Y] + 1) * 4 + 0x1f40,
                 (game_state[STATE_WILDERNESS_X] + 1) * 4 + 0x1f40, 4, 4);
}
