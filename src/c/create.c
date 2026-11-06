/* DQK.EXE overlay 05 module 25a4: creating a party member.  The pick
 * screen for race, gender, class and alignment, the ability rolls and their
 * racial, age and class limits, the name, a cleric's deity, the icon grid,
 * and the new member's starting levels, spells and items.
 *
 * EXE 0x03a4c0..0x03deef, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 25a4, the first module of RTLink section 5 (payload
 * 0x3a4c0, link segment 25a4, parent section 3).  Its final retf is byte e
 * of paragraph 2946, where modify.c begins at 2946:000f (section 5's table
 * files every fixup site before it under frame 25a4, and every one in
 * modify.c under 2946).  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Sections 4, 6, 7 and 8 hold other bytes at link segment 25a4.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, an ovlNN_ name which section and stored
 * segment, and a mod1d99_ callee is an RTLink vector stub whose target is
 * named beside it.  Code outside the section reaches three of these
 * routines through vector stubs: 1d99:139b -> 0001, 1d99:13ff -> 0cf1,
 * 1d99:1409 -> 2f95; modify.c far-calls 30d3 directly.
 *
 * The routines pair with DKK's party.pas (dkk-disassembly): hero_create and
 * the routines it calls with createCharacter, icon_default with
 * setDefaultIcon, icon_customize and its callbacks with customizeIcon,
 * gear_start with createStartingItems.  align_default, gear_start,
 * deity_pick and deity_apply are static: each is near-called ahead of its
 * definition.
 *
 * _DATA is align_base and 55 stored literals, DGROUP 7276..7488; -d stores
 * "" as the NUL of "DUNGCOM1", "Fighter" as the tail of "Cleric/Fighter",
 * and each literal used more than once once.  _BSS is 30 statics, DGROUP
 * c74e..c78e; their names are chosen to fit Turbo C's name order, and
 * spare_word and unused_byte exist only to hold the layout.
 */

#include "dqk.h"

struct Choices {                /* the pick screen's radio groups         */
    int race;                   /* +0  from 1                             */
    unsigned law;               /* +2  from 1: lawful, neutral, chaotic   */
    unsigned good;              /* +4  from 1: good, neutral, evil        */
    int cls;                    /* +6  from 1                             */
    int gender;                 /* +8  from 1                             */
};

extern void far ui_mark(int id, int attr, ...);                  /* 1aa6:188f */

static unsigned char align_base = 0;    /* 7276  class_aligns row: 0x11 for dwarves */

static int key_y;                                               /* c74e */
static int key_col;                                              /* c750 */
static char align_ok;                                            /* c752 */
static int icon_sel;                                             /* c753 */
static int title_hue;                                            /* c755 */
static struct Character far *new_hero;                                /* c757 */
static unsigned char law_try;                                    /* c75b */
static unsigned char good_scan;                                  /* c75c */
static int spare_word;                                           /* c75d */
static struct Character far *work_hero;                               /* c75f */
static int list_attr;                                            /* c763 */
static struct Character far *char_before;                             /* c765 */
static char icon_closed;                                         /* c769 */
static int icon_left;                                            /* c76a */
static struct Node far *cur_deity;                               /* c76c */
static char unused_byte;                                         /* c770 */
static unsigned char align_total;                                /* c771 */
static int icon_row_no;                                          /* c772 */
static int stat_button;                                          /* c774 */
static int patron_row;                                           /* c776 */
static struct Node far *deity_nodes;                             /* c778 */
static char patron_box;                                          /* c77c */
static char moral_matched;                                       /* c77d */
static char order_matched;                                       /* c77e */
static char icon_redraw;                                         /* c77f */
static unsigned char portrait_orig;                              /* c780 */
static int byte_c5_kept;                                         /* c781 */
static char icon_cancelled;                                      /* c783 */
static unsigned char deity_blessing;                             /* c784 */
static struct Choices create_choices;                            /* c785 */

static void align_default(void);
static char deity_pick(void);
static void gear_start(struct Character far *character);
void far mu_learn(struct Character far *character);
static void deity_apply(void);

/* The translated Pascal unit's empty initialization block (DKK chargen.pas). */
void far create_unit_init(void)                                       /* 25a4:0000 */
{
}

void far icon_default(void)                                      /* 25a4:0001 */
{
    if (cur_node->gender == 0) {
        if (cur_node->race == 5) {
            switch (cur_node->cls) {
            case 2:
            case 8:
            case 9:
            case 13:
            case 14:
            case 15:
                cur_node->icon_body = 0x2b;
                break;
            case 4:
            case 10:
                cur_node->icon_body = 0x2a;
                break;
            case 6:
            case 12:
            case 16:
                cur_node->icon_body = 0x2e;
                break;
            case 0:
            case 11:
                cur_node->icon_body = 0x2f;
                break;
            case 1:
            case 3:
            case 5:
            case 7:
                cur_node->icon_body = 0;
                break;
            }
        } else if (cur_node->race == 3 || cur_node->race == 4) {
            switch (cur_node->cls) {
            case 2:
            case 3:
            case 4:
            case 8:
            case 9:
            case 10:
            case 13:
            case 14:
            case 15:
                cur_node->icon_body = 0x22;
                break;
            case 0:
            case 11:
                cur_node->icon_body = 0x25;
                break;
            case 6:
            case 12:
            case 16:
                cur_node->icon_body = 0x27;
                break;
            case 1:
            case 5:
            case 7:
                cur_node->icon_body = 0;
                break;
            }
        } else {
            switch (cur_node->cls) {
            case 2:
            case 8:
            case 9:
            case 13:
            case 14:
            case 15:
                cur_node->icon_body = 3;
                break;
            case 1:
                cur_node->icon_body = 0;
                break;
            case 3:
                cur_node->icon_body = 2;
                break;
            case 4:
            case 10:
                cur_node->icon_body = 8;
                break;
            case 6:
            case 12:
            case 16:
                cur_node->icon_body = 0x11;
                break;
            case 0:
            case 11:
                if (cur_node->alignment % 3)
                    cur_node->icon_body = 0x1a;
                else
                    cur_node->icon_body = 0x17;
                break;
            case 5:
                if (cur_node->alignment % 3)
                    cur_node->icon_body = 0x1c;
                else
                    cur_node->icon_body = 0x1b;
                break;
            case 7:
                cur_node->icon_body = 1;
                break;
            }
        }
    } else {
        if (cur_node->race == 5) {
            switch (cur_node->cls) {
            case 4:
            case 10:
                cur_node->icon_body = 0x2c;
                break;
            case 2:
            case 6:
            case 8:
            case 9:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
                cur_node->icon_body = 0x2d;
                break;
            case 0:
            case 11:
                cur_node->icon_body = 0x2b;
                break;
            case 1:
            case 3:
            case 5:
            case 7:
                cur_node->icon_body = 0;
                break;
            }
        } else if (cur_node->race == 3 || cur_node->race == 4) {
            switch (cur_node->cls) {
            case 2:
            case 3:
            case 4:
            case 8:
            case 9:
            case 10:
            case 13:
            case 14:
            case 15:
                cur_node->icon_body = 0x24;
                break;
            case 0:
            case 11:
                cur_node->icon_body = 0x26;
                break;
            case 6:
            case 12:
            case 16:
                cur_node->icon_body = 0x28;
                break;
            case 1:
            case 5:
            case 7:
                cur_node->icon_body = 0;
                break;
            }
        } else {
            switch (cur_node->cls) {
            case 2:
            case 8:
            case 9:
            case 13:
            case 14:
            case 15:
                cur_node->icon_body = 0xa;
                break;
            case 1:
                cur_node->icon_body = 0xc;
                break;
            case 3:
                cur_node->icon_body = 0xb;
                break;
            case 4:
            case 10:
                cur_node->icon_body = 0xe;
                break;
            case 6:
            case 12:
            case 16:
                cur_node->icon_body = 0x14;
                break;
            case 0:
            case 11:
                if (cur_node->alignment % 3)
                    cur_node->icon_body = 0x19;
                else
                    cur_node->icon_body = 0x18;
                break;
            case 5:
                if (cur_node->alignment % 3)
                    cur_node->icon_body = 0x20;
                else
                    cur_node->icon_body = 0x1f;
                break;
            case 7:
                cur_node->icon_body = 0;
                break;
            }
        }
    }
}

