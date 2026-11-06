/* DQK.EXE overlay 05 module 2946: modifying a party member.
 *
 * EXE 0x03deef..0x03f4f1, one Turbo C large-model _TEXT segment starting at
 * byte 0xf of paragraph 2946 in RTLink section 5 (payload 0x3a4c0, link
 * segment 25a4).  Bytes 0..0xe of paragraph 2946 are the tail of create.c,
 * section 5's module 25a4, which near-calls the routine they end; this
 * module's last retf is byte 0 of paragraph 2aa7, where train.c starts at
 * 2aa7:0001.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are
 * defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in, and a mod1d99_ callee is an RTLink vector
 * stub whose target is named beside it.
 *
 * _DATA is field and six literals, DGROUP 748a..74f9.  _BSS is nine statics,
 * DGROUP c790..c7c5; their names are chosen to fit Turbo C's name order.
 *
 * menu.c reaches click_set through vector stub 1d99:0f9f.  modify_hero
 * (vector stub 1d99:146d, from party.c) edits cur_node in place:
 * the six abilities, the name and the maximum hit points, one field (field,
 * 0..7) at a time from the "Modify:" menu.  Only a member whose experience
 * is 1000001, 500000 or 333333 may be changed; the HALIBUT test beside them
 * compares pointers, not text.  Keep recomputes what depends on the
 * abilities; Exit puts back what was saved on entry.
 */

#include "dqk.h"

extern void far status_msg(char far *fmt, char fg, char bg, ...); /* 0a62:0904 */
extern char far extra_row(void);                                 /* 0bdb:21dd */

static unsigned char field = 0xff;                               /* 748a */

static char picked;                                              /* c790 */
static unsigned char hp_save;                                    /* c791 */
static char dirty;                                               /* c792 */
static char extended;                                            /* c793 */
static char finished;                                            /* c794 */
static char old_name[0x29];                                      /* c795 */
static unsigned char exc_saved;                                  /* c7be */
static unsigned char name_col;                                   /* c7bf */
static unsigned char stats_saved[6];                             /* c7c0 */

void far show_field(char active, unsigned char field_id);
static void mu_forget(void);

void far set_field(char field_id)                                       /* 2946:000f */
{
    if (field_id == 6 && field != 6) {
        pointer_hide();
        box_draw();
        status_printf(0, 0x18, 0xf, 8, "Edit Name: Press return when done.");
        show_field(0, 6);
    } else if (field == 6 && field_id != 6) {
        input_pending_clear();
        pointer_show();
    }
    field = field_id;
}

void far click_set(char value)                                       /* 2946:006f */
{
    picked = value;
}

char far click_get(void)                                         /* 2946:007a */
{
    return picked;
}

unsigned char far hp_minimum(struct Character far *hero)              /* 2946:007e */
{
    unsigned char count;
    short total;
    unsigned char result;
    short class_index;
    unsigned char over;
    register int bonus;

    bonus = 0;
    count = 0;
    total = 0;
    for (class_index = 0; class_index <= 6; class_index++) {
        if (hero->level[class_index] > 0) {
            if (hero->level[class_index] < hit_dice_level_limit[class_index]) {
                total += hero->level[class_index];
                count++;
                if (class_index == 4 || class_index == 1)
                    total++;
                bonus += hp_bonus(hero, class_index) * hero->level[class_index];
            } else {
                count++;
                over = hero->level[class_index] - hit_dice_level_limit[class_index] + 1;
                switch (class_index) {
                case 0:
                    total += over * 2 + 9;
                    break;
                case 2:
                case 3:
                    total += over * 3 + 9;
                    break;
                case 4:
                    total += over * 2 + 0xb;
                    break;
                case 5:
                    total += over + 0xb;
                    break;
                case 6:
                    total += over * 2 + 0xa;
                    break;
                }
                bonus += hp_bonus(hero, class_index) * (hit_dice_level_limit[class_index] - 1);
            }
        }
    }
    if (bonus > 0)
        total = (total + bonus) / count;
    else
        total = total / count;
    result = total;
    return result;
}

