/* DQK.EXE overlay 03 module 2177: saving and loading.
 * Characters are <name>.qch files and saved games SavGamA..J.qsv in the
 * save directory, "%c:\SAVE" on config's save_drive, else disk S.  A
 * character file is the 0x19e-byte record, each item's 17-byte record (an
 * 'I' item's sub chain after it) and each 9-byte effect; a saved game is
 * 0x400 bytes from game_state + 1, parts of geo and the view state, the member
 * count and each member, padded out to 0x1f1d bytes from item_templates.  Also
 * the vault, Vault<c3ea>.DAT through poolfile.c's pool_load and pool_save;
 * creatures from the moncha1 data file joining the party; and importing
 * Death Knights of Krynn characters (.pc, .eff, .jnk, .wiz under config's
 * save_path) and saved games (savgam<c>.pty and chrdat<c><n>.gam), their
 * item, effect and spell ids mapped by three switches.
 * (v1.0: dkk_load does not re-equip the imported 0x3a items with two
 * item_equip_toggle calls; dkk_convert, dkk_effects_cb and dkk_items_cb
 * assign several word and long fields to themselves after copying them, as
 * shipped; dkk_effects_cb reads at most 0x80 bytes and dkk_items_cb 0x41e,
 * refusing a file that fills the buffer, and dkk_items_cb does not clamp
 * the name length; nibbles holds a permutation of 0..15.)
 *
 * EXE 0x03316e..0x03679c, one Turbo C large-model _TEXT segment starting at
 * byte 0xe of paragraph 2177 in RTLink section 3 (payload 0x30ed0, link
 * segment 1f4e).  The first 0xe bytes of paragraph 2177 are the tail of
 * eclrun.c (module 2033), whose cmd_nop (2033:1449) its own code
 * near-calls, and the range ends where picview.c (module 24da) begins, at
 * 24da:000c.  The near call at 2177:35f6 back to 000e shows the range is
 * one segment.  Built with Turbo C 2.01, tcc -ml -O -Z -d.  Functions are
 * defined in shipped order; bake consumes them in that order.  Section 0's
 * 1f4e link segment holds other bytes.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee and variable
 * names are placeholders chosen from use; a mod1d99_ callee is an RTLink
 * vector stub whose target is named beside it.  The ovl03_2177_ names are
 * the placeholders other modules call these routines by.  Sections 3, 5,
 * 6, 7 and 8 far-call routines here directly.  diskfile.c passes a file
 * callback (fd, path); these read only fd and take (int fd), the form
 * poolfile.c's pool_save and pool_load, passed here, have.  Eight routines
 * are proven static: called near, or pushed as a callback, ahead of their
 * definitions.  item_id_map and spell_map carry Turbo C's sparse-switch
 * value search (2e 3b 07), effect_map a jump table.
 *
 * _DATA is nibbles, 16 bytes nothing found reads, and 34 literals stored as
 * 33, DGROUP 5eec..6072: -d stores "pty" on the tail of "savgama.pty".
 * _BSS is acc6..acd0; save_drive and save_path are fields of config.c's
 * config (ace0, ace2).
 */

#include "dqk.h"

struct Dkk {                    /* DKK's TCharacter, 0xd8 bytes           */
    unsigned char name[16];     /* +0   String[15]                        */
    unsigned char stats[12];    /* +10                                    */
    unsigned char str_ex;       /* +1c                                    */
    unsigned char str_ex_base;  /* +1d                                    */
    unsigned char spell_prep;   /* +1e                                    */
    unsigned char thac0_base;   /* +1f                                    */
    unsigned char race;         /* +20                                    */
    unsigned char cls;          /* +21                                    */
    unsigned char knight_order; /* +22                                    */
    unsigned char deity;        /* +23                                    */
    unsigned char robe_order;   /* +24                                    */
    unsigned char cure;         /* +25                                    */
    int age;                    /* +26                                    */
    unsigned char hp_max;       /* +28                                    */
    unsigned char icon_dim;     /* +29                                    */
    unsigned char saves[5];     /* +2a                                    */
    unsigned char move_base;    /* +2f                                    */
    unsigned char level_high;   /* +30                                    */
    unsigned char level_undead; /* +31                                    */
    unsigned char thief[8];     /* +32                                    */
    void far *effects;          /* +3a                                    */
    unsigned char npc;          /* +3e                                    */
    int coins[3];               /* +3f                                    */
    unsigned char level[8];     /* +45                                    */
    unsigned char level_old[8]; /* +4d                                    */
    unsigned char gender;       /* +55                                    */
    unsigned char alignment;    /* +56                                    */
    unsigned char attack_rate[2]; /* +57                                  */
    unsigned char unarmed_rolls[2]; /* +59                                */
    unsigned char unarmed_dice[2]; /* +5b                                 */
    unsigned char unarmed_mods[2]; /* +5d                                 */
    unsigned char ac_base;      /* +5f                                    */
    unsigned char str_bonus;    /* +60                                    */
    unsigned char identity;     /* +61                                    */
    long xp;                    /* +62                                    */
    long xp_old;                /* +66                                    */
    unsigned char hp_old_max;   /* +6a                                    */
    unsigned char class_mask;   /* +6b                                    */
    unsigned char hp_base;      /* +6c                                    */
    int xp_award;               /* +6d                                    */
    unsigned char hp_rolled;    /* +6f                                    */
    unsigned char icon_head;    /* +70                                    */
    unsigned char icon_body;    /* +71                                    */
    unsigned char order_no;     /* +72                                    */
    unsigned char icon_size;    /* +73                                    */
    unsigned char icon_colors[6]; /* +74                                  */
    unsigned char type_flags;   /* +7a                                    */
    unsigned char immunity;     /* +7b                                    */
    unsigned char item_count;   /* +7c                                    */
    void far *items;            /* +7d                                    */
    void far *equipped[13];     /* +81                                    */
    unsigned char hands;        /* +b5                                    */
    unsigned char save_bonus;   /* +b6                                    */
    int encumbrance;            /* +b7                                    */
    void far *next;             /* +b9                                    */
    void far *combat;           /* +bd                                    */
    unsigned char spell_resist; /* +c1                                    */
    void far *spells;           /* +c2                                    */
    unsigned char level_up;     /* +c6                                    */
    unsigned char state;        /* +c7                                    */
    unsigned char enabled;      /* +c8                                    */
    unsigned char side;         /* +c9                                    */
    unsigned char quick_fight;  /* +ca                                    */
    unsigned char thac0_adj;    /* +cb                                    */
    unsigned char ac[2];        /* +cc                                    */
    unsigned char attacks_left[2]; /* +ce                                 */
    unsigned char rolls[2];     /* +d0                                    */
    unsigned char dice[2];      /* +d2                                    */
    unsigned char mods[2];      /* +d4                                    */
    unsigned char hp;           /* +d6                                    */
    unsigned char move;         /* +d7                                    */
};

