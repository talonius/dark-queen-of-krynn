/* dqk.h -- the records more than one DQK module declares, declared once.
 *
 * Every record more than one module declares is here, reconciled from the modules'
 * partial views of it (docs/frontier/climb.md, gametools/reconcile/). A module that
 * includes this file names it in its goldbox.toml `includes`, which stages it beside
 * the source for Turbo C.
 *
 * CRLF: Turbo C 2.01 reads an LF-only file as one line; .gitattributes checks this
 * file out CRLF, and bake refuses a bare LF in it.
 */

/* --- script state ---------------------------------------------------------------- */

/* VM bank is byte-addressed 0001..0400. Names describe native behavior;
 * VM reads of the geometry/effect aliases can override the stored bytes.
 * Full census and limits: v11/analysis/ecl-glb/STATE-FIELDS.md. */
#define STATE_FIRST_BYTE 1
#define STATE_BYTE_COUNT 0x400

/* Signed attack adjustment for side != 0; cleared after combat. */
#define STATE_NONPARTY_HIT_BONUS 0x01

/* Nonzero suppresses spellcasting and several magic menu/AI routes. */
#define STATE_MAGIC_DISABLED 0x02

/* 0xff suppresses the pending move; smaller values allow it. */
#define STATE_MOVE_BLOCK 0x03

/* Smallest clock counter; carries at 10 into minutes. */
#define STATE_CLOCK_TENTH_MINUTE 0x05

/* Minute ones digit; carries at 10. */
#define STATE_CLOCK_MINUTE 0x06

/* Minute tens digit; carries at 6. */
#define STATE_CLOCK_TEN_MINUTES 0x07

/* Hour counter; carries at 24 and selects night lighting. */
#define STATE_CLOCK_HOUR 0x08

/* Day counter; carries at 30. */
#define STATE_CLOCK_DAY 0x09

/* Month counter; carries at 12. */
#define STATE_CLOCK_MONTH 0x0a

/* Year low counter; wraps at 100 and ages party members. */
#define STATE_CLOCK_YEAR_LO 0x0b

/* Year high byte stored by clock_set; not advanced by clock_advance. */
#define STATE_CLOCK_YEAR_HI 0x0c

/* Subtracts from the turning-table column, floored at zero. */
#define STATE_TURN_UNDEAD_PENALTY 0x0f

/* VM read override: any node_list character has effect 5, Detecting Magic; no side filter. */
#define STATE_DETECT_MAGIC 0x10

/* VM read/write override: dungeon cardinal facing; outdoor read is raw facing. */
#define STATE_FACING 0x11

/* Menu setting 0..9; larger values increase delays and animation repetitions. */
#define STATE_SPEED 0x12

/* Requested GEO area; used to reload geometry after loading a save. */
#define STATE_AREA_ID 0x13

/* Value 1 routes the scripted service through temple_shop. */
#define STATE_TEMPLE_SERVICE 0x14

/* Cleanup counts disabled characters with not_pc == 1 or side == 1. */
#define STATE_DISABLED_ENEMY_COUNT 0x15

/* Saved/resumed ECL script number. */
#define STATE_SCRIPT_ID 0x16

/* Position before a move; wilderness movement copies wilderness X. */
#define STATE_PREVIOUS_X 0x17

/* Position before a move; wilderness movement copies wilderness Y. */
#define STATE_PREVIOUS_Y 0x18

/* Bit 0 persistent search; bit 1 requests one-shot search. */
#define STATE_SEARCH_FLAGS 0x19

/* Nonzero hides coordinates and restricts mapping; VM write closes a shown map. */
#define STATE_MAP_RESTRICTED 0x1a

/* Second NPC morale test compares 100 minus this byte to morale. */
#define STATE_MORALE_CHECK_PERCENT 0x1b

/* Signed movement adjustment for side 0, before doubling and underwater scaling. */
#define STATE_PARTY_MOVE_BONUS 0x1c

/* Post-combat recovery and continuation after a loss when party members survive. */
#define STATE_NONLETHAL_COMBAT 0x1d

/* Exactly 1 suppresses enemy item collection; coins and XP remain separate. */
#define STATE_NO_ITEM_LOOT 0x1e

/* Nonzero suppresses XP awards and the XP announcement. */
#define STATE_NO_XP 0x1f

/* Party members, including recruited NPCs; maintained on append/drop/load. */
#define STATE_PARTY_COUNT 0x20

/* Boundary-crossing result; coordinates are clamped back into the GEO area. */
#define STATE_MAP_EXIT 0x21

/* Dungeon geometry/combat flag; zero selects outdoor behavior. */
#define STATE_DUNGEON 0x22

/* VM read override: raw eight-direction geo.facing. */
#define STATE_FACING_OCTANT 0x23

/* Wilderness view flag; most consumers require exactly 1. */
#define STATE_WILDERNESS 0x24

/* Wilderness column, clamped by movement to 0..37. */
#define STATE_WILDERNESS_X 0x25

/* Wilderness row, clamped by movement to 0..14. */
#define STATE_WILDERNESS_Y 0x26

/* Combat difficulty 1..5; default 3, Veteran. */
#define STATE_DIFFICULTY 0x27

/* 1/2/4/8 divide prices by 16/8/4/2; 0x20/0x40/0x80 multiply by 2/4/8. */
#define STATE_SHOP_PRICE_SCALE 0x28

/* Low byte of the remainder from ECL division. */
#define STATE_DIVISION_REMAINDER 0x29

/* Encounter check interval in five-minute rest iterations; zero disables checks. */
#define STATE_REST_ENCOUNTER_INTERVAL 0x2b

/* Rest encounter succeeds when 1d100 is <= this byte. */
#define STATE_REST_ENCOUNTER_CHANCE 0x2c

/* VM read override: geo.flag_4, the current area-cell attribute. */
#define STATE_AREA_CELL_ATTR 0x2d

/* Mask tested against side + 1; selected initiative decreases by 6, with out-of-range results reset to 0. */
#define STATE_INITIATIVE_SIDE_MASK 0x2e

/* Signed attack adjustment for side 0, alongside difficulty; cleared after combat. */
#define STATE_PARTY_HIT_BONUS 0x2f

/* Training hall class mask; creation temporarily sets ff and restores it. */
#define STATE_TRAINING_CLASSES 0x30

/* Value 1 invokes the vault service, then clears. */
#define STATE_VAULT_SERVICE 0x31

/* VM read override: geo.byte_3, current wall_side result. */
#define STATE_WALL_AHEAD 0x32

/* Nonzero adds the wilderness combat-map river. */
#define STATE_WILDERNESS_RIVER 0x33

/* VM read/write override: geo.x; writes mark the script view dirty. */
#define STATE_X 0x34

/* VM read/write override: geo.y; writes mark the script view dirty. */
#define STATE_Y 0x35

/* Latch set on confirmation of an attack on the same team; reset at combat setup. */
#define STATE_ALLY_ATTACK_CONFIRMED 0x36

/* Current encounter spacing, clamped during setup/approach/combat. */
#define STATE_ENCOUNTER_DISTANCE 0x37

/* Encounter spacing cap; Setup Monster value 10 hex forces both distances to 2. */
#define STATE_ENCOUNTER_DISTANCE_LIMIT 0x38

/* PRINT uses color 7 when zero, color 3 otherwise; not a literal color ID. */
#define STATE_SCRIPT_TEXT_COLOR_MODE 0x39

/* Realm type selects frame palette, wandering template and movement sound. */
#define STATE_REALM 0x3a

/* Nonzero enables underwater combat behavior. */
#define STATE_UNDERWATER 0x3c

/* VM read override: area_cell_flags at the current GEO coordinates. */
#define STATE_AREA_CELL_FLAGS 0x3d

/* Picture selected by big_open in wilderness presentation. */
#define STATE_WILDERNESS_PICTURE_ID 0x3e

/* Area number recorded by geo_load; also lies within the script-local clear range. */
#define STATE_LOADED_AREA_ID 0xc6

/* Wilderness combat-map terrain selector. */
#define STATE_WILDERNESS_TERRAIN 0x1a2

/* First moon phase, normally 0..3; affects robe-order spell behavior and glyphs. */
#define STATE_MOON_PHASE_1 0x1fa

/* Second moon phase, normally 0..3. */
#define STATE_MOON_PHASE_2 0x1fb

/* Third moon phase, normally 0..3. */
#define STATE_MOON_PHASE_3 0x1fc

/* First moon day counter; phase advances when old counter >= 8, giving 9 ticks. */
#define STATE_MOON_DAYS_1 0x1fd

/* Second moon day counter; threshold 1, giving 2 ticks. */
#define STATE_MOON_DAYS_2 0x1fe

/* Third moon day counter; threshold 6, giving 7 ticks. */
#define STATE_MOON_DAYS_3 0x1ff

/* Indexed ranges; loaded-area c6 overlaps the script-local range. */
#define STATE_QUEST_ITEMS 0x6f
#define STATE_QUEST_ITEMS_COUNT 40
#define STATE_SCRIPT_LOCALS 0x97
#define STATE_SCRIPT_LOCALS_COUNT 50

/* --- the party's place ------------------------------------------------------------- */

struct Geo {                    /* the party's place and the view state   */
    char x;                     /* +0   c9f0                              */
    char y;                     /* +1   c9f1                              */
    unsigned char facing;       /* +2   c9f2                              */
    char byte_3;                /* +3   c9f3                              */
    char flag_4;                /* +4   c9f4                              */
    char attr_5;                /* +5   c9f5                              */
    char attr_6;                /* +6   c9f6                              */
    long long_7[3];             /* +7   c9f7, zeroed by section 4         */
    int pic_set;                /* +13  ca03                              */
    int word_15;                /* +15  ca05                              */
    char show_map;              /* +17  ca07  draw the overhead map       */
    struct Area far *area;      /* +18  ca08                              */
    char attr_1c;               /* +1c  ca0c                              */
    char attr_1d;               /* +1d  ca0d                              */
};

struct Area {                   /* a geo area, from GEO1, 0xc48 bytes     */
    unsigned char rows;         /* +0                                     */
    unsigned char cols;         /* +1                                     */
    unsigned char set_0;        /* +2   wall set for slot 0               */
    unsigned char set_1;        /* +3   wall set for slot 1, 0xff none    */
    unsigned char set_2;        /* +4   wall set for slot 2, 0xff none    */
    unsigned char tile_5;       /* +5   back tile when 0807 gives 0       */
    unsigned char tile_6;       /* +6   back tile when 0807 gives 1       */
    unsigned char tile_7;       /* +7   back tile when 0807 gives 2       */
    unsigned char walls[0x310]; /* +8   per cell: dir 0 high, dir 2 low   */
    unsigned char walls2[0x310]; /* +318 per cell: dir 4 high, dir 6 low  */
    unsigned char cells[0x310]; /* +628 per cell                          */
    unsigned char flags[0x310]; /* +938 per cell                          */
};

/* --- menus, pools and resources ---------------------------------------------------- */

struct Node {                   /* a menu list entry, 0x28 bytes          */
    struct Node far *next;      /* +0                                     */
    char key;                   /* +4                                     */
    char text[35];              /* +5                                     */
};

struct Pool {                   /* strutil.c's slot pool                  */
    int capacity;               /* +0  slots in the pool                  */
    int stride;                 /* +2  bytes per slot                     */
    char far *base;             /* +4  backing storage                    */
    unsigned char bitmap[1];    /* +8  one bit per slot, 1 = occupied     */
};

struct Res {                    /* a resource: a slot and an index in it  */
    int slot;                   /* +0   -1 when free                      */
    int index;                  /* +2                                     */
    char tail[2];               /* +4                                     */
};

struct Pic {                    /* picview.c's picture window state       */
    unsigned char frames;       /* +0   res_item_count's count, less 1 for PIC  */
    unsigned char anim;         /* +1   1 for a PIC library picture       */
    struct Res far *res;        /* +2                                     */
};

struct HotKey {                 /* a menu choice's key, 2 bytes           */
    char key;                   /* +0   code the menu returns             */
    char ch;                    /* +1   letter that selects it            */
};

struct ListBox {                /* list_menu's frame, handed to list_box  */
    int col;                    /* +00                                    */
    int row;                    /* +02                                    */
    int width;                  /* +04  entries shown                     */
    int height;                 /* +06                                    */
    int flags;                  /* +08  1 arrows; 2 a scroll bar, more entries than width */
    int save_top;               /* +0a  first entry shown                 */
    int top;                    /* +0c  the chosen entry                  */
    int count;                  /* +0e  dialog item id base               */
    char far *(far *fetch)(int n, char far *key); /* +10                  */
    char (far *done)(void);     /* +14                                    */
    void (far *extra)(void);    /* +18  list_select calls it on result 0  */
};

typedef int (far *ItemProc)();  /* a dialog item's handler: (item, msg, a, b) */
typedef int (far *ItemCb)(int id, ...);

struct Wid {                    /* one dialog item, 0x20 bytes            */
    ItemProc fn;                /* +00  handler                           */
    ItemCb cb;                  /* +04  called on activate                */
    int far *val;               /* +08  value a check or group writes     */
    char far *text;             /* +0c                                    */
    int row;                    /* +10                                    */
    int col;                    /* +12                                    */
    int cursor;                 /* +14  pointer frame while over it       */
    int len;                    /* +16  text argument; group's count      */
    int width;                  /* +18  4-column cells; group's selection */
    int glyph;                  /* +1a  bank<<10|index; < -1 framed, -1 text */
    char flags;                 /* +1c                                    */
    char key1;                  /* +1d  hotkeys                           */
    char key2;                  /* +1e                                    */
    unsigned char color;        /* +1f                                    */
};

/* --- glyphs, palettes and libraries ------------------------------------------------ */

struct Glyph {                  /* the header before a glyph's bits       */
    int rows;                   /* +0                                     */
    int dy;                     /* +2   subtracted from the row           */
    int dx;                     /* +4   subtracted from the column        */
    unsigned char width;        /* +6   bytes per plane row               */
    unsigned char type;         /* +7   low nibble: format; 0x80 one plane; 0x40 to blit_packed */
};

struct CycSpec {                /* one palette cycle as pal_set is given it */
    unsigned char flags;        /* +0   bit 0: rotate towards the first entry */
    unsigned char rate;         /* +1   ticks between steps               */
    unsigned char first;        /* +2   first palette entry               */
    unsigned char count;        /* +3   entries in the run                */
};

struct LibHdr {                 /* the 16 bytes at the front of a library */
    long tag;                   /* +0   "HLIB"                            */
    long size;                  /* +4   total bytes, header included      */
    int count;                  /* +8   items in the index                */
    char method;                /* +a   packing, 0 once expanded          */
    char itype;                 /* +b   nonzero: item 0 is not converted  */
    long sig;                   /* +c   selects the registered converter  */
};

/* --- the ECL interpreter ----------------------------------------------------------- */

struct EclFrame {               /* a gosub's return, 6 bytes from ecl_frame_pool */
    unsigned ip;                /* +0                                     */
    struct EclFrame far *next;  /* +2                                     */
};

/* --- items ----------------------------------------------------------------------- */

/* An item's saved fields: the 0x11 bytes a file holds for it, read and written whole
 * (DKK's TItemTemplate, "TItem from itemType on", in another order). The last three
 * bytes are a magic item's charges, effect and special, or a scroll's three spells. */
struct ItemRec {
    unsigned char type;         /* +0   'I': a bundle, its items in more  */
    unsigned char name_ids[3];  /* +1   the words of its name             */
    unsigned weight;            /* +4                                     */
    unsigned value;             /* +6                                     */
    signed char plus;           /* +8                                     */
    signed char save_bonus;     /* +9                                     */
    unsigned char equipped;     /* +a                                     */
    unsigned char unid_mask;    /* +b   0x10, 0x20: robe orders allowed   */
    unsigned char cursed;       /* +c                                     */
    unsigned char quantity;     /* +d   a bundle's: items in more         */
    union {                     /* +e                                     */
        struct {
            unsigned char charges;  /* +e                                 */
            unsigned char effect;   /* +f                                 */
            unsigned char special;  /* +10  bit 7: an effect when readied */
        } magic;
        unsigned char spells[3];    /* +e   a scroll's; bit 7: still to scribe */
    } u;
};