void far modify_undo(void)                                       /* 2946:01e5 */
{
    short stat_index;

    for (stat_index = 0; stat_index <= 5; stat_index++)
        cur_node->stats[stat_index].cur = stats_saved[stat_index];
    cur_node->str_ex = exc_saved;
    cur_node->hp_max = hp_save;
    cur_node->hp = cur_node->hp_max;
    str_copy(cur_node->name, old_name);
    recalc_stats(cur_node);
    finished = 1;
}

void far modify_keep(void)                                       /* 2946:025e */
{
    short stat_index;
    short class_index;

    recalc_spells(cur_node);
    if (cur_node->level[5] > 0) {
        mu_forget();
        mu_learn(cur_node);
    }
    hp_save = 0;
    tick_slot = 0;
    for (class_index = 0; class_index <= 6; class_index++) {
        if (cur_node->level[class_index] > 0) {
            if ((int)cur_node->level[class_index] < hit_dice_level_limit[class_index]) {    /* (int): cmp ax,dx, not a byte cmp */
                if (class_index == 4)
                    hp_save += con_hp_adj(cur_node, class_index) * (cur_node->level[class_index] + 1);
                else
                    hp_save += con_hp_adj(cur_node, class_index) * cur_node->level[class_index];
            } else
                hp_save += con_hp_adj(cur_node, class_index) * (hit_dice_level_limit[class_index] - 1);
            tick_slot++;
        }
    }
    hp_save = hp_save / tick_slot;
    cur_node->hp_base = cur_node->hp_max - hp_save;
    for (stat_index = 0; stat_index <= 5; stat_index++)
        cur_node->stats[stat_index].base = cur_node->stats[stat_index].cur;
    cur_node->str_ex_base = cur_node->str_ex;
    finished = 1;
}

void far edit_str(unsigned char key)                             /* 2946:03c7 */
{
    switch (key) {
    case 0:
    case 0x84:
    case 0x85:
        set_field(1);
        break;
    case 1:
    case 0x87:
    case 0x88:
        set_field(7);
        break;
    case 2:
    case 0x82:
        cur_node->stats[0].cur++;
        if (cur_node->stats[0].cur > race_limits[cur_node->race].str_max[cur_node->gender]) {
            if (cur_node->stats[0].cur > 18 &&
                (cur_node->level[2] > 0 || cur_node->level[3] > 0 ||
                 cur_node->level[4] > 0 || cur_node->level[1] > 0) &&
                race_limits[cur_node->race].exc_max[cur_node->gender] > cur_node->str_ex)
                cur_node->str_ex++;
            cur_node->stats[0].cur = race_limits[cur_node->race].str_max[cur_node->gender];
        }
        break;
    case 3:
    case 0x86:
        cur_node->stats[0].cur--;
        if (cur_node->str_ex > 0) {
            cur_node->str_ex--;
            cur_node->stats[0].cur++;
        } else if (cur_node->stats[0].cur < race_limits[cur_node->race].str_min[cur_node->gender])
            cur_node->stats[0].cur = race_limits[cur_node->race].str_min[cur_node->gender];
        if (cur_node->stats[0].cur < class_mins[cur_node->cls].min[0])
            cur_node->stats[0].cur = class_mins[cur_node->cls].min[0];
        break;
    case 4:
        modify_keep();
        break;
    case 5:
        modify_undo();
        break;
    }
    show_field(0, 0);
}

void far edit_int(unsigned char key)                             /* 2946:05be */
{
    switch (key) {
    case 0:
    case 0x84:
    case 0x85:
        set_field(2);
        break;
    case 1:
    case 0x87:
    case 0x88:
        set_field(0);
        break;
    case 2:
    case 0x82:
        cur_node->stats[1].cur++;
        if (cur_node->stats[1].cur > race_limits[cur_node->race].int_max)
            cur_node->stats[1].cur = race_limits[cur_node->race].int_max;
        break;
    case 3:
    case 0x86:
        cur_node->stats[1].cur--;
        if (cur_node->stats[1].cur < race_limits[cur_node->race].int_min)
            cur_node->stats[1].cur = race_limits[cur_node->race].int_min;
        if (cur_node->stats[1].cur < class_mins[cur_node->cls].min[1])
            cur_node->stats[1].cur = class_mins[cur_node->cls].min[1];
        break;
    case 4:
        modify_keep();
        break;
    case 5:
        modify_undo();
        break;
    }
    show_field(0, 1);
}

