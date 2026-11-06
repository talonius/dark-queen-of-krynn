/* DQK.EXE resident module 1bdb: menus.  Hotkey tables, the button hook,
 * menu text parsing and drawing, the key wait with its animation and
 * timeout, and the menu shapes built on them (prompts, lists, grids).
 *
 * EXE 0x00e1b5..0x010391, one Turbo C large-model _TEXT segment starting at
 * byte 5 of paragraph 1bdb.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a modNNNN_ name says only which
 * segment the callee lives in.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is; those
 * carry prototypes below.  menu_split is static: menu_setup near-calls it
 * ahead of its definition.
 *
 * _BSS follows the statics' names (Turbo C's symbol-table order), not their
 * declaration order; these names reproduce the shipped a658..a7b9 layout,
 * chosen with goldbox.bss_order.  The originals are not recoverable from it.
 *
 * A menu's fkeys flag selects key_filter, which also takes key codes
 * 0x100..0x108 and 0x140..0x159 (key_get's translated codes), over
 * key_filter_esc, which takes only Esc and backquote.
 */

#include "dqk.h"

extern void far ui_mark(int id, int attr, ...);                  /* 1aa6:188f */

void far frame_invalidate(void);
char far step_mode(void);
char far menu_run(char far *title, char far *text, char fkeys, char anim);
void far set_menu_kind(int kind);
char far menu_input(char anim, char far * far *items, char far *esc);
void far set_item_count(int item_count);
char far extra_row(void);
static void far menu_split(char far *menu_text);

static int menu_kind = 2;                                        /* 1d0c */
static char timed_out = 0;                                       /* 1d0e */
static char aborted = 0;                                         /* 1d0f */
static char picked = 0;                                          /* 1d10 */
static char space_hit = 0;                                       /* 1d11 */
static char has_default = 0;                                     /* 1d12 */
static char redraw = 1;                                          /* 1d13 */
static int list_top = 0;                                         /* 1d14 */

static int num;                                                  /* a658 items in the menu */
static unsigned char width;                                      /* a65a */
static struct Node far *cursor;                                  /* a65b */
static unsigned char height;                                     /* a65f */
static char text_buf[80];                                        /* a660 */
static int last_num;                                             /* a6b0 */
static int raw_key;                                              /* a6b2 */
static char show_extra;                                          /* a6b4 */
static char full_frame;                                          /* a6b5 */
static char last_text[80];                                       /* a6b6 */
static ItemProc saved_hook;                                      /* a706 */
static int click_row;                                            /* a70a */
static unsigned char def_index;                                  /* a70c */
static struct Node far *list_first;                              /* a70d */
static int click_column;                                         /* a711 */
static char last_prompt[80];                                     /* a713 */
static char prompt_text[81];                                     /* a763 */
static char stepping_mode;                                       /* a7b4 */
static struct Node far *selected_node;                           /* a7b5 */

void far hotkey_add(char key, unsigned char far *count)          /* 1bdb:0005 */
{
    if (*count == 0)
        mem_fill(hotkeys, sizeof hotkeys, 0xff);
    hotkeys[*count].key = key;
    (*count)++;
}

void far hotkey_reset(unsigned char last_hotkey)                           /* 1bdb:0047 */
{
    unsigned char index;

    mem_fill(hotkeys, sizeof hotkeys, 0xff);
    for (index = 0; index <= last_hotkey; index++)
        hotkeys[index].key = index;
}

int far button_hook(struct Wid far *widget, int msg, int x, int y) /* 1bdb:008e */
{
    short top;
    short left;
    short bottom;
    short right;
    register int hi;
    register int attr;

    if (widget->flags & 0x20) {
        if (msg == 1) {
            widget->flags |= 0x80;
            if (widget->flags & 2)
                return 0;
            if (widget->flags & 9)
                hi = 3;
            else
                hi = 0;
            draw_glyph(widget->row, widget->col, 1, hi + 13);
            for (left = 0; left < widget->width; left++)
                draw_glyph(widget->row, widget->col + left * 4, 1, hi + 14);
            draw_glyph(widget->row, widget->col + (left - 1) * 4, 1, hi + 15);
            if (hi)
                hi = 1;
            left = widget->col;
            text_xy(widget->row, left, &top, &left);
            if (widget->flags & 1)
                attr = 0x80;
            else if (widget->flags & 0x40) {
                if (hi)
                    attr = 0x8b;
                else
                    attr = 0x8f;
            } else {
                if (hi)
                    attr = 0x83;
                else
                    attr = 0x87;
            }
            text_print_at(top + 1, left, attr, widget->text);
            if (!(widget->flags & 0x41))
                text_print_at(top + 1, left, (hi ? 0xb : 0xf) + 0x80, "%c", *widget->text);
            return 0;
        } else if (msg == 2) {
            if (widget->flags & 3)
                return 0;
            text_xy(widget->row - 1, widget->col - 2, &top, &left);
            text_xy(widget->row + 5, widget->col + widget->width * 4 + 2, &bottom, &right);
            return top <= x && x < bottom && left <= y && y < right;
        }
    }
    return (*saved_hook)(widget, msg, x, y);
}