extern void far status_msg(char far *fmt, char foreground, char background);     /* 0a62:0904 */
extern int  far path_default_ext(char far *path, char far *ext, int force);
                                                                 /* 1c60:025c */
extern int  far str_match(char far *text_cursor, char far *pattern_cursor);            /* 1d10:006d */

static char nibbles[16] = { 0, 7, 12, 13, 2, 14, 11, 9, 8, 6, 5, 15, 3, 10, 4, 1 };
                                /* 5eec, referenced by nothing found;     */
                                /* v1.0: a permutation, v1.1: 0..15       */

static char old_frame;                                           /* acc6 */
static char cur_slot;                                            /* acc7 */
static struct Character far *load_hero;                               /* acc8 */
static char pty_count;                                           /* accc */
static char far *entry_text;                                     /* accd */

void far party_character_append(struct Character far *hero);
void far vault_load_current(void);
void far vault_save_current(void);
static char far dkk_hero_cb(int file_handle);
static void far dkk_convert(struct Dkk far *dkk, struct Character far *hero);
static char far dkk_effects_cb(int file_handle);
static char far dkk_items_cb(int file_handle);
static char far dkk_spells_cb(int file_handle);
static int far item_id_map(register int id);
static int far effect_map(register int id);
static int far spell_map(register int id);

void far save_open(char far *name, int flags,
                   char (far *filename)(int fd))       /* 2177:000e */
{
    short disk;
    char path[80];

    if (save_drive != 0) {
        disk = 0;
        str_format(path, "%c:\\SAVE\\%s", save_drive, name);
    } else {
        disk = 'S';
        str_copy(path, name);
    }
    file_open(disk, path, flags, filename);
}

char far save_ready(void)                                        /* 2177:0073 */
{
    char disk;
    char path[79];              /* the frame holds 79: is_dir sits at -1  */
    char is_dir;

    if (save_drive != 0) {
        disk = 0;
        str_format(path, "%c:\\SAVE", save_drive);
    } else
        disk = 'S';
    while (!dir_first(disk, disk ? (char far *)0 : path, 0, 1, 0)) {
        hotkey_reset(1);
        if (menu_run("Please insert save disk.", "Ok Exit", 0, 0) == 1)
            return 0;
    }
    while (dir_next(&is_dir) != 0)
        ;
    return 1;
}

char far save_write(char far *name, char (far *filename)(int fd))
{                                                                /* 2177:010b */
    char disk;
    char path[80];

    hotkey_reset(1);
    if (save_drive != 0) {
        disk = 0;
        str_format(path, "%c:\\SAVE\\%s", save_drive, name);
    } else {
        disk = 'S';
        str_copy(path, name);
    }
    while (!archive_open(disk, path, 4, filename)
           && menu_run("Save error:", "Try again Exit", 0, view_mode == 2) != 1)
        hotkey_reset(1);
    /* no return: al is archive_open's, or menu_run's 1 when Exit is picked */
}

char far hero_name_cb(int file_handle)                                    /* 2177:01af */
{
    struct Character far *hero;

    hero = 0;                   /* hero->name is then the field's offset  */
    if (file_seek(file_handle, (long)hero->name, 0) < 0)
        return 0;
    return dos_io(0x3f00, entry_text, file_handle, 16) == 16;
}

char far pc_name_cb(int file_handle)                                      /* 2177:0211 */
{
    char err;
    char name[16];

    err = dos_io(0x3f00, name, file_handle, 16) != 16;
    mem_move(name + 1, entry_text, 15);
    entry_text[int_min(15, int_max(0, name[0]))] = 0;
    return !err;
}

void far file_scan(char far *ext, struct Node far * far *list,
                   struct Node far * far *files)                 /* 2177:028a */
{
    struct Node far *node;
    struct Node far *file;
    char far *name;
    char first;
    char is_dir;
    char path[80];

    *list = *files = 0;
    if (file_kind == 1) {
        if (save_drive != 0) {
            str_format(path, "%c:\\save", save_drive);
            dir_first(0, path, ext, 1, 0);
        } else
            dir_first('S', 0, ext, 1, 0);
    } else if (file_kind == 4)
        dir_first(0, save_path, ext, 1, 0);
    else
        dir_first('S', 0, ext, 1, 0);
    if ((name = dir_next(&is_dir)) == 0)
        return;
    first = 1;
    do {
        if (first) {
            pool_alloc(&node, sizeof(struct Node), node_pool);
            pool_alloc(&file, sizeof(struct Node), node_pool);
            if (file == 0) {
                *files = *list = 0;
                return;
            }
            *files = file;
            *list = node;
            first = 0;
        } else {
            pool_alloc(&node->next, sizeof(struct Node), node_pool);
            pool_alloc(&file->next, sizeof(struct Node), node_pool);
            if (file->next == 0) {
                node->next = *list;
                *list = (*list)->next;
                file->next = *files;
                *files = (*files)->next;
            }
            node = node->next;
            file = file->next;
        }
        mem_fill(node, sizeof(struct Node), 0);
        mem_fill(file, sizeof(struct Node), 0);
        str_copy(file->text, name);
        entry_text = node->text;
        switch (file_kind) {
        case 1:
            save_open(name, 0, hero_name_cb);
            break;
        case 4:
            str_copy(path, save_path);
            path_append_name(path, name);
            file_open(0, path, 0, pc_name_cb);
            break;
        }
    } while ((name = dir_next(&is_dir)) != 0);
}

void far character_files_scan(struct Node far * far *list,
                         struct Node far * far *files, char kind) /* 2177:054a */
{
    if (!save_ready()) {
        *list = *files = 0;
        return;
    }
    switch (file_kind) {
    case 1:
        file_scan("qch", list, files);
        break;
    case 4:
        file_scan("pc", list, files);
        break;
    }
    if (*list == 0)
        bottom_msg(kind ? "No characters to delete." : "No characters to load.");
}

void far character_icon_load(char unused_flag)                                /* 2177:05d7 */
{
    icon_load("CBODYS", cur_node->icon_body, cur_node->order_no);
    data_disk_pop();
}