void far icon_pair(unsigned char col, unsigned char row)         /* 25a4:0372 */
{
    col = col + 7;
    row = row - 1;
    icon_draw(col, row - 1000, 0, 0, 0x1e);
    icon_draw(col + 3, row - 1000, 0, 0, 0x1f);
    icon_draw(col, row - 1000, 0, 0, 0xf);
    icon_draw(col + 3, row - 1000, 0, 1, 0xf);
    screen_flush();
}

void far icon_click(int id, int x, int y)                        /* 25a4:040e */
{
    int row;
    int col;
    short pos;

    screen_draw_begin(1);
    text_xy_to(0x1f44, 0x1f44, x, y, &row, &col);
    icon_row_no = row / 12;
    icon_left = col / 12;
    pos = icon_row_no * 7 + icon_left;
    cur_node->icon_body = icon_sel;
    if (pos != icon_sel) {
        icon_draw(icon_sel % 7, icon_sel / 7, 0, 0, 0x1e);
        character_icon_load(1);
        icon_draw(icon_sel % 7, icon_sel / 7, 0, 0, 0xf);
        cur_node->icon_body = pos;
        icon_draw(icon_left, icon_row_no, 0, 0, 0x1f);
        screen_flush();
        character_icon_load(1);
        icon_draw(icon_left, icon_row_no, 0, 0, 0xf);
        screen_flush();
    }
    ui_mark(id, 0x14);
    icon_sel = pos;
    screen_flush();
}

void far icon_refresh(void)                                      /* 25a4:0531 */
{
    character_icon_load(1);
}

void far icon_done_cb(void)                                      /* 25a4:053b */
{
    icon_closed = 1;
    icon_clear(0x1e);
    icon_clear(0x1f);
}

void far icon_exit_cb(void)                                      /* 25a4:0553 */
{
    icon_cancelled = 1;
    icon_closed = 1;
    icon_clear(0x1e);
    icon_clear(0x1f);
}

void far icon_screen(void)                                       /* 25a4:0570 */
{
    char buf[0x50];
    unsigned char saved;
    struct Res far *res;

    saved = cur_node->icon_body;
    icon_sel = saved;
    cur_node->icon_body = 0;
    if (icon_redraw) {
        screen_draw_begin(0);
        text_clear(1, 1, 0x26, 0x16);
        frame_right_panel();
        pal_select(0);
        screen_flush();
        set_open_mode(0x32);
        res = 0;
        tile_open_file("DUNGCOM1", 1, 0, 0, &res);
        res_palette_load(res);
        res_free(&res);
        for (tick_slot = 0; tick_slot <= 0x30; tick_slot++) {
            screen_draw_begin(1);
            if (tick_slot == icon_sel)
                icon_draw(tick_slot % 7, tick_slot / 7, 0, 0, 0x1f);
            else
                icon_draw(tick_slot % 7, tick_slot / 7, 0, 0, 0x1e);
            character_icon_load(1);
            icon_draw(tick_slot % 7, tick_slot / 7, 0, 0, 0xf);
            screen_flush();
            cur_node->icon_body++;
            if (cur_node->icon_body > 0x30)
                cur_node->icon_body = 0;
        }
        icon_redraw = 0;
    }
    cur_node->icon_body = saved;
    icon_clear(0xf);
    status_printf(0x17, 8, 7, 8, "%s", cur_node->name);
    if (cur_node->npc & 0x80)
        status_printf(str_len(cur_node->name) + 3, 0x17, 7, 8, "(NPC)");
    str_copy(buf, sex_names[cur_node->gender]);
    status_printf(0x17, 9, 7, 8, buf);
    str_copy(buf, race_names[cur_node->race]);
    status_printf(0x17, 0xa, 7, 8, buf);
    switch (cur_node->cls) {
    case 9:
        str_copy(buf, "Cleric/Fighter");
        status_printf(0x17, 0xc, 7, 8, buf);
        str_copy(buf, "Magic-User");
        status_printf(0x17, 0xd, 7, 8, buf);
        break;
    case 10:
        str_copy(buf, "Cleric");
        status_printf(0x17, 0xc, 7, 8, buf);
        str_copy(buf, "Ranger");
        status_printf(0x17, 0xd, 7, 8, buf);
        break;
    case 11:
        str_copy(buf, "Cleric");
        status_printf(0x17, 0xc, 7, 8, buf);
        str_copy(buf, "Magic-User");
        status_printf(0x17, 0xd, 7, 8, buf);
        break;
    case 12:
        str_copy(buf, "Cleric");
        status_printf(0x17, 0xc, 7, 8, buf);
        str_copy(buf, "Thief");
        status_printf(0x17, 0xd, 7, 8, buf);
        break;
    case 13:
        str_copy(buf, "Fighter");
        status_printf(0x17, 0xc, 7, 8, buf);
        str_copy(buf, "Magic-User");
        status_printf(0x17, 0xd, 7, 8, buf);
        break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 14:
        str_copy(buf, class_names[cur_node->cls]);
        status_printf(0x17, 0xc, 7, 8, buf);
        break;
    case 15:
        str_copy(buf, "Fighter");
        status_printf(0x17, 0xc, 7, 8, buf);
        str_copy(buf, "Magic-User");
        status_printf(0x17, 0xd, 7, 8, buf);
        str_copy(buf, "Thief");
        status_printf(0x17, 0xe, 7, 8, buf);
        break;
    case 16:
        str_copy(buf, "Magic-User");
        status_printf(0x17, 0xc, 7, 8, buf);
        str_copy(buf, "Thief");
        status_printf(0x17, 0xd, 7, 8, buf);
        break;
    }
    text_print_at(0x1f48, 0x1f9e, list_attr, "ready    action");
}

