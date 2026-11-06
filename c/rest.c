/* DQK.EXE resident module 1396: the game clock and resting in camp.
 * Advancing the clock with its carries, three phase counters drawn as
 * glyphs (read here as the moons), effects running out as time passes,
 * the rest-time menu and its display, and the rest loop: healing,
 * memorizing and scribing spells while the party rests, and the random
 * interruption.
 *
 * EXE 0x005d6e..0x006cf4, one Turbo C large-model _TEXT segment starting at
 * byte 0xe of paragraph 1396.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * The lone retf at 0x006cf4 is left to module 148f; no call or stored
 * pointer found in the image reaches it.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is: memorize_one
 * far-calls spell_study_level_fallback, so that one carries a prototype.
 *
 * A time is seven counters, smallest first.  time_add carries each of the
 * first six into the next when it reaches its time_units entry; the last
 * wraps and ages every party member.  The clock keeps its counters as bytes
 * at game_state + 5, the rest time as words in rest_time; clock_set also
 * stores the year's high byte at game_state + 12, which nothing here reads.
 */

#include "dqk.h"

int far spell_study_level_fallback(int spell);

static char fx_live[0x35];                                       /* a260 */

void far moon_step(unsigned short moon, unsigned char days,
                   unsigned char far *color, unsigned char far *glyph)
{                                                                /* 1396:000e */
    if (game_state[moon + 1] >= days) {
        if (game_state[moon - 2] >= 3)
            game_state[moon - 2] = 0;
        else
            game_state[moon - 2]++;
        switch (moon) {
        case STATE_MOON_DAYS_1 - 1:
            *glyph = game_state[STATE_MOON_PHASE_1] + 0x1d;
            *color = 8;
            break;
        case STATE_MOON_DAYS_2 - 1:
            *glyph = game_state[STATE_MOON_PHASE_2] + 0x21;
            *color = 0x13;
            break;
        case STATE_MOON_DAYS_3 - 1:
            *glyph = game_state[STATE_MOON_PHASE_3] + 0x25;
            *color = 0x1e;
            break;
        }
        if (view_mode != 3)
            draw_glyph(0x1f40, 0x1f40, 1, *glyph);
        game_state[moon + 1] = 0;
    } else
        game_state[moon + 1]++;
}

void far moons_step(void)                                        /* 1396:00f8 */
{
    unsigned char color;
    unsigned char glyph;

    moon_step(STATE_MOON_DAYS_1 - 1, 8, &color, &glyph);
    moon_step(STATE_MOON_DAYS_2 - 1, 1, &color, &glyph);
    moon_step(STATE_MOON_DAYS_3 - 1, 6, &color, &glyph);
}

void far clock_set(char tenth_minute, char minute, char ten_minutes, char hour, char day, char month, short year)
{                                                                /* 1396:014a */
    game_state[STATE_CLOCK_TENTH_MINUTE] = tenth_minute;
    game_state[STATE_CLOCK_MINUTE] = minute;
    game_state[STATE_CLOCK_TEN_MINUTES] = ten_minutes;
    game_state[STATE_CLOCK_HOUR] = hour;
    game_state[STATE_CLOCK_DAY] = day;
    game_state[STATE_CLOCK_MONTH] = month;
    game_state[STATE_CLOCK_YEAR_LO] = lo_byte(year);
    game_state[STATE_CLOCK_YEAR_HI] = hi_byte(year);
}