char far hero_read_cb(register int file_handle)                           /* 2177:05fd */
{
    struct Effect far *eff_new;
    struct Effect far *eff;
    struct Item far *item;
    struct Item far *sub;
    struct Item far *list;
    struct Item far *prev;
    struct Item tmp;
    unsigned char extra;
    char err;
    char scrolls;
    register int remaining_items;

    err = dos_io(0x3f00, load_hero, file_handle, sizeof(struct Character)) != sizeof(struct Character);
    if (!err) {
        extra = 0;
        scrolls = 0;
        list = load_hero->items;
        load_hero->items = 0;
        remaining_items = (int)list;          /* the file holds the item count here     */
        item = 0;
        while (list != 0 && !err) {
            if (load_hero->items != 0) {
                pool_alloc(&item->next, sizeof(struct Item), &item_record_pool);
                prev = item;
                item = item->next;
            } else {
                pool_alloc(&load_hero->items, sizeof(struct Item), &item_record_pool);
                item = load_hero->items;
                prev = 0;
            }
            mem_fill(item, sizeof(struct Item), 0);
            err = dos_io(0x3f00, &item->rec, file_handle, sizeof(struct ItemRec))
                  != sizeof(struct ItemRec);
            if (--remaining_items > 0)
                list = item;
            else
                list = 0;
            item->next = 0;
            item->more = 0;
            if (item->rec.type == 'I') {
                sub = item;
                if (party_scrolls() + item->rec.quantity + extra > 0) {
                    scrolls = 1;
                    for (tick_slot = 1; tick_slot <= item->rec.quantity; tick_slot++)
                        if (!err)
                            err = dos_io(0x3f00, &tmp.rec, file_handle, sizeof(struct ItemRec))
                                  != sizeof(struct ItemRec);
                    if (prev != 0) {
                        pool_free(prev->next, sizeof(struct Item), &item_record_pool);
                        prev->next = 0;
                    } else {
                        pool_free(load_hero->items, sizeof(struct Item), &item_record_pool);
                        load_hero->items = 0;
                    }
                    item = prev;
                } else
                    for (tick_slot = 1; tick_slot <= item->rec.quantity; tick_slot++)
                        if (!err) {
                            pool_alloc(&sub->more, sizeof(struct Item), &item_record_pool);
                            sub = sub->more;
                            mem_fill(sub, sizeof(struct Item), 0);
                            err = dos_io(0x3f00, &sub->rec, file_handle, sizeof(struct ItemRec))
                                  != sizeof(struct ItemRec);
                            sub->next = 0;
                            sub->more = 0;
                            if (err)
                                item->rec.quantity = tick_slot - 1;
                            else
                                extra++;
                        }
            }
        }
        if (err) {
            for (item = load_hero->items; item != 0; ) {
                if (item->rec.type == 'I') {
                    sub = item->more;
                    /* cur_item, not item: pool_clear's loop, as shipped */
                    for (tick_slot = 1; tick_slot <= cur_item->rec.quantity; tick_slot++)
                        if (sub != 0) {
                            prev = sub;
                            sub = sub->more;
                            pool_free(prev, sizeof(struct Item), &item_record_pool);
                        }
                }
                prev = item;
                item = item->next;
                pool_free(prev, sizeof(struct Item), &item_record_pool);
            }
            load_hero->items = 0;
        }
        if (!err) {
            eff = load_hero->effects;
            load_hero->effects = 0;
            eff_new = 0;
            while (eff != 0 && !err) {
                if (load_hero->effects != 0) {
                    pool_alloc(&eff_new->next, sizeof(struct Effect), &effect_pool);
                    eff_new = eff_new->next;
                } else {
                    pool_alloc(&load_hero->effects, sizeof(struct Effect), &effect_pool);
                    eff_new = load_hero->effects;
                }
                err = dos_io(0x3f00, eff_new, file_handle, sizeof(struct Effect))
                      != sizeof(struct Effect);
                eff = eff_new->next;
                eff_new->next = 0;
            }
            if (err) {
                eff_new = load_hero->effects;
                while (eff_new != 0) {
                    eff = eff_new;
                    eff_new = eff_new->next;
                    pool_free(eff, sizeof(struct Effect), &effect_pool);
                }
                load_hero->effects = 0;
            }
        }
        load_hero->combat = 0;
        load_hero->next = 0;
        if (!err && scrolls)
            status_msg("SCROLLS DROPPED!", 0xb, 0);
    }
    return !err;
}

void far qch_load(char far *name, struct Character far *hero)         /* 2177:0b46 */
{
    char file[42];

    str_copy(file, name);
    load_hero = hero;
    save_open(file, 0, hero_read_cb);
}

void far dkk_load(char far *name, struct Character far *hero)         /* 2177:0b82 */
{
    char disk;
    char path[80];

    str_copy(path, save_path);
    path_append_name(path, name);
    disk = 0;
    load_hero = hero;
    file_open(disk, path, 0, dkk_hero_cb);
    path_default_ext(path, "eff", 1);
    archive_open(disk, path, 0, dkk_effects_cb);
    path_default_ext(path, "jnk", 1);
    archive_open(disk, path, 0, dkk_items_cb);
    path_default_ext(path, "wiz", 1);
    archive_open(disk, path, 0, dkk_spells_cb);
}

static char far dkk_hero_cb(int file_handle)                              /* 2177:0cd7 */
{
    char err;
    struct Dkk dkk;

    err = dos_io(0x3f00, &dkk, file_handle, sizeof dkk) != sizeof dkk;
    if (!err) {
        dkk_convert(&dkk, load_hero);
        load_hero->items = 0;
        load_hero->effects = 0;
        load_hero->combat = 0;
        load_hero->next = 0;
    }
    return !err;
}