int far icon_key(int id, int key)                                /* 25a4:09e4 */
{
    short pos;
    register int moved;

    pos = icon_sel;
    key = (key & 0x7f) + 0x80;
    switch (key) {
    case 0x86:                          /* leaves moved unset, as shipped */
        if (pos % 7)
            pos = pos - 1;
        else
            pos = pos + 6;
        break;
    case 0x88:
        if (pos / 7)
            pos = pos - 7;
        else
            pos = pos + 42;
        moved = 1;
        break;
    case 0x82:
        if (pos % 7 != 6)
            pos = pos + 1;
        else
            pos = pos - 6;
        moved = 1;
        break;
    case 0x84:
        if (pos / 7 != 6)
            pos = pos + 7;
        else
            pos = pos - 42;
        moved = 1;
        break;
    default:
        moved = 0;
        break;
    }
    cur_node->icon_body = icon_sel;
    icon_draw(icon_sel % 7, icon_sel / 7, 0, 0, 0x1e);
    character_icon_load(1);
    icon_draw(icon_sel % 7, icon_sel / 7, 0, 0, 0xf);
    cur_node->icon_body = pos;
    icon_draw(pos % 7, pos / 7, 0, 0, 0x1f);
    character_icon_load(1);
    icon_draw(pos % 7, pos / 7, 0, 0, 0xf);
    ui_mark(id, 0x14);
    icon_sel = pos;
    screen_flush();
    return moved;
}

char far icon_menu(void)                                         /* 25a4:0b85 */
{
    portrait_orig = cur_node->icon_body;
    icon_closed = 0;
    icon_load("COMSPR", 0xf, 0x1e);
    icon_load("COMSPR", 0x10, 0x1f);
    list_attr = 0x87;
    set_extra_row(1);
    icon_screen();
    icon_refresh();
    icon_pair(1, 2);
    if (icon_redraw) {
        icon_row_no = portrait_orig / 7;
        icon_left = portrait_orig % 7;
        icon_sel = portrait_orig;
    }
    frame_invalidate();
    dl_begin();
    ui_add(5, 0x1f44, 0x1f44, 0x54, 0x54, 0x22, icon_click, 0);
    ui_add(1, 0x1f9e, 0x1f44, "Done", 0x14, 0x20, 'D', 0x24, 4, 0x22, icon_done_cb, 0x15,
           1, 0x1f9e, 0x1f58, "Exit", 0x14, 0x20, 'E', 0x21, 0x23, 0x24, 4, 0x22, icon_exit_cb,
           0x15,
           7, icon_key, 0);
    dl_draw(1);
    screen_flush();
    ui_run(0);
    dl_end();
    set_extra_row(0);
    icon_clear(0x1e);
    icon_clear(0x1f);
    return icon_closed;
}

char far icon_customize(void)                                    /* 25a4:0cf1 */
{
    struct Character save;
    struct Character copy;

    character_icon_load(1);
    byte_c5_kept = cur_node->order_no;
    cur_node->order_no = 0xf;
    char_before = &save;
    mem_move(cur_node, char_before, sizeof(struct Character));
    work_hero = &copy;
    mem_move(cur_node, work_hero, sizeof(struct Character));
    icon_cancelled = 0;
    icon_redraw = 1;
    while (!icon_menu())
        ;
    if (icon_cancelled)
        mem_move(char_before, cur_node, sizeof(struct Character));
    cur_node->order_no = byte_c5_kept;
    character_icon_load(1);
    return !icon_cancelled;
}

void far class_start(struct Character far *character)                         /* 25a4:0dbf */
{
    switch (character->cls) {
    case 0:
    case 2:
    case 5:
    case 6:
        character->level[character->cls] = 1;
        break;
    case 3:
        character->cure = 1;
        character->level[3] = 1;
        fx_add(character, 8, 0, 0xff, 0);
        break;
    case 4:
        character->level[4] = 1;
        fx_add(character, 0x69, 0, 0xff, 0);
        break;
    case 1:
        character->level[1] = 1;
        character->xp = 1000001L;
        character->knight_order = 1;
        break;
    case 8:
        character->level[0] = 1;
        character->level[2] = 1;
        character->xp = 500000L;
        break;
    case 9:
        character->level[0] = 1;
        character->level[2] = 1;
        character->level[5] = 1;
        character->xp = 333333L;
        break;
    case 10:
        character->level[0] = 1;
        character->level[4] = 1;
        fx_add(character, 0x69, 0, 0xff, 0);
        character->xp = 500000L;
        break;
    case 11:
        character->level[0] = 1;
        character->level[5] = 1;
        character->xp = 500000L;
        break;
    case 12:
        character->level[0] = 1;
        character->level[6] = 1;
        character->xp = 500000L;
        break;
    case 13:
        character->level[2] = 1;
        character->level[5] = 1;
        character->xp = 500000L;
        break;
    case 14:
        character->level[2] = 1;
        character->level[6] = 1;
        character->xp = 500000L;
        break;
    case 15:
        character->level[2] = 1;
        character->level[5] = 1;
        character->level[6] = 1;
        character->xp = 333333L;
        break;
    case 16:
        character->level[5] = 1;
        character->level[6] = 1;
        character->xp = 500000L;
        break;
    }
}