void far button_hook_install(void)                               /* 1bdb:02a9 */
{
    saved_hook = ui_set_hook(1, button_hook);
}

char far get_picked(void)                                        /* 1bdb:02c2 */
{
    return picked;
}

void far set_picked(char value)                                      /* 1bdb:02c6 */
{
    picked = value;
}

void far set_selected(struct Node far *node)                        /* 1bdb:02d1 */
{
    selected_node = node;
}

struct Node far *far get_selected(void)                          /* 1bdb:02e1 */
{
    return selected_node;
}

void far set_aborted(char value)                                     /* 1bdb:02e9 */
{
    aborted = value;
}

char far get_aborted(void)                                       /* 1bdb:02f4 */
{
    return aborted;
}

struct Node far *far list_nth(int entry_index, struct Node far *head)      /* 1bdb:02f8 */
{
    struct Node far *node;
    register int index;

    index = 0;
    for (node = head; node && index != entry_index; index++)
        node = node->next;
    if (index == entry_index)
        return node;
    return 0;
}

int far list_count(struct Node far *head)                        /* 1bdb:033c */
{
    struct Node far *node;
    register int entry_count;

    entry_count = 0;
    for (node = head; node; node = node->next)
        entry_count++;
    return entry_count;
}

void far box_draw(void)                                          /* 1bdb:036c */
{
    clip_set(0x1f9d, 0x1f40, 0x1fa4, 0x1fe0);
    draw_glyph(0x1f40, 0x1f40, 1, 4);
    clip_reset();
    frame_invalidate();
}

void far box_draw_top(void)                                      /* 1bdb:03a4 */
{
    clip_set(0x1f98, 0x1f40, 0x1f9d, 0x1fe0);
    draw_glyph(0x1f40, 0x1f40, 1, 6);
    clip_reset();
}

void far box_draw_both(void)                                     /* 1bdb:03d7 */
{
    clip_set(0x1f9d, 0x1f40, 0x1fa4, 0x1fe0);
    draw_glyph(0x1f40, 0x1f40, 1, 4);
    clip_reset();
    draw_glyph(0x1f40, 0x1f40, 1, 6);
    frame_invalidate();
}

/* Every caller pushes a second byte, 0 or 1, that the body never reads; the
 * prompt animates by confirm_anim instead. */
char far confirm(char far *prompt, char anim)                    /* 1bdb:0424 */
{
    unsigned char key;

    hotkey_reset(1);
    do {
        key = menu_run(prompt, confirm_items, 1, confirm_anim);
        if (menu_escaped) {
            if (key == 13)
                key = 0;
            else if (key == 0x1b || key == '`')
                key = 1;
            else
                key = 2;
        }
    } while (key < 0 || key > 1);
    return !key;
}

void far press_return(void)                                      /* 1bdb:0495 */
{
    input_pending_clear();
    screen_draw_begin(1);
    box_draw();
    status_printf(7, 0x18, 0, 7, "Press        to continue.");
    frame_invalidate();
    dl_begin();
    ui_add(1, 0x1f9e, 0x1f78, "Return", 0x24, 6, 0x20, 0x40, 0x14, 0x16, 0x15, 0);
    dl_draw(1);
    screen_flush();
}

void far press_return_clear(void)                                /* 1bdb:0511 */
{
    press_return();
    ui_run(0);
}

void far press_return_wait(char anim)                            /* 1bdb:0521 */
{
    press_return();
    set_menu_kind(2);
    menu_input(anim, 0, &menu_escaped);
}