void far edit_wis(unsigned char key)                             /* 2946:0705 */
{
    switch (key) {
    case 0:
    case 0x84:
    case 0x85:
        set_field(3);
        break;
    case 1:
    case 0x87:
    case 0x88:
        set_field(1);
        break;
    case 2:
    case 0x82:
        cur_node->stats[2].cur++;
        if (cur_node->stats[2].cur > race_limits[cur_node->race].wis_max)
            cur_node->stats[2].cur = race_limits[cur_node->race].wis_max;
        if (cur_node->slots[0][0] > 0)
            cur_node->slots[0][0] = 1;
        break;
    case 3:
    case 0x86:
        cur_node->stats[2].cur--;
        if (cur_node->stats[2].cur < race_limits[cur_node->race].wis_min)
            cur_node->stats[2].cur = race_limits[cur_node->race].wis_min;
        if (cur_node->stats[2].cur < class_mins[cur_node->cls].min[2])
            cur_node->stats[2].cur = class_mins[cur_node->cls].min[2];
        if (cur_node->slots[0][0] > 0)
            cur_node->slots[0][0] = 1;
        break;
    case 4:
        modify_keep();
        break;
    case 5:
        modify_undo();
        break;
    }
    show_field(0, 2);
}

void far edit_dex(unsigned char key)                             /* 2946:085f */
{
    switch (key) {
    case 0:
    case 0x84:
    case 0x85:
        set_field(4);
        break;
    case 1:
    case 0x87:
    case 0x88:
        set_field(2);
        break;
    case 2:
    case 0x82:
        cur_node->stats[3].cur++;
        if (cur_node->stats[3].cur > race_limits[cur_node->race].dex_max)
            cur_node->stats[3].cur = race_limits[cur_node->race].dex_max;
        break;
    case 3:
    case 0x86:
        cur_node->stats[3].cur--;
        if (cur_node->stats[3].cur < race_limits[cur_node->race].dex_min)
            cur_node->stats[3].cur = race_limits[cur_node->race].dex_min;
        if (cur_node->stats[3].cur < class_mins[cur_node->cls].min[3])
            cur_node->stats[3].cur = class_mins[cur_node->cls].min[3];
        break;
    case 4:
        modify_keep();
        break;
    case 5:
        modify_undo();
        break;
    }
    show_field(0, 3);
}

void far edit_con(unsigned char key)                             /* 2946:09a6 */
{
    switch (key) {
    case 0:
    case 0x84:
    case 0x85:
        set_field(5);
        break;
    case 1:
    case 0x87:
    case 0x88:
        set_field(3);
        break;
    case 2:
    case 0x82:
        cur_node->stats[4].cur++;
        if (cur_node->stats[4].cur > race_limits[cur_node->race].con_max)
            cur_node->stats[4].cur = race_limits[cur_node->race].con_max;
        if (hp_minimum(cur_node) > cur_node->hp_max)
            cur_node->hp_max = hp_minimum(cur_node);
        cur_node->hp = cur_node->hp_max;
        show_field(0, 4);               /* row 4; the decrement redraws row 7 */
        break;
    case 3:
    case 0x86:
        cur_node->stats[4].cur--;
        if (cur_node->stats[4].cur < race_limits[cur_node->race].con_min)
            cur_node->stats[4].cur = race_limits[cur_node->race].con_min;
        if (cur_node->stats[4].cur < class_mins[cur_node->cls].min[4])
            cur_node->stats[4].cur = class_mins[cur_node->cls].min[4];
        if (hp_floor(cur_node) < cur_node->hp_max)
            cur_node->hp_max = hp_floor(cur_node);
        cur_node->hp = cur_node->hp_max;
        show_field(0, 7);
        break;
    case 4:
        modify_keep();
        break;
    case 5:
        modify_undo();
        break;
    }
    show_field(0, 4);
}