void far stat_adjust(short stat, struct Character far *character)             /* 25a4:0f47 */
{
    switch (stat) {
    case 0:
        if (character->stats[stat].cur > 0 && character->age > age_limits[character->race].age[1])
            character->stats[stat].cur++;
        if (character->stats[stat].cur > 0 && character->age > age_limits[character->race].age[2])
            character->stats[stat].cur--;
        if (character->stats[stat].cur > 0 && character->age > age_limits[character->race].age[3])
            character->stats[stat].cur -= 2;
        if (character->stats[stat].cur > 0 && character->age > age_limits[character->race].age[4])
            character->stats[stat].cur--;
        if (character->stats[stat].cur < race_limits[character->race].str_min[character->gender])
            character->stats[stat].cur = race_limits[character->race].str_min[character->gender];
        if (character->stats[stat].cur > race_limits[character->race].str_max[character->gender])
            character->stats[stat].cur = race_limits[character->race].str_max[character->gender];
        if (character->stats[stat].cur < class_mins[character->cls].min[stat])
            character->stats[stat].cur = class_mins[character->cls].min[stat];
        if (character->stats[0].cur == 18 &&
            (character->level[2] > 0 || character->level[4] > 0 || character->level[3] > 0 || character->level[1] > 0)) {
            character->str_ex = random_n(100) + 1;
            if (character->str_ex > race_limits[character->race].exc_max[character->gender])
                character->str_ex = race_limits[character->race].exc_max[character->gender];
        }
        break;
    case 1:
        if (character->age > age_limits[character->race].age[2])
            character->stats[stat].cur++;
        if (character->age > age_limits[character->race].age[4])
            character->stats[stat].cur++;
        if (character->stats[stat].cur < race_limits[character->race].int_min)
            character->stats[stat].cur = race_limits[character->race].int_min;
        if (character->stats[stat].cur > race_limits[character->race].int_max)
            character->stats[stat].cur = race_limits[character->race].int_max;
        if (character->stats[stat].cur < class_mins[character->cls].min[stat])
            character->stats[stat].cur = class_mins[character->cls].min[stat];
        break;
    case 2:
        if (character->age <= age_limits[character->race].age[1])
            character->stats[stat].cur--;
        if (character->age > age_limits[character->race].age[2])
            character->stats[stat].cur++;
        if (character->age > age_limits[character->race].age[3])
            character->stats[stat].cur++;
        if (character->age > age_limits[character->race].age[4])
            character->stats[stat].cur++;
        if (character->stats[stat].cur < race_limits[character->race].wis_min)
            character->stats[stat].cur = race_limits[character->race].wis_min;
        if (character->stats[stat].cur > race_limits[character->race].wis_max)
            character->stats[stat].cur = race_limits[character->race].wis_max;
        if (character->stats[stat].cur < class_mins[character->cls].min[stat])
            character->stats[stat].cur = class_mins[character->cls].min[stat];
        if (character->stats[stat].cur < 13 && character->cls <= 8 && character->cls >= 12) /* never true */
            character->stats[stat].cur = 13;
        break;
    case 3:
        if (character->age > age_limits[character->race].age[3])
            character->stats[stat].cur -= 2;
        if (character->age > age_limits[character->race].age[4])
            character->stats[stat].cur--;
        if (character->stats[stat].cur < race_limits[character->race].dex_min)
            character->stats[stat].cur = race_limits[character->race].dex_min;
        if (character->stats[stat].cur > race_limits[character->race].dex_max)
            character->stats[stat].cur = race_limits[character->race].dex_max;
        if (character->stats[stat].cur < class_mins[character->cls].min[stat])
            character->stats[stat].cur = class_mins[character->cls].min[stat];
        break;
    case 4:
        if (character->age > age_limits[character->race].age[2])
            character->stats[stat].cur--;
        if (character->age > age_limits[character->race].age[3])
            character->stats[stat].cur--;
        if (character->age > age_limits[character->race].age[4])
            character->stats[stat].cur--;
        if (character->stats[stat].cur < race_limits[character->race].con_min)
            character->stats[stat].cur = race_limits[character->race].con_min;
        if (character->stats[stat].cur > race_limits[character->race].con_max)
            character->stats[stat].cur = race_limits[character->race].con_max;
        if (character->stats[stat].cur < class_mins[character->cls].min[stat])
            character->stats[stat].cur = class_mins[character->cls].min[stat];
        break;
    case 5:
        if (character->stats[stat].cur < race_limits[character->race].cha_min)
            character->stats[stat].cur = race_limits[character->race].cha_min;
        if (character->stats[stat].cur > race_limits[character->race].cha_max)
            character->stats[stat].cur = race_limits[character->race].cha_max;
        if (character->stats[stat].cur < class_mins[character->cls].min[stat])
            character->stats[stat].cur = class_mins[character->cls].min[stat];
        break;
    }
}

void far race_start(struct Character far *character)                         /* 25a4:1839 */
{
    switch (character->race) {
    case 5:
        character->icon_size = 1;
        fx_add(character, 0x5c, 0, 0xff, 0);
        fx_add(character, 0x5e, 0, 0xff, 0);
        break;
    case 3:
    case 4:
        character->icon_size = 1;
        fx_add(character, 0x5e, 0, 0xff, 0);
        fx_add(character, 0x1a, 0, 0xff, 0);
        fx_add(character, 0x2f, 0, 0xff, 0);
        break;
    case 0:
    case 1:
        character->icon_size = 2;
        fx_add(character, 0x5f, 0, 0xff, 0);
        break;
    case 2:
        character->icon_size = 2;
        fx_add(character, 0x12, 0, 0xff, 0);
        break;
    default:
        character->icon_size = 2;
        break;
    }
}

char far name_input(struct Character far *character)                          /* 25a4:193f */
{
    char buf[0x10];

    do {
        str_copy(character->name, input_text("Character name:", 0, 0, 0xf));
        str_copy(buf, character->name);
        name_clean(buf);
    } while (buf[0] == 0 && input_result() || buf[0] >= '0' && buf[0] <= '9');
    return input_result();
}

int far stats_menu(void)                                         /* 25a4:19b8 */
{
    frame_invalidate();
    dl_begin();
    ui_add(1, 0x1f9e, 0x1f94, "Done", 0x14, 0x20, 'D', 0x24, 4, 0x15,
           1, 0x1f9e, 0x1f44, "Reroll Stats", 0x14, 0x20, 'R', 0x24, 0xc, 0x15,
           1, 0x1f9e, 0x1f78, "Modify", 0x14, 0x20, 'M', 0x24, 6, 0x15,
           1, 0x1f9e, 0x1fa8, "Exit", 0x14, 0x20, 'E', 0x21, 0x23, 0x24, 4, 0x15,
           0);
    stat_button = ui_run(0);
    dl_end();
    return stat_button;
}