static void far dkk_convert(struct Dkk far *dkk, struct Character far *hero)
{                                                                /* 2177:0d65 */
    struct Character far *saved;
    char class_index;

    mem_zero(hero, sizeof(struct Character));
    mem_move5(dkk->name + 1, hero->name, 15);
    hero->name[int_max(0, int_min(15, dkk->name[0]))] = 0;
    mem_move5(dkk->stats, hero->stats, 12);
    mem_move5(&dkk->str_ex, &hero->str_ex, 1);
    mem_move5(&dkk->str_ex_base, &hero->str_ex_base, 1);
    mem_move5(&dkk->spell_prep, &hero->spell_prep, 1);
    mem_move5(&dkk->thac0_base, &hero->thac0_base, 1);
    mem_move5(&dkk->race, &hero->race, 1);
    mem_move5(&dkk->cls, &hero->cls, 1);
    if (hero->cls == 7)
        hero->cls = 1;
    mem_move5(&dkk->knight_order, &hero->knight_order, 1);
    mem_move5(&dkk->deity, &hero->deity, 1);
    mem_move5(&dkk->robe_order, &hero->robe_order, 1);
    mem_move5(&dkk->cure, &hero->cure, 1);
    mem_move5(&dkk->age, &hero->age, 2);
    mem_move5(&dkk->hp_max, &hero->hp_max, 1);
    mem_move5(&dkk->icon_dim, &hero->icon_dim, 1);
    mem_move5(dkk->saves, hero->saves, 5);
    mem_move5(&dkk->move_base, &hero->move_base, 1);
    mem_move5(&dkk->level_high, &hero->level_high, 1);
    mem_move5(&dkk->level_undead, &hero->level_undead, 1);
    mem_move5(dkk->thief, hero->thief, 8);
    mem_move5(&dkk->npc, &hero->npc, 1);
    mem_move5(dkk->coins, hero->coins, 6);
    for (class_index = 0; class_index <= 2; class_index++)    /* v1.0: these self-assignments are as shipped */
        hero->coins[class_index] = hero->coins[class_index];
    mem_move5(dkk->level_old, hero->level_max, 8);
    hero->level_max[1] = hero->level_max[7];
    mem_move5(dkk->level, hero->level, 8);
    hero->level[1] = hero->level[7];
    for (class_index = 0; class_index <= 7; class_index++)    /* [7] is attack_rate[0], copied below    */
        hero->old_level[class_index] = 0;
    mem_move5(&dkk->gender, &hero->gender, 1);
    mem_move5(&dkk->alignment, &hero->alignment, 1);
    mem_move5(dkk->attack_rate, hero->attack_rate, 2);
    mem_move5(dkk->unarmed_rolls, hero->unarmed_rolls, 2);
    mem_move5(dkk->unarmed_dice, hero->unarmed_dice, 2);
    mem_move5(dkk->unarmed_mods, hero->unarmed_mods, 2);
    mem_move5(&dkk->ac_base, &hero->ac_base, 1);
    mem_move5(&dkk->str_bonus, &hero->str_bonus, 1);
    mem_move5(&dkk->identity, &hero->identity, 1);
    mem_move5(&dkk->xp, &hero->xp, 4);
    hero->xp = hero->xp;
    mem_move5(&dkk->xp_old, &hero->old_xp, 4);
    hero->old_xp = hero->old_xp;
    mem_move5(&dkk->hp_old_max, &hero->old_hp_max, 1);
    mem_move5(&dkk->class_mask, &hero->class_mask, 1);
    mem_move5(&dkk->hp_base, &hero->hp_base, 1);
    mem_move5(&dkk->xp_award, &hero->xp_award, 2);
    hero->xp_award = hero->xp_award;
    mem_move5(&dkk->icon_head, &hero->icon_head, 1);
    mem_move5(&dkk->icon_body, &hero->icon_body, 1);
    mem_move5(&dkk->order_no, &hero->order_no, 1);
    mem_move5(&dkk->icon_size, &hero->icon_size, 1);
    mem_move5(&dkk->type_flags, &hero->type_flags, 1);
    mem_move5(&dkk->immunity, &hero->immunity, 1);
    mem_move5(&dkk->item_count, &hero->item_count, 1);
    mem_move5(&dkk->hands, &hero->hands, 1);
    mem_move5(&dkk->save_bonus, &hero->save_bonus, 1);
    mem_move5(&dkk->encumbrance, &hero->encumbrance, 2);
    load_hero->encumbrance = load_hero->encumbrance;
    mem_move5(&dkk->spell_resist, &hero->spell_resist, 1);
    mem_move5(&dkk->level_up, &hero->level_up, 1);
    mem_move5(&dkk->state, &hero->status, 1);
    mem_move5(&dkk->enabled, &hero->enabled, 1);
    mem_move5(&dkk->side, &hero->side, 1);
    mem_move5(&dkk->quick_fight, &hero->quick_fight, 1);
    mem_move5(&dkk->thac0_adj, &hero->thac0_adj, 1);
    mem_move5(dkk->ac, hero->ac, 2);
    mem_move5(dkk->attacks_left, hero->attacks_left, 2);
    mem_move5(dkk->rolls, hero->rolls, 2);
    mem_move5(dkk->dice, hero->dice, 2);
    mem_move5(dkk->mods, hero->mods, 2);
    mem_move5(&dkk->hp, &hero->hp, 1);
    if (hp_floor(hero) < hero->hp_max)
        hero->hp_max = hp_floor(hero);
    if (hp_floor(hero) < hero->old_hp_max)
        hero->old_hp_max = hp_floor(hero);
    if (hp_floor(hero) < hero->hp_base)
        hero->hp_base = hp_floor(hero);
    if (hp_floor(hero) < hero->hp)
        hero->hp = hp_floor(hero);
    saved = cur_node;
    cur_node = hero;
    icon_default();
    cur_node = saved;
}

static char far dkk_effects_cb(int file_handle)                           /* 2177:16f8 */
{
    unsigned short bytes_read;
    struct Effect far *eff;
    char buf[0x80];             /* DKK TEffects, 9 bytes each             */
    register int record_offset;

    bytes_read = dos_io(0x3f00, buf, file_handle, sizeof buf);
    if (bytes_read != 0) {
        if (bytes_read >= 0x80)
            error_box("Too many effects!");
        else {
            record_offset = 0;
            pool_alloc(&load_hero->effects, sizeof(struct Effect), &effect_pool);
            eff = load_hero->effects;
            do {
                eff->id = effect_map(buf[record_offset]);
                mem_move5(&buf[record_offset + 1], &eff->duration, 2);
                eff->duration = eff->duration;
                mem_move5(&buf[record_offset + 3], &eff->strength, 2);
                eff->next = 0;
                record_offset += 9;
                if (bytes_read - 2 > record_offset) {
                    pool_alloc(&eff->next, sizeof(struct Effect), &effect_pool);
                    eff = eff->next;
                }
            } while (bytes_read - 2 > record_offset);
            return 1;
        }
    }
    return 0;
}