void far edit_cha(unsigned char key)                             /* 2946:0b84 */
{
    switch (key) {
    case 0:
    case 0x84:
    case 0x85:
        if (!extra_row())
            set_field(6);
        else
            set_field(7);
        break;
    case 1:
    case 0x87:
    case 0x88:
        set_field(4);
        break;
    case 2:
    case 0x82:
        cur_node->stats[5].cur++;
        if (cur_node->race == 3 && cur_node->cls == 3) {
            if (cur_node->stats[5].cur > 0x11)
                cur_node->stats[5].cur = 0x11;
        } else if (cur_node->stats[5].cur > race_limits[cur_node->race].cha_max)
            cur_node->stats[5].cur = race_limits[cur_node->race].cha_max;
        break;
    case 3:
    case 0x86:
        cur_node->stats[5].cur--;
        if (cur_node->race == 3 && cur_node->cls == 3) {
            if (cur_node->stats[5].cur < 0x11)
                cur_node->stats[5].cur = 0x11;
        } else {
            if (cur_node->stats[5].cur < race_limits[cur_node->race].cha_min)
                cur_node->stats[5].cur = race_limits[cur_node->race].cha_min;
            if (cur_node->stats[5].cur < class_mins[cur_node->cls].min[5])
                cur_node->stats[5].cur = class_mins[cur_node->cls].min[5];
        }
        break;
    case 4:
        modify_keep();
        break;
    case 5:
        modify_undo();
        break;
    }
    show_field(0, 5);
}

void far edit_name(void)                                         /* 2946:0d30 */
{
    short key;
    struct Character far *character_cursor;
    char found;
    register int key_code;

    extended = 0;
    dirty = 1;
    do {
        if ((timer_ticks() & 0x1f) == 0) {
            fill_rect(0x1f44, name_col * 4 + 0x1f44, 0x1f48, name_col * 4 + 0x1f48, 0x10f);
            while ((timer_ticks() & 0x1f) == 0)
                ;
        }
    } while (!screen_sync());
    key = game_key_get();
    if (key & 0x80)
        extended = 1;
    else
        extended = 0;
    if (key == 0x60)
        key = 0x1b;
    show_field(1, field);
    key_code = key;
    if (key_code >= 0x20 && key_code <= 0x7a)
        key_code = -1;
    switch (key_code) {
    case 0x8a:
        if (str_length(cur_node->name) > 0 &&
            str_length(cur_node->name) - 1 >= name_col) {
            if (str_length(cur_node->name) - 1 == name_col)
                cur_node->name[name_col] = 0;
            else if (str_length(cur_node->name))
                for (tick_slot = name_col + 1; str_length(cur_node->name) >= tick_slot;
                     tick_slot++)
                    cur_node->name[tick_slot - 1] = cur_node->name[tick_slot];
        }
        break;
    case 0xd:
    case 0x84:
    case 0x85:
        character_cursor = node_list;
        found = 0;
        while (character_cursor) {
            if (str_same(character_cursor->name, cur_node->name) && character_cursor != cur_node)
                found = 1;
            character_cursor = character_cursor->next;
        }
        if (found)
            status_msg("Already a %s in party!", 0xb, 0, cur_node->name);
        else {
            show_field(0, field);
            set_field(7);
        }
        break;
    case 0x87:
    case 0x88:
        character_cursor = node_list;
        found = 0;
        while (character_cursor) {
            if (str_same(character_cursor->name, cur_node->name) && character_cursor != cur_node)
                found = 1;
            character_cursor = character_cursor->next;
        }
        if (found)
            status_msg("Already a %s in party!", 0xb, 0, cur_node->name);
        else {
            show_field(0, field);
            set_field(5);
        }
        break;
    case 0x86:
        if (name_col == 0)
            name_col = str_length(cur_node->name);
        else
            name_col--;
        break;
    case 0x82:
        if (str_length(cur_node->name) <= name_col)
            name_col = 0;
        else
            name_col++;
        break;
    case 8:
        if (name_col > 0) {
            name_col--;
            if (str_length(cur_node->name) - 1 > name_col)
                for (tick_slot = name_col + 1; str_length(cur_node->name) >= tick_slot;
                     tick_slot++)
                    cur_node->name[tick_slot - 1] = cur_node->name[tick_slot];
            else if (str_length(cur_node->name) - 1 == name_col)
                cur_node->name[name_col] = 0;
        }
        break;
    case -1:
        if (str_length(cur_node->name) < 0xf) {
            for (tick_slot = str_length(cur_node->name) + 1; tick_slot > name_col; tick_slot--)
                cur_node->name[tick_slot] = cur_node->name[tick_slot - 1];
            cur_node->name[name_col] = key;
            cur_node->name[0xf] = 0;
            name_col++;
            if (name_col > 0xf)
                name_col = 0xf;
        }
        break;
    }
}