void far list_alloc(int entry_count, struct Node far * far *head)          /* 1bdb:0547 */
{
    struct Node far *node;
    register int index;

    pool_alloc((void far * far *)head, 40, node_pool);
    if (*head == 0)
        return;
    (*head)->next = 0;
    (*head)->key = 0;
    (*head)->text[0] = 0;
    node = *head;
    for (index = 1; index < entry_count; index++) {
        pool_alloc((void far * far *)node, 40, node_pool);
        if (node->next == 0)
            return;
        node = node->next;
        node->next = 0;
        node->key = 0;
        node->text[0] = 0;
    }
}

void far list_free(struct Node far * far *head)                  /* 1bdb:05fb */
{
    struct Node far *node;
    struct Node far *released_node;

    node = *head;
    while (node) {
        released_node = node;
        node = node->next;
        if (released_node)
            pool_free(released_node, 40, node_pool);
    }
}

int far key_filter(int id, register int key)                     /* 1bdb:064f */
{
    while (key_poll()) {
        if (key == 0x152)
            sound_toggle();
        else if (key == 0x153)
            music_toggle();
        key = key_get();
    }
    if ((key >= 0x100 && key <= 0x108) || key == 0x1b || key == '`' ||
        (key >= 0x140 && key <= 0x159)) {
        if (key == 0x152)
            sound_toggle();
        if (key == 0x153)
            music_toggle();
        raw_key = key;
        return 1;
    }
    return 0;
}

int far key_filter_esc(int id, register int key)                 /* 1bdb:06c6 */
{
    while (key_poll()) {
        if (key == 0x152)
            sound_toggle();
        else if (key == 0x153)
            music_toggle();
        key = key_get();
    }
    if (key == 0x1b || key == '`') {
        raw_key = key;
        return 1;
    }
    if (key == 0x152)
        sound_toggle();
    else if (key == 0x153)
        music_toggle();
    return 0;
}

int far menu_parse(char far *menu_text, char far * far *items)           /* 1bdb:0727 */
{
    char far *text_cursor;
    short count;
    char done;
    char separator_seen;

    has_default = 0;
    count = 0;
    for (text_cursor = menu_text; *text_cursor == ' '; text_cursor++)
        ;
    if (*text_cursor) {
        done = 0;
        do {
            items[count] = text_cursor;
            separator_seen = 0;
            do {
                text_cursor++;
                if (*text_cursor == '@')
                    separator_seen = 1;
            } while (*text_cursor && (*text_cursor < 'A' || *text_cursor > 'Z') && (*text_cursor < '0' || *text_cursor > '9'));
            if (separator_seen) {
                separator_seen = 0;
                def_index = count;
                has_default = 1;
            }
            if (*text_cursor == 0)
                done = 1;
            else
                text_cursor[-1] = 0;
            count++;
        } while (!done);
    } else
        items[0] = 0;
    return count;
}

void far menu_draw_items(int col, int item_count, char far * far *items)  /* 1bdb:0801 */
{
    short len;
    short start;
    register int index;
    register int attr;

    start = col;
    for (index = 0; index < item_count; index++) {
        len = str_len(items[index]);
        if (has_default) {
            if (def_index >= index)
                attr = 0x1f99;
            else
                attr = 0x1f9f;
        } else
            attr = 0x1f9e;
        ui_add(1, attr, col, items[index], 0x24, len, 0x20, to_upper(*items[index]),
               0x21, to_lower(*items[index]), 0x14, 0x15, 0);
        if (has_default && def_index == index)
            col = start;
        else
            col += (len + 1) * 4;
    }
}

void far menu_show(int item_count, char far * far *items, char far *title,
                   char fkeys, char show)                        /* 1bdb:08ef */
{
    char buf[42];
    register int len;
    register int col;

    menu_escaped = 0;
    if (*title) {
        str_copy(buf, title);
        len = str_len(buf);
        if (title[len - 1] == ' ' || title[len - 1] == '@') {
            if (title[len - 2] == ':')
                buf[--len] = 0;
        } else if (title[len - 1] != ':') {
            str_cat(buf, " ");
            len++;
        }
        if (show)
            status_printf(0, 0x18, 0, 7, "%s", buf);
        col = len * 4 + 0x1f44;
    } else
        col = 0x1f44;
    if (show) {
        ui_add(7, fkeys ? key_filter : key_filter_esc, 0x14, 0);
        if (**items)
            menu_draw_items(col, item_count, items);
    }
}