void far effects_tick(unsigned char elapsed_units, short unit)               /* 1396:019f */
{
    short unit_index;
    unsigned char index;
    unsigned char step;
    unsigned short total;
    struct Character far *hero;
    struct Effect far *next;
    struct Effect far *effect;

    if (view_mode != 2)
        mem_fill(fx_live, sizeof fx_live, 1);
    else {
        index = 1;
        do
            index++;
        while (fx_live[index] == 0 && game_state[STATE_PARTY_COUNT] >= index);
        if (fx_live[index] == 0)
            return;
    }
    total = elapsed_units;
    for (unit_index = unit; unit_index > 1; unit_index = unit_index - 1)
        total *= time_units[unit_index - 1];
    while (total > 0) {
        if (total > 10)
            step = 10;
        else
            step = total;
        for (index = 1, hero = node_list; hero != 0; index++, hero = hero->next) {
            if (fx_live[index] != 0) {
                fx_live[index] = 0;
                effect = hero->effects;
                while (effect != 0) {
                    if (effect->duration == 0)
                        effect = effect->next;
                    else if (effect->duration > step) {
                        effect->duration -= step;
                        fx_live[index] = 1;
                        effect = effect->next;
                    } else {
                        next = effect->next;
                        fx_remove(hero, effect->id, effect);
                        if (effect->id == 0xc4 && view_mode != 5 && view_mode != 3 &&
                            view_mode != 0 && view_mode != 10)
                            party_rows(hero, 0);
                        fx_live[index] = 1;
                        effect = next;
                    }
                }
            }
        }
        if (total > 10)
            total -= 10;
        else
            total = 0;
    }
}

void far time_add(unsigned far *clock_fields)                               /* 1396:0368 */
{
    short index;
    struct Character far *hero;

    for (index = 0; index <= 6; index++) {
        if (clock_fields[index] >= time_units[index]) {
            if (index != 6) {
                clock_fields[index + 1]++;
                clock_fields[index] = clock_fields[index] - time_units[index];
                if (index == 3)
                    moons_step();
            } else {
                for (hero = node_list; hero != 0; hero = hero->next)
                    hero->age++;
                clock_fields[6] = clock_fields[6] - time_units[6];
                year_rolled = 1;
            }
        }
    }
}

void far rest_carry(void)                                        /* 1396:041c */
{
    time_add(rest_time);
    if (rest_time[5] > 0) {
        rest_time[4] += time_units[4] * rest_time[5];
        rest_time[5] = 0;
        if (rest_time[4] > 99)
            rest_time[4] = 99;
    }
}

void far clock_advance(unsigned char elapsed_units, short unit)              /* 1396:044d */
{
    unsigned clock_fields[7];
    short index;
    unsigned char scan_index;

    for (index = 0; index <= 6; index++)
        clock_fields[index] = game_state[index + STATE_CLOCK_TENTH_MINUTE];
    for (scan_index = 1; scan_index <= elapsed_units; scan_index++) {
        clock_fields[unit]++;
        time_add(clock_fields);
    }
    for (index = 0; index <= 6; index++)
        game_state[index + STATE_CLOCK_TENTH_MINUTE] = clock_fields[index];
    effects_tick(elapsed_units, unit);
}

void far rest_take(unsigned char amount, short unit)             /* 1396:04de */
{
    short borrow_unit;
    short scan_index;

    if (rest_time[4] != 0 || rest_time[3] != 0 || rest_time[2] != 0 ||
        rest_time[1] != 0) {
        while (rest_time[unit] < amount) {
            for (borrow_unit = unit + 1; rest_time[borrow_unit] == 0 && borrow_unit < 5; borrow_unit = borrow_unit + 1)
                ;
            if (borrow_unit == 5) {
                mem_zero(rest_time, sizeof rest_time);
                amount = 0;
            } else
                for (scan_index = borrow_unit; unit + 1 <= scan_index; scan_index--) {
                    rest_time[scan_index]--;
                    rest_time[scan_index - 1] += time_units[scan_index - 1];
                }
        }
        rest_time[unit] -= amount;
        rest_carry();
    }
}

void far two_digits(unsigned char value, char far *buf)              /* 1396:05aa */
{
    if (value < 10)
        str_format(buf, "0%d", value);
    else
        str_format(buf, "%d", value);
}

void far rest_show(short sel)                                    /* 1396:05e7 */
{
    short index;
    int attr[7];
    unsigned char row;
    char buf[42];

    for (index = 0; index <= 6; index++)
        attr[index] = 7;
    attr[sel] = 11;
    status_printf(1, 0x11, 7, 0, "Rest Time:");
    row = 11;
    for (index = 4; index >= 3; index--) {
        two_digits(rest_time[index], buf);
        status_printf(row + 1, 0x11, attr[index], 0, buf);
        status_printf(row + 3, 0x11, 7, 0, ":");
        row += 3;
    }
    two_digits(rest_time[2] * 10 + rest_time[1], buf);
    status_printf(row + 1, 0x11, attr[2], 0, buf);
}