static char far dkk_items_cb(int file_handle)                             /* 2177:181b */
{
    unsigned short bytes_read;
    struct Item far *item;
    char buf[0x41e];            /* DKK TItemRecords, 0x3f bytes each      */
    register int record_offset;

    bytes_read = dos_io(0x3f00, buf, file_handle, sizeof buf);
    if (bytes_read != 0) {
        if (bytes_read >= 0x41e)
            error_box("Too many items!");
        else {
            record_offset = 0;
            pool_alloc(&load_hero->items, sizeof(struct Item), &item_record_pool);
            item = load_hero->items;
            do {
                mem_move5(&buf[record_offset + 1], item->name, 35);
                item->name[buf[record_offset]] = 0;
                mem_move5(&buf[record_offset + 0x29], &item->in_use, 1);
                mem_move5(&buf[record_offset + 0x2e], &item->rec.type, 1);
                for (tick_slot = 0; tick_slot <= 2; tick_slot++)
                    item->rec.name_ids[tick_slot] = item_id_map(buf[record_offset + 0x2f + tick_slot]);
                mem_move5(&buf[record_offset + 0x32], &item->rec.plus, 1);
                mem_move5(&buf[record_offset + 0x33], &item->rec.save_bonus, 1);
                mem_move5(&buf[record_offset + 0x34], &item->rec.equipped, 1);
                mem_move5(&buf[record_offset + 0x35], &item->rec.unid_mask, 1);
                mem_move5(&buf[record_offset + 0x36], &item->rec.cursed, 1);
                mem_move5(&buf[record_offset + 0x37], &item->rec.weight, 2);
                item->rec.weight = item->rec.weight;
                mem_move5(&buf[record_offset + 0x39], &item->rec.quantity, 1);
                mem_move5(&buf[record_offset + 0x3a], &item->rec.value, 2);
                item->rec.value = item->rec.value;
                mem_move5(&buf[record_offset + 0x3c], item->rec.u.spells, 3);
                if (item_casts(item)) {
                    item->rec.u.spells[0] = spell_map(item->rec.u.spells[0] & 0x7f)
                                             | item->rec.u.spells[0] & 0x80;
                    item->rec.u.spells[1] = spell_map(item->rec.u.spells[1] & 0x7f)
                                             | item->rec.u.spells[1] & 0x80;
                    item->rec.u.spells[2] = spell_map(item->rec.u.spells[2] & 0x7f)
                                             | item->rec.u.spells[2] & 0x80;
                } else if (item->rec.u.magic.special == 0x80)
                    item->rec.u.magic.effect = effect_map(item->rec.u.magic.effect);
                else
                    item->rec.u.magic.effect = spell_map(item->rec.u.magic.effect & 0x7f)
                                               | item->rec.u.magic.effect & 0x80;
                if (item->rec.type == 0x34) {
                    item->rec.type = 0x33;
                    item->rec.u.magic.special = 0;
                }
                item->next = 0;
                record_offset += 0x3f;
                if (bytes_read - 2 > record_offset) {
                    pool_alloc(&item->next, sizeof(struct Item), &item_record_pool);
                    item = item->next;
                }
            } while (bytes_read - 2 > record_offset);
            for (item = load_hero->items; item != 0; item = item->next)
                if (item->rec.type == 0x36)
                    item_remove(load_hero, item);
            return 1;
        }
    }
    return 0;
}

static int far item_id_map(register int id)                      /* 2177:1bf1 */
{
    switch (id) {
    case 0x0c: id = 0x8a; break;
    case 0x26: id = 0x8b; break;
    case 0x28: id = 0x8c; break;
    case 0x34: id = 0x8d; break;
    case 0x3b: id = 0x8e; break;
    case 0x44: id = 0x8f; break;
    case 0x4a: id = 0x90; break;
    case 0x4b: id = 0x91; break;
    case 0x4c: id = 0x92; break;
    case 0x54: id = 0x93; break;
    case 0x55: id = 0x94; break;
    case 0x56: id = 0x95; break;
    case 0x60: id = 0x96; break;
    case 0x77: id = 0x97; break;
    case 0x78: id = 0x98; break;
    case 0x7c: id = 0x99; break;
    case 0x43: id = 0x7e; break;
    case 0x45: id = 0x7f; break;
    case 0x4d: id = 0x81; break;
    case 0x69: id = 0x87; break;

    }
    return id;
}

static int far effect_map(register int id)                       /* 2177:1cc7 */
{
    switch (id) {
    case 0x02: id = 0x81; break;
    case 0x06: id = 0x82; break;
    case 0x07: id = 0x83; break;
    case 0x09: id = 0x4a; break;
    case 0x0d: id = 0x47; break;
    case 0x20: id = 0x45; break;
    case 0x22: id = 0x20; break;
    case 0x30: id = 0x0d; break;
    case 0x39: id = 0x76; break;
    case 0x3a: id = 0x7b; break;
    case 0x3b: id = 0x39; break;
    case 0x3c: id = 0x7a; break;
    case 0x3d: id = 0x44; break;
    case 0x41: id = 0x06; break;
    case 0x42: id = 0x3c; break;
    case 0x43: id = 0x65; break;
    case 0x44: id = 0x66; break;
    case 0x45: id = 0x84; break;
    case 0x46: id = 0x85; break;
    case 0x4a: id = 0x79; break;
    case 0x4b: id = 0x86; break;
    case 0x4c: id = 0x87; break;
    case 0x4d: id = 0x88; break;
    case 0x4f: id = 0x89; break;
    case 0x50: id = 0x8a; break;
    case 0x51: id = 0x7c; break;
    case 0x57: id = 0x4f; break;
    case 0x5b: id = 0x8c; break;
    case 0x5d: id = 0x42; break;
    case 0x60: id = 0x8d; break;
    case 0x61: id = 0x8e; break;
    case 0x63: id = 0x8f; break;
    case 0x72: id = 0xc8; break;
    case 0x73: id = 0x91; break;
    case 0x74: id = 0x92; break;
    case 0x75: id = 0x93; break;
    case 0x76: id = 0x94; break;
    case 0x28: case 0x4e: case 0x55: case 0x5a: case 0x64:
    case 0x68: case 0x6a: case 0x6c: case 0x78: case 0x79: id = 0x00; break;

    }
    return id;
}