void far grid_hit(int id, int x, int y)                          /* 1bdb:09f7 */
{
    short col;
    short row;

    text_xy_to(0x1f44, 0x1f44, x, y, &col, &row);
    click_column = col / 12;
    click_row = row / 12;
}

void far node_hit(int id, int x, int y)                          /* 1bdb:0a33 */
{
    short col;
    short row;
    struct Character far *character_cursor;
    char found;
    register int index;
    register int target;

    picked = 0;
    text_xy_to(frame_row, frame_col, x, y, &col, &row);
    target = col / 4;
    index = 0;
    character_cursor = node_list;
    found = 0;
    while (found != 1) {
        if (character_cursor == cur_node)
            found = 1;
        else {
            character_cursor = character_cursor->next;
            index++;
        }
    }
    if (index == target && (menu_kind == 13 || menu_kind == 9)) {
        ui_mark(id, 0x14);
        picked = 1;
    }
    while (index != target) {
        if (!step_mode())
            party_key_select(0x85);
        else
            order_down();
        index = (index + 1) % node_count;
    }
    if (!picked) {
        if (menu_kind == 12)
            set_aborted(1);
        party_display(cur_node);
    }
}

void far item_hit(int id, int x, int y)                          /* 1bdb:0b21 */
{
    short col;
    short row;
    struct Character far *character_cursor;
    char found;
    register int index;
    register int target;

    text_xy_to(frame_row, frame_col, x, y, &col, &row);
    target = col / 4;
    index = 0;
    character_cursor = node_list;
    found = 0;
    while (found != 1) {
        if ((struct Character far *)get_selected() == character_cursor)
            found = 1;
        else {
            character_cursor = character_cursor->next;
            index++;
        }
    }
    if (index == target && (menu_kind == 13 || menu_kind == 7)) {
        ui_mark(id, 0x14);
        picked = 1;
    }
    while (index != target) {
        set_selected(get_selected()->next);
        if (!get_selected())
            set_selected((struct Node far *)node_list);
        index = (index + 1) % node_count;
    }
    party_display((struct Character far *)get_selected());
}

void far menu_frame(register int row, register int col)          /* 1bdb:0c00 */
{
    struct Character far *character_cursor;

    character_cursor = node_list;
    for (node_count = 0; character_cursor && node_count < 9; node_count++)
        character_cursor = character_cursor->next;
    if (!full_frame) {
        if (last_num != node_count)
            ui_mark(num, 0x2a, node_count * 4);
    } else if (menu_kind == 7) {
        ui_add(5, row, col, node_count * 4, 0x1fe0 - (0xa0 - col), 0x22, item_hit, 0);
    } else if (menu_kind == 9 || menu_kind == 12) {
        ui_add(5, row, col, node_count * 4, 0x1fe0 - (0xa0 - col), 0x22, node_hit, 0x14, 0);
    } else {
        ui_add(5, row, col, node_count * 4, 0x1fe0 - (0xa0 - col), 0x22, node_hit, 0);
    }
    last_num = node_count;
}

void far frame_invalidate(void)                                  /* 1bdb:0d1d */
{
    redraw = full_frame = 1;
}

int far menu_setup(char far *text, char far * far *items, char far *title,
                   char far *fkeys)                              /* 1bdb:0d26 */
{
    register int item_count;

    menu_split(text);
    if (str_cmp(last_text, prompt_text) || str_cmp(last_prompt, title))
        redraw = 1;
    else if (menu_kind == 8)
        redraw = 0;
    str_copy(text_buf, prompt_text);
    str_copy(last_text, prompt_text);
    str_copy(last_prompt, title);
    item_count = menu_parse(text_buf, items);
    set_item_count(item_count);
    if (redraw) {
        dl_begin();
        screen_draw_begin(1);
        screen_flush_mode_set(1);
        box_draw();
    }
    menu_show(item_count, items, title, *fkeys, redraw);
    if (redraw) {
        dl_draw(1);
        screen_flush_mode_set(0);
        screen_flush();
    }
    full_frame = redraw;
    redraw = 0;
    return item_count;
}

void far set_item_count(int item_count)                                   /* 1bdb:0e37 */
{
    num = item_count;
}

int far get_item_count(void)                                     /* 1bdb:0e42 */
{
    return num;
}