void far stats_roll(struct Character far *character)                          /* 25a4:1a90 */
{
    short stat_index;
    unsigned char scan_index;
    unsigned char roll;
    char count;
    unsigned char saved;
    char any;
    char buf[0x2a];
    short class_index;
    char train_saved;

    for (class_index = 0; class_index <= 6; class_index++)
        if (cur_node->level[class_index] > 0)
            cur_node->level[class_index] = 1;
    character->str_ex = 0;
    for (stat_index = 0; stat_index <= 5; stat_index++) {
        character->stats[stat_index].cur = 0;
        for (scan_index = 1; scan_index <= 6; scan_index++) {
            roll = fx_roll_dice(3, 6) + 1;
            switch (character->race) {
            case 3:
            case 4:
                switch (stat_index) {
                case 4:
                    roll++;
                    break;
                case 5:
                    roll--;
                    break;
                }
                break;
            case 0:
            case 1:
                switch (stat_index) {
                case 3:
                    roll++;
                    break;
                case 4:
                    roll--;
                    break;
                }
                break;
            case 2:
                switch (stat_index) {
                case 3:
                    roll += 2;
                    break;
                }
                break;
            case 5:
                switch (stat_index) {
                case 0:
                    roll--;
                    break;
                case 3:
                    roll += 2;
                    break;
                }
                break;
            }
            if (character->stats[stat_index].cur < roll)
                character->stats[stat_index].cur = roll;
        }
        stat_adjust(stat_index, character);
        character->stats[stat_index].base = character->stats[stat_index].cur;
        if (stat_index == 0)
            character->str_ex_base = character->str_ex;
        stat_show(stat_index, 0);
    }
    character->hp = character->hp_max;
    character->attack_rate[0] = 2;
    character->unarmed_rolls[0] = 1;
    character->unarmed_dice[0] = 2;
    character->move_base = 0xc;
    count = 0;
    switch (character->alignment) {
    case 0:
    case 3:
    case 6:
        if (character->level[5] > 0)
            character->robe_order = 1;
        break;
    case 1:
    case 4:
    case 7:
        if (character->level[5] > 0)
            character->robe_order = 2;
        break;
    }
    mem_fill(character->slots, 0x24, 0);
    for (class_index = 0; class_index <= 6; class_index++) {
        if (character->level[class_index] > 0) {
            switch (class_index) {
            case 0:
                if (character->deity <= 4)
                    character->slots[0][0] = 1;
                else
                    character->slots[0][0] = 2;
                break;
            case 5:
                character->slots[2][0] = 1;
                break;
            }
            switch (class_index) {
            case 0:
                recalc_cleric_slots(character);
                for (tick_slot = 1, scan_index = 0xff; tick_slot <= 10 && scan_index != 0; tick_slot++) {
                    scan_index = spells_by_level[0][0][tick_slot - 1];
                    if (scan_index != 0)
                        character->known[(scan_index - 1) / 8] = character->known[(scan_index - 1) / 8] | bit_mask[(scan_index - 1) % 8];
                }
                break;
            case 5:
                character->known[2] = character->known[2] | bit_mask[2];
                character->known[1] = character->known[1] | bit_mask[2];
                character->known[1] = character->known[1] | bit_mask[3];
                character->known[1] = character->known[1] | bit_mask[6];
                character->known[1] = character->known[1] | bit_mask[7];
                character->known[2] = character->known[2] | bit_mask[1];
                character->known[2] = character->known[2] | bit_mask[2];
                break;
            }
            count++;
        }
    }
    character->coins[0] = 0xc8;
    character->hp_max = 0;
    character->hp_base = 0;
    character->hp = 0;
    hp_adjust(character, 0xff, count, 1);
    saved = game_state[STATE_TRAINING_CLASSES];
    game_state[STATE_TRAINING_CLASSES] = 0xff;
    training_done = 0;
    set_extra_row(1);
    train_saved = free_train;
    free_train = 0;
    do
        train_hero();
    while (!training_done);
    free_train = train_saved;
    game_state[STATE_TRAINING_CLASSES] = saved;
    set_extra_row(0);
    any = 0;
    buf[0] = 0;
    for (class_index = 0; class_index <= 6; class_index++) {
        if (cur_node->level[class_index] > 0 ||
            first_level(cur_node) > cur_node->old_level[class_index] && cur_node->old_level[class_index] > 0) {
            if (any)
                str_cat(buf, "/");
            str_cat(buf, byte_str(cur_node->level[class_index] + cur_node->old_level[class_index]));
            any = 1;
        }
    }
    str_cat(buf, " ");
    status_printf(7, 7, 0xb, 0, buf);
    combat_show();
    money_show();
}

char far hero_finish(struct Character far *character)                         /* 25a4:1f25 */
{
    short stat_index;

    switch (character->alignment) {
    case 0:
    case 3:
    case 6:
        if (character->level[5] > 0)
            character->robe_order = 1;
        break;
    case 1:
    case 4:
    case 7:
        if (character->level[5] > 0)
            character->robe_order = 2;
        break;
    }
    if (character->cls <= 6)
        character->age = fx_roll_dice(start_ages[character->race][character->cls].dice,
                              start_ages[character->race][character->cls].sides) +
                 start_ages[character->race][character->cls].base + 10;
    else
        switch (character->cls) {
        case 14:
            character->age = start_ages[character->race][2].sides * start_ages[character->race][2].dice +
                     start_ages[character->race][2].base;
            break;
        case 13:
        case 15:
        case 16:
            character->age = start_ages[character->race][5].sides * start_ages[character->race][5].dice +
                     start_ages[character->race][5].base;
            break;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            character->age = start_ages[character->race][0].sides * start_ages[character->race][0].dice +
                     start_ages[character->race][0].base;
            break;
        }
    cur_node = character;
    char_status();
    stats_roll(character);
    do {
        stats_menu();
        if (stat_button == 2) {
            set_extra_row(1);
            modify_hero();
            set_extra_row(0);
            char_status();
        }
        if (stat_button == 1) {
            stats_roll(character);
            input_pending_clear();
        }
    } while (stat_button != 3 && stat_button != 0);
    if (stat_button == 3)
        return 0;
    char_status();
    for (stat_index = 0; stat_index <= 5; stat_index++)
        character->stats[stat_index].base = character->stats[stat_index].cur;
    character->str_ex_base = character->str_ex;
    return 1;
}

void far hero_setup(void)                                        /* 25a4:220a */
{
    register int class_index;

    race_start(new_hero);
    new_hero->xp = 1000001L;
    new_hero->level_high = 1;
    class_start(new_hero);
    if (new_hero->level[6] > 0)
        recalc_thief_skills(new_hero);
    new_hero->class_mask = 0;
    new_hero->thac0_base = 0;
    for (class_index = 0; class_index <= 6; class_index++) {
        if (new_hero->level[class_index] > 0) {
            if (attack_scores_by_level[class_index][new_hero->level[class_index]] > new_hero->thac0_base)
                new_hero->thac0_base = attack_scores_by_level[class_index][new_hero->level[class_index]];
            new_hero->class_mask += class_mask_bits[class_index];
        }
    }
    recalc_saves(new_hero);
    cur_node = new_hero;
}

void far pick_exit(void)                                         /* 25a4:2306 */
{
    icon_cancelled = 1;
}

void far align_set(void)                                         /* 25a4:230c */
{
    new_hero->alignment = (create_choices.law - 1) * 3 + create_choices.good - 1;
}

void far align_fix(void)                                         /* 25a4:2323 */
{
    char found;

    align_total = class_aligns[new_hero->cls + align_base].count;
    align_ok = 0;
    for (tick_slot = 0; tick_slot < align_total && !align_ok; tick_slot++) {
        new_hero->alignment = (create_choices.law - 1) * 3 + create_choices.good - 1;
        if (class_aligns[new_hero->cls + align_base].aligns[tick_slot] == new_hero->alignment)
            align_ok = 1;
    }
    if (align_ok)
        return;
    found = 0;
    for (create_choices.good = 1; create_choices.good <= 2 && !found; create_choices.good++) {
        new_hero->alignment = (create_choices.law - 1) * 3 + create_choices.good - 1;
        for (tick_slot = 0; tick_slot < align_total && !found; tick_slot++)
            if (class_aligns[new_hero->cls + align_base].aligns[tick_slot] == new_hero->alignment)
                found = 1;
    }
    create_choices.good--;
    if (!found)
        align_default();
}