char far rest_menu(void)                                         /* 1396:06dc */
{
    char rest_requested;
    unsigned char key;
    unsigned char code;
    short sel;

    ui_mode = 1;
    rest_requested = 0;
    sel = 2;
    do {
        rest_show(sel);
        hotkey_reset(6);
        key = menu_run(empty_text, rest_items, 1, 0);
        if (menu_escaped) {
            code = key;
            switch (code) {
            case 0x88:
                key = 4;
                break;
            case 0x84:
                key = 5;
                break;
            case 0x82:
            case 0x86:
                key = 0xff;
                break;
            default:
                key = 6;
            }
        }
        switch (key) {                  /* rest_items: Rest Days Hours */
        case 0:                         /* Mins Add Subtract Exit      */
            rest_requested = 1;
            break;
        case 1:
            sel = 4;
            break;
        case 2:
            sel = 3;
            break;
        case 3:
            sel = 2;
            break;
        case 4:
            if (sel == 2)
                rest_time[1] += 5;
            else
                rest_time[sel]++;
            rest_carry();
            break;
        case 5:
            if (sel == 2)
                rest_take(5, 1);
            else
                rest_take(1, sel);
            rest_carry();
            break;
        }
    } while (key != 6 && key != 0);
    return rest_requested;
}

void far heal_check(char show)                                   /* 1396:07f6 */
{
    char healed;
    struct Character far *hero;

    heal_ticks++;
    if (heal_ticks >= 0x120) {
        healed = 0;
        for (hero = node_list; hero != 0; hero = hero->next)
            if (fx_heal(hero, 1, 0))
                healed = 1;
        if (show)
            rest_show(0);
        status_printf(1, 0x13, 7, 0, "The Whole Party Is Healed");
        if (healed)
            party_display(cur_node);
        msg_pause();
        combat_text_clear();
        heal_ticks = 0;
    }
}

char far memorize_one(struct Character far *hero, char far *shown)    /* 1396:089b */
{
    char result;
    char found;
    unsigned char spell_slot;

    if (no_study)
        result = 0;
    else {
        found = 0;
        spell_slot = 0;
        if (hero->enabled)
            for (; spell_slot <= 0x88 && found == 0; spell_slot++)
                if (hero->spells[spell_slot] & 0x80)
                    if (*shown) {
                        found = (spell_info - 1)[hero->spells[spell_slot] & 0x7f].level;
                        if (found == 0)
                            found = spell_study_level_fallback(hero->spells[spell_slot] & 0x7f);
                    } else {
                        hero->spells[spell_slot] -= 0x80;
                        rest_show(0);
                        cast_announce(hero, "has memorized", hero->spells[spell_slot]);
                        *shown = 1;
                    }
        result = found;
    }
    return result;
}

char far scribe_one(struct Character far *hero, char far *shown)      /* 1396:0995 */
{
    char result;
    unsigned char inner_index;
    unsigned char spell;
    char found;
    struct Item far *item;
    struct Item far *sub;

    result = 0;
    if (!no_study) {
        found = 0;
        for (item = hero->items; item != 0 && found == 0; item = item->next) {
            inner_index = 1;
            if (item_casts(item)) {
                if (item->rec.type == 'I') {
                    for (sub = item->more, tick_slot = 1; item->rec.quantity >= tick_slot;
                         sub = sub->more, tick_slot++)
                        for (inner_index = 1; inner_index <= 3 && found == 0; inner_index++)
                            if ((sub->rec.u.spells - 1)[inner_index] > 0x80)
                                if (*shown)
                                    found = (spell_info - 1)[(sub->rec.u.spells - 1)[inner_index] & 0x7f].level;
                                else {
                                    spell = (sub->rec.u.spells - 1)[inner_index] & 0x7f;
                                    hero->known[(spell - 1) / 8] =
                                        hero->known[(spell - 1) / 8] |
                                        bit_mask[(spell - 1) % 8];
                                    item_spend(hero, item, spell);
                                    rest_show(0);
                                    cast_announce(hero, "has scribed", spell);
                                    *shown = 1;
                                }
                } else
                    for (inner_index = 1; inner_index <= 3 && found == 0; inner_index++)
                        if ((item->rec.u.spells - 1)[inner_index] > 0x80)
                            if (*shown)
                                found = (spell_info - 1)[(item->rec.u.spells - 1)[inner_index] & 0x7f].level;
                            else {
                                spell = (item->rec.u.spells - 1)[inner_index] & 0x7f;
                                hero->known[(spell - 1) / 8] =
                                    hero->known[(spell - 1) / 8] |
                                    bit_mask[(spell - 1) % 8];
                                item_spend(hero, item, spell);
                                rest_show(0);
                                cast_announce(hero, "has scribed", spell);
                                *shown = 1;
                            }
            }
        }
        result = found;
    }
    return result;
}