static void far menu_split(char far *menu_text)                          /* 1bdb:0e46 */
{
    unsigned char scan_index;
    unsigned char start;
    unsigned char index;
    unsigned char hotkey_index;
    unsigned char len;
    char buf[82];

    len = str_length(menu_text);
    str_copy(prompt_text, "");
    hotkey_index = 0;
    scan_index = 0;
    index = 0;
    while (index < len) {
        if ((menu_text[index] >= 'A' && menu_text[index] <= 'Z') || (menu_text[index] >= '0' && menu_text[index] <= '9')) {
            if (hotkeys[hotkey_index].key == scan_index) {
                hotkeys[hotkey_index].ch = menu_text[index];
                start = index;
                do
                    index++;
                while ((menu_text[index] < 'A' || menu_text[index] > 'Z' || (menu_text[index] >= '0' && menu_text[index] <= '9')) && index < len);
                str_copy(buf, substr(menu_text, start + 1, index - start));
                str_copy(prompt_text, msg_format("%s%s", prompt_text, buf));
                hotkey_index++;
            } else
                index++;
            scan_index++;
        } else
            index++;
    }
    len = str_length(prompt_text);
    if (prompt_text[len - 1] == ' ' || prompt_text[len - 1] == '@')
        str_delete(prompt_text, len, 1);
}

int far menu_wait_key(char anim)                                 /* 1bdb:1008 */
{
    long tick[5];
    long flash_at;
    long flash_end;
    long frame_at;
    char key;
    register int return_code;

    timed_out = 0;
    if (anim_off || anim_last < 1)
        anim = 0;
    if (menu_kind == 2 || menu_kind == 7 || menu_kind == 13 || menu_kind == 12) {
        for (tick_slot = 1; tick_slot <= 4; tick_slot++)
            tick[tick_slot] = ticks_now();
        frame_at = ticks_now();
        flash_at = ticks_now() + 30;
        flash_end = flash_at + 50;
    }
    do {
        if (is_xvga() || !pixel_shift())
            input_poll();
        key = key_pressed();
        return_code = ui_poll();
        if (!key && (menu_kind == 2 || menu_kind == 7 || menu_kind == 13 || menu_kind == 12)) {
            if (view_mode == 3 && view_flag == 'y' && view_kind != 'P' && ticks_now() >= flash_at)
                flash_at = flash_end + 30;
            if (anim && anim_speed > 0) {
                pic_frame(3, 3, anim, anim_frame);
                if (anim_first)
                    moon_display_update(1);
                if (ticks_now() - frame_at >= 50) {
                    if (++anim_frame > anim_last)
                        anim_frame = anim_first;
                    frame_at = ticks_now();
                }
            }
            if (menu_timeout > 0 && ticks_now() - frame_at >= menu_timeout) {
                timed_out = 1;
                break;
            }
            if (view_mode == 3 && view_flag == 'y' && view_kind != 'P' && ticks_now() >= flash_end)
                flash_end = flash_at + 50;
        }
    } while (return_code < 0);
    if (get_aborted())
        return_code = 0;
    return return_code;
}

char far menu_input(char anim, char far * far *items, char far *esc) /* 1bdb:1206 */
{
    int return_code;
    char key;
    unsigned char inner_index;
    char hotkey_char;
    register int item_count;

    return_code = menu_wait_key(anim);
    if (timed_out)
        return timeout_key;
    item_count = get_item_count();
    if (menu_kind == 1)
        item_count++;
    if ((menu_kind == 4 || menu_kind == 1) && return_code > item_count && item_count + 8 >= return_code) {
        menu_escaped = 1;
        switch (return_code - item_count - 1) {
        case 0: key = 0x86; break;
        case 1: key = 0x88; break;
        case 2: key = 0x82; break;
        case 3: key = 0x84; break;
        case 4: key = 0x87; break;
        case 5: key = 0x81; break;
        case 6: key = 0x83; break;
        case 7: key = 0x85; break;
        }
    } else if (menu_kind == 4 && item_count + 9 == return_code) {
        menu_escaped = 1;
        key = ' ';
    } else if (menu_kind == 5 && return_code > item_count && return_code != 0xff) {
        view_click(return_code - item_count, click_column, click_row);
        key = 0x1b;
    } else if (menu_kind == 10 && return_code > item_count && return_code != 0xff) {
        set_item_count(get_item_count() + 1);
        return_code = return_code - get_item_count();
        key = return_code;
        click_set(1);
    } else if ((menu_kind == 7 && picked) || (menu_kind == 13 && picked) ||
               (menu_kind == 12 && get_aborted())) {
        key = 'S';
    } else {
        if (return_code == 0) {
            if (raw_key > 0xff)
                key = (raw_key & 0x7f) + 0x80;
            else {
                key = raw_key;
                if (raw_key < 0x20)
                    menu_escaped = 1;
            }
        } else {
            inner_index = 0;
            hotkey_char = *items[return_code - 1];
            while (inner_index < 20 && hotkeys[inner_index].ch != hotkey_char)
                inner_index++;
            if (inner_index < 20)
                key = hotkeys[inner_index].key;
            else
                key = 0xff;
        }
        if (key == '`')
            key = 0x1b;
        if ((unsigned char)key > 20)
            menu_escaped = 1;
    }
    menu_kind = 2;
    return key;
}