struct ItemInfo {               /* an item type, 0x10 bytes: DKK's TBaseItem */
    unsigned char slot;         /* +0   index into Character.equipped          */
    unsigned char hands;        /* +1                                     */
    unsigned char rolls_large;  /* +2   damage against a large target     */
    unsigned char dice_large;   /* +3                                     */
    char mod_large;             /* +4                                     */
    unsigned char rate_of_fire; /* +5   a missile weapon's attacks a round */
    unsigned char armor_bonus;  /* +6   bit 7: counts toward armour class */
    unsigned char wpn_class;    /* +7   0 slashing, 1 piercing, 0x80 bashing */
    unsigned char byte_8;       /* +8                                     */
    unsigned char rolls;        /* +9   damage                            */
    unsigned char dice;         /* +a                                     */
    char mod;                   /* +b                                     */
    unsigned char range;        /* +c                                     */
    unsigned char class_mask;   /* +d   tested against Character.class_mask    */
    unsigned char flags;        /* +e                                     */
    unsigned char byte_f;       /* +f                                     */
};

struct Item {                   /* a carried item, 0x3d bytes             */
    struct Item far *next;      /* +0                                     */
    unsigned char in_use;       /* +4                                     */
    char name[0x23];            /* +5                                     */
    struct ItemRec rec;         /* +28                                    */
    struct Item far *more;      /* +39  a bundle's items                  */
};

struct Drop {                   /* an item a member has lost              */
    struct Item item;           /* +0                                     */
    struct Character far *owner;     /* +3d                                    */
    struct Drop far *next;      /* +41                                    */
};

/* --- characters -------------------------------------------------------------------- */

struct AgeLimits {              /* 0xa bytes a race                       */
    unsigned age[5];            /* +0                                     */
};

struct RaceClasses {            /* 0x1d bytes a race                      */
    unsigned char count;        /* +0                                     */
    int classes[14];            /* +1                                     */
};

struct ClassAligns {            /* 0x15 bytes a class                     */
    unsigned char count;        /* +0                                     */
    int aligns[10];             /* +1                                     */
};

struct AgeRoll {                /* 4 bytes a class                        */
    int base;                   /* +0                                     */
    unsigned char dice;         /* +2                                     */
    unsigned char sides;        /* +3                                     */
};

struct IconCell {               /* one cell of an icon, from its corner   */
    char dx;                    /* +0   -1: no more cells                 */
    char dy;                    /* +1                                     */
};

struct Slot {                   /* 5 bytes                                */
    unsigned char flag;         /* +0                                     */
    long value;                 /* +1                                     */
};

struct Stat {                   /* one ability score                      */
    unsigned char base;         /* +0                                     */
    unsigned char cur;          /* +1                                     */
};

struct RaceLimits {             /* 0x10 bytes a race                      */
    unsigned char str_min[2];   /* +0   by gender                         */
    unsigned char str_max[2];   /* +2                                     */
    unsigned char exc_max[2];   /* +4                                     */
    unsigned char int_min;      /* +6                                     */
    unsigned char int_max;      /* +7                                     */
    unsigned char wis_min;      /* +8                                     */
    unsigned char wis_max;      /* +9                                     */
    unsigned char dex_min;      /* +a                                     */
    unsigned char dex_max;      /* +b                                     */
    unsigned char con_min;      /* +c                                     */
    unsigned char con_max;      /* +d                                     */
    unsigned char cha_min;      /* +e                                     */
    unsigned char cha_max;      /* +f                                     */
};

struct ClassMins {              /* 6 bytes a class                        */
    unsigned char min[6];       /* +0   str, int, wis, dex, con, cha      */
};

struct Effect {                 /* an active effect, 9 bytes: DKK's TEffect */
    unsigned char id;           /* +0                                     */
    unsigned duration;          /* +1   minutes left; 0 never runs out    */
    unsigned char strength;     /* +3                                     */
    unsigned char on_remove;    /* +4   1: the handler runs again on removal */
    struct Effect far *next;    /* +5                                     */
};

/* Every character, party member or not: DKK's TCharacter (dkk-disassembly gamedat3.pas),
 * reorganised -- pointers moved to the front, the spell block inlined -- with the field
 * names DKK's reconstruction gives it, 0x19e bytes. */
struct Character {
    struct Character far *next;      /* +0                                     */
    struct Effect far *effects; /* +4                                     */
    struct Item far *items;     /* +8                                     */
    struct Item far *equipped[13]; /* +c  readied, by ItemInfo.slot      */
    struct Combat far *combat;  /* +40                                    */
    long xp;                    /* +44                                    */
    long old_xp;                /* +48  before a drain                    */
    unsigned coins[3];          /* +4c  steel, gems, jewelry              */
    unsigned age;               /* +52  years                             */
    unsigned xp_award;          /* +54                                    */
    unsigned encumbrance;       /* +56                                    */
    int race;                   /* +58                                    */
    int cls;                    /* +5a  3 paladin                         */
    int level_undead;           /* +5c                                    */
    int word_5e;                /* +5e                                    */
    int gender;                 /* +60  0 male                            */
    int alignment;              /* +62                                    */
    int status;                 /* +64  0 okay, 5 dying, 6 dead, 7 stone  */
    int side;                   /* +66                                    */
    char name[0x10];            /* +68                                    */
    struct Stat stats[6];       /* +78  str int wis dex con cha           */
    unsigned char str_ex;       /* +84  exceptional strength              */
    unsigned char str_ex_base;  /* +85                                    */
    char spell_prep;            /* +86  hours of study left                */
    unsigned char thac0_base;   /* +87                                    */
    unsigned char cure;         /* +88  cure disease left                 */
    unsigned char hp_max;       /* +89                                    */
    unsigned char icon_dim;     /* +8a                                    */
    unsigned char saves[5];     /* +8b                                    */
    unsigned char move_base;    /* +90                                    */
    unsigned char level_high;   /* +91                                    */
    unsigned char byte_92;      /* +92                                    */
    unsigned char thief[8];     /* +93                                    */
    unsigned char npc;          /* +9b  0x80 and up: not a party member   */
    unsigned char byte_9c[2];   /* +9c                                    */
    unsigned char level_max[7]; /* +9e                                    */
    unsigned char level[7];     /* +a5  one per class                     */
    unsigned char old_level[7]; /* +ac  before a drain                    */
    unsigned char attack_rate[2]; /* +b3                                  */
    unsigned char unarmed_rolls[2]; /* +b5                                */
    unsigned char unarmed_dice[2]; /* +b7                                 */
    char unarmed_mods[2];       /* +b9                                    */
    unsigned char ac_base;      /* +bb                                    */
    unsigned char str_bonus;    /* +bc                                    */
    unsigned char identity;     /* +bd                                    */
    unsigned char old_hp_max;   /* +be                                    */
    unsigned char class_mask;   /* +bf  tested against ItemInfo.classes   */
    unsigned char hp_base;      /* +c0                                    */
    unsigned char byte_c1[2];   /* +c1                                    */
    unsigned char icon_head;    /* +c3                                    */
    unsigned char icon_body;    /* +c4                                    */
    unsigned char order_no;     /* +c5                                    */
    unsigned char icon_size;    /* +c6                                    */
    unsigned char type_flags;   /* +c7                                    */
    unsigned char immunity;     /* +c8                                    */
    unsigned char item_count;   /* +c9                                    */
    unsigned char hands;        /* +ca  hands in use                      */
    char save_bonus;            /* +cb                                    */
    unsigned char spell_resist; /* +cc                                    */
    char level_up;              /* +cd                                    */
    unsigned char spells[0x89]; /* +ce  bits 0-6 a memorized spell; bit 7 still to memorize */
    unsigned char known[0x10];  /* +157 one bit per spell, from 1         */
    unsigned char slots[4][9];  /* +167 spells allowed, by list and level */
    char enabled;               /* +18b 0: can do nothing                 */
    char quick_fight;           /* +18c nonzero: the computer plays it    */
    unsigned char thac0_adj;    /* +18d                                   */
    unsigned char ac[2];        /* +18e front, rear                       */
    unsigned char attacks_left[2]; /* +190                                */
    unsigned char rolls[2];     /* +192 damage dice                       */
    unsigned char dice[2];      /* +194                                   */
    char mods[2];               /* +196                                   */
    unsigned char hp;           /* +198                                   */
    unsigned char movement;     /* +199                                   */
    unsigned char knight_order; /* +19a 1 crown .. 3 rose                 */
    int deity;                  /* +19b                                   */
    unsigned char robe_order;   /* +19d                                   */
};

/* A spell, 0x16 bytes: DKK's TSpell (gamedat3.pas) in the same order, with five fields
 * widened to an int. */
struct SpellInfo {
    int cast_class;             /* +0   slots row: 0 cleric, 1 druid, 2 mage, 3 deity gift */
    unsigned char level;        /* +2                                     */
    unsigned char range;        /* +3   0xff touch                        */
    unsigned char range_per_level; /* +4                                  */
    unsigned char duration;     /* +5   minutes                           */
    unsigned char duration_per_level; /* +6                               */
    unsigned char targeting;    /* +7                                     */
    int camp_target;            /* +8   0 combat only, 1 caster, 2 one, 3 party */
    int save_effect;            /* +a                                     */
    int save_type;              /* +c                                     */
    unsigned char effect_id;    /* +e                                     */
    int combat_ok;              /* +f   0: a camp-only spell              */
    unsigned char casting_time; /* +11  segments                          */
    unsigned char ai_priority;  /* +12                                    */
    int hostile;                /* +13  0: aims at the caster's own side  */
    unsigned char byte_15;      /* +15                                    */
};

/* --- combat ------------------------------------------------------------------------ */

struct TileInfo {               /* a combat terrain type, 4 bytes: DKK's TTerrain */
    unsigned char cost;         /* +0   movement cost                     */
    unsigned char height;       /* +1                                     */
    unsigned char block;        /* +2                                     */
    char code;                  /* +3                                     */
};

struct Map {                    /* the combat arena, 0x4eb bytes: DKK's TArena */
    unsigned char pad_0[2];     /* +0                                     */
    int ox;                     /* +2   the view's origin cell            */
    int oy;                     /* +4                                     */
    char cursor_on;             /* +6   the target cursor's icon overlay  */
    char cursor_size;           /* +7                                     */
    char dist_mode;             /* +8   1: line walks ignore terrain blocking */
    unsigned char cells[25][50]; /* +9  terrain index per cell            */
};

/* A creature's state for one fight, 0x18 bytes: DKK's TCombat (gamedat3.pas) with the
 * initiative widened to an int, and one byte more. */
struct Combat {
    unsigned char spell_cast;   /* +0   the spell being cast              */
    unsigned char can_cast;     /* +1                                     */
    unsigned char can_use_item; /* +2                                     */
    int initiative;             /* +3                                     */
    unsigned char attack_form;  /* +5   which attack count is in use      */
    unsigned char sweep_targets; /* +6  foes a sweep may take             */
    unsigned char movement;     /* +7                                     */
    unsigned char guarding;     /* +8                                     */
    unsigned char attacked;     /* +9                                     */
    unsigned char direction;    /* +a                                     */
    struct Character far *target;    /* +b   the character it is after         */
    unsigned char dying_turns;  /* +f                                     */
    unsigned char attacks_taken; /* +10                                   */
    unsigned char panicked;     /* +11                                    */
    unsigned char turn_attempts; /* +12                                   */
    unsigned char attack_angles; /* +13                                   */
    unsigned char not_pc;       /* +14  1 for a monster                   */
    unsigned char fleeing;      /* +15                                    */
    unsigned char ai_stance;    /* +16                                    */
    unsigned char byte_17;      /* +17                                    */
};

struct Rec {                    /* a fallen combatant, 9 bytes            */
    struct Character far *hero;      /* +0                                     */
    int x;                      /* +4                                     */
    int y;                      /* +6                                     */
    char terrain;               /* +8   the terrain under it              */
};

struct Combatant {              /* a combatant's place on the combat map  */
    int x;                      /* +0                                     */
    int y;                      /* +2                                     */
    char byte_4;                /* +4                                     */
    unsigned char size;         /* +5   icon size; 0: off the map         */
};

struct Step {                   /* one move in a combatant's path         */
    unsigned char who;          /* +0   index into combatants             */
    unsigned char cost;         /* +1                                     */
    unsigned char dir;          /* +2                                     */
};

struct Line {                   /* approach.c's line stepper              */
    int x0;                     /* +0                                     */
    int y0;                     /* +2                                     */
    int x1;                     /* +4                                     */
    int y1;                     /* +6                                     */
    int err;                    /* +8                                     */
    int dx;                     /* +a   |x1 - x0|                         */
    int dy;                     /* +c   |y1 - y0|                         */
    int x;                      /* +e                                     */
    int y;                      /* +10                                    */
    int sx;                     /* +12  -1, 0 or 1                        */
    int sy;                     /* +14                                    */
    unsigned char steps;        /* +16                                    */
    char dir;                   /* +17  direction of the last step        */
};

struct Aim {                    /* a spell's chosen target and cell       */
    struct Character far *target;    /* +0                                     */
    int x;                      /* +4                                     */
    int y;                      /* +6                                     */
    char byte_8;                /* +8                                     */
};

struct Cloud {                  /* 0x21 bytes: a cloud on the combat map  */
    struct Character far *caster;    /* +0                                     */
    struct Cloud far *next;     /* +4                                     */
    unsigned char tile[10];     /* +8   [1..count] the tile under it      */
    char on[10];                /* +12  [1..count] nonzero: on the map    */
    char x;                     /* +1c                                    */
    char y;                     /* +1d                                    */
    unsigned char strength;     /* +1e  rounds left; 0xff: dispelled      */
    unsigned char level;        /* +1f  the caster's level                */
    unsigned char kind;         /* +20  the map cell it becomes           */
};

/* --- shared declarations: gametools/reconcile/externs.py writes this section --- */

/* What more than one module declares extern, spelled alike everywhere. Beside a routine
 * is the far pointer as a shipped call stores it (link-time segment), beside a variable
 * its DGROUP offset. A name the modules declare differently, or a module defines, stays
 * in the modules (gametools/reconcile/externs.py says which and why). */

struct DriverDesc;
struct Lzw;
struct Pointer;

/* routines, by address */
extern int far ail_startup(void);                                /* 000c:06b9 */
extern void far ail_shutdown(char far *msg);                     /* 000c:06fd */
extern int far ail_register_timer(void (far *fn)(void));         /* 000c:0762 */
extern void far ail_release_timer_handle(int timer);             /* 000c:080e */
extern void far ail_start_timer(int timer);                      /* 000c:0870 */
extern void far ail_set_timer_divisor(int timer, unsigned d);    /* 000c:0992 */
extern int far ail_register_driver(void far *image);             /* 000c:09e2 */
extern struct DriverDesc far *far ail_describe_driver(int drv);  /* 000c:0a9a */
extern int far ail_detect_device(int drv, int io, int irq, int dma, int drq); /* 000c:0ac0 */
extern void far ail_init_driver(int drv, int io, int irq, int dma, int drq); /* 000c:0ac6 */
extern void far ail_play_voc_file(int drv, void far *voc, int marker); /* 000c:0bb7 */
extern int far ail_voc_playback_status(int drv);                 /* 000c:0bbd */
extern void far ail_start_digital_playback(int drv);             /* 000c:0bc3 */
extern void far ail_stop_digital_playback(int drv);              /* 000c:0bc9 */
extern void far ail_pause_digital_playback(int drv);             /* 000c:0bcf */
extern void far ail_resume_digital_playback(int drv);            /* 000c:0bd5 */
extern void far ail_set_digital_playback_volume(int drv, int volume); /* 000c:0bdb */
extern unsigned far ail_state_table_size(int drv);               /* 000c:0bf3 */
extern int far ail_register_sequence(int drv, void far *file, int n, void far *state, void far *ctl);
                                                                 /* 000c:0bf9 */