static int far spell_map(register int id)                        /* 2177:1ea3 */
{
    switch (id) {
    case 0x02: id = 0x5a; break;
    case 0x04: id = 0x60; break;
    case 0x07: id = 0x69; break;
    case 0x11: id = 0x66; break;
    case 0x28: id = 0x38; break;
    case 0x35: id = 0x24; break;
    case 0x38: id = 0x6e; break;
    case 0x39: id = 0x7f; break;
    case 0x3b: id = 0x35; break;
    case 0x3c: id = 0x6f; break;
    case 0x3d: id = 0x82; break;
    case 0x3e: id = 0x83; break;
    case 0x3f: id = 0x70; break;
    case 0x40: id = 0x85; break;
    case 0x41: id = 0x86; break;
    case 0x42: id = 0x71; break;
    case 0x48: id = 0x72; break;
    case 0x5a: id = 0x77; break;
    case 0x5f: id = 0x87; break;
    case 0x60: id = 0x84; break;
    case 0x61: id = 0x88; break;
    case 0x62: id = 0x76; break;
    case 0x63: id = 0x89; break;
    case 0x65: id = 0x39; break;
    case 0x66: id = 0x3b; break;
    case 0x67: id = 0x3c; break;
    case 0x68: id = 0x3d; break;
    case 0x69: id = 0x3e; break;
    case 0x6a: id = 0x3f; break;
    case 0x6b: id = 0x40; break;
    case 0x6c: id = 0x73; break;
    case 0x6d: id = 0x74; break;
    case 0x6e: id = 0x75; break;
    case 0x6f: id = 0x78; break;
    case 0x70: id = 0x7a; break;
    case 0x71: id = 0x79; break;
    case 0x72: id = 0x7b; break;

    }
    return id;
}

static char far dkk_spells_cb(int file_handle)                            /* 2177:2021 */
{
    short spell;
    unsigned short bytes_read;
    char buf[0xdd];
    register int spell_slot;
    register int base;

    bytes_read = dos_io(0x3f00, buf, file_handle, sizeof buf);
    if (bytes_read != 0) {
        if (bytes_read >= 0xdd)
            error_box("Too much magic!");
        else {
            for (spell_slot = base = 0; spell_slot <= 0x49; spell_slot++)
                load_hero->spells[spell_slot] = spell_map(buf[base + spell_slot]);
            base = 0x4a;
            mem_zero(load_hero->known, 16);
            for (spell_slot = 0; spell_slot <= 0x71; spell_slot++) {
                spell = spell_map(spell_slot + 1);
                if (buf[base + spell_slot] != 0)
                    load_hero->known[(spell - 1) / 8] = load_hero->known[(spell - 1) / 8]
                                                       | bit_mask[(spell - 1) % 8];
            }
            mem_move5(&buf[base + 0x72], load_hero->slots[0], 8);
            mem_move5(&buf[base + 0x7a], load_hero->slots[1], 8);
            mem_move5(&buf[base + 0x82], load_hero->slots[3], 8);
            mem_move5(&buf[base + 0x8a], load_hero->slots[2], 8);
            return 1;
        }
    }
    return 0;
}

void far character_file_load(char far *name, struct Character far *hero, char unused_arg_a, char unused_arg_b)
{                                                                /* 2177:2198 */
    mem_fill(hero, sizeof(struct Character), 0);
    switch (file_kind) {
    case 1:
        qch_load(name, hero);
        break;
    case 4:
        dkk_load(name, hero);
        break;
    }
    recalc_stats(hero);
    recalc_from_levels(hero);
}

char far hero_write_cb(register int file_handle)                          /* 2177:2200 */
{
    struct Effect far *eff;
    struct Item far *item;
    struct Item far *sub;
    char err;
    register int item_count;

    item_count = 0;
    item = load_hero->items;
    while (item != 0) {
        item = item->next;
        item_count++;
    }
    item = load_hero->items;
    load_hero->items = (struct Item far *)(long)item_count; /* hero_read_cb's count */
    err = dos_io(0x4000, load_hero, file_handle, sizeof(struct Character)) != sizeof(struct Character);
    load_hero->items = item;
    for (item = load_hero->items; item != 0 && !err; item = item->next) {
        err = dos_io(0x4000, &item->rec, file_handle, sizeof(struct ItemRec)) != sizeof(struct ItemRec);
        if (!err && item->rec.type == 'I') {
            sub = item;
            for (tick_slot = 1; tick_slot <= item->rec.quantity; tick_slot++)
                if (!err) {
                    sub = sub->more;
                    err = dos_io(0x4000, &sub->rec, file_handle, sizeof(struct ItemRec))
                          != sizeof(struct ItemRec);
                }
        }
    }
    for (eff = load_hero->effects; eff != 0 && !err; eff = eff->next)
        err = dos_io(0x4000, eff, file_handle, sizeof(struct Effect)) != sizeof(struct Effect);
    return !err;
}

void far character_file_save(struct Character far *hero, char far *file)  /* 2177:23a7 */
{
    char name[41];
    char done;
    char msg[42];
    char far *ext;
    short disk;
    char path[80];

    ext = "qch";
    if (*file == 0) {
        str_copy(name, hero->name);
        name_clean(name);
        path_default_ext(name, ext, 1);
        done = 0;
    } else {
        str_copy(name, file);
        path_default_ext(name, ext, 1);
        done = 1;
    }
    if (!save_ready())
        return;
    while (!done) {
        if (save_drive != 0) {
            disk = 0;
            str_format(path, "%c:\\SAVE\\%s", save_drive, name);
        } else {
            disk = 's';
            str_copy(path, name);
        }
        if (!archive_open(disk, path, 0, 0))
            break;
        frame_border();
        str_copy(msg, msg_format("Update %s?", hero->name));
        done = confirm(msg, 0);
        if (!done) {
            box_draw();
            do {
                str_copy(name, input_text("New file name:", 0, 0, 0xf));
                name_clean(name);
            } while (name[0] == 0 && input_result());
            if (!input_result())
                return;
            path_default_ext(name, ext, 1);
        }
    }
    load_hero = hero;
    save_write(name, hero_write_cb);
}