void far set_menu_kind(int kind)                                 /* 1bdb:141e */
{
    menu_kind = kind;
}

char far menu_bar(char far *title, char far *text, char fkeys, char anim) /* 1bdb:1429 */
{
    char far *items[20];
    char key;

    set_item_count(menu_setup(text, items, title, &fkeys));
    if (full_frame) {
        ui_add(5, 0x1f64, 0x1f40, 4, 0x50, 0x14,
               5, 0x1f68, 0x1f40, 4, 0x50, 0x14,
               5, 0x1f6c, 0x1f40, 4, 0x50, 0x14,
               5, 0x1f70, 0x1f40, 4, 0x50, 0x14,
               5, 0x1f74, 0x1f40, 4, 0x50, 0x14,
               5, 0x1f78, 0x1f40, 4, 0x50, 0x14,
               5, 0x1f44, 0x1f40, 4, 0x50, 0x14, 0);
        if (extra_row())
            ui_add(0x11, 0);
        ui_add(5, 0x1f4c, 0x1f90, 4, 0x50, 0x14, 0);
    }
    set_menu_kind(10);
    key = menu_input(anim, items, &menu_escaped);
    return key;
}

int far space_cb(int id, int key)                                /* 1bdb:1570 */
{
    if (key == ' ') {
        space_hit = 1;
        ui_mark(id, 0x14);
        return 1;
    }
    return 0;
}

char far grid_choose(char far *title, char far *text, char fkeys, char anim,
                     unsigned char rows, unsigned char cols)     /* 1bdb:1595 */
{
    char far *items[20];
    char key;

    if (height != rows || width != cols) {
        frame_invalidate();
        height = rows;
        width = cols;
    }
    set_item_count(menu_setup(text, items, title, &fkeys));
    if (full_frame) {
        ui_add(5, rows * 12 + 0x1f44, 0x1f40, 0xc, cols * 12 + 4, 0x29, 9, 0x14,
               5, 0x1f40, cols * 12 + 0x1f44, rows * 12 + 2, 0xc, 0x29, 0xb, 0x14,
               5, rows * 12 + 0x1f44, (cols + 1) * 12 + 0x1f42, 0xc, (6 - cols) * 12 + 6, 0x29, 5, 0x14,
               5, (rows + 1) * 12 + 0x1f46, cols * 12 + 0x1f42, (6 - rows) * 12 + 4, 0xc, 0x29, 7, 0x14,
               0);
    }
    if (full_frame) {
        ui_add(5, 0x1f40, 0x1f40, rows * 12 + 4, cols * 12 + 4, 0x29, 0xa, 0x14,
               5, 0x1f40, (cols + 1) * 12 + 0x1f44, rows * 12 + 4, (6 - cols) * 12 + 4, 0x29, 4, 0x14,
               5, (rows + 1) * 12 + 0x1f44, (cols + 1) * 12 + 0x1f44, (6 - rows) * 12 + 4,
               (6 - cols) * 12 + 4, 0x29, 6, 0x14,
               5, (rows + 1) * 12 + 0x1f44, 0x1f40, (6 - rows) * 12 + 4, cols * 12 + 4, 0x29, 8, 0x14,
               0);
    }
    if (full_frame)
        ui_add(7, space_cb, 0);
    set_menu_kind(4);
    key = menu_input(anim, items, &menu_escaped);
    if (space_hit) {
        space_hit = 0;
        key = ' ';
    }
    return key;
}