extern void far ail_release_sequence_handle(int drv, int seq);   /* 000c:0bff */
extern unsigned far ail_default_timbre_cache_size(int drv);      /* 000c:0c05 */
extern void far ail_define_timbre_cache(int drv, void far *cache, unsigned size); /* 000c:0c0b */
extern unsigned far ail_timbre_request(int drv, int seq);        /* 000c:0c11 */
extern void far ail_install_timbre(int drv, int bank, int patch, void far *data); /* 000c:0c17 */
extern void far ail_start_sequence(int drv, int seq);            /* 000c:0c2f */
extern void far ail_stop_sequence(int drv, int seq);             /* 000c:0c35 */
extern void far ail_resume_sequence(int drv, int seq);           /* 000c:0c3b */
extern int far ail_sequence_status(int drv, int seq);            /* 000c:0c41 */
extern void far ail_set_relative_volume(int drv, int seq, int percent, int ms); /* 000c:0c53 */
extern void far sfx_play(unsigned short sound_id);                      /* 1023:0006 */
extern void far sfx_setup(void);                              /* 1023:016e */
extern void far sfx_init(void);                                  /* 1023:016f */
extern void far game_sound_stop(void);                           /* 1023:0241 */
extern void far sound_toggle(void);                              /* 1023:0247 */
extern void far music_toggle(void);                              /* 1023:0272 */
extern void far pending_load_set(char load_id);                        /* 2033:0004 */
extern char far pending_load_get(void);                          /* 2033:000f */
extern void far ecl_load_args(unsigned char count);              /* 2033:0013 */
extern unsigned far ecl_arg(char idx);                           /* 2033:0112 */
extern void far game_loop(void);                                 /* 2033:10cd */
extern void far icon_clear(unsigned char idx);                   /* 104c:004d */
extern void far icon_load(char far *name, unsigned char set, unsigned char idx); /* 104c:00df */
extern void far icon_draw(int col, int row, unsigned char kind, unsigned char bank, unsigned char idx);
                                                                 /* 104c:030f */
extern void far pic_free(void);                                  /* 104c:0468 */
extern void far misc_unit_init(void);                                 /* 1093:0005 */
extern void far esc_set(char enabled);                                /* 1093:0006 */
extern char far esc_get(void);                                   /* 1093:0011 */
extern void far mem_zero(void far *buffer, int byte_count);                    /* 1093:0015 */
extern void far game_exit(void);                                 /* 1093:002d */
extern unsigned char far game_key_get(void);                     /* 1093:0078 */
extern void far input_pending_clear(void);                       /* 1093:0130 */
extern void far input_press_discard(void);                                  /* 1093:0136 */
extern void far *far rec_alloc(void);                            /* 1093:0162 */
extern void far rec_free(void far * far *record_ref);                    /* 1093:0188 */
extern char far *far byte_str(unsigned char value);                  /* 1093:01a7 */
extern char far *far int_str(int value);                             /* 1093:01c3 */
extern char far *far ulong_str(unsigned long value);                 /* 1093:01dc */
extern void far frame_border(void);                              /* 10b2:0030 */
extern void far frame_character_sheet(void);                                /* 10b2:00a4 */
extern void far frame_main(void);                                /* 10b2:00e4 */
extern void far moon_display_update(char redraw_mode);                     /* 10b2:022c */
extern void far frame_lower_panel(void);                                /* 10b2:02d3 */
extern void far frame_lower_panel_palette(void);                                /* 10b2:02e1 */
extern void far frame_right_panel(void);                                /* 10b2:02fa */
extern void far frame_list_panel(void);                                /* 10b2:03b5 */
extern void far frame_gen(void);                                 /* 10b2:03ce */
extern void far frame_select(int frame_id);                             /* 10b2:047c */
extern void far icon_facing(unsigned char cell, unsigned char half, unsigned char row, char mirror);
                                                                 /* overlay 0 20cb:0005 */
extern void far combat_sides_count(void);                        /* 20cb:0920 */
extern unsigned char far combat_targets_near(struct Character far *character, unsigned char range, char same);
                                                                 /* overlay 0 20cb:0980 */
extern unsigned char far get_distance(struct Character far *character, struct Character far *character_cursor);
                                                                 /* overlay 0 20cb:0b26 */
extern char far combat_guard(struct Character far *character);           /* overlay 0 20cb:0c52 */
extern void far combat_view_show(void);                          /* 20cb:0c94 */
extern char far has_ranged_weapon(struct Character far *character);      /* overlay 0 20cb:0cbf */
extern char far has_throwable_weapon(struct Character far *character);   /* overlay 0 20cb:0cfe */
extern char far bandage_dying(char apply);                           /* overlay 0 20cb:0d63 */
extern char far ranged_ammo_find(struct Character far *character, struct Item far * far *sel);
                                                                 /* overlay 0 20cb:10c3 */