void far align_mark(void)                                        /* 25a4:2459 */
{
    char hit;
    unsigned char scan_index;
    unsigned char alignment_count;

    alignment_count = class_aligns[new_hero->cls + align_base].count;
    for (scan_index = 1; scan_index <= 2; scan_index++) {
        hit = 0;
        for (tick_slot = 0; tick_slot < alignment_count && !hit; tick_slot++) {
            new_hero->alignment = (create_choices.law - 1) * 3 + (scan_index - 1);
            if (class_aligns[new_hero->cls + align_base].aligns[tick_slot] == new_hero->alignment)
                hit = 1;
        }
        if (hit)
            ui_mark(scan_index + 0x21, 0x18);
        else
            ui_mark(scan_index + 0x21, 0x10);
    }
}

static void align_default(void)                                  /* 25a4:2532 */
{
    switch (create_choices.cls) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 14:
        create_choices.law = 1;
        create_choices.good = 1;
        break;
    case 7:
    case 13:
    case 15:
    case 16:
    case 17:
        create_choices.law = 2;
        create_choices.good = 2;
        break;
    }
}

void far race_set(void)                                          /* 25a4:2581 */
{
    unsigned char class_count;
    unsigned char first;
    unsigned char class_id;
    char cur_bad;
    char hit;

    new_hero->race = create_choices.race - 1;
    class_count = race_classes[new_hero->race].count;
    first = 0xff;
    hit = 0;
    cur_bad = 0;
    for (class_id = 0; class_id <= 16; class_id++) {
        for (tick_slot = 0; tick_slot < class_count && !hit; tick_slot++) {
            if (race_classes[new_hero->race].classes[tick_slot] == class_id) {
                if (first == 0xff)
                    first = class_id;
                hit = 1;
            }
        }
        if (!hit) {
            if (class_id == create_choices.cls - 1)
                cur_bad = 1;
            ui_mark(class_id + 0xc, 0x10);
        } else
            ui_mark(class_id + 0xc, 0x18);
        hit = 0;
    }
    if (cur_bad)
        ui_mark(0xb, 4, first + 1);
    if (new_hero->race == 4 || new_hero->race == 3)
        align_base = 0x11;
    else
        align_base = 0;
}

void far align_update(void)                                      /* 25a4:26a1 */
{
    align_mark();
    align_fix();
    ui_mark(0x21, 4, create_choices.good);
    align_set();
}

void far good_set(void)                                          /* 25a4:26c2 */
{
    align_set();
}

void far gender_set(void)                                        /* 25a4:26c7 */
{
    new_hero->gender = create_choices.gender - 1;
}

void far class_set(void)                                         /* 25a4:26d4 */
{
    new_hero->cls = create_choices.cls - 1;
    switch (create_choices.cls - 1) {
    case 1:
    case 3:
        ui_mark(0x1e, 0x18);
        ui_mark(0x1f, 0x10);
        ui_mark(0x20, 0x10);
        break;
    case 0:
    case 2:
    case 4:
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        ui_mark(0x1e, 0x18);
        ui_mark(0x1f, 0x18);
        ui_mark(0x20, 0x18);
        break;
    }
    align_total = class_aligns[new_hero->cls + align_base].count;
    moral_matched = 0;
    for (good_scan = 1; good_scan <= 2 && !moral_matched; good_scan++)
        for (tick_slot = 0; tick_slot < align_total && !moral_matched; tick_slot++) {
            new_hero->alignment = (create_choices.law - 1) * 3 + (good_scan - 1);
            if (class_aligns[new_hero->cls + align_base].aligns[tick_slot] == new_hero->alignment)
                moral_matched = 1;
        }
    good_scan--;
    if (moral_matched)
        align_fix();
    else {
        order_matched = 0;
        for (law_try = 1; law_try <= 3 && !order_matched; law_try++)
            for (tick_slot = 0; tick_slot < align_total && !order_matched; tick_slot++) {
                new_hero->alignment = (law_try - 1) * 3 + create_choices.good - 1;
                if (class_aligns[new_hero->cls + align_base].aligns[tick_slot] ==
                    new_hero->alignment)
                    order_matched = 1;
            }
        law_try--;
        if (order_matched)
            create_choices.law = law_try;
        else
            align_default();
    }
    align_mark();
    ui_mark(0x1d, 4, create_choices.law);
    ui_mark(0x21, 4, create_choices.good);
}

void far pick_titles(void)                                       /* 25a4:2900 */
{
    frame_border();
    text_print_at(0x1f46, 0x1f46, title_hue, "PICK RACE");
    text_print_at(0x1f6c, 0x1f46, title_hue, "PICK ALIGNMENT");
    text_print_at(0x1f8c, 0x1f46, title_hue, "PICK GENDER");
    text_print_at(0x1f46, 0x1f88, title_hue, "PICK CLASS");
}

int far pick_arrows(int id, int key)                             /* 25a4:2967 */
{
    if (id < 0) {
        key_y = 0;
        key_col = 0;
        mouse_move_to(0x1f4d, 0x1f48);
ignore:
        return 0;
    }
    if (key == 0x106 || key == 0x102)
        key_col = 1 - key_col;
    else if (key == 0x108 && key_y != 0)
        key_y--;
    else if (key == 0x104)
        key_y++;
    else if (key != 0xd)
        goto ignore;
    if (key_col) {
        if (key_y > 0xf)
            key_y = 0xf;
    } else if (key_y == 7)
        key_y = 0xa;
    else if (key_y == 9)
        key_y = 6;
    else if (key_y == 0xf)
        key_y = 0x12;
    else if (key_y == 0x11)
        key_y = 0xe;
    else if (key_y > 0x13)
        key_y = 0x13;
    mouse_move_to(key_y * 4 + 0x1f4d, (key_col ? 0x1f8a : 0x1f46) + 2);
    if (key == 0xd)
        mouse_fake(1, 1);
    return 1;
}