char far grid_pick(char far *title, char far *text, char fkeys, char anim) /* 1bdb:1862 */
{
    char far *items[20];
    char key;

    set_item_count(menu_setup(text, items, title, &fkeys));
    if (full_frame) {
        ui_add(5, 0x1f44, 0x1f40, 0x54, 4, 0x29, 9, 0x14, 0x22, grid_hit,
               5, 0x1f40, 0x1f44, 4, 0x54, 0x29, 0xb, 0x14, 0x22, grid_hit,
               5, 0x1f44, 0x1f98, 0x54, 8, 0x29, 5, 0x14, 0x22, grid_hit,
               5, 0x1f98, 0x1f44, 0xc, 0x54, 0x29, 7, 0x14, 0x22, grid_hit,
               0);
    }
    if (full_frame)
        ui_add(5, 0x1f44, 0x1f44, 0x54, 0x54, 0x29, 0x1c, 0x14, 0x22, grid_hit, 0);
    key = menu_input(anim, items, &menu_escaped);
    return key;
}

char far node_menu(char far *title, char far *text, char fkeys, char anim) /* 1bdb:1997 */
{
    char far *items[20];
    char key;

    set_menu_kind(7);
    set_item_count(menu_setup(text, items, title, &fkeys));
    if (view_mode == 0) {
        frame_row = 0x1f50;
        frame_col = 0x1f44;
    } else {
        frame_row = 0x1f50;
        frame_col = 0x1f84;
    }
    menu_frame(frame_row, frame_col);
    picked = 0;
    key = menu_input(anim, items, &menu_escaped);
    return key;
}

char far node_prompt(char far *title, char far *text, char fkeys, char anim) /* 1bdb:1a1b */
{
    char key;
    char far *items[20];

    frame_row = 0x1f50;
    frame_col = 0x1f84;
    if (str_same(text, continue_items)) {
        set_item_count(1);
        set_menu_kind(2);
        press_return();
        menu_frame(frame_row, frame_col);
        menu_input(anim, 0, &menu_escaped);
        menu_escaped = 0;
        key = 13;
    } else {
        if (menu_kind != 12 && menu_kind != 13)
            set_menu_kind(2);
        set_item_count(menu_setup(text, items, title, &fkeys));
        menu_frame(frame_row, frame_col);
        key = menu_input(anim, items, &menu_escaped);
    }
    return key;
}

char far node_dir_menu(char far *title, char far *text, char fkeys, char anim) /* 1bdb:1aef */
{
    char far *items[20];
    char key;

    frame_row = 0x1f50;
    frame_col = 0x1f84;
    set_menu_kind(1);
    set_item_count(menu_setup(text, items, title, &fkeys));
    menu_frame(frame_row, frame_col);
    if (full_frame) {
        ui_add(5, 0x1f40, 0x1f40, 0x32, 0x14, 0x29, 0x1a, 0x14,
               5, 0x1f40, 0x1f54, 0x32, 0x1c, 0x29, 0xb, 0x14,
               5, 0x1f40, 0x1f70, 0x32, 0x14, 0x29, 0x19, 0x14,
               5, 0x1f72, 0x1f40, 0x1e, 0x44, 0x29, 0x1b, 0x14,
               0);
    }
    key = menu_input(anim, items, &menu_escaped);
    return key;
}

char far grid_small(char far *title, char far *text, char fkeys, char anim,
                    unsigned char rows, unsigned char cols)      /* 1bdb:1beb */
{
    char far *items[20];
    char key;

    frame_invalidate();
    rows *= 4;
    cols *= 4;
    set_menu_kind(4);
    set_item_count(menu_setup(text, items, title, &fkeys));
    if (full_frame) {
        ui_add(5, rows + 0x1f42, 0x1f3f, 6, cols + 3, 0x29, 9, 0x14,
               5, 0x1f3f, cols + 0x1f42, rows + 3, 6, 0x29, 0xb, 0x14,
               5, rows + 0x1f42, cols + 0x1f48, 6, 0xff - cols, 0x29, 5, 0x14,
               5, rows + 0x1f48, cols + 0x1f42, 0xff - rows, 6, 0x29, 7, 0x14,
               0);
    }
    if (full_frame) {
        ui_add(5, 0x1f3f, 0x1f3f, rows + 3, cols + 3, 0x29, 0xa, 0x14,
               5, 0x1f3f, cols + 0x1f48, rows + 3, 0xff - cols, 0x29, 4, 0x14,
               5, rows + 0x1f48, cols + 0x1f48, 0xff - rows, 0xff - cols, 0x29, 6, 0x14,
               5, rows + 0x1f48, 0x1f3f, 0xff - rows, cols + 3, 0x29, 8, 0x14,
               0);
    }
    if (full_frame)
        ui_add(5, rows + 0x1f42, cols + 0x1f42, 6, 6, 0x29, 3, 0);
    key = menu_input(anim, items, &menu_escaped);
    return key;
}