extern void far fx_snakes(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:0009 */
extern void far fx_noop(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:0075 */
extern void far fx_charm(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:007a */
extern void far fx_silence(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:012c */
extern void far fx_attack_bonus_vs_type80(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:01c7 */
extern void far fx_nausea(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:01fb */
extern void far fx_dispel_evil_attack(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:028c */
extern void far fx_confusion(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:033f */
extern void far fx_blink(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:058c */
extern void far fx_poison_attack(struct Character far *character, char save); /* 30cc:05b9 */
extern void far fx_petrify_attack(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:069f */
extern void far fx_feeblemind(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:06f6 */
extern void far fx_damage_vs_immunity1(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:07c8 */
extern void far fx_berserk(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:081f */
extern void far fx_drain_one_level(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:093b */
extern void far fx_drain_two_levels(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0960 */
extern void far fx_fire_damage_4d6(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0985 */
extern void far fx_berserk_creature(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:09e8 */
extern void far fx_fire_damage_2d6(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0ad9 */
extern void far fx_fear_remove(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:0b3c */
extern void far fx_vorpal(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:0b74 */
extern void far fx_entangle_attack(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0bc0 */
extern void far fx_fixed_dice_attack(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0c28 */
extern void far fx_damage_vs_immunity4(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0d07 */
extern void far fx_disruption_attack(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0d36 */
extern void far fx_movement_stop(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0dc6 */
extern void far fx_hp_damage_vs_immunity80(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0ded */
extern void far fx_attack_vs_immunity80(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0e24 */
extern void far fx_poison_steam_breath(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:0ebf */
extern void far fx_lightning_breath(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:10ab */
extern void far fx_spell_gaze(struct Character far *character, struct Effect far *effect, char removed); /* 30cc:11d6 */
extern void far fx_breath_weapon(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 30cc:12a1 */
extern void far timer_tick(void);                                /* 00d7:0009 */
extern int far bios_key(int cmd);                                /* 00d7:005c */
extern void far column_2bpp(char far *bits, int rows, int and_mask, int xor_mask, int odd_mask, int even_mask);
                                                                 /* 00d7:008f */
extern void far column_1bpp(char far *bits, int rows, int and_mask, int xor_mask, int odd_mask, int even_mask);
                                                                 /* 00d7:0102 */
extern void far pointer_hide(void);                              /* 00d7:0141 */
extern void far timer_install(void far *isr);                    /* 00d7:0153 */
extern void far timer_isr(void);                                 /* 00d7:019e */
extern void far mouse_handler_set(void (far *fn)(void), int mask); /* 00d7:0203 */
extern int far ega_query(void);                                  /* 00d7:021d */
extern int far display_code(void);                               /* 00d7:0230 */
extern void far blit_words(void far *p, int a, int b, int c);    /* 00d7:0241 */
extern void far blit_bytes(void far *p, int a, int b, int c);    /* 00d7:0286 */
extern void far blit_masked_w(void far *p, void far *q, int a, int b, int c); /* 00d7:02ba */
extern void far blit_masked_b(void far *p, void far *q, int a, int b, int c); /* 00d7:030a */
extern void far fill_bytes(int rows, int n, int attr);           /* 00d7:0348 */
extern void far fill_words(int rows, int n, int fill);           /* 00d7:037b */
extern void far mask_words(int fill, int keep, int rows, int n); /* 00d7:03b0 */
extern void far pattern_rows(int rows, int n, int far *pat, int row); /* 00d7:03ee */
extern void far rle_draw(int top, int rows, int left, int width, void far *q, void far *p); /* 00d7:057e */
extern void far expand_1to2(void far *src, void far *dst, int n); /* 00d7:0708 */
extern void far expand_1to4(void far *src, void far *dst, int n); /* 00d7:0754 */
extern void far expand_1to8(void far *src, void far *dst, int n); /* 00d7:07cb */
extern void far fill_far_words();                                /* 00d7:08a2 */
extern void far far_move(unsigned src, unsigned src_seg, unsigned dst, unsigned dst_seg, unsigned n);
                                                                 /* 00d7:0918 */
extern void far cga_copy(unsigned from_seg, unsigned to_seg);    /* 00d7:0956 */
extern int far swap16(int v);                                    /* 00d7:098b */
extern long far swap32(long v);                                  /* 00d7:0994 */
extern void far grab_words(void far *p, int a, int b);           /* 00d7:09e4 */
extern char far *far packbits_expand(char far *in, char far *out, int n); /* 00d7:0a1c */
extern void far split_nibbles(char far *in, char far *out, int n); /* 00d7:0a5c */
extern void far nibbles_to_planar(char far *in, char far *out, int stride, int n); /* 00d7:0a80 */
extern int far mouse_call(int fn, ...);                          /* 00d7:0b3d */
extern void far mouse_call_regs(int far *ax, int far *bx, int far *cx, int far *dx); /* 00d7:0b5a */
extern void far mouse_handler(void);                             /* 00d7:0b8b */
extern void far blit_shifted(char far *image, void far *mask, int rows, int bytes, int skip, int first_mask, int last_mask, int shift);
                                                                 /* 00d7:0bbc */
extern void far put_bytes(void far *buf, int rows, int cols);    /* 00d7:0c46 */
extern void far grab_bytes(void far *buf, int rows, int cols);   /* 00d7:0c7f */
extern char far *far huff_expand(char far *in, char far *out, int n, void far *table); /* 00d7:0cbb */
extern void far pattern_column(int rows, int mask, int far *pat, int row); /* 00d7:0d31 */
extern void far *far huge_add(void far *p, long n);              /* 00d7:0d7d */
extern void far reg_write(int port, int idx, int val);           /* 00d7:0da0 */
extern char far *far runs_draw(int top, int bottom, int rows, int left, int width, char far *p);
                                                                 /* 00d7:0db6 */
extern void far mem_swap(void far *a, void far *b, unsigned n);  /* 00d7:0e16 */
extern void far dissolve_bytes(unsigned from_seg, unsigned to_seg); /* 00d7:0e35 */
extern void far dissolve_planes(unsigned from_seg, unsigned to_seg); /* 00d7:0e64 */
extern void far dissolve_nibbles(unsigned from_seg, unsigned to_seg); /* 00d7:0ebf */
extern void far dissolve_bits(unsigned from_seg, unsigned to_seg); /* 00d7:0f27 */
extern void far dissolve_2bpp(unsigned from_seg, unsigned to_seg); /* 00d7:0fdc */
extern void far dissolve_2bank(unsigned from_seg, unsigned to_seg); /* 00d7:1034 */
extern void far speaker_play(char far *samples, unsigned len, unsigned divisor, int times); /* 00d7:10df */
extern void far row_mirror1(char far *p, int n);                 /* 00d7:1f37 */
extern void far row_mirror2(char far *p, int n);                 /* 00d7:1f8f */
extern void far row_mirror4(char far *p, int n);                 /* 00d7:1fd8 */
extern void far row_mirror8(char far *p, int n);                 /* 00d7:2018 */
extern void far row_recolor(char far *p, int n, unsigned char far *tab); /* 00d7:2046 */
extern void far plane_recolor(char far *p, int a, int b, unsigned char far *tab); /* 00d7:206e */
extern void far dac_load(int first, int count, int wait, void far *rgb); /* 00d7:20f6 */
extern void far keyed_blit(char far *src, int rows, int width, int skip, char key); /* 00d7:212e */
extern void far xvga_char(int fore, int back, unsigned char far *bits); /* 00d7:2171 */
extern int far lzw_emit(char far * far *out, int code, struct Lzw far *t); /* 00d7:224b */
extern char far *far lzw_decode(unsigned w, int code, unsigned char far *in, int next, char far *end, struct Lzw far *t, char far * far *out);
                                                                 /* 00d7:2286 */
extern char far *far lz_decode(char far *in, char far *out, int n); /* 00d7:233d */
extern int far con_getch(void);                                  /* 00d7:2450 */
extern void far con_putc(char c);                                /* 00d7:247d */
extern int far con_direct(int c);                                /* 00d7:2495 */
extern void far str_cat(char far *dst, char far *src);           /* 00d7:24b7 */
extern int far str_cmp(char far *a, char far *b);                /* 00d7:24dc */
extern int far str_same(char far *a, char far *b);               /* 00d7:2543 */
extern void far str_copy(char far *dst, char far *src);          /* 00d7:2564 */
extern int far str_len(char far *s);                             /* 00d7:257d */
extern char far *far str_find(char far *s, char c);              /* 00d7:2594 */
extern int far to_lower(int c);                                  /* 00d7:25b1 */
extern int far to_upper(int c);                                  /* 00d7:25c2 */
extern void far str_upr(char far *s);                            /* 00d7:25fb */
extern int far int_max(int a, int b);                            /* 00d7:262b */
extern int far int_min(int a, int b);                            /* 00d7:263c */
extern unsigned far uint_min(unsigned a, unsigned b);            /* 00d7:265e */
extern void far *far mem_alloc(unsigned n);                      /* 00d7:266f */
extern int far dos_call(int ax, ...);                            /* 00d7:269d */
extern int far dos_open(int ax, char far *name, ...);            /* 00d7:26c2 */
extern int far dos_io(int ax, void far *buf, int bx, int cx);    /* 00d7:26ef */
extern int far dos_regs(int ah, ...);                            /* 00d7:270b */
extern long far file_seek(int fd, long pos, int whence);         /* 00d7:273a */
extern void far mem_move(void far *src, void far *dst, unsigned n); /* 00d7:2776 */
extern void far mem_fill(void far *p, unsigned n, int c);        /* 00d7:27a6 */
extern int far mem_scan(void far *p, int n, int c);              /* 00d7:27dc */
extern void far *far far_alloc(long n);                          /* 00d7:2820 */
extern int far far_free(void far *p);                            /* 00d7:285b */
extern int far peek_byte(unsigned seg, unsigned off);            /* 00d7:2873 */
extern void far poke_byte(unsigned seg, unsigned off, int b);    /* 00d7:2883 */
extern int far peek_word(unsigned seg, unsigned off);            /* 00d7:2895 */
extern void far poke_word(unsigned seg, unsigned off, int w);    /* 00d7:28a3 */
extern int far int_call(int n);                                  /* 00d7:28b5, executes int n */
extern void far crit_install(void);                              /* 00d7:2924 */
extern int far video_bios(int ah, ...);                          /* 00d7:29a6 */
extern long far clock_bios(int ah, ...);                         /* 00d7:2a46 */
extern int far env_get(char far *name, char far *value);         /* 00d7:2b94 */
extern void far sys_init(int argc, char far *far *argv, int need_kb, int want_kb); /* 1102:000c */
extern void far sys_shutdown(void);                              /* 1102:0073 */
extern char far key_pressed(void);                               /* 1102:00a1 */
extern void far key_flush(void);                                 /* 1102:00c4 */
extern void far key_wait(void);                                  /* 1102:00fc */
extern void far beep(void);                                      /* 1102:0193 */
extern int far random_int(int upper_bound);                                /* 1102:0234 */
extern void far clear_screen(int attribute);                             /* 1102:02a4 */
extern void far text_print_at(int row, int col, int a, char far *fmt, ...); /* 1102:03a0 */
extern void far error_box(char far *fmt, ...);                   /* 1102:03cf */
extern void far prompt_window_push(int row, int col);            /* 114b:0018 */
extern void far prompt_attr_set(int attr);                       /* 114b:0052 */
extern void far prompt_beep_set(char enabled);                        /* 114b:0064 */
extern void far prompt_window_pop(void);                         /* 114b:006f */
extern void far win_text_printf(char far *fmt, ...);             /* 114b:0173 */
extern void far character_files_scan(struct Node far * far *list, struct Node far * far *files, char kind);
                                                                 /* 2177:054a */
extern void far character_icon_load(char unused_flag);                    /* 2177:05d7 */
extern void far character_file_load(char far *name, struct Character far *hero, char unused_arg_a, char unused_arg_b);
                                                                 /* 2177:2198 */
extern void far character_file_save(struct Character far *hero, char far *file); /* 2177:23a7 */
extern void far mon_load(unsigned char id, char recs, struct Character far *hero); /* 2177:257c */
extern void far mon_add(unsigned char id);                       /* 2177:2886 */
extern void far game_load_menu(void);                            /* 2177:2e87 */
extern void far game_save_menu(void);                            /* 2177:3395 */
extern void far party_character_append(struct Character far *hero); /* 2177:3491 */
extern void far character_file_lists_free(struct Node far *list, struct Node far *files); /* 2177:359f */
extern void far vault_load_current(void);                        /* 2177:35bc */
extern void far vault_save_current(void);                        /* 2177:3600 */
extern char far archive_open(char disk, char far *name, int flags, char (far *filename)()); /* 117e:01e3 */
extern void far file_open(char disk, char far *name, int flags, char (far *filename)()); /* 117e:03f2 */
extern void far disk_setup(void (far *handler)(), char subdirs, char far *arg, char save); /* 117e:04a6 */
extern char far dir_first(char disk, char far *name, char far *ext, char files, char dirs); /* 117e:04cd */
extern char far *far dir_next(char far *is_dir);                 /* 117e:05ef */
extern void far combat_round_begin(struct Character far *character);     /* 21e4:0004 */
extern void far combat_move_step(struct Character far *character, unsigned char dir); /* overlay 0 21e4:06c0 */
extern void far opportunity_attacks(struct Character far *character, unsigned char dir); /* overlay 0 21e4:08e6 */
extern char far combat_flee(struct Character far *character);            /* overlay 0 21e4:0e26 */
extern void far attacks_left_recalc(struct Character far *character);    /* overlay 0 21e4:0edf */
extern char far combat_target_allowed(struct Character far *character, struct Character far *character_cursor, char any);
                                                                 /* overlay 0 21e4:1030 */
extern void far turn_undead(struct Character far *character);            /* overlay 0 21e4:1117 */
extern char far turn_target_find(struct Character far *character, struct Character far * far *target, int far *level);
                                                                 /* overlay 0 21e4:135e */
extern void far attack_register(struct Character far *character, struct Character far *character_cursor); /* overlay 0 21e4:1853 */
extern void far combat_attack(struct Character far *character, struct Character far *character_cursor, char mode, struct Item far *missile, char far *done);
                                                                 /* overlay 0 21e4:18d6 */
extern void far combat_spell_targets(unsigned char spell, char mode, char far *result); /* 21e4:1f78 */
extern void far combat_cast_begin(unsigned char spell, char mode, char far *done); /* overlay 0 21e4:2543 */
extern void far hostile_hp_percent_update(void);                 /* 21e4:29f1 */
extern unsigned char far fastest_enemy_speed(struct Character far *character); /* overlay 0 21e4:2a76 */
extern char far attack_confirm(struct Character far *character, struct Character far *character_cursor); /* 21e4:2aef */
extern void far combat_aim(struct Character far *character, char range, char gate, char flag, char mode, char far *result, struct Aim far *aim);
                                                                 /* 21e4:37dd */
extern char far combat_target_pick(struct Character far *character, char range, char any, char fresh, char flag);
                                                                 /* overlay 0 21e4:3d9e */
extern char far gods_intervene(void);                            /* overlay 0 21e4:3feb */
extern char far spell_target_rating(struct Character far *character, unsigned char spell); /* overlay 0 21e4:4789 */
extern void far combat_leadership_apply(void);                   /* 21e4:4dd2 */
extern char far combat_sweep_attack(struct Character far *character, struct Character far *character_cursor);
                                                                 /* overlay 0 21e4:52e4 */
extern int far scroll_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b); /* 11e9:082d */
extern void far video_restore(void);                             /* 12b7:000d */
extern void far fill_rect(int row1, int col1, int row2, int col2, int attr); /* 12b7:0078 */
extern void far screen_page_save(void);                          /* 12b7:0849 */
extern void far screen_page_show(void);                          /* 12b7:0a0b */
extern void far set_text_mode(char mode);                        /* 12b7:0c21 */
extern char far text_mode(void);                                 /* 12b7:0c92 */
extern int far draw_char(int row, int col, char character_code, char attr, char flags); /* 12b7:0c96 */
extern int far screen_rows(void);                                /* 12b7:14ac */
extern int far screen_cols(void);                                /* 12b7:14b0 */
extern int far pixel_shift(void);                                /* 12b7:14b4 */
extern int far video_planes(void);                               /* 12b7:14b8 */
extern char far is_xvga(void);                                   /* 12b7:14bc */
extern void far video_plane_set(int plane);                      /* 12b7:14cb */
extern void far video_address_set(int row, int col);             /* 12b7:15fc */
extern void far mouse_pointer_init(int argc, char far * far *argv);    /* 12b7:1be3 */
extern void far mouse_pointer_done(void);                              /* 12b7:1c7b */
extern int far pointer_mode_set(int mode);                          /* 12b7:1c8c */
extern void far pointer_show(void);                              /* 12b7:2122 */
extern void far pointer_hide_rect(int row1, int col1, int row2, int col2); /* 12b7:2288 */
extern void far pointer_draw_over_buffer(char far *buf);                     /* 12b7:22f6 */
extern void far pointer_restore_from_buffer(char far *buf);                     /* 12b7:2333 */
extern int far mouse_press(char mask, int far *x, int far *y);   /* 12b7:235f */
extern char far mouse_clicked(int far *x, int far *y);           /* 12b7:2418 */
extern void far mouse_pos(int far *x, int far *y);               /* 12b7:2485 */
extern void far mouse_move_to(int row, int col);                 /* 12b7:2519 */
extern void far mouse_fake(int button, char both);               /* 12b7:254b */
extern int far mouse_pending(void);                              /* 12b7:25e9 */
extern void far pointer_shape_set(struct Pointer far *shape);        /* 12b7:25ff */
extern void far clock_advance(unsigned char elapsed_units, short unit);      /* 0396:044d */
extern void far two_digits(unsigned char value, char far *buf);      /* 0396:05aa */
extern char far camp_rest(char interactive);                     /* 0396:0d7d */
extern int far spell_study_level_fallback(int spell);                            /* 0396:0f69 */
extern void far fx_taunt(struct Character far *character);               /* 048f:0005 */
extern void far fx_disable(struct Character far *character, int how, char far *msg); /* 048f:00d4 */
extern void far fx_remove(struct Character far *character, unsigned char kind, struct Effect far *effect);
                                                                 /* 048f:017d */
extern void far fx_trigger_group(unsigned char when, struct Character far * far *character_ref); /* 048f:0512 */
extern void far fx_terrain_apply(struct Character far *character, char may_save, char skip_1d); /* 048f:12ab */
extern char far fx_attack_roll_no_attacker(struct Character far *character, unsigned char need); /* 048f:16e0 */
extern char far fx_attack_roll(struct Character far *character, struct Character far *target, unsigned char need);
                                                                 /* 048f:1771 */
extern char far fx_saving_throw(struct Character far *character, int type, char adj); /* 048f:1886 */
extern int far fx_roll_dice(unsigned char dice_count, unsigned char sides); /* 048f:1968 */
extern int far fx_roll_damage(unsigned char dice_count, unsigned char sides); /* 048f:199e */
extern void far fx_add(struct Character far *character, unsigned char kind, int dur, unsigned char strength, char on_remove);
                                                                 /* 048f:19c2 */
extern void far fx_out_of_play(struct Character far *character, int how, char far *msg); /* 048f:1a69 */
extern void far fx_invisibility_remove(struct Character far *character); /* 048f:1b2a */
extern void far fx_hp_effects_remove(struct Character far *character);   /* 048f:1b66 */
extern char far fx_cure(struct Character far *character, unsigned char kind); /* 048f:1bc6 */
extern char far fx_strength_upgrade(struct Character far *character, unsigned char base, unsigned char exceptional_strength, unsigned char far *out);
                                                                 /* 048f:1c87 */
extern void far fx_stat_recalc(struct Character far *character, short stat); /* 048f:1dfc */
extern void far fx_damage(struct Character far *character, unsigned char dmg, int reduce, char saved); /* 048f:2214 */
extern void far fx_add_message(struct Character far *character, unsigned char kind, int dur, unsigned char strength, char on_remove, int save_negates, char saved, char far *msg);
                                                                 /* 048f:252c */
extern char far fx_heal(struct Character far *character, unsigned char healing_amount, char show); /* 048f:25f7 */
extern char far fx_revive(struct Character far *character, unsigned char restored_hp, char far *msg); /* 048f:26c1 */
extern void far fx_clouds_tick(char now);                          /* 048f:2817 */
extern void far fx_death_explosions(void);                       /* 048f:2ab8 */
extern void far fx_ice_explosions(void);                         /* 048f:2c4f */
extern void far fx_meteor_explosions(void);                      /* 048f:2d9b */
extern void far pic_frame(int col, int row, char show, unsigned char frame_index); /* 24da:000c */
extern void far pic_open(char far *name, unsigned char id, char unused, struct Pic far *picture); /* 24da:0078 */
extern void far pic_release(void);                               /* 24da:02b0 */
extern void far pic_show(unsigned char frame_index);                       /* 24da:02e5 */
extern void far big_open(unsigned char id);                      /* 24da:0321 */
extern void far big_redraw(void);                                /* 24da:03b0 */
extern void far bigpic_release(void);                            /* 24da:03eb */
extern void far pool_clear(void);                                /* 251a:0002 */
extern char far pool_save(int file_handle);                               /* 251a:00df */
extern char far pool_load(int file_handle);                               /* 251a:0239 */
extern void far *far db_buffer(void);                            /* 151e:0007 */
extern void far db_init(long size);                              /* 151e:000f */
extern void far timer_rehook(void);                              /* 151e:004d */
extern void far timer_unhook(void);                              /* 151e:0085 */
extern long far timer_ticks(void);                               /* 151e:00c6 */
extern long far sound_ticks(void);                               /* 151e:00ce */
extern void far lowio_init(void);                              /* 151e:00da */
extern void far lowio_done(void);                              /* 151e:00db */
extern char far key_poll(void);                                  /* 151e:00dc */
extern int far key_get(void);                                    /* 151e:00e7 */
extern void far console_beep(void);                              /* 151e:02b5 */
extern void far pal_entry(int idx, unsigned rgb);                /* 151e:02c0 */
extern void far palette_load(unsigned char far *pal);            /* 151e:049d */
extern void far palette_page_load(unsigned char far *pal, char flag); /* 151e:056e */
extern void far text_xy(int row, int col, int far *out_row, int far *out_col); /* 151e:0595 */
extern void far text_xy_to(int row, int col, int x, int y, int far *out_row, int far *out_col);
                                                                 /* 151e:05e6 */
extern void far text_offset_xy(int row, int col, int row2, int col2, int far *out_row, int far *out_col);
                                                                 /* 151e:0653 */
extern void far clip_set(int row1, int col1, int row2, int col2); /* 151e:06cb */
extern void far clip_reset(void);                                /* 151e:075c */
extern void far plane_split(char far *src, int byte_count, char far *dst); /* 151e:083c */
extern int far memory_above_320k(void);                          /* 151e:087a */
extern void far config_load(void);                               /* 25a4:0000 */
extern char far too_heavy(struct Character far *hero, long amount, long far *room); /* 25a4:0000 */
extern void far video_check(void);                               /* 25a4:00e2 */
extern void far pool_money(void);                                /* 25a4:00f6 */
extern void far share_money(void);                               /* 25a4:01ee */
extern void far take_menu(void);                                 /* 25a4:0683 */
extern void far pool_check(char far *has_coins, char far *has_items); /* 25a4:092c */
extern void far appraise(char far *done);                        /* 25a4:097c */
extern void far mu_learn(struct Character far *character);               /* 25a4:30d3 */
extern void far win_scroll(int row1, int col1, int row2, int col2, int drow, int dcol, int attr);
                                                                 /* 15a7:0006 */
extern void far font_load(char disk, char far *name);            /* 15a7:053e */
extern void far party_pool_init(int record_count);                          /* 25ca:0001 */
extern void far npc_pool_init(int record_count);                            /* 25ca:0025 */
extern void far item_pool_init(int record_count);                           /* 25ca:0049 */
extern void far item_pool_select(int unused_count);                         /* 25ca:006d */
extern void far effect_pool_init(int record_count);                         /* 25ca:007c */
extern void far cloud_pool_init(int record_count);                          /* 25ca:00a0 */
extern void far combat_pool_init(int record_count);                         /* 25ca:00c4 */
extern void far ecl_stack_pool_init(int record_count);                      /* 25ca:00e8 */
extern void far drop_pool_init(int record_count);                           /* 25ca:010c */
extern void far debug_password_init(void);                       /* 25ca:0130 */
extern void far *far arena_alloc(int byte_count);                         /* 25de:06e6 */
extern void far speaker_init(void);                              /* 15ff:0009 */
extern void far speaker_done(void);                              /* 15ff:0043 */
extern void far speaker_silence(void);                           /* 15ff:005b */
extern void far voice_set(int voice, unsigned freq, int volume); /* 15ff:0079 */
extern void far speaker_enable(char enabled);                         /* 15ff:01b2 */
extern char far speaker_enabled(void);                           /* 15ff:01c7 */
extern void far pal_init(void);                                  /* 161b:000b */
extern void far pal_set(int first, int count, unsigned char far *rgb, int nspec, struct CycSpec far *spec);
                                                                 /* 161b:00b1 */
extern void far pal_commit(void);                                /* 161b:0455 */
extern void far input_poll(void);                                /* 161b:05be */
extern void far glyph_library_init(void);                              /* 169b:0006 */
extern void far glyph_library_done(void);                              /* 169b:002e */
extern void far glyph_library_open(char mode, char far *name, int slot); /* 169b:002f */
extern char far *far glyph_find(char far *lib, int glyph_index, struct Glyph far *hdr); /* 169b:0075 */
extern void far color_map_set(char far *map);                    /* 169b:011c */
extern void far fill_pattern(int color, int far *out);           /* 169b:0137 */
extern void far glyph_draw_at(int row, int col, char far *lib, int glyph); /* 169b:0a9c */
extern void far draw_glyph(int row, int col, int library_slot, int glyph);  /* 169b:0bd0 */
extern void far glyph_bounds(int row, int col, int slot, int glyph, int far *top, int far *left, int far *bottom, int far *right);
                                                                 /* 169b:0bef */
extern void far pointer_glyph_load(int slot, int glyph_index);         /* 169b:0cf8 */
extern void far loot_take(char far *coins, char far *items); /* 26b7:0d01 */
extern void far display_options_set(int opts);                   /* 2726:08c6 */
extern char far icon_cell_delta(unsigned char size, unsigned char cell_index, char far *dx, char far *dy);
                                                                 /* overlay 0 2740:000a */
extern void far combat_palette_set(void);                        /* overlay 0 2740:00e4 */
extern void far cell_redraw(char x, char y);                     /* overlay 0 2740:025d */
extern void far combat_grid_build(void);                         /* overlay 0 2740:0330 */
extern char far in_combat_view(char x, char y);                  /* overlay 0 2740:05f7 */
extern char far combatant_visible(struct Character far *character, char whole); /* overlay 0 2740:0626 */
extern void far combat_view_scroll(char x, char y, unsigned char margin, unsigned char dir);
                                                                 /* overlay 0 2740:0846 */
extern void far combat_direction_probe(struct Character far *character, unsigned char dir, unsigned char far *who, unsigned char far *cell, unsigned char far *kind, char far *special_terrain);
                                                                 /* overlay 0 2740:0bd1 */
extern void far view_center(struct Character far *character, unsigned char margin, char cursor);
                                                                 /* overlay 0 2740:12f5 */
extern unsigned char far combatant_direction(struct Character far *from, struct Character far *target);
                                                                 /* overlay 0 2740:16dd */
extern void far hero_frame_flag_set(char redraw_flag);                     /* 077e:000b */
extern void far item_name(struct Character far *character, struct Item far *item, char col, char row, char full, char show);
                                                                 /* 077e:03fc */
extern void far show_ac(struct Character far *character, char col, char row); /* 077e:0724 */
extern void far show_hp(struct Character far *character, char col, char row, char hilite, char max); /* 077e:07b4 */
extern void far hero_panel(struct Character far *character);             /* 077e:085f */
extern char far is_helpless(struct Character far *character);            /* 077e:0a19 */
extern unsigned char far calc_move(struct Character far *character);     /* 077e:0a5f */
extern void far recalc_stats(struct Character far *character);        /* 077e:0be2 */
extern char far dex_missile(struct Character far *character);            /* 077e:104d */
extern char far str_hit(struct Character far *character);                /* 077e:1168 */
extern char far str_damage(struct Character far *character);             /* 077e:11f9 */
extern int far str_carry(struct Character far *character);               /* 077e:1273 */
extern void far spell_mark(struct Character far *character, unsigned char spell); /* 077e:1346 */
extern char far has_ready_8c(struct Character far *character);        /* 077e:1389 */
extern void far item_remove(struct Character far *character, struct Item far *item); /* 077e:13d2 */
extern void far item_append(struct Character far *character, struct Item far *src); /* 077e:14aa */
extern void far bottom_msg(char far *msg);                       /* 077e:153c */
extern void far hero_text(struct Character far *character, char far *text, char row, char pause); /* 077e:156e */
extern void far combat_text_clear(void);                         /* 077e:1665 */
extern void far character_name_show(struct Character far *character, char col, char row, char poss); /* 077e:1697 */
extern char far find_effect(struct Character far *character, unsigned char kind, struct Effect far * far *out);
                                                                 /* 077e:1734 */
extern void far hero_damage(struct Character far *character, unsigned short dmg); /* 077e:17a0 */
extern void far heal_msg(struct Character far *character);               /* 077e:187a */
extern unsigned char far caster_level(unsigned char spell, char cap); /* 077e:1900 */
extern void far view_redraw(void);                               /* 077e:1ad1 */
extern void far hero_drop(char keep, char icon);                       /* 077e:1e31 */
extern int far class_progression_row(struct Character far *character, int what); /* 077e:1f39 */
extern char far can_level_up(struct Character far *character);        /* 077e:1fe5 */
extern char far con_hp_adj(struct Character far *character, short arg);  /* 077e:20ce */
extern char far chance(unsigned char pct);                       /* 077e:2164 */
extern char far wields_43(struct Character far *character);              /* 077e:218e */
extern char far old_class_abilities_available(struct Character far *character);           /* 077e:21bd */
extern unsigned char far first_level(struct Character far *character);   /* 077e:21eb */
extern unsigned char far class_level(struct Character far *character, int cls); /* 077e:2234 */
extern void far glyph_mirror(char far *lib, int glyph_index);              /* 1796:000c */
extern void far glyph_recolor(char far *lib, int glyph_index, unsigned char far *map); /* 1796:02f4 */
extern void far glyph_palette_load(char far *lib, int glyph_index);        /* 1796:0536 */
extern long far file_read(int file_handle, char far *buf, long remaining_bytes);        /* 17fa:000d */
extern void far file_cache_init(int need_kb, int want_kb);          /* 17fa:00cc */
extern void far file_cache_done(void);                              /* 17fa:0240 */
extern char far slot_open(char far *name, int slot);             /* 17fa:025b */
extern char far grp_append(void far *src, int byte_count);                /* 17fa:03bc */
extern void far mem_report(long need);                           /* 17fa:0448 */
extern void far grp_pop(void);                                   /* 17fa:06c3 */
extern void far slot_close(int slot);                            /* 17fa:06fa */
extern void far slot_release(char far *name);                    /* 17fa:0707 */
extern char far grp_read(int file_handle, long byte_count);                      /* 17fa:0796 */
extern void far *far slot_base(int slot);                        /* 17fa:08fd */
extern long far slot_used(int slot);                             /* 17fa:0920 */
extern char far slot_reserve(int slot, long additional_bytes);                  /* 17fa:09ef */
extern void far slot_insert_delete(int slot, long pos, long delta); /* 17fa:0b20 */
extern long far huge_diff(char far *left_pointer, char far *right_pointer);             /* 17fa:0fd5 */
extern void far item_give(struct Item far *item, char far *failed); /* 280a:0295 */
extern void far party_drop(void);                                /* 289c:0ee9 */
extern void far sample_library_set(int slot);                    /* 18fc:0006 */
extern void far sample_sequence_play(int count, ...);            /* 18fc:0016 */
extern void far lib_init(void);                                  /* 193f:0007 */
extern void far lib_done(void);                                  /* 193f:0026 */
extern void far lib_register(long tag, long (far *converter)());        /* 193f:0027 */
extern char far lib_load(int file_handle, long pos, int slot);            /* 193f:0071 */
extern int far lib_items(void far *lib);                         /* 193f:0174 */
extern char far *far lib_item(void far *lib, int item_index);             /* 193f:01fd */
extern long far lib_item_size(void far *lib, int item_index);             /* 193f:02a1 */
extern int far lib_itype(void far *lib);                           /* 193f:033d */
extern int far lib_index_find(void far *lib, int id);               /* 193f:0380 */
extern int far lib_find(int file_handle, int id);                         /* 193f:0419 */
extern long far lib_find_item(int file_handle, int item_index);                    /* 193f:04c9 */
extern void far modify_hero(void);                               /* 2946:1293 */
extern void far cursor_hide(void);                               /* 2999:0072 */
extern void far res_free(struct Res far * far *resource_ref);              /* 09a4:0009 */
extern void far set_open_mode(char mode);                        /* 09a4:01d1 */
extern void far tile_open_file(char far *name, unsigned char set, char kind, char depth, struct Res far * far *resource_ref);
                                                                 /* 09a4:01dc */
extern char far tile_opened(void);                               /* 09a4:042d */
extern void far res_alloc(struct Res far * far *resource_ref, unsigned remaining_items); /* 09a4:0431 */
extern int far res_item_count(struct Res far *resource);                      /* 09a4:04c7 */
extern void far glyph_cell(int col, int row, int glyph, char unused, struct Res far *resource); /* 09a4:04e1 */
extern void far glyph_at(int col, int row, unsigned char glyph, char unused, struct Res far *resource);
                                                                 /* 09a4:0515 */
extern void far glyph_put(int col, int row, unsigned char glyph, char unused, struct Res far *resource);
                                                                 /* 09a4:053c */
extern void far res_glyph_mirror(struct Res far *resource, unsigned char glyph_index);    /* 09a4:055a */
extern void far res_draw(int col, int row, struct Res far *resource);   /* 09a4:057a */
extern void far glyph_draw(int col, int row, int glyph, struct Res far *resource); /* 09a4:05a6 */
extern void far screen_draw_begin(char full_redraw);                      /* 09a4:05ee */
extern void far screen_flush_mode_set(char mode);                  /* 09a4:061a */
extern void far pal_select(struct Res far *resource);                   /* 09a4:0630 */
extern void far screen_flush(void);                              /* 09a4:06d3 */
extern void far res_glyph_recolor(unsigned char far *color_map, int glyph_index, struct Res far *resource); /* 09a4:085e */
extern void far res_copy(struct Res far *src, unsigned char source_item, char map, struct Res far *dst, unsigned char destination_item);
                                                                 /* 09a4:0881 */
extern void far res_reset(void);                                 /* 09a4:0984 */
extern void far cell_fill(int col, int row, int width, unsigned height, unsigned char attr); /* 09a4:09a5 */
extern void far rows_save(int row, int col, int height, int width); /* 09a4:09eb */
extern void far rows_restore(int row, int col, int height, int width); /* 09a4:0aba */
extern void far res_palette_load(struct Res far *resource);                     /* 09a4:0b8c */
extern void far *far tile_loader_identity(int unused_arg_a, int unused_arg_b, int unused_arg_c, void far *data); /* 09a4:0bd9 */
extern void far lib_create(char far *name, int slot, long tag);  /* 1a3d:000e */
extern void far lib_insert(int slot, int item_index, long size);           /* 1a3d:0085 */
extern void far lib_resize(int slot, int item_index, long size);          /* 1a3d:0382 */
extern char far lib_read_convert(int file_handle, long len, int slot, int item_index, long tag); /* 1a3d:04f5 */
extern char far input_result(void);                              /* 0a62:001f */
extern void far input_done(void);                                /* 0a62:0023 */
extern void far status_printf(unsigned char col, unsigned char row, unsigned char fg, unsigned char bg, char far *fmt, ...);
                                                                 /* 0a62:0046 */
extern void far text_wrap(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char foreground, unsigned char background, char clear, char far *text);
                                                                 /* 0a62:0311 */
extern void far status_char(char col, char row, char attr, unsigned char character_code); /* 0a62:05a7 */
extern char far *far input_text(char far *prompt, unsigned char foreground, unsigned char background, unsigned char max);
                                                                 /* 0a62:05c9 */
extern int far input_number(char far *prompt, unsigned char foreground, unsigned char background); /* 0a62:0781 */
extern void far continue_prompt(char unused_arg_a, char unused_arg_b, char far *items); /* 0a62:093f */
extern void far text_pause(void);                                /* 0a62:0949 */
extern void far msg_pause(void);                                 /* 0a62:0960 */
extern long far ticks_now(void);                                 /* 0a62:0979 */
extern void far text_clear(char x1, char y1, char x2, char y2);  /* 0a62:09a1 */
extern void far str_pad_left(char far *text, unsigned char width);  /* 0a62:09fb */
extern int far ui_item_proc(struct Wid far *widget, int msg, int message_arg_a, int message_arg_b); /* 1aa6:01c7 */
extern void far dl_init(int max);                                /* 1aa6:11eb */
extern void far dl_done(void);                                    /* 1aa6:1236 */
extern ItemProc far ui_set_hook(int kind, ItemProc handler);          /* 1aa6:1237 */
extern void far dl_begin(void);                                  /* 1aa6:1277 */
extern void far ui_add(int op, ...);                             /* 1aa6:1294 */
extern void far dl_draw(char all);                               /* 1aa6:151f */
extern int far ui_run(int (far *idle)(void));                    /* 1aa6:1586 */
extern int far ui_poll(void);                                    /* 1aa6:15e3 */
extern void far dl_end(void);                                    /* 1aa6:1875 */
extern void far train_hero(void);                                /* 2aa7:0022 */
extern void far list_box(struct ListBox far *box, char redraw);  /* 0b05:08b3 */
extern void far fx_end_actions(struct Character far *character, struct Effect far *effect, char removed); /* 2b3a:010d */
extern void far hotkey_add(char key, unsigned char far *count);  /* 0bdb:0005 */
extern void far hotkey_reset(unsigned char last_hotkey);                   /* 0bdb:0047 */
extern void far button_hook_install(void);                       /* 0bdb:02a9 */
extern char far get_picked(void);                                /* 0bdb:02c2 */
extern void far set_picked(char value);                              /* 0bdb:02c6 */
extern void far set_selected(struct Node far *node);                /* 0bdb:02d1 */
extern void far set_aborted(char value);                             /* 0bdb:02e9 */
extern char far get_aborted(void);                               /* 0bdb:02f4 */
extern struct Node far *far list_nth(int entry_index, struct Node far *head); /* 0bdb:02f8 */
extern void far box_draw(void);                                  /* 0bdb:036c */
extern void far box_draw_top(void);                              /* 0bdb:03a4 */
extern void far box_draw_both(void);                             /* 0bdb:03d7 */
extern char far confirm(char far *prompt, char anim);            /* 0bdb:0424 */
extern void far press_return_clear(void);                        /* 0bdb:0511 */
extern void far press_return_wait(char anim);                    /* 0bdb:0521 */
extern void far list_alloc(int entry_count, struct Node far * far *head);  /* 0bdb:0547 */
extern void far list_free(struct Node far * far *head);          /* 0bdb:05fb */
extern void far menu_frame(int row, int col);                    /* 0bdb:0c00 */
extern void far frame_invalidate(void);                          /* 0bdb:0d1d */
extern void far set_menu_kind(int kind);                         /* 0bdb:141e */
extern char far menu_bar(char far *title, char far *text, char fkeys, char anim); /* 0bdb:1429 */
extern char far grid_choose(char far *title, char far *text, char fkeys, char anim, unsigned char rows, unsigned char cols);
                                                                 /* 0bdb:1595 */
extern char far grid_pick(char far *title, char far *text, char fkeys, char anim); /* 0bdb:1862 */
extern char far node_menu(char far *title, char far *text, char fkeys, char anim); /* 0bdb:1997 */
extern char far node_prompt(char far *title, char far *text, char fkeys, char anim); /* 0bdb:1a1b */
extern char far node_dir_menu(char far *title, char far *text, char fkeys, char anim); /* 0bdb:1aef */
extern char far grid_small(char far *title, char far *text, char fkeys, char anim, unsigned char rows, unsigned char cols);
                                                                 /* 0bdb:1beb */
extern char far menu_run(char far *title, char far *text, char fkeys, char anim); /* 0bdb:1e29 */
extern char far list_menu(char far *title, char far *text, char row, char col, char row2, char col2, struct Node far *list, char unused_arg_a, char unused_arg_b, char far *redraw_box, int far *top, struct Node far * far *out);
                                                                 /* 0bdb:1f25 */
extern void far set_step_mode(char enabled);                           /* 0bdb:21c3 */
extern char far step_mode(void);                                 /* 0bdb:21ce */
extern void far set_extra_row(char enabled);                           /* 0bdb:21d2 */
extern void far sfx_slots_init(int argc, char far * far *argv);    /* 1c41:0005 */
extern void far sfx_slots_done(void);                              /* 1c41:004c */
extern void far sfx_reset(void);                                 /* 1c41:0057 */
extern void far sfx_update(void);                                /* 1c41:0089 */
extern void far sfx_tick(long ticks);                            /* 1c41:0098 */
extern int far inp_press(char mask, int far *x, int far *y);     /* 1c4c:0002 */
extern char far inp_clicked(int far *x, int far *y);             /* 1c4c:001d */
extern void far inp_pos(int far *x, int far *y);                 /* 1c4c:0035 */
extern char far inp_pending(void);                               /* 1c4c:004d */
extern char far kbd_waiting(void);                               /* 1c4c:0053 */
extern int far kbd_read(void);                                   /* 1c4c:0059 */
extern char far kbd_state_5f(void);                              /* 1c4c:005f */
extern long far file_size(int file_handle);                               /* 1c60:01de */
extern void far path_name(char far *path, char far *out);        /* 1c8e:02b4 */
extern void far path_ext(char far *path, char far *out);         /* 1c8e:02ed */
extern void far path_base_copy(char far *path, char far *out);   /* 1c8e:0337 */
extern void far path_append_name(char far *dir, char far *name); /* 1c8e:03b2 */
extern int far file_find_first(char far *path, int attr);        /* 1cd9:000a */
extern char far * far file_find_next(void);                      /* 1cd9:0169 */
extern int far file_find_attrib(void);                           /* 1cd9:02ec */
extern void far con_printf(char far *fmt, ...);                  /* 1d1b:0004 */
extern int far str_format(char far *buf, char far *fmt, ...);    /* 1d1b:003a */
extern void far text_format(char far *fmt, void far *args, void (far *put)(), char far *codes, ...);
                                                                 /* 1d22:011d */
extern int far str_to_num(char far *text);                          /* 1d84:000b */
extern void far cast_announce(struct Character far *character, char far *text, unsigned char spell); /* 1d99:0e55 */
extern void far item_spend(struct Character far *character, struct Item far *item, unsigned char spell);
                                                                 /* 1d99:0e5f, vector to overlay 10 2b3a:442e */
extern char far item_casts(struct Item far *item);               /* 1d99:0e69, vector to overlay 10 2b3a:43f7 */
extern void far fx_dispatch(unsigned char id, struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 1d99:0e7d, vector to overlay 9 2b3a:58e5 */
extern void far hero_fall(struct Character far *character);              /* 1d99:0e87, vector to overlay 0 2740:0d55 */
extern unsigned char far combatant_slot(struct Character far *character);
                                                                 /* 1d99:0e91, vector to overlay 0 2740:0b79 */
extern void far plan_steps(char x, char y, unsigned short budget, unsigned char dir, unsigned char size);
                                                                 /* 1d99:0e9b, vector to overlay 2 2a89:07d7 */
extern char far combatant_x(struct Character far *character);            /* 1d99:0ea5, vector to overlay 0 2740:0aea */
extern char far combatant_y(struct Character far *character);            /* 1d99:0eaf, vector to overlay 0 2740:0b19 */
extern unsigned char far combatant_size(struct Character far *character);
                                                                 /* 1d99:0eb9, vector to overlay 0 2740:0b49 */
extern char far combat_actions_end(struct Character far *character);     /* 1d99:0ee1, vector to overlay 0 20cb:0c11 */
extern void far tile_update(char x, char y, unsigned char who);  /* 1d99:0ef5, vector to overlay 0 2740:04d4 */
extern void far hit_flash(struct Character far *character, char effect, char far *msg);
                                                                 /* 1d99:0f09, vector to overlay 0 20cb:0731 */
extern char far hero_place(struct Character far *character, unsigned char x, unsigned char y, char moving);
                                                                 /* 1d99:0f1d, vector to overlay 0 2740:10bd */
extern void far turn_checks(void);                               /* 1d99:0f27, vector to overlay 10 2ffc:000c */
extern void far spell_run(unsigned char spell);                  /* 1d99:0f31, vector to overlay 10 2ffc:21b5 */
extern void far icon_scatter(unsigned char radius, unsigned char cell, unsigned char sound);
                                                                 /* 1d99:0f3b, vector to overlay 0 20cb:0def */
extern long far level_xp_threshold(struct Character far *character, unsigned char level, short cls);
                                                                 /* 1d99:0f6d, vector to overlay 11 304e:0e9c */
extern void far order_down(void);                                /* 1d99:0f8b */
extern void far click_set(char value);                               /* 1d99:0f9f */
extern void far view_click(int zone, int ty, int tx);             /* 1d99:0fa9 */
extern int far arg_hex(char far *key, int value);                /* 1d99:0fb3 */
extern void far game_done(void);                                  /* 1d99:0fbd, vector to overlay 4 25de:0130 */
extern void far combat_main(void);                               /* 1d99:0fc7, vector to overlay 0 1f4e:059d */
extern void far camp_spell_targets(unsigned char spell, char mode, char far *ok);
                                                                 /* 1d99:0fd1, vector to overlay 10 2ffc:135e */
extern void far combat_setup(void);                              /* 1d99:0fdb, vector to overlay 1 28d0:1dd7 */
extern void far char_view(char far *done);                       /* 1d99:0fef, vector to overlay 11 2b3a:0b0c */
extern void far ai_turn(struct Character far *character);                /* 1d99:0ff9, vector to overlay 2 28d0:0000 */
extern void far items_menu(char far *done);                      /* 1d99:1003, vector to overlay 11 2b3a:0f41 */
extern void far spell_cast(unsigned char spell, char quiet, char announce, char far *done);
                                                                 /* 1d99:100d, vector to overlay 10 2ffc:145b */
extern void far combatant_facing_set(struct Character far *character, unsigned char dir, char frame, char erase);
                                                                 /* 1d99:1021, vector to overlay 0 2740:09ab */
extern void far icon_fly(int x0, int y0, int x1, int y1, unsigned char frames, unsigned char pace);
                                                                 /* 1d99:103f, vector to overlay 0 20cb:00df */
extern int far enemy_team(struct Character far *character);              /* 1d99:105d, vector to overlay 0 20cb:08fd */
extern void far missile_facing(unsigned char dir, unsigned char cell);
                                                                 /* 1d99:1067, vector to overlay 0 20cb:0fc8 */
extern void far icon_facings(unsigned char cell);                /* 1d99:1071, vector to overlay 0 20cb:0053 */
extern char far line_step(struct Line far *line);                   /* 1d99:107b, vector to overlay 2 2a89:020f */
extern void far line_init(struct Line far *line);                   /* 1d99:1085, vector to overlay 2 2a89:01a3 */
extern int far int_sign(int value);                                  /* 1d99:108f, vector to overlay 2 2a89:0010 */
extern void far eye_rays(struct Character far *character, struct Effect far *effect, char removed);
                                                                 /* 1d99:10a3, vector to overlay 0 21e4:4fd1 */
extern char far dir_ok(char x, char y, char x2, char y2, unsigned char dir);
                                                                 /* 1d99:10ad, vector to overlay 2 2a89:048f */
extern unsigned char far spell_range(unsigned char spell);       /* 1d99:10c1, vector to overlay 10 2ffc:0f34 */
extern unsigned char far spell_menu(unsigned char list, unsigned char type, int far *choice, char far *more);
                                                                 /* 1d99:10cb, vector to overlay 10 2ffc:2080 */
extern char far line_clear(char x, char y, char far *tx, char far *ty, unsigned short far *cost);
                                                                 /* 1d99:10d5, vector to overlay 2 2a89:0328 */
extern void far cell_get(char x, char y, unsigned char far *who, unsigned char far *kind);
                                                                 /* 1d99:10df, vector to overlay 0 2740:0468 */
extern char far combat_terrain_under(char x, char y);            /* 1d99:10e9, vector to overlay 0 2740:137a */
extern unsigned char far wall_side(char x, char y, unsigned char dir);
                                                                 /* 1d99:10f3, vector to overlay 11 2f0d:0628 */
extern char far wall_passable(char x, char y, unsigned char dir);
                                                                 /* 1d99:10fd, vector to overlay 11 2f0d:04a4 */
extern void far item_equip_toggle(struct Item far *item);        /* 1d99:1107, vector to overlay 11 2b3a:1965 */
extern void far view_draw(char x, char y, unsigned char dir);    /* 1d99:1111, vector to overlay 11 2f0d:085a */
extern void far wall_sets_free(char reset);                         /* 1d99:1139, vector to overlay 11 2f0d:13ef */
extern void far back_free(void);                                 /* 1d99:1143, vector to overlay 11 2f0d:118a */
extern void far game_main(int argc, char far *far *argv);        /* 1d99:114d */
extern void far title_show(void);                                /* 1d99:1157, vector to overlay 4 264f:0345 */
extern void far game_reset(void);                                /* 1d99:1161, vector to overlay 4 25de:04ec */
extern void far view_attrs(char backdrop_attribute, char wall_attribute, char view_attribute_5, char view_attribute_6);      /* 1d99:116b, vector to overlay 11 2f0d:01de */
extern void far party_menu(char in_game);                             /* 1d99:1175, vector to overlay 7 289c:046f */
extern char far journal_check(void);                             /* 1d99:117f, vector to overlay 4 26f8:008b */
extern void far post_reset(void);                                /* 1d99:1189, vector to overlay 8 26b7:151b */
extern void far game_init(void);                                 /* 1d99:1193, vector to overlay 4 25de:0131 */
extern void far game_setup(void);                                /* 1d99:119d, vector to overlay 4 25de:0033 */
extern void far gb_write(unsigned short addr, unsigned short val, unsigned char kind);
                                                                 /* 1d99:11a7, vector to overlay 6 25a4:06d2 */
extern void far compare_int(unsigned short left_value, unsigned short right_value);
                                                                 /* 1d99:11b1, vector to overlay 6 25a4:1086 */
extern void far compare_str(char far *left_value, char far *right_value);           /* 1d99:11bb, vector to overlay 6 25a4:0fc6 */
extern void far ecl_gosub(unsigned char argument_slot);                      /* 1d99:11c5, vector to overlay 6 25a4:10ec */
extern unsigned far gb_read(unsigned short addr, unsigned char kind);
                                                                 /* 1d99:11cf, vector to overlay 6 25a4:0897 */
extern void far gb_fetch_str(unsigned char string_slot, unsigned short addr);
                                                                 /* 1d99:11d9, vector to overlay 6 25a4:0c18 */
extern void far gb_unpack(unsigned char len, unsigned char string_slot);   /* 1d99:11e3, vector to overlay 6 25a4:0ae2 */
extern void far ecl_skip(void);                                  /* 1d99:11ed, vector to overlay 6 25a4:1385 */
extern void far cursor_show(void);                               /* 1d99:11f7, vector to overlay 6 2999:0011 */
extern void far gb_write_str(unsigned short addr, char far *text);  /* 1d99:1201, vector to overlay 6 25a4:0a59 */
extern void far camp_menu(char far *interrupted);                         /* 1d99:120b, vector to overlay 7 25a4:2d3c */
extern void far step_forward(void);                              /* 1d99:1215, vector to overlay 6 25a4:1186 */
extern void far view_update(void);                               /* 1d99:121f, vector to overlay 6 25a4:1685 */
extern void far post_combat(void);                               /* 1d99:1229, vector to overlay 8 26b7:135c */
extern void far wander_load(void);                               /* 1d99:1233, vector to overlay 6 25a4:170e */
extern void far monsters_place(void);                            /* 1d99:123d, vector to overlay 6 25a4:2d82 */
extern unsigned char far monster_dist(char x, char y, unsigned char facing);
                                                                 /* 1d99:1247, vector to overlay 6 25a4:01c3 */
extern void far temple_shop(void);                               /* 1d99:1251, vector to overlay 8 2910:0af6 */
extern void far vault_menu(void);                                /* 1d99:125b, vector to overlay 8 280a:09a0 */
extern void far cmd_award_xp(void);                              /* 1d99:1265, vector to overlay 6 25a4:2719 */
extern void far cmd_destroy_items(void);                         /* 1d99:126f, vector to overlay 6 25a4:30ec */
extern void far cmd_shop(void);                                  /* 1d99:1279, vector to overlay 6 25a4:335e */
extern void far cmd_spell(void);                                 /* 1d99:1283, vector to overlay 6 25a4:260f */
extern void far cmd_who(void);                                   /* 1d99:128d, vector to overlay 6 25a4:2fe0 */
extern void far cmd_find_name(void);                             /* 1d99:1297, vector to overlay 6 25a4:31e8 */
extern void far cmd_add_npc(void);                               /* 1d99:12a1, vector to overlay 6 25a4:307b */
extern void far cmd_find_item(void);                             /* 1d99:12ab, vector to overlay 6 25a4:2f3b */
extern void far cmd_horiz_menu(char prompt);                          /* 1d99:12b5, vector to overlay 6 25a4:1ff7 */
extern void far cmd_damage(void);                                /* 1d99:12bf, vector to overlay 6 25a4:22ac */
extern void far cmd_rob(void);                                   /* 1d99:12c9, vector to overlay 6 25a4:28f8 */
extern void far cmd_treasure(void);                              /* 1d99:12d3, vector to overlay 6 25a4:2127 */
extern void far cmd_load_files(void);                            /* 1d99:12dd, vector to overlay 6 25a4:1dec */
extern void far cmd_new_ecl(void);                               /* 1d99:12e7, vector to overlay 6 25a4:1d8d */
extern void far cmd_journal(void);                               /* 1d99:12f1, vector to overlay 6 25a4:32b2 */
extern void far cmd_vert_menu(void);                             /* 1d99:12fb, vector to overlay 6 25a4:1e91 */
extern void far cmd_print(void);                                 /* 1d99:1305, vector to overlay 6 25a4:1cd3 */
extern void far cmd_input_string(void);                          /* 1d99:130f, vector to overlay 6 25a4:2ec8 */
extern void far cmd_input_number(void);                          /* 1d99:1319, vector to overlay 6 25a4:2e7c */
extern void far cmd_approach(void);                              /* 1d99:1323, vector to overlay 6 25a4:2e4f */
extern void far cmd_setup_monster(void);                         /* 1d99:132d, vector to overlay 6 25a4:2db1 */
extern void far cmd_load_monster(void);                          /* 1d99:1337, vector to overlay 6 25a4:29a3 */
extern void far fx_drain(struct Character far *character, unsigned char count);
                                                                 /* 1d99:1341, vector to overlay 9 2b3a:17dd */
extern void far ending_show(void);                               /* 1d99:134b, vector to overlay 4 264f:03cb */
extern unsigned char far menu_horiz(char far *title, char far *menu, char enter_ok, char anim);
                                                                 /* 1d99:1355, vector to overlay 6 25a4:0e3f */
extern void far move_resolve(void);                              /* 1d99:135f, vector to overlay 6 291e:0753 */
extern char far adv_menu(void);                                  /* 1d99:1369, vector to overlay 6 291e:0365 */
extern void far ecl_init(void);                                  /* 1d99:1373, vector to overlay 6 25a4:0001 */
extern void far ecl_load(unsigned char id);                       /* 1d99:137d, vector to overlay 6 25a4:0118 */
extern char far area_cell_at(char x, char y);                    /* 1d99:1387, vector to overlay 11 2f0d:075b */
extern int far party_scrolls(void);                              /* 1d99:1391, vector to overlay 11 2b3a:1f75 */
extern void far icon_default(void);                              /* 1d99:139b, vector to overlay 5 25a4:0001 */
extern unsigned char far hp_floor(struct Character far *character);   /* 1d99:13a5, vector to overlay 11 2b3a:354b */
extern void far recalc_from_levels(struct Character far *character);
                                                                 /* 1d99:13af, vector to overlay 11 304e:07f1 */
extern void far geo_load(char area_id);                                /* 1d99:13b9, vector to overlay 11 2f0d:12e3 */
extern int far pool_records(void);                               /* 1d99:13c3, vector to overlay 3 251a:00db */
extern void far pool_set_records(int record_count);                         /* 1d99:13cd, vector to overlay 3 251a:00d0 */
extern void far ticks_start(void);                               /* 1d99:13d7, vector to overlay 11 2b3a:34fc */
extern void far area_alloc(void);                                /* 1d99:13e1, vector to overlay 11 2f0d:01cc */
extern void far fx_install(void);                                /* 1d99:13eb, vector to overlay 9 2b3a:4fb2 */
extern void far spellfx_init(void);                              /* 1d99:13f5, vector to overlay 10 2b3a:4892 */
extern char far icon_customize(void);                            /* 1d99:13ff, vector to overlay 5 25a4:0cf1 */
extern void far hero_create(void);                               /* 1d99:1409, vector to overlay 5 25a4:2f95 */
extern void far stat_show(short stat, char active);              /* 1d99:1413, vector to overlay 11 2b3a:096c */
extern void far money_show(void);                                /* 1d99:141d, vector to overlay 11 2b3a:0590 */
extern void far combat_show(void);                               /* 1d99:1427, vector to overlay 11 2b3a:065e */
extern void far hp_adjust(struct Character far *character, char mask, unsigned char divisor, char apply);
                                                                 /* 1d99:143b, vector to overlay 11 2b3a:380e */
extern void far recalc_cleric_slots(struct Character far *character);    /* 1d99:1445, vector to overlay 11 304e:000f */
extern void far recalc_saves(struct Character far *character);           /* 1d99:144f, vector to overlay 11 304e:0c72 */
extern void far recalc_thief_skills(struct Character far *character);    /* 1d99:1459, vector to overlay 11 304e:0af3 */
extern void far char_status(void);                               /* 1d99:1463, vector to overlay 11 2b3a:0000 */
extern void far recalc_spells(struct Character far *character);       /* 1d99:1477, vector to overlay 11 304e:01e7 */
extern char far hp_bonus(struct Character far *character, short cls);    /* 1d99:1481, vector to overlay 11 2b3a:3a5b */
extern unsigned char far area_cell_flags(char x, char y);        /* 1d99:148b, vector to overlay 11 2f0d:0807 */
extern void far pick_hero(char far *prompt, char allow_cancel, char selection_mode, struct Character far * far *out);
                                                                 /* 1d99:1495, vector to overlay 11 2b3a:3ae5 */
extern void far shop_menu(void);                                 /* 1d99:149f, vector to overlay 8 280a:06f4 */
extern void far cast_menu(void);                                 /* 1d99:14a9, vector to overlay 7 25a4:0688 */
extern char far spell_list_build(unsigned char list);            /* 1d99:14b3, vector to overlay 10 2ffc:0b39 */
extern void far spell_remove_curse(void);                        /* 1d99:14bd, vector to overlay 10 2b3a:13ef */
extern void far weight_add(struct Character far *character, int weight);   /* 1d99:14c7, vector to overlay 11 2b3a:2799 */
extern void far weight_sub(struct Character far *character, int weight);   /* 1d99:14d1, vector to overlay 11 2b3a:2769 */
extern unsigned far carry_limit(struct Character far *character);        /* 1d99:14db, vector to overlay 11 2b3a:277c */
extern long far input_amount(long most, char far *prompt, char color);
                                                                 /* 1d99:14e5, vector to overlay 11 2b3a:2586 */
extern int far coin_kind(char far *text, char far *name);         /* 1d99:14ef, vector to overlay 11 2b3a:24d9 */
extern void far item_sell(struct Character far *hero, struct Item far *item);
                                                                 /* 1d99:14f9, vector to overlay 8 280a:0be4 */
extern void far item_identify(struct Character far *hero, struct Item far *item, char far *redraw);
                                                                 /* 1d99:1503, vector to overlay 8 280a:0e61 */
extern void far join_item(struct Item far *item);                /* 1d99:150d, vector to overlay 11 2b3a:1fda */
extern char far cant_carry(struct Character far *hero, char mode, struct Item far *item);
                                                                 /* 1d99:1517, vector to overlay 11 2b3a:2fbf */
extern void far drop(char in_view);                                    /* 1d99:1521, vector to overlay 11 2b3a:2c32 */
extern void far spell_path_targets(char x0, char y0, char x1, char y1, unsigned char width, unsigned char len);
                                                                 /* 1d99:152b, vector to overlay 10 2ffc:19ff */
extern void far bolt_run(unsigned range, int dmg, char bounce);
                                                                 /* 1d99:1535, vector to overlay 10 2b3a:1772 */
extern long far clock_now(void);                                 /* 1d99:153f */
extern void far display_init(int argc, char far * far *argv);    /* 1d99:1549, vector to overlay 4 2726:08d1 */
extern char far char_abs(char value);                                /* 0df9:0011 */
extern unsigned far umin(unsigned left_value, unsigned right_value);                /* 0df9:0034 */
extern void far int_to_str(int value, char far *buf);            /* 0df9:0057 */
extern char far *far substr(char far *text, int start, int len);    /* 0df9:0071 */
extern int far str_length(char far *text);                          /* 0df9:009e */
extern void far str_to_uint(char far *text, long far *out, int far *err); /* 0df9:00e6 */
extern void far str_to_long(char far *text, long far *out, int far *err); /* 0df9:0195 */
extern void far str_insert(char far *inserted_text, char far *destination, int pos);   /* 0df9:025d */
extern void far str_delete(char far *text, int pos, int delete_length);         /* 0df9:02bd */
extern void far pool_alloc(void far * far *out, int unused, struct Pool far *pool); /* 0df9:02fc */
extern void far pool_free(void far *ptr, int unused, struct Pool far *pool); /* 0df9:03c1 */
extern char far pool_lowbit(int value);                          /* 0df9:0428 */
extern void far mem_move5(void far *src, void far *dst, unsigned byte_count); /* 0df9:0432 */
extern int far random_n(int upper_bound);                                  /* 0df9:044d */
extern char far screen_sync(void);                               /* 0df9:045b */
extern unsigned char far hi_byte(int value);                         /* 0df9:0466 */
extern unsigned char far lo_byte(int value);                         /* 0df9:0474 */
extern void far msg_set_table(int count, char far *far *table);  /* 0df9:047e */
extern char far *far msg_text(int message_index);                            /* 0df9:0494 */
extern char far *far msg_format(char far *fmt, ...);             /* 0df9:04f3 */
extern void far delay(unsigned milliseconds);                              /* 0df9:052a */
extern int far sound_init(char far *music_drv, char far *voc_drv); /* 0e56:0006 */
extern void far sound_shutdown(void);                            /* 0e56:017d */
extern int far sound_play(char far *file, int index, int volume); /* 0e56:0265 */
extern int far sound_volume(char far *file, int index, int volume); /* 0e56:04cb */
extern int far sound_wait(char far *file, int index, char interruptible); /* 0e56:0620 */
extern int far sound_stop(char far *file, int index);            /* 0e56:07ab */
extern void far view_refresh(void);                              /* 1f4e:0000 */
extern void far party_rows(struct Character far *highlighted_character, char hilite);    /* 1f4e:0065 */
extern void far party_display(struct Character far *highlighted_character);          /* 1f4e:023b */
extern void far party_info_display(void);                        /* 1f4e:03f8 */
extern void far party_key_select(unsigned char key);             /* 1f4e:067c */
extern void far name_clean(char far *name);                      /* 1fc4:0044 */
extern void far set_data_disk(char disk);                        /* 1fc4:0088 */
extern void far data_load(char far *name, long unused, unsigned char entry, int far *len, char far *buf);
                                                                 /* 1fc4:0130 */
extern void far data_disk_pop(void);                           /* 1fc4:01ff */
extern void far mode_change(int mode);                           /* 1fc4:0200 */
extern int far sound_open(char far *digital);                        /* 0fd3:000b */
extern void far sound_close(void);                               /* 0fd3:0164 */
extern int far sound_event(int event_id);                               /* 0fd3:0176 */
extern int far spell_duration(unsigned char spell);              /* 2ffc:101b */
extern void far spell_damage_apply(unsigned char spell, char level, char on_remove, unsigned char dmg, int damage_element_mask, char far *msg);
                                                                 /* 2ffc:10f9 */
extern void far spell_effect_apply(char save_modifier, char far *msg);       /* 2ffc:1e0d */

/* data, by DGROUP offset */
extern long far_fallback;                                        /* a25c */
extern int sound_card;                                           /* acd8 */
extern int save_drive;                                           /* ace0 */
extern char save_path[];                                         /* ace2 */
extern struct Character far *cur_node;                           /* ad67 */
extern unsigned char next_monster_group;                         /* ad6b */
extern int node_count;                                           /* ad6c */
extern unsigned char ice_explosion_count;                        /* ad6e */
extern unsigned char meteor_explosion_count;                     /* ad6f */
extern struct Character far *ice_explosion_queue[];              /* ad70 */
extern struct Character far *meteor_explosion_queue[];           /* adc4 */
extern unsigned char panel_x1;                                   /* ae1a */
extern unsigned char panel_y1;                                   /* ae1b */
extern unsigned char panel_x2;                                   /* ae20 */
extern unsigned char panel_y2;                                   /* ae21 */
extern char tick_slot;                                           /* ae26 */
extern char redraw_wanted;                                       /* ae27 */
extern char debug_password[];                                          /* ae28 */
extern struct ItemInfo far *item_table;                          /* ae3e */
extern char view_kind;                                           /* ae42 */
extern char view_flag;                                           /* ae43 */
extern char turn_done;                                           /* ae48 */
extern char view_kind_saved;                                     /* ae4b */
extern unsigned char combat_round;                               /* ae4c */
extern unsigned char text_col;                                   /* ae4d */
extern unsigned char text_row;                                   /* ae4e */
extern long exp_share;                                           /* ae4f */
extern int damage;                                               /* ae55 */
extern int view_mode;                                            /* ae57 */
extern char demo_active;                                           /* ae59 */
extern char pooled;                                              /* ae5a */
extern char game_saved;                                          /* ae5b */
extern unsigned char far *ecl_memory;                            /* ae5c */
extern unsigned char move_rate;                                  /* ae61 */
extern char ecl_id;                                              /* ae63 */
extern char confirm_anim;                                        /* ae64 */
extern char text_slow;                                           /* ae65 */
extern char refused;                                             /* ae66 */
extern int saved_view_mode;                                      /* ae67 */
extern unsigned char prev_op;                                    /* ae69 */
extern unsigned char ecl_hi[];                                   /* ae6a */
extern char grid_x[];                                            /* aeac */
extern char grid_y[];                                            /* aee0 */
extern struct HotKey hotkeys[20];                                /* af15 */
extern char exploration_menu_context;                            /* af3d */
extern char same_team_attack_approved;                                           /* af3e */
extern char combat_won;                                          /* af3f */
extern char resting;                                             /* af40 */
extern unsigned ecl_after_camp;                                  /* af41 */
extern unsigned ecl_main_entry;                                  /* af43 */
extern void (far *spell_fn[])(void);                             /* af45 */
extern void (far *ice_storm_handler)(void);                      /* b0a1 */
extern struct Res far *icon_res;                                 /* b17d */
extern char effect_id;                                           /* b181 */
extern char save_succeeded;                                      /* b182 */
extern char training_done;                                       /* b183 */
extern char fx_lock;                                             /* b184 */
extern unsigned char ecl_lo[];                                   /* b185 */
extern unsigned char attack_rate;                                /* b1c6 */
extern unsigned char ecl_op;                                     /* b1c7 */
extern unsigned ecl_before_move;                                 /* b1c8 */
extern unsigned char encounter_picture;                          /* b1ca */
extern char magic_on;                                            /* b1cb */
extern unsigned rest_time[7];                                    /* b1cc */
extern unsigned char down_count;                                 /* b1da */
extern long pool[3];                                             /* b1db */
extern struct Item far *pool_items;                              /* b1e7 */
extern int menu_top_entry; /* b1eb, DKK menuVerticalTop; only set here */
extern char game_loaded;                                         /* b1ed */
extern char anim_state[];                                          /* b1f0 */
extern char byte_b20b;                                           /* b20b */
extern char anim_first;                                          /* b20c */
extern unsigned char anim_last;                                  /* b20d */
extern unsigned char anim_frame;                                 /* b20e */
extern unsigned char far *game_state;                            /* b20f */
extern unsigned char ecl_type[];                                 /* b215 */
extern char picture_hides_roster;                                /* b256 */
extern char death_explosion_active;                              /* b257 */
extern struct Pool dropped_item_pool;                                    /* b258 */
extern struct Slot portrait_slots[]; /* b295, Champions portraits[1..2], the head/body cache; no reader here */
extern unsigned ecl_before_camp;                                 /* b298 */
extern struct Pool party_pool;                                    /* b2a4 */
extern unsigned char save_bonus;                                 /* b2e7 */
extern unsigned ecl_after_move;                                  /* b2e8 */
extern unsigned char current_spell;                              /* b2ea */
extern int word_b2eb;                                            /* b2eb */
extern unsigned char attacks_used[];                             /* b2ef */
extern char ecl_conditions[4];                                   /* b2f5 */
extern char script_text_cleared;                                 /* b2fb */
extern struct Character far *heroes[];                           /* b2fc */
extern struct EclFrame far *ecl_stack;                           /* b3cc */
extern struct Item far *item_cur;                                /* b3d0 */
extern struct Pool cloud_pool;                                    /* b3d4 */
extern char picker_direct_select;                                /* b414 */
extern struct Drop far *drops;                                   /* b415 */
extern char encounter_sprite_shown;                              /* b419 */
extern struct Pool ecl_frame_pool;                                    /* b41a */
extern unsigned char combat_round_limit;                         /* b45a */
extern struct Character far *taunting_character;                 /* b45b */
extern char ecl_str[][40];                                       /* b45f */
extern char adventure_exit;                                      /* b7a7 */
extern struct Res far *big_res;                                  /* b7a8 */
extern int file_kind;                                            /* b7ac */
extern struct Item far *cur_item;                                /* b7ae */
extern struct Pool npc_pool;                                    /* b7b2 */
extern unsigned char side_count[];                               /* b7f2 */
extern unsigned char damage_dice_count;                                 /* b7f4 */
extern char movement_stops_attack_rate;                          /* b7f5 */
extern char monster_unpack_in_progress;                                           /* b7f6 */
extern char script_view_dirty;                                   /* b7f8 */
extern unsigned ecl_ip;                                          /* b7f9 */
extern char no_study;                                            /* b7fb */
extern char item_cast_active;                                    /* b7fc */
extern char occupant_grid[25][50];                                   /* b7fd */
extern char hits_left[];                                         /* bcdf */
extern unsigned heal_ticks;                                      /* bce1 */
extern struct Character far *fx_target;                          /* bce3 */
extern char year_rolled;                                         /* bce7 */
extern unsigned char near_ids[];                                 /* bce8 */
extern char can_reach;                                           /* bd1c */
extern char byte_bd1d;                                           /* bd1d */
extern int word_bd1e;                                            /* bd1e */
extern struct Res far *back_res;                                 /* bd24 */
extern char view_cx;                                             /* bd2c */
extern unsigned char order_idx;                                  /* bd2d */
extern char combat_action_done;                                  /* bd2e */
extern char saved_search_flag;                                   /* bd2f */
extern int frame_col;                                            /* bd30 */
extern int frame_row;                                            /* bd32 */
extern char pic_name[];                                          /* bd34 */
extern char view_cy;                                             /* bd49 */
extern char ecl_initial_redraw;                                  /* bd4b */
extern unsigned char death_explosion_count;                      /* bd4c */
extern char pictures_enabled; /* bd4d, DKK picturesEnabled or animationsEnabled (one byte for the pair); no reader here */
extern char scripted_monsters_queued;                            /* bd4e */
extern char party_click_enabled; /* bd50, DKK mouseMode, Champions partyPickEnabled; no reader here */
extern struct Pool effect_pool;                                    /* bd51 */
extern char item_handler_active;                                 /* bd91 */
extern char free_train;                                          /* bd92 */
extern char skip_startup_party_menu;                             /* bd93 */
extern long long_bd94;                                           /* bd94 */
extern struct Character far *combat_order[0x49];                 /* bd99 */
extern int element_mask;                                         /* bebd */
extern char copyprot_markers[];                                  /* bebf */
extern char attack_roll;                                         /* bec5 */
extern char pic_saved[];                                         /* bec6 */
extern struct Map far *cur_map;                                  /* bedb */
extern char script_box_clear_marker;                                           /* bee0 */
extern unsigned char encounter_sprite;                           /* bee8 */
extern char shop_temple_frame;                                   /* bee9 */
extern char noncombat_redraw_pending;                            /* beea */
extern char disk_number; /* beeb, DKK/Champions diskNumber; no reader here */
extern char mirror_image_bypass;                                 /* c280 */
extern unsigned char bonus_slots;                                /* c281  odd; c280 is used elsewhere */
extern struct Character far *death_explosion_queue[];            /* c282 */
extern struct Cloud far *cloud_list;                             /* c2d6 */
extern char previous_nonrose_knight;                             /* c2da */
extern char character_select_failed;                             /* c2db */
extern char adventure_restart;                                   /* c2dc */
extern unsigned char step_count;                                 /* c2dd */
extern unsigned char morale;                                     /* c2de */
extern char encounter_menu_active; /* c2e1, DKK inEncounterMenu; only reset here */
extern char treasure_has_items;                                  /* c2e2 */
extern struct Item far *dropped_item_boundary;                   /* c2e3 */
extern unsigned char encounter_mode;                                  /* c2e7 */
extern unsigned encounter_ticks;                                 /* c2e8 */
extern int current_save_type;                                    /* c2eb */
extern unsigned char ai_level;                                   /* c2ed */
extern struct Pic pic;                                           /* c2ee */
extern unsigned char anim_speed;                                 /* c2ef */
extern char pick_special;                                        /* c2f4 */
extern char menu_escaped;                                        /* c2f5 */
extern struct Step steps[];                                      /* c2f6 */
extern unsigned char pic_disk;                                   /* c392 */
extern struct Res far *wall_set_res[3];                              /* c393 */
extern char inventory_redraw;                                    /* c3a3 */
extern char flag_c3a4;                                           /* c3a4 */
extern struct Character far *picker_start_character;             /* c3a5 */
extern struct Pool item_record_pool;                                    /* c3a9 */
extern char has_coins;                                           /* c3e9 */
extern char vault_slot;                                          /* c3ea */
extern struct Character far *selected_target;                    /* c3eb */
extern unsigned char combatant_count;                            /* c3ef */
extern char aim_cursor_active; /* c3f0, DKK cursorActive; only reset here */
extern char move_menu_active; /* c3f1, DKK inMoveMenu; only reset here */
extern char anim_off;                                            /* c3f4 */
extern char move_flag;                                           /* c3f5 */
extern char wall_color_select; /* c3f6, DKK CGA view colour 0xdb; only set here */
extern struct Combatant combatants[52];                          /* c3f7 */
extern struct Rec recs[8];                                       /* c52f */
extern struct Pool far *node_pool;                               /* c578 */
extern int (far *target_scores)[37];                             /* c57c */
extern struct Pool combat_pool;                                  /* c58d */
extern long menu_timeout;                                        /* c5ce */
extern char timeout_key;                                         /* c5d2 */
extern unsigned char rec_count;                                  /* c5d3 */
extern struct Res far *pic_res;                                  /* c5d4 */
extern char print_return_flag;                                           /* c5d8 */
extern char view_update_pending;                                 /* c5d9 */
extern unsigned char study_wait[];                               /* c5da */
extern struct Character far *targets[];                          /* c5df */
extern struct Character far *spell_target;                       /* c5e3 */
extern char panel_dirty;                                         /* c6b7 */
extern unsigned char morale_base;                                /* c6b8 */
extern char crit_hit;                                            /* c6b9 */
extern char arr_c6ba[];                                          /* c6ba */
extern struct ItemRec far *item_templates;                       /* c6de */
extern char order_mode;                                          /* c6e2 */
extern char horizon_color_select; /* c6e3, DKK CGA horizon colour 0xe8; only set here */
extern char ui_mode;                                             /* c6e4 */
extern struct Character far *node_list;                          /* c6e5 */
extern struct Character far *previous_highlighted_character;     /* c6e9 */
extern void (far *hit_proc)(unsigned char spell, char mode, char far *result); /* c6ed */
extern unsigned char encounter_npc_count;                        /* c6f1 */
extern char ice_explosion_active;                                /* c6f2 */
extern char meteor_explosion_active;                             /* c6f3 */
extern unsigned char target_count;                               /* c6f4 */
extern char training_selection_changed;                                           /* c6f5 */
extern char encounter_images_shown[];                            /* c6f6 */
extern long (far *sig_convert[3])(int slot, long off, long len); /* dab0 */
extern char far *bss_end[];                                      /* db60  the end of the zeroed region     */

/* chartab.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_CHARTAB before including this file. */
#ifndef DQK_DEFINES_CHARTAB
extern long high_level_xp_step[11];                                       /* 028c */
extern struct RaceLimits race_limits[7];                         /* 02b8 */
extern struct RaceClasses race_classes[7];                       /* 0328 */
extern struct AgeRoll start_ages[7][7];                          /* 03f3 */
extern struct AgeLimits age_limits[7];                           /* 04b7 */
extern struct ClassMins class_mins[17];                          /* 04fd */
extern struct ClassAligns class_aligns[34];                      /* 0563 */
extern unsigned char constitution_hp_bonus[26];                  /* 082d  hit point bonus by CON    */
extern unsigned char class_spell_rows[11];                       /* 0847  a spell_slots_by_level row, 0xff none */
extern long xp_by_level[11][19];                                     /* 0852 */
extern unsigned char spell_slots_by_level[8][29][9];             /* 0b96 */
extern unsigned char saving_throws[7][22][5];                    /* 13cc */
extern unsigned char spells_by_level[3][9][10];                  /* 16ce  spells a slot level knows */
extern unsigned char max_level[7][7];                            /* 17dc */
extern int start_items[7][17][5];                                /* 180d */
#endif

/* combtab.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_COMBTAB before including this file. */
#ifndef DQK_DEFINES_COMBTAB
extern char dir_dx[9];                                           /* 6189 */
extern char dir_dy[9];                                           /* 6192 */
extern struct TileInfo tile_info[76];                            /* 619b */
extern struct IconCell icon_cells[6][6];                         /* 62cb */
extern char helpless_kinds[4];                                   /* 6313 */
extern unsigned char disease_fx[7];                              /* 6317  [1..7] are read here */
extern unsigned char fx_clear[18];                               /* 631e */
extern unsigned char patch_dir[10];                              /* 6341 */
extern unsigned time_units[7];                                   /* 634b */
extern unsigned char attack_scores_by_level[7][22];              /* 6359 */
extern unsigned char class_mask_bits[7];                         /* 63f3 */
extern unsigned char class_bit[7];                               /* 63fa */
extern unsigned char hit_dice_level_limit[7];                    /* 640a */
extern unsigned char class_hit_dice[7];                          /* 6411 */
extern unsigned char hit_die_sides[7];                           /* 6418 */
extern unsigned char thief_skill_base[18][8];                    /* 6425 */
extern char thief_race_adjust[7][8];                             /* 64b5 */
extern char thief_dex_adjust[11][5];                             /* 64ed */
extern char far *fx_names[230];                                  /* 6524 */
extern unsigned char bit_mask[8];                                /* 68bc */
#endif

/* dialog.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_DIALOG before including this file. */
#ifndef DQK_DEFINES_DIALOG
extern struct Wid far *dl_items;                                 /* dac7 */
#endif

/* dirnames.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_DIRNAMES before including this file. */
#ifndef DQK_DEFINES_DIRNAMES
extern char dir_names[8][3];                                     /* 6158 */
#endif

/* dispinit.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_DISPINIT before including this file. */
#ifndef DQK_DEFINES_DISPINIT
extern char ega_pals[2][17];                                     /* 9288, 2726's _DATA */
#endif

/* eclrun.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_ECLRUN before including this file. */
#ifndef DQK_DEFINES_ECLRUN
extern char ecl_done;                                            /* acbe */
extern char ecl_new;                                             /* acbf */
#endif

/* gametext.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_GAMETEXT before including this file. */
#ifndef DQK_DEFINES_GAMETEXT
extern char far *game_text[6];                                   /* 3aca */
extern char far *special_label;                                  /* 3ae2 */
extern char far *bonus_label;                                    /* 3ae6 */
extern char far *wants_text;                                     /* 3aea */
extern char far *continue_items;                                 /* 3aee */
extern char far *press_text;                                     /* 3af2 */
extern char far *type_names[18];                                 /* 3af6 */
extern char far *class_names[18];                                /* 3b3e */
extern char far *race_names[8];                                  /* 3b86 */
extern char far *align_names[9];                                 /* 3ba6 */
extern char far *sex_names[2];                                   /* 3bca */
extern char far *coin_names[3];                                  /* 3bd2 */
extern char far *status_names[11];                               /* 3bde */
extern char far *deity_names[8];                                 /* 3c0a */
extern char far *robe_names[5];                                  /* 3c2a */
extern char party_items[12];                                     /* 3c3e */
extern char far *txt_lose_it;                                    /* 3c4a */
extern char far *txt_cant_cast_here;                             /* 3c4e */
extern char far *txt_that_item;                                  /* 3c52 */
extern char far *txt_combat_only;                                /* 3c56 */
extern char far *txt_use_it;                                     /* 3c5a */
extern char far *txt_casts;                                      /* 3c5e */
extern char far *txt_abort_spell;                                /* 3c62 */
extern char far *txt_spell_aborted;                              /* 3c66 */
extern char far *how_many_text;                                  /* 3c6a */
extern char far *take_text;                                      /* 3c6e */
extern char far *how_much_text;                                  /* 3c72 */
extern char far *gems_text;                                      /* 3c76 */
extern char far *jewelry_text;                                   /* 3c7a */
extern char far *collection_text;                                /* 3c7e */
extern char far *gem_value_text;                                 /* 3c82 */
extern char far *jewel_value_text;                               /* 3c86 */
extern char far *coin_title;                                     /* 3c8a */
extern char far *trade_to_text;                                  /* 3c8e */
extern char far *trade_amount_text;                              /* 3c92 */
extern char far *items_label;                                    /* 3c9e */
extern char far *ready_label;                                    /* 3ca2 */
extern char far *your_text;                                      /* 3ca6 */
extern char far *gone_text;                                      /* 3caa */
extern char far *must_ready_text;                                /* 3cae */
extern char far *exit_text;                                      /* 3cd2 */
extern char far *exit_unsaved_text;                              /* 3cd6 */
extern char far *deity_title;                                    /* 3ce6 */
extern char far *cant_modify_text;                               /* 3d12 */
extern char far *add_where_text;                                 /* 3d16 */
extern char far *add_text;                                       /* 3d1a */
extern char far *delete_text;                                    /* 3d1e */
extern char far *petition_text;                                  /* 3d4a */
extern char far *sword_text;                                     /* 3d4e */
extern char far *rose_text;                                      /* 3d52 */
extern char far *knight_text;                                    /* 3d56 */
extern char far *inexperienced_text;                             /* 3d5a */
extern char far *unqualified_text;                               /* 3d5e */
extern char far *may_become_text;                                /* 3d62 */
extern char far *selected_text;                                  /* 3d82 */
extern char far *speed_text;                                     /* 3d86 */
extern char far *camp_text;                                      /* 3d8a */
extern char far *not_here_text;                                  /* 3d8e, "not here"               */
extern char far *ally_prompt;                                    /* 3d92 */
extern char far *range_text;                                     /* 3d96 */
extern char far *magic_on_text;                                  /* 3d9a */
extern char far *magic_off_text;                                 /* 3d9e */
extern char far *flee_text;                                      /* 3da2 */
extern char far *cant_go_text;                                   /* 3da6 */
extern char far *not_enough_text;                                /* 3db2 */
extern char far *leave_text1;                                    /* 3db6 */
extern char far *leave_text2;                                    /* 3dba */
extern char far *fled_text;                                      /* 3dbe */
extern char far *lost_text;                                      /* 3dc2 */
extern char far *treasure_text;                                  /* 3dd2 */
extern char far *no_treasure_text;                               /* 3dd6 */
extern char far *receives_text;                                  /* 3dda */
extern char far *xp_text;                                        /* 3dde */
extern char far *left_text;                                      /* 3de2 */
extern char far *claim_text;                                     /* 3de6 */
extern char far *cure_anyway_text;                               /* 3dea */
extern char far *not_blind_text;                                 /* 3df6 */
extern char far *cannot_raise_text;                              /* 3dfe */
extern char far *beyond_help_text;                               /* 3e02 */
extern char far *not_dead_text;                                  /* 3e06 */
extern char far *not_cursed_text;                                /* 3e0e */
extern char far *not_stoned_text;                                /* 3e12 */
extern char far *no_lost_text;                                   /* 3e16 */
extern char far *help_you_text;                                  /* 3e1a */
extern char far *empty_text;                                     /* 3e1e */
extern char far *title_text;                                     /* 3e22, "The Dark Queen of Krynn" */
extern char far *version_text;                                   /* 3e26, "Version 1.1 ..."        */
extern char far *txt_choose_spell;                               /* 3e2a */
extern char far *txt_flame_type;                                 /* 3e2e */
extern char far *appraise_title;                                 /* 3e32 */
extern char far *pick_text;                                      /* 3e36 */
extern char far *load_where_text;                                /* 3e46 */
extern char far *load_which_text;                                /* 3e4a */
extern char far *save_which_text;                                /* 3e4e */
extern char far *txt_spells_ok;                                  /* 3e52 */
extern char far *txt_scribe_ok;                                  /* 3e56 */
extern char far *order_title;                                    /* 3e5a */
extern char far *speed_title;                                    /* 3e5e */
extern char far *blank_text;                                     /* 3e62 */
extern char far *continue_text;                                  /* 3e6a */
extern char far *items_title;                                    /* 3e72 */
extern char far *main_menu_text;                                 /* 3e7e, "Play Demo Quit"         */
extern char far *confirm_items;                                  /* 3e82 */
extern char far *vmenu_text;                                     /* 3e86 */
extern char far *select_items;                                   /* 3e8a */
extern char far *txt_cast_exit;                                  /* 3e8e */
extern char far *txt_memorize_exit;                              /* 3e92 */
extern char far *txt_keep_exit;                                  /* 3e96 */
extern char far *txt_scribe_exit;                                /* 3e9a */
extern char far *txt_learn_exit;                                 /* 3e9e */
extern char far *txt_hot_cold;                                   /* 3ea2 */
extern char far *appraise_items;                                 /* 3ea6 */
extern char far *sell_items;                                     /* 3eaa */
extern char far *rest_items;                                     /* 3eae */
extern char far *view_items;                                     /* 3eb2 */
extern char far *items_items;                                    /* 3eb6 */
extern char far *source_items;                                   /* 3ec2 */
extern char far *add_items;                                      /* 3ec6 */
extern char far *delete_items;                                   /* 3eca */
extern char far *change_items;                                   /* 3ece */
extern char far *game_items;                                     /* 3ed6 */
extern char far *order_items;                                    /* 3eda */
extern char far *speed_items;                                    /* 3ede */
extern char far *explore_items;                                  /* 3ee2, "Move Area Cast ..."     */
extern char far *exit_items;                                     /* 3ee6 */
extern char far *command_items;                                  /* 3eea */
extern char far *move_items;                                     /* 3eee */
extern char far *aim_items;                                      /* 3ef2 */
extern char far *target_items;                                   /* 3ef6 */
extern char far *buy_items;                                      /* 3efa */
extern char spaces[35];                                          /* 3f02  34 spaces            */
extern char far *take_items;                                     /* 3f25 */
extern char far *treasure_items;                                 /* 3f2d */
extern char far *heal_items;                                     /* 3f31 */
extern char far *temple_items;                                   /* 3f35 */
extern char far *vault_items;                                    /* 3f39 */
extern char far *camp_menus[4];                                  /* 3f41  camp, magic, alter, level */
extern char far *item_words[122];                                /* 3f55 */
extern char far *gem_text;                                       /* 413d */
extern char far *old_text;                                       /* 41c1 */
extern char far *strain_text;                                    /* 41c5 */
extern char far *breath_text;                                    /* 41c9 */
extern char far *he_text;                                        /* 41cd */
extern char far *she_text;                                       /* 41d1 */
extern char far *age_death_text;                                 /* 41d5 */
extern char far *no_witness_text;                                /* 41d9 */
extern char far *one_witness_text;                               /* 41dd */
extern char far *many_witness_text;                              /* 41e1 */
extern char far *search_body_text;                               /* 41e5 */
#endif

/* library.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_LIBRARY before including this file. */
#ifndef DQK_DEFINES_LIBRARY
extern long sig_tags[4];                                         /* da9a */
extern int sig_count;                                            /* daaa */
#endif

/* lowio.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_LOWIO before including this file. */
#ifndef DQK_DEFINES_LOWIO
extern int clip_top;                                             /* cdae */
extern int clip_left;                                            /* cdb3 */
extern int clip_right;                                           /* cdb5 */
extern int clip_bottom;                                          /* cdb9 */
#endif

/* pathfind.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_PATHFIND before including this file. */
#ifndef DQK_DEFINES_PATHFIND
extern char path_sep;                                            /* a248 */
#endif

/* prompt.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_PROMPT before including this file. */
#ifndef DQK_DEFINES_PROMPT
extern int win_top;                                              /* 91ba */
extern int prompt_icon;                                          /* 91bc */
extern int win_row[5];                                           /* 91be */
extern int win_col[5];                                           /* 91c8 */
extern int win_attr[5];                                          /* 91d2 */
extern int prompt_frame;                                         /* ca1a */
#endif

/* spelltab.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_SPELLTAB before including this file. */
#ifndef DQK_DEFINES_SPELLTAB
extern char scroll_spell_kinds[140];                             /* 253f */
extern char far *spell_names[141];                               /* 25cb */
extern struct SpellInfo spell_info[141];                         /* 27ff */
#endif

/* startdat.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_STARTDAT before including this file. */
#ifndef DQK_DEFINES_STARTDAT
extern unsigned _psp;                                            /* 0020 */
extern char far *far *heap_brk;                                  /* 0022 */
#endif

/* vidmouse.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_VIDMOUSE before including this file. */
#ifndef DQK_DEFINES_VIDMOUSE
extern unsigned char font_cells[0x480];                          /* 93de */
extern unsigned char far *font_ext;                              /* 985e */
extern char dos_mode;                                            /* ca32 */
extern unsigned draw_seg;                                        /* ca33 */
extern int video_mode;                                           /* ca41 */
extern unsigned video_len;                                       /* ca43 */
extern unsigned display_seg;                                     /* cb45 */
extern unsigned mem_top_seg;                                     /* cb47 */
extern char visible_on;                                          /* cb4c */
extern char back_screen;                                         /* cb5f */
extern int pix_shifts;                                           /* cb63 */
extern int pixel_cols;                                           /* cb65 */
extern int text_bytes;                                           /* cb6c */
extern int shape_boot;                                           /* cb6e */
extern int letter_step;                                          /* cb7a */
extern int base_offset;                                          /* cb80 */
extern char saved_equip;                                         /* cb82 */
extern int extent_text;                                          /* cb83 */
extern unsigned screen_addr;                                     /* cb85 */
extern void far *spare_buffer;                                   /* cd98 */
extern int screen_lines;                                         /* cd9c */
extern int planes_count;                                         /* cd9e */
extern int txthigh_dots;                                         /* cda0 */
#endif

/* view3d.c defines these: an extern ahead of its own definitions would reorder its own
 * data, so it defines DQK_DEFINES_VIEW3D before including this file. */
#ifndef DQK_DEFINES_VIEW3D
extern struct Geo geo;                                           /* c9f0 */
#endif

/* --- end of shared declarations --- */