void far study_tick(void)                                        /* 1396:0c2c */
{
    unsigned char index;
    unsigned char res;
    struct Character far *hero;
    char shown;

    for (index = 1, hero = node_list; hero != 0; hero = hero->next, index++) {
        if (study_wait[index] > 0)
            study_wait[index]--;
        if (study_wait[index] == 0 && hero->spell_prep == 0) {
            shown = 0;
            res = scribe_one(hero, &shown);
            if (res == 0)
                res = memorize_one(hero, &shown);
            study_wait[index] = res * 3;
        }
    }
}

void far study_hour(unsigned char far *count)                    /* 1396:0cda */
{
    unsigned char index;
    unsigned char res;
    struct Character far *hero;
    char shown;

    (*count)++;
    if (*count >= 12) {
        *count = 0;
        for (index = 1, hero = node_list; hero != 0; hero = hero->next, index++) {
            if (hero->spell_prep > 0) {
                hero->spell_prep--;
                if (hero->spell_prep == 0) {
                    shown = 1;
                    res = scribe_one(hero, &shown);
                    if (res == 0)
                        res = memorize_one(hero, &shown);
                    study_wait[index] = res * 3;
                }
            }
        }
    }
}

char far camp_rest(char interactive)                             /* 1396:0d7d */
{
    char result;
    struct Character far *hero;
    unsigned char index;
    unsigned char ticks;
    unsigned char steps;
    char stop;

    result = 0;
    for (index = 1, hero = node_list; hero != 0; index++, hero = hero->next)
        study_wait[index] = 0;
    mem_fill(fx_live, sizeof fx_live, 1);
    ticks = 0;
    steps = 0;
    if (interactive) {
        text_clear(1, 0x11, 0x26, 0x16);
        rest_show(0);
    }
    resting = 1;
    if (interactive)
        stop = !rest_menu();
    else
        stop = 0;
    while (!stop && (rest_time[4] > 0 || rest_time[3] > 0 || rest_time[2] > 0 ||
                     rest_time[1] > 0)) {
        if (interactive && screen_sync()) { /* 1396:0e27 tests al */
            game_key_get();
            rest_show(0);
            if (confirm("Stop Resting?", 1)) /* 1bdb:0424 reads only prompt */
                stop = 1;
            else
                box_draw();
        }
        if (!stop) {
            rest_take(5, 1);
            steps++;
            if (interactive && steps >= 5) {
                rest_show(0);
                steps = 0;
            }
            clock_advance(5, 1);
            heal_check(interactive);
            study_tick();
            study_hour(&ticks);
            if (game_state[STATE_REST_ENCOUNTER_INTERVAL] > 0) {
                encounter_ticks++;
                if (game_state[STATE_REST_ENCOUNTER_INTERVAL] <= encounter_ticks) {
                    encounter_ticks = 0;
                    if (fx_roll_dice(1, 100) <= game_state[STATE_REST_ENCOUNTER_CHANCE]) {
                        combat_text_clear();
                        rest_show(0);
                        status_printf(1, 0x13, 0xb, 0,
                                      "Your repose is suddenly interrupted!");
                        stop = 1;
                        result = 1;
                        msg_pause();
                    }
                }
            }
        }
    }
    text_clear(1, 0x11, 0x26, 0x16);
    resting = 0;
    return result;
}

int far spell_study_level_fallback(int spell)                                    /* 1396:0f69 */
{
    switch (spell) {
    case 0x39:
        return 4;
    case 0x3d:
        return 3;
    case 0x3b:
        return 2;
    default:
        return 1;
    }
}