void far mon_load(unsigned char id, char recs, struct Character far *hero) /* 2177:257c */
{
    int len;
    unsigned short n_effects;
    unsigned short n_items;
    unsigned short pos;
    short class_index;
    struct Item far *item;
    struct Effect far *eff;
    unsigned char buf[0x272];
    char far *packed;

    set_data_disk('2');
    data_load("moncha1", 0, id, &len, buf);
    pos = buf[0] << 8 | buf[1];
    mem_move(buf + 2, buf + 0x269 - len, len);
    packed = buf + 0x269 - len;
    packbits_expand(packed, buf, pos);
    mem_move(buf, hero, sizeof(struct Character));
    hero->items = 0;
    hero->effects = 0;
    hero->next = 0;
    hero->combat = 0;
    mem_fill(hero->equipped, sizeof hero->equipped, 0);
    pos = sizeof(struct Character);
    monster_unpack_in_progress = 1;
    n_effects = buf[pos++];
    n_items = buf[pos++];
    hero->item_count = n_items;
    if (n_effects != 0) {
        pool_alloc(&hero->effects, sizeof(struct Effect), &effect_pool);
        eff = hero->effects;
        do {
            mem_move5(&buf[pos], eff, 5);
            eff->next = 0;
            pos += 5;
            if (n_effects > 1) {
                pool_alloc(&eff->next, sizeof(struct Effect), &effect_pool);
                eff = eff->next;
            }
        } while (--n_effects > 0);
    }
    if (n_items != 0) {
        if (recs)
            hero->items = rec_alloc();
        else
            pool_alloc(&hero->items, sizeof(struct Item), &item_record_pool);
        item = hero->items;
        do {
            item->name[0] = 0;
            item->in_use = 0;
            item->next = 0;
            mem_move5(&buf[pos], &item->rec, sizeof(struct ItemRec));
            item->more = 0;
            pos += sizeof(struct ItemRec);
            if (n_items > 1) {
                if (recs)
                    item->next = rec_alloc();
                else
                    pool_alloc(&item->next, sizeof(struct Item), &item_record_pool);
                item = item->next;
            }
        } while (--n_items > 0);
    }
    monster_unpack_in_progress = 0;
    hero->class_mask = 0;
    for (class_index = 0; class_index <= 6; class_index++)
        if (hero->level[class_index] > 0)
            hero->class_mask += class_mask_bits[class_index];
}

void far mon_add(unsigned char id)                               /* 2177:2886 */
{
    struct Character far *hero;

    if (game_state[STATE_PARTY_COUNT] <= 7) {
        pool_alloc(&hero, sizeof(struct Character), &party_pool);
        mon_load(id, 1, hero);
        party_character_append(hero);
        hero->npc |= 0x80;
        hero->identity = id;
        icon_load("CPIC", id, hero->order_no);
    }
}

char far game_read_cb(register int file_handle)                           /* 2177:28f8 */
{
    char err;
    short count;
    struct Character far *hero;
    register int index;

    err = dos_io(0x3f00, game_state + STATE_FIRST_BYTE, file_handle, STATE_BYTE_COUNT) != STATE_BYTE_COUNT;
    err |= dos_io(0x3f00, &geo, file_handle, 5) != 5;
    err |= dos_io(0x3f00, &saved_view_mode, file_handle, 2) != 2;
    err |= dos_io(0x3f00, &view_mode, file_handle, 2) != 2;
    err |= dos_io(0x3f00, &geo.pic_set, file_handle, 2) != 2;
    err |= dos_io(0x3f00, &geo.word_15, file_handle, 2) != 2;
    err |= dos_io(0x3f00, &count, file_handle, 2) != 2;
    pictures_enabled = 1;
    geo.show_map = 0;
    game_state[STATE_PARTY_COUNT] = 0;
    for (index = 0; index < count && !err; index++) {
        pool_alloc(&hero, sizeof(struct Character), &party_pool);
        load_hero = hero;
        err |= !hero_read_cb(file_handle);
        if (err)
            pool_free(hero, sizeof(struct Character), &party_pool);
        else {
            recalc_stats(hero);
            party_character_append(hero);
        }
    }
    cur_node = node_list;
    return !err;
}

void far game_load_file(char far *name)                          /* 2177:2ab4 */
{
    old_frame = game_state[STATE_REALM];
    vault_slot = cur_slot;
    save_open(name, 0, game_read_cb);
    while (cur_node != 0) {
        if (cur_node->npc < 0x80)
            character_icon_load(1);
        else
            icon_load("CPIC", cur_node->identity, cur_node->order_no);
        cur_node = cur_node->next;
    }
    cur_node = node_list;
    if (game_state[STATE_DUNGEON] != 0 || game_state[STATE_WILDERNESS] != 1) {
        if (geo.pic_set > 0)
            geo_load(game_state[STATE_AREA_ID]);
    } else
        big_open(game_state[STATE_WILDERNESS_PICTURE_ID]);
    if (game_state[STATE_REALM] != old_frame)
        frame_select(game_state[STATE_REALM]);
}

char far pty_count_cb(int file_handle)                                    /* 2177:2b8a */
{
    if (file_seek(file_handle, 0xa18L, 0) < 0 || dos_io(0x3f00, &pty_count, file_handle, 1) != 1)
        return 0;
    return 1;
}

void far dkk_game_load(char letter)                              /* 2177:2bcf */
{
    char disk;
    char dir[80];
    char path[80];
    char file[80];
    struct Character far *hero;
    struct Item far *item;
    struct Effect far *eff;
    register int index;

    game_reset();
    str_copy(dir, msg_format("chrdat%c", letter));
    str_copy(path, save_path);
    path_append_name(path, dir);
    disk = 0;
    str_copy(file, path);
    str_copy((char far *)file + str_len(file) - 7, "savgama.pty");
    file[str_len(file) - 5] = letter;
    if (!archive_open(disk, file, 0, pty_count_cb))
        pty_count = '0';
    else
        pty_count += '0';
    if (pty_count < '0' || pty_count > '8')
        pty_count = '8';
    index = '1';
    box_draw();
    status_printf(0, 0x18, 7, 0, "Loading...Please Wait");
    while (index <= pty_count && archive_open(disk, msg_format("%s%c.gam", path, index), 0, 0)) {
        pool_alloc(&hero, sizeof(struct Character), &party_pool);
        dkk_load(msg_format("%s%c.gam", dir, index), hero);
        if (hero->npc & 0x80) {
            while (hero->items != 0) {
                item = hero->items->next;
                rec_free(&hero->items);
                hero->items = item;
            }
            while (hero->effects != 0) {
                eff = hero->effects->next;
                pool_free(hero->effects, sizeof(struct Effect), &effect_pool);
                hero->effects = eff;
            }
            pool_free(hero, sizeof(struct Character), &party_pool);
        } else {
            recalc_stats(hero);
            party_character_append(hero);
        }
        index++;
    }
    for (cur_node = node_list; cur_node != 0; cur_node = cur_node->next)
        if (cur_node->npc < 0x80)
            character_icon_load(1);
        else
            icon_load("CPIC", cur_node->identity, cur_node->order_no);
    cur_node = node_list;
    box_draw();
}