char far menu_run(char far *title, char far *text, char fkeys, char anim) /* 1bdb:1e29 */
{
    char far *items[20];
    char key;

    if (str_same(text, continue_items)) {
        press_return_wait(anim);
        key = 13;
        menu_escaped = 0;
    } else {
        menu_setup(text, items, title, &fkeys);
        key = menu_input(anim, items, &menu_escaped);
    }
    return key;
}

char far *far list_next(int step_count, char far *key)                    /* 1bdb:1e97 */
{
    register int index;

    if (step_count < 0) {
        if (!cursor->next)
            return 0;
        cursor = cursor->next;
    } else {
        cursor = list_first;
        for (index = 0; index < step_count; index++) {
            if (!cursor)
                return 0;
            cursor = cursor->next;
        }
    }
    if (!cursor)
        return 0;
    *key = cursor->key;
    return cursor->text;
}

char far list_done(void)                                         /* 1bdb:1f13 */
{
    ui_mark(1, 4);
    return 1;
}

/* a and b: every caller pushes two bytes here that the body never reads. */
char far list_menu(char far *title, char far *text, char row, char col,
                   char row2, char col2, struct Node far *list, char unused_arg_a, char unused_arg_b,
                   char far *redraw_box, int far *top,
                   struct Node far * far *out)                   /* 1bdb:1f25 */
{
    short return_code;
    char choice_char;
    char key;
    struct ListBox box;
    char far *items[20];
    struct Node far *node;
    char fkeys;                 /* never set: menu_setup reads a stale byte */
    char result;
    unsigned char inner_index;
    int index;
    register int item_count;

    if (!list) {
        top = 0;                /* the pointers, not *top and *out: 1bdb:1f35 */
        out = 0;
        return 0x1b;
    }
    item_count = menu_setup(text, items, title, &fkeys);
    if (*top < list_top) {
        list_top = *top;
        *redraw_box = 1;
    }
    box.save_top = list_top;
    node = list_first = list;
    for (index = 0; node; node = node->next, index++)
        if (*top == index && node->key)
            (*top)++;
    box.col = col * 4;
    box.row = row * 4;
    box.width = col2 - col;
    box.height = row2 - row;
    if (col2 - col < index)
        box.flags = 2;
    else
        box.flags = 0;
    box.top = *top;
    box.count = item_count + 1;
    box.fetch = list_next;
    box.done = list_done;
    box.extra = 0;
    if (box.flags) {
        if (row2 == '&')
            box.height--;
    } else if (row2 != '&')
        box.height++;
    frame_invalidate();
    dl_begin();
    screen_draw_begin(1);
    screen_flush_mode_set(1);
    box_draw();
    menu_show(item_count, items, title, 0, 1);
    dl_draw(1);
    screen_flush_mode_set(0);
    screen_flush();
    list_box(&box, *redraw_box);
    *redraw_box = 0;
    ui_mark(1, 0x21, 0x40);
    return_code = menu_wait_key(0);
    if (timed_out)
        return timeout_key;
    dl_end();
    if (return_code == 0) {
        choice_char = 0x1b;
        menu_escaped = 1;
    } else if (return_code > item_count)
        choice_char = *items[0];
    else
        choice_char = *items[return_code - 1];
    for (inner_index = 0; inner_index < 20 && hotkeys[inner_index].ch != choice_char; inner_index++)
        ;
    if (inner_index < 20)
        result = hotkeys[inner_index].key;
    else if (choice_char != 0x1b)
        result = 0xff;
    else
        result = 0x1b;
    *top = box.top;
    list_next(*top, &key);
    *out = cursor;
    list_top = box.save_top;
    return result;
}

void far set_step_mode(char enabled)                                   /* 1bdb:21c3 */
{
    stepping_mode = enabled;
}

char far step_mode(void)                                         /* 1bdb:21ce */
{
    return stepping_mode;
}

void far set_extra_row(char enabled)                                   /* 1bdb:21d2 */
{
    show_extra = enabled;
}

char far extra_row(void)                                         /* 1bdb:21dd */
{
    return show_extra;
}