char far hero_pick(void)                                         /* 25a4:2a6c */
{
    icon_cancelled = 0;
    list_attr = 0x87;
    title_hue = 0x8c;
    pick_titles();
    screen_flush();
    create_choices.race = 7;
    create_choices.law = 1;
    create_choices.good = 1;
    create_choices.gender = 1;
    create_choices.cls = 3;
    new_hero->race = 6;
    new_hero->alignment = 0;
    new_hero->gender = 0;
    new_hero->cls = 2;
    frame_invalidate();
    dl_begin();
    ui_add(2, 6, 0x22, race_set, 0x23, &create_choices.race,
           3, 0x1f4c, 0x1f46, "SILVANESTI ELF", 0x26, list_attr,
           3, 0x1f50, 0x1f46, "QUALINESTI ELF", 0x26, list_attr,
           3, 0x1f54, 0x1f46, "HALF-ELF", 0x26, list_attr,
           3, 0x1f58, 0x1f46, "MOUNTAIN DWARF", 0x26, list_attr,
           3, 0x1f5c, 0x1f46, "HILL DWARF", 0x26, list_attr,
           3, 0x1f60, 0x1f46, "KENDER", 0x26, list_attr,
           3, 0x1f64, 0x1f46, "HUMAN", 0x26, list_attr,
           0);
    ui_add(2, 2, 0x22, gender_set, 0x23, &create_choices.gender,
           3, 0x1f92, 0x1f46, "MALE", 0x26, list_attr,
           3, 0x1f96, 0x1f46, "FEMALE", 0x26, list_attr,
           0);
    ui_add(2, 0x11, 0x22, class_set, 0x23, &create_choices.cls,
           3, 0x1f4c, 0x1f8a, "CLERIC", 0x26, list_attr,
           3, 0x1f50, 0x1f8a, "KNIGHT", 0x26, list_attr,
           3, 0x1f54, 0x1f8a, "FIGHTER", 0x26, list_attr,
           3, 0x1f58, 0x1f8a, "PALADIN", 0x26, list_attr,
           3, 0x1f5c, 0x1f8a, "RANGER", 0x26, list_attr,
           0);
    ui_add(3, 0x1f60, 0x1f8a, "MAGIC-USER", 0x26, list_attr,
           3, 0x1f64, 0x1f8a, "THIEF", 0x26, list_attr,
           3, 0x1f64, 0x1f8a, "MONK", 0x26, list_attr, 0x11,
           3, 0x1f68, 0x1f8a, "CLERIC/FIGHTER", 0x26, list_attr,
           3, 0x1f6c, 0x1f8a, "CLERIC/FIGHTER/M-U", 0x26, list_attr,
           3, 0x1f70, 0x1f8a, "CLERIC/RANGER", 0x26, list_attr,
           0);
    ui_add(3, 0x1f74, 0x1f8a, "CLERIC/MAGIC-USER", 0x26, list_attr,
           3, 0x1f78, 0x1f8a, "CLERIC/THIEF", 0x26, list_attr,
           3, 0x1f7c, 0x1f8a, "FIGHTER/MAGIC-USER", 0x26, list_attr,
           3, 0x1f80, 0x1f8a, "FIGHTER/THIEF", 0x26, list_attr,
           3, 0x1f84, 0x1f8a, "FIGHTER/M-U/THIEF", 0x26, list_attr,
           3, 0x1f88, 0x1f8a, "MAGIC-USER/THIEF", 0x26, list_attr,
           0);
    ui_add(2, 3, 0x22, align_update, 0x23, &create_choices.law,
           3, 0x1f72, 0x1f46, "LAWFUL", 0x26, list_attr,
           3, 0x1f76, 0x1f46, "NEUTRAL", 0x26, list_attr,
           3, 0x1f7a, 0x1f46, "CHAOTIC", 0x26, list_attr,
           2, 3, 0x22, good_set, 0x23, &create_choices.good,
           3, 0x1f80, 0x1f46, "GOOD", 0x26, list_attr,
           3, 0x1f84, 0x1f46, "NEUTRAL", 0x26, list_attr,
           3, 0x1f88, 0x1f46, "EVIL", 0x26, list_attr, 0x11,
           0);
    ui_add(1, 0x1f9e, 0x1f44, "Done", 0x14, 0x20, 'D', 0x24, 4, 0x22, hero_setup, 0x15,
           1, 0x1f9e, 0x1f58, "Exit", 0x14, 0x20, 'E', 0x21, 0x23, 0x24, 4, 0x22, pick_exit,
           0x15,
           0);
    if (!pointer_mode_set(-1)) {
        ui_add(7, pick_arrows, 0);
        pick_arrows(-1, 0);
        pointer_show();
    }
    dl_draw(1);
    ui_run(0);
    dl_end();
    if (!pointer_mode_set(-1))
        pointer_hide();
    return !icon_cancelled;
}

void far hero_create(void)                                       /* 25a4:2f95 */
{
    struct Character hero;
    struct Character far *saved;
    char buf[0x2a];

    new_hero = &hero;
    mem_fill(new_hero, sizeof(struct Character), 0);
    new_hero->ac_base = 0x32;
    new_hero->thac0_base = 0x28;
    new_hero->status = 0;
    new_hero->enabled = 1;
    new_hero->icon_dim = 1;
    new_hero->order_no = 0xd;
    list_attr = 0x87;
    saved = cur_node;
    if (hero_pick())
        if (deity_pick())
            if (hero_finish(new_hero))
                if (name_input(new_hero)) {
                    icon_default();
                    if (icon_customize()) {
                        new_hero->type_flags = 2;
                        new_hero->immunity = 2;
                        str_copy(buf, msg_format("Save %s?", new_hero->name));
                        if (confirm(buf, 1)) {
                            mu_learn(new_hero);
                            gear_start(new_hero);
                            character_file_save(new_hero, "");
                        }
                    }
                }
    cur_node = saved;
}