void far game_load_menu(void)                                   /* 2177:2e87 */
{
    char file[41];
    char scan_index;
    unsigned char hotkey_index;
    unsigned char pick;
    char far *name;
    char found[10];
    char is_dir;
    char far *ext;
    char path[80];

    frame_border();
    hotkey_reset(2);
    pick = menu_run(load_where_text, source_items, 0, 0);
    switch (pick) {
    case 0:
        file_kind = 1;
        ext = "qsv";
        break;
    case 1:
        file_kind = 4;
        ext = "pty";
        break;
    case 2:
        return;
    default:                    /* ext stays unset here, as shipped       */
        if (menu_escaped && pick == 0x1b)
            return;
    }
    mem_fill(found, sizeof found, 0);
    str_format(file, "%sX", "SavGam");
    path_default_ext(file, ext, 1);
    if (file_kind == 1) {
        if (!save_ready())
            return;
        if (save_drive != 0) {
            str_format(path, "%c:\\save", save_drive);
            dir_first(0, path, ext, 1, 0);
        } else
            dir_first('S', 0, ext, 1, 0);
    } else if (file_kind == 4)
        dir_first(0, save_path, ext, 1, 0);
    else {
        if (!save_ready())
            return;
        dir_first('S', 0, ext, 1, 0);
    }
    while ((name = dir_next(&is_dir)) != 0)
        if (str_match("SavGam", name) && name[6] >= 'A' && name[6] <= 'J')
            found[name[6] - 'A'] = 1;
    mem_fill(hotkeys, sizeof hotkeys, 0xff);
    hotkey_index = 0;
    scan_index = 0;
    for (cur_slot = 'A'; cur_slot <= 'J'; cur_slot++) {
        if (found[cur_slot - 'A']) {
            hotkeys[hotkey_index].key = scan_index;
            hotkey_index++;
        }
        scan_index++;
    }
    if ((unsigned char)hotkeys[0].key == 0xff)
        return;
    do {
        pick = menu_run(load_which_text, game_items, 0, 0);
        if (menu_escaped && pick == 0x1b)
            cur_slot = 0x1b;
        else
            cur_slot = pick + 'A';
    } while (cur_slot != 'A' && cur_slot != 'B' && cur_slot != 'C'
             && cur_slot != 'D' && cur_slot != 'E' && cur_slot != 'F'
             && cur_slot != 'G' && cur_slot != 'H' && cur_slot != 'I'
             && cur_slot != 'J' && cur_slot != 0x1b);
    if (cur_slot == 0x1b)
        return;
    str_find(file, '.')[-1] = cur_slot;
    box_draw();
    status_printf(0, 0x18, 7, 0, "Loading...Please Wait");
    game_loaded = 1;
    switch (file_kind) {
    case 1:
        game_load_file(file);
        break;
    case 4:
        dkk_game_load(cur_slot);
        break;
    }
    box_draw();
    view_mode = 0;
}

char far game_write_cb(register int file_handle)                          /* 2177:31a7 */
{
    struct Character far *hero;
    long pos;
    short count;
    char err;

    err = dos_io(0x4000, game_state + STATE_FIRST_BYTE, file_handle, STATE_BYTE_COUNT) != STATE_BYTE_COUNT;
    err |= dos_io(0x4000, &geo, file_handle, 5) != 5;
    err |= dos_io(0x4000, &saved_view_mode, file_handle, 2) != 2;
    err |= dos_io(0x4000, &view_mode, file_handle, 2) != 2;
    err |= dos_io(0x4000, &geo.pic_set, file_handle, 2) != 2;
    err |= dos_io(0x4000, &geo.word_15, file_handle, 2) != 2;
    count = 0;
    for (hero = node_list; hero != 0 && count < 8; hero = hero->next)
        count++;
    err |= dos_io(0x4000, &count, file_handle, 2) != 2;
    for (hero = node_list, count = 0; hero != 0 && count < 8; hero = hero->next, count++) {
        load_hero = hero;
        err |= !hero_write_cb(file_handle);
    }
    pos = file_seek(file_handle, 0L, 1);
    if (pos < 0x1f1d && dos_io(0x4000, item_templates, file_handle, 0x1f1d - pos) != 0x1f1d - pos)
        return 0;
    return !err;
}

void far game_save_menu(void)                                   /* 2177:3395 */
{
    char letter;
    unsigned char pick;
    char file[42];

    do {
        hotkey_reset(9);
        pick = menu_run(save_which_text, game_items, 0, view_mode == 2);
    } while (pick >= 10 && (!menu_escaped || pick != 0x1b));
    if (pick > 9)
        return;
    letter = pick + 'A';
    if (!save_ready())
        return;
    box_draw();
    status_printf(0, 0x18, 7, 0, "Saving...Please Wait");
    str_format(file, "%s%c", "SavGam", letter);
    path_default_ext(file, "qsv", 1);
    file_kind = 1;
    if (save_write(file, game_write_cb)) {  /* true after Exit too: 010b */
        if (vault_slot != letter) {
            vault_load_current();
            vault_slot = letter;
            vault_save_current();
        }
        game_saved = 1;
    }
    box_draw();
}

void far party_character_append(struct Character far *hero)                  /* 2177:3491 */
{
    struct Character far *character_cursor;
    char used[8];

    hero->order_no = 0xff;
    if (node_list == 0)
        node_list = hero;
    else {
        for (character_cursor = node_list; character_cursor->next != 0; character_cursor = character_cursor->next)
            ;
        character_cursor->next = hero;
    }
    cur_node = hero;
    mem_fill(used, sizeof used, 0);
    for (character_cursor = node_list; character_cursor != 0; character_cursor = character_cursor->next)
        if (character_cursor->order_no >= 0 && character_cursor->order_no <= 7)
            used[character_cursor->order_no] = 1;
    for (cur_node->order_no = 0; cur_node->order_no < 8 && used[cur_node->order_no];
         cur_node->order_no++)
        ;
    game_state[STATE_PARTY_COUNT]++;
    if (hero->npc & 0x80)
        recalc_from_levels(hero);
}

void far character_file_lists_free(struct Node far *list, struct Node far *files) /* 2177:359f */
{
    list_free(&list);
    list_free(&files);
}

void far vault_load_current(void)                                   /* 2177:35bc */
{
    char name[42];

    pool_clear();
    if (vault_slot != 'Z') {
        str_format(name, "Vault%c.DAT", vault_slot);
        save_open(name, 0, pool_load);
    }
}

void far vault_save_current(void)                                   /* 2177:3600 */
{
    char name[42];

    if (vault_slot != 'Z') {
        str_format(name, "Vault%c.DAT", vault_slot);
        save_write(name, pool_save);
    }
}