void far edit_hp(unsigned char key)                              /* 2946:1188 */
{
    switch (key) {
    case 0:
    case 0x84:
    case 0x85:
        set_field(0);
        break;
    case 1:
    case 0x87:
    case 0x88:
        if (extra_row())
            set_field(5);
        else
            set_field(6);
        break;
    case 2:
    case 0x82:
        cur_node->hp_max++;
        if (hp_floor(cur_node) < cur_node->hp_max)
            cur_node->hp_max = hp_floor(cur_node);
        cur_node->hp = cur_node->hp_max;
        break;
    case 3:
    case 0x86:
        cur_node->hp_max--;
        if (hp_minimum(cur_node) > cur_node->hp_max)
            cur_node->hp_max = hp_minimum(cur_node);
        cur_node->hp = cur_node->hp_max;
        break;
    case 4:
        modify_keep();
        break;
    case 5:
        modify_undo();
        break;
    }
    show_field(0, 7);
}

void far modify_hero(void)                                       /* 2946:1293 */
{
    struct Item far *item;
    short stat_index;
    char key;

    if (cur_node->xp != 1000001L && cur_node->xp != 500000L &&
        cur_node->xp != 333333L && msg_text(3) != "HALIBUT") { /* compares addresses */
        status_msg("%s%s", 0xb, 0, cur_node->name, cant_modify_text);
        return;
    }
    for (item = cur_node->items; item; item = item->next)
        if (item->rec.equipped)
            item_equip_toggle(item);
    char_status();
    for (stat_index = 0; stat_index <= 5; stat_index++)
        stats_saved[stat_index] = cur_node->stats[stat_index].cur = cur_node->stats[stat_index].base;
    exc_saved = cur_node->str_ex;
    hp_save = cur_node->hp_max;
    str_copy(old_name, cur_node->name);
    name_col = 0;
    if (!extra_row())
        show_field(0, 6);
    set_field(0);
    show_field(1, field);
    finished = 0;
    dirty = 1;
    finished = 0;
    while (!finished) {
        if (field != 6) {
            click_set(0);
            hotkey_reset(5);
            key = menu_bar("Modify:", "Next Previous Add Sub Keep Exit", 1, 0);
            if (click_get()) {
                show_field(0, field);
                set_field(key);
            }
        }
        if (!click_get()) {
            switch (field) {
            case 0:
                edit_str(key);
                break;
            case 1:
                edit_int(key);
                break;
            case 2:
                edit_wis(key);
                break;
            case 3:
                edit_dex(key);
                break;
            case 4:
                edit_con(key);
                break;
            case 5:
                edit_cha(key);
                break;
            case 6:
                edit_name();
                break;
            case 7:
                edit_hp(key);
                break;
            }
        } else
            click_set(0);
        combat_show();
        show_field(1, field);
    }
}

void far show_field(char active, unsigned char field_id)                /* 2946:14e6 */
{
    switch (field_id) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        stat_show(field_id, active);
        break;
    case 7:
        show_hp(cur_node, 0x1f, 3, active, 0);
        break;
    case 6:
        if (active) {
            status_char(1, 1, 8, 0x10);
            status_printf(1, 1, 0xb, 8, cur_node->name);
        } else
            status_printf(1, 1, 7, 8, cur_node->name);
        break;
    }
}

static void mu_forget(void)                                      /* 2946:1599 */
{
    unsigned char index;

    for (index = 1; index <= 0x7e; index++)
        if ((spell_info - 1)[index].cast_class == 2)
            cur_node->known[index / 8] = cur_node->known[index / 8] & ~bit_mask[index % 8];
                                        /* clears spell i + 1's bit, where mu_learn sets spell i's: as shipped */
}