void far mu_learn(struct Character far *character)                            /* 25a4:30d3 */
{
    unsigned char index;

    if (character->level[5] > 0)
        for (index = 1; index <= 0x7e; index++)
            if ((spell_info - 1)[index].cast_class == 2 && (spell_info - 1)[index].level == 1)
                character->known[(index - 1) / 8] = character->known[(index - 1) / 8] | bit_mask[(index - 1) % 8];
    if (character->robe_order == 1) {
        if (character->slots[2][1] > 0) {
            character->known[3] = character->known[3] | bit_mask[4];
            character->known[4] = character->known[4] | bit_mask[0];
            character->known[4] = character->known[4] | bit_mask[1];
        }
        if (character->slots[2][2] > 0) {
            character->known[5] = character->known[5] | bit_mask[5];
            character->known[5] = character->known[5] | bit_mask[6];
            character->known[6] = character->known[6] | bit_mask[0];
            character->known[6] = character->known[6] | bit_mask[3];
            character->known[6] = character->known[6] | bit_mask[5];
        }
        if (character->slots[2][3] > 0) {
            character->known[0xa] = character->known[0xa] | bit_mask[1];
            character->known[0xa] = character->known[0xa] | bit_mask[7];
            character->known[0xa] = character->known[0xa] | bit_mask[0];
            character->known[0xa] = character->known[0xa] | bit_mask[4];
            character->known[0xa] = character->known[0xa] | bit_mask[6];
        }
        if (character->slots[2][4] > 0) {
            character->known[0xb] = character->known[0xb] | bit_mask[2];
            character->known[0xb] = character->known[0xb] | bit_mask[3];
        }
        if (character->slots[2][5] > 0)
            character->known[0xd] = character->known[0xd] | bit_mask[7];
        if (character->slots[2][6] > 0) {
            character->known[0xe] = character->known[0xe] | bit_mask[2];
            character->known[0xe] = character->known[0xe] | bit_mask[4];
        }
        if (character->slots[2][7] > 0)
            character->known[0xf] = character->known[0xf] | bit_mask[0];
    } else {
        if (character->slots[2][1] > 0) {
            character->known[3] = character->known[3] | bit_mask[5];
            character->known[3] = character->known[3] | bit_mask[6];
            character->known[3] = character->known[3] | bit_mask[7];
            character->known[4] = character->known[4] | bit_mask[1];
            character->known[4] = character->known[4] | bit_mask[2];
        }
        if (character->slots[2][2] > 0) {
            character->known[5] = character->known[5] | bit_mask[4];
            character->known[5] = character->known[5] | bit_mask[6];
            character->known[5] = character->known[5] | bit_mask[7];
            character->known[6] = character->known[6] | bit_mask[1];
            character->known[6] = character->known[6] | bit_mask[2];
            character->known[6] = character->known[6] | bit_mask[6];
        }
        if (character->slots[2][3] > 0) {
            character->known[0xa] = character->known[0xa] | bit_mask[3];
            character->known[0xa] = character->known[0xa] | bit_mask[2];
            character->known[0xa] = character->known[0xa] | bit_mask[4];
            character->known[0xa] = character->known[0xa] | bit_mask[6];
        }
        if (character->slots[2][4] > 0) {
            character->known[0xb] = character->known[0xb] | bit_mask[3];
            character->known[0xe] = character->known[0xe] | bit_mask[5];
            character->known[0xe] = character->known[0xe] | bit_mask[6];
        }
        if (character->slots[2][5] > 0) {
            character->known[0xe] = character->known[0xe] | bit_mask[0];
            character->known[0xe] = character->known[0xe] | bit_mask[1];
            character->known[0xd] = character->known[0xd] | bit_mask[6];
        }
        if (character->slots[2][6] > 0)
            character->known[0xe] = character->known[0xe] | bit_mask[4];
        if (character->slots[2][7] > 0)
            character->known[0xf] = character->known[0xf] | bit_mask[2];
    }
    if (character->slots[2][8] > 0)
        for (index = 1; index <= 0x7e; index++)
            if ((spell_info - 1)[index].cast_class == 2)
                character->known[(index - 1) / 8] = character->known[(index - 1) / 8] | bit_mask[(index - 1) % 8];
}

static void gear_start(struct Character far *character)                       /* 25a4:34e4 */
{
    struct Item far *prev;
    struct Item far *item;
    char armed;
    unsigned char id;
    unsigned char scan_index;

    character->item_count = 0;
    armed = 0;
    for (scan_index = 0; scan_index <= 4; scan_index++) {
        item = rec_alloc();             /* on every pass, placed or not */
        mem_zero(item, 4);
        id = start_items[character->race][character->cls][scan_index];
        if (id != 0 && (char)item_templates[id].type != -1) {
            character->item_count++;
            mem_move5(item_templates + id, &item->rec.type, 0x11);
            item->next = 0;
            item->in_use = 0;
            if (character->items == 0)
                character->items = item;
            else {
                prev = character->items;
                character->items = item;
                character->items->next = prev;
            }
            character->items->rec.unid_mask = character->items->rec.unid_mask & 0xf8;
            if (id == 0x50 || id == 0x56 || id == 0x5b)
                item->rec.quantity = 0x28;
            character->items->rec.equipped = 0;
            if (id != 0x52 && id != 0x53 && id != 0x54 && id != 0x56 && id != 0x5d) {
                if (item_table[character->items->rec.type].slot == 0) {
                    if (!armed) {
                        armed = 1;
                        character->items->rec.equipped = 1;
                    }
                } else
                    character->items->rec.equipped = 1;
            }
        }
    }
    recalc_stats(character);
}

static char deity_pick(void)                                     /* 25a4:36b0 */
{
    unsigned char key;
    char more;
    char cancel;
    char good;

    cancel = 0;
    frame_border();
    if (new_hero->level[0] > 0) {
        if (new_hero->race != 3 && new_hero->race != 4) {
            switch (new_hero->alignment) {  /* 2, 5, 8: no list, good unset */
            case 0:
            case 3:
            case 6:
                good = 1;
                list_alloc(5, &deity_nodes);
                deity_nodes->key = 1;
                str_copy(deity_nodes->text, deity_title);
                for (tick_slot = 1; tick_slot <= 4; tick_slot++) {
                    cur_deity = list_nth(tick_slot, deity_nodes);
                    str_copy(cur_deity->text, msg_format("%s%s", "\t", deity_names[tick_slot]));
                }
                break;
            case 1:
            case 4:
            case 7:
                good = 0;
                list_alloc(4, &deity_nodes);
                deity_nodes->key = 1;
                str_copy(deity_nodes->text, deity_title);
                for (tick_slot = 1; tick_slot <= 3; tick_slot++) {
                    cur_deity = list_nth(tick_slot, deity_nodes);
                    str_copy(cur_deity->text,
                             msg_format("%s%s", "\t", deity_names[tick_slot + 4]));
                }
                break;
            }
            cur_deity = deity_nodes;
            patron_row = 0;
            patron_box = 1;
            more = 1;
            do {
                key = list_menu(empty_text, select_items, 1, 2, 0x26, 0x16, deity_nodes,
                                patron_box, 0, &more, &patron_row, &cur_deity);
                if (menu_escaped && key == 0x1b || !menu_escaped && key > 0)
                    cancel = 1;
            } while (key != 0 && !cancel);
            list_free(&deity_nodes);
            if (!cancel) {
                if (good)
                    new_hero->deity = patron_row;
                else
                    new_hero->deity = patron_row + 4;
            }
            deity_apply();
        } else {
            new_hero->deity = 6;
            deity_apply();
        }
    } else {
        new_hero->deity = 0;
        deity_blessing = 0xff;
    }
    return !cancel;
}

static void deity_apply(void)                                    /* 25a4:393f */
{
    switch (new_hero->deity) {
    case 1:
        deity_blessing = 0x15;
        new_hero->known[7] = new_hero->known[7] | bit_mask[0];
        break;
    case 2:
        deity_blessing = 0x15;
        new_hero->known[7] = new_hero->known[7] | bit_mask[2];
        break;
    case 3:
        deity_blessing = 0x15;
        new_hero->known[7] = new_hero->known[7] | bit_mask[3];
        new_hero->thac0_base++;
        break;
    case 4:
        deity_blessing = 0x15;
        new_hero->known[7] = new_hero->known[7] | bit_mask[4];
        new_hero->known[7] = new_hero->known[7] | bit_mask[5];
        new_hero->known[7] = new_hero->known[7] | bit_mask[6];
        break;
    case 5:
        deity_blessing = 0x2e;
        new_hero->known[7] = new_hero->known[7] | bit_mask[7];
        break;
    case 6:
        deity_blessing = 0x2e;
        if (new_hero->race == 3 || new_hero->race == 4)
            new_hero->thac0_base++;
        break;
    case 7:
        deity_blessing = 0x2e;
        new_hero->known[7] = new_hero->known[7] | bit_mask[6];
        break;
    }
}
