/* DQK.EXE data-only module between charpic.c and misc.c: the game's text, DGROUP
 * 3aca..58c0 -- the menus, prompts, messages and name lists the modules print, each a
 * far pointer (or a table of them) to a string literal; the literals follow,
 * DGROUP 41e9..58c0, as Turbo C emits them, -d sharing equal ones. Unmeasured table
 * spans follow the next declared name; measured temple/menu bounds are recorded in
 * v11/analysis/data-tables/MENUS.md. No routines; its empty _TEXT sits at misc.c's start.
 * Drafted by
 * gametools/reconcile/datagen.py.
 */

#define DQK_DEFINES_GAMETEXT
#include "dqk.h"

char far *game_text[6] = {
    "is disrupted",
    "Affected",
    "emits an evil stench",
    "goes berserk",
    "is healed",
    "is stunned"
};

char far *special_label = "   Special :";

char far *bonus_label = "     Bonus :";

char far *wants_text = "wants to kill ";

char far *continue_items = "Press button or enter to continue";

char far *press_text = "~Press button or enter to continue";

char far *type_names[18] = {
    "Cleric",
    "Knight",
    "Fighter",
    "Paladin",
    "Ranger",
    "Mage",
    "Thief",
    "Monk",
    "Cleric/Fighter",
    "Cleric/Fighter/Mage",
    "Cleric/Ranger",
    "Cleric/Mage",
    "Cleric/Thief",
    "Fighter/Mage",
    "Fighter/Thief",
    "Fighter/Mage/Thief",
    "Mage/Thief",
    "Illegal type"
};

char far *class_names[18] = {
    "Cleric",
    "Knight",
    "Fighter",
    "Paladin",
    "Ranger",
    "Magic-User",
    "Thief",
    "Monk",
    "Cleric/Fighter",
    "Cleric/Fighter/Magic-User",
    "Cleric/Ranger",
    "Cleric/Magic-User",
    "Cleric/Thief",
    "Fighter/Magic-User",
    "Fighter/Thief",
    "Fighter/Magic-User/Thief",
    "Magic-User/Thief",
    "Illegal type"
};

char far *race_names[8] = {
    "Silvanesti Elf",
    "Qualinesti Elf",
    "Half-Elf",
    "Mountain Dwarf",
    "Hill Dwarf",
    "Kender",
    "Human",
    "Monster"
};

char far *align_names[9] = {
    "Lawful Good",
    "Lawful Neutral",
    "Lawful Evil",
    "Neutral Good",
    "True Neutral",
    "Neutral Evil",
    "Chaotic Good",
    "Chaotic Neutral",
    "Chaotic Evil"
};

char far *sex_names[2] = {
    "Male",
    "Female"
};

char far *coin_names[3] = {
    "Steel",
    "Gems",
    "Jewelry"
};

char far *status_names[11] = {
    "Okay",
    "Animated",
    "tempgone",
    "Running",
    "Unconscious",
    "Dying",
    "Dead",
    "Petrified",
    "Gone",
    "",
    "Explode"
};

char far *deity_names[8] = {
    "None",
    "Paladine",
    "Majere",
    "Kiri-Jolith",
    "Mishakal",
    "Sirrion",
    "Reorx",
    "Shinare"
};

char far *robe_names[5] = {
    "",
    "White",
    "Red",
    "Black",
    "gains "
};

char party_items[12] = {
    1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1
};

char far *txt_lose_it = "Lose it? ";

char far *txt_cant_cast_here = "Can't be cast here...";

char far *txt_that_item = "That item";

char far *txt_combat_only = "Is a combat-only item...";

char far *txt_use_it = "Use it? ";

char far *txt_casts = "casts";

char far *txt_abort_spell = "Abort Spell";

char far *txt_spell_aborted = "Spell Aborted";

char far *how_many_text = "How many ";

char far *take_text = "will you take? ";

char far *how_much_text = "How much ";

char far *gems_text = "gems : ";

char far *jewelry_text = "jewelry : ";

char far *collection_text = "You have a fine collection of:";

char far *gem_value_text = "The gem is valued at ";

char far *jewel_value_text = "The jewelry is valued at ";

char far *coin_title = "select type of coin ";

char far *trade_to_text = "trade to?";

char far *trade_amount_text = "will you trade? ";
char far *stop_resting_text = "Stop resting? ";
char far *scribe_scroll_warning = " was going to scribe from that scroll";

char far *items_label = "Items";

char far *ready_label = "Ready Item";

char far *your_text = "Your ";

char far *gone_text = "will be gone forever";

char far *must_ready_text = "Must be readied";
char far *sale_offer_text = "I'll give you ";
char far *sale_deal_text = "is it a deal? ";
char far *sold_text = "sold!";
char far *overloaded_pool_text = "overloaded. money will be put in pool.";
char far *identify_nothing_new_text = "i cannot tell anything new about your ";
char far *identify_kind_text = "it looks like some sort of ";
char far *drop_amount_text = "will you drop? ";
char far *deposit_amount_text = "will you deposit? ";

char far *exit_text = "Exit to DOS? ";

char far *exit_unsaved_text = "Game not saved. Exit anyway? ";
char far *race_title = "Pick Race";
char far *gender_title = "Pick Gender";
char far *class_title = "Pick Class";

char far *deity_title = "Pick Diety";
char far *alignment_title = "Pick Alignment";
char far *reroll_text = "reroll stats? ";
char far *character_name_text = "character name: ";
char far *save_character_text = "save character? ";
char far *drop_text = "drop ";
char far *forever_text = " forever ";
char far *are_you_sure_text = "Are you sure? ";
char far *dumped_text = " is dumped out back.";
char far *farewell_text = " bids you farewell.";
char far *relief_text = " breathe a sigh of relief.";

char far *cant_modify_text = " can't be modified.";

char far *add_where_text = "add from where? ";

char far *add_text = "add a character: ";

char far *delete_text = "delete a character: ";
char far *cga_unavailable_text = "sorry not in CGA";
char far *icon_action_text = "ready   action";
char far *new_icon_text = "new";
char far *old_icon_text = "old";
char far *icon_confirm_text = "is this icon ok? ";
char far *insufficient_xp_text = "not enough experience";
char far *will_become_text = " will become:";
char far *first_level_text = "    a level ";
char far *another_level_text = "and a level ";
char far *train_confirm_text = "do you wish to train? ";

char far *petition_text = "is petitioning to become a";

char far *sword_text = "sword";

char far *rose_text = "rose";

char far *knight_text = "knight of the ";

char far *inexperienced_text = " is too inexperienced";

char far *unqualified_text = " does not qualify.";

char far *may_become_text = " may become a";
char far *save_disk_text_1 = "is your save disk in drive ";
char far *save_disk_text_2 = "is your save disk in drive ";
char far *drive_question_text = ":? ";
char far *disk_full_text = "Can't save. Disk is full.";
char far *overwrite_text = "overwrite ";
char far *filename_question_text = "? ";
char far *new_filename_text = "new file name : ";

char far *selected_text = "has been selected";

char far *speed_text = "Game Speed (0=fastest 9=slowest) = ";

char far *camp_text = "the party makes camp...";

char far *not_here_text = "not here";

char far *ally_prompt = "attack ally? ";

char far *range_text = "Aim: range = ";

char far *magic_on_text = "magic on";

char far *magic_off_text = "magic off";

char far *flee_text = "flee ? ";

char far *cant_go_text = "can't go there";
char far *buys_text = " buys ";
char far *buys_one_text = " buys a ";

char far *not_enough_text = "not enough money.";

char far *leave_text1 = "as you leave, the shopkeeper says, 'excuse me but you have left some money here.' ";

char far *leave_text2 = "do you want to go back and get your money?";

char far *fled_text = "the party has fled.";

char far *lost_text = "you have lost the fight.";
char far *won_text = "the party has won.";
char far *withdrawal_text = "the party makes a withdrawl. a small";
char far *bank_fee_text = "fee has been assessed by the bank.";

char far *treasure_text = "the party has found treasure";

char far *no_treasure_text = "the party has found no treasure";

char far *receives_text = "each character receives ";

char far *xp_text = "experience points.";

char far *left_text = "there is still treasure left. ";

char far *claim_text = "do you want to go back and claim it?";

char far *cure_anyway_text = "cast cure anyway? ";
char far *pay_cure_text = "pay for cure? ";
char far *cured_text = "is cured.";

char far *not_blind_text = "is not blind.";
char far *not_diseased_text = "is not diseased.";

char far *cannot_raise_text = "cannot be raised.";

char far *beyond_help_text = "is beyond our help";

char far *not_dead_text = "is not dead.";
char far *not_poisoned_text = "is not poisoned.";

char far *not_cursed_text = "is not cursed.";

char far *not_stoned_text = "is not stoned.";

char far *no_lost_text = "has no lost levels.";

char far *help_you_text = ", how can we help you?";

char far *empty_text = "";

char far *title_text = "The Dark Queen of Krynn";

char far *version_text = "Version 1.1        July 29,1992";

char far *txt_choose_spell = "Choose Spell:";

char far *txt_flame_type = "Flame type: ";

char far *appraise_title = "Appraise : ";

char far *pick_text = "Pick Character ";
char far *modify_title = "Modify : ";
char far *init_pointer_text = "Init mouse/joystick: ";
char far *lose_character_text = "lose character? ";

char far *load_where_text = "load from where? ";

char far *load_which_text = "load which game: ";

char far *save_which_text = "save which game: ";

char far *txt_spells_ok = "spells OK? ";

char far *txt_scribe_ok = "scribe these spells? ";

char far *order_title = "party order ";

char far *speed_title = "game speed: ";

char far *blank_text = "";
char far *cursor_keys_text = "(use cursor keys) ";

char far *continue_text = "continue battle? ";
char far *movement_left_text = "move/attack, move left = ";

char far *items_title = "items : ";
char far *take_title = "take: ";
char far *take_blank_text = "";

char far *main_menu_text = "Play Demo Quit";

char far *confirm_items = "Yes No";

char far *vmenu_text = "Select";

char far *select_items = "Select Exit";

char far *txt_cast_exit = "Cast Exit";

char far *txt_memorize_exit = "Memorize Exit";

char far *txt_keep_exit = "Keep Exit";

char far *txt_scribe_exit = "Scribe Exit";

char far *txt_learn_exit = "Learn Exit";

char far *txt_hot_cold = "Hot Cold";

char far *appraise_items = "Gems Jewelry Exit";

char far *sell_items = "Sell Keep";

char far *rest_items = "Rest Days Hours Mins Add Subtract Exit";

char far *view_items = "Items Spells Trade Deposit Drop Lay Cure Exit";

char far *items_items = "Rdy Use Trade Deposit Drop Halve Join Sell Id Exit";
char far *amount_items = "Add Subtract Keep Exit";
char far *older_game_items = "Pools Secret Exit";

char far *source_items = "Tdqk Dkk Exit";

char far *add_items = "Add Exit";

char far *delete_items = "Delete Exit";

char far *change_items = "Change Exit";
char far *try_disk_items = "Ok Try another disk";

char far *game_items = "A B C D E F G H I J";

char far *order_items = "Select Place Exit";

char far *speed_items = "Faster Slower Exit";

char far *explore_items = "Move Area Cast View Encamp Search Look Inv";

char far *exit_items = "Exit";

char far *command_items = "Move Aim Use Cast Turn Guard Quick@Delay Bandage Yell View Speed End";

char far *move_items = "Undo Done";

char far *aim_items = "Next Prev Manual Target Center Exit";

char far *target_items = "Target Center Exit";

char far *buy_items = "Buy Exit";

char far *shop_items[1] = {
    "Buy View Take Pool Share Appraise Exit"
};

char spaces[35] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0
};

char far *take_items = "Take Exit";
char far *money_items_menu = "Money Items Exit";

char far *treasure_items = "View Take Pool Share Detect Exit";

char far *heal_items = "Heal Exit";

char far *temple_items = "Heal View Repair Exit";

char far *vault_items = "View Take Pool Money Items Exit";
/* DKK iconMenus[1..5], CustomizeIcon's menus: DQK builds that editor from
 * buttons instead, leaving this one null pointer. */
char far *icon_menus = 0;

char far *camp_menus[4] = {
    "View Magic Rest Alt Fix Load Save Exit",
    "Cast Memorize Scribe Display Rest Exit",
    "Order Drop Speed Icon Level Exit",
    "Novice Squire Veteran Adept Champion"
};

/* 3f51: adjacent label; no measured consumer. */
char far *level_suffix_text = " Level";

char far *item_words[122] = {
    "",
    "Battle Axe",
    "Hand Axe",
    "Club",
    "Dagger",
    "Dart",
    "Hammer",
    "Javelin",
    "Mace",
    "Morning Star",
    "Military Pick",
    "Awl Pike",
    "Bolt",
    "Scimitar",
    "Spear",
    "Quarter Staff",
    "Bastard Sword",
    "Broad Sword",
    "Long Sword",
    "Short Sword",
    "Two-Handed Sword",
    "Trident",
    "Composite Long Bow",
    "Composite Short Bow",
    "Long Bow",
    "Short Bow",
    "Fine",
    "Light Crossbow",
    "Sling",
    "Staff",
    "Arrow",
    "Leather",
    "Ring",
    "Scale",
    "Chain",
    "Banded",
    "Plate",
    "Shield",
    "Cleric",
    "Scroll",
    "Mage",
    "Helm",
    "Belt",
    "Robe",
    "Cloak",
    "Boots",
    "Ring",
    "Mail",
    "Armor",
    "Of Prot",
    "Bracers",
    "Wand",
    "Elixir",
    "Potion",
    "Youth",
    "Ruby",
    "Boulder",
    "Dragon Breath",
    "Displacement",
    "Eyes",
    "Drow",
    "Elfin Chain",
    "Ice Storm",
    "Saphire",
    "Emerald",
    "Wizardry",
    "Hornet's Nest",
    "Fire Resistance",
    "Stone",
    "Good Luck",
    "Flail",
    "Halberd",
    "Gauntlets",
    "Periapt",
    "Health",
    "Cursed",
    "Blessed",
    "Bundle of",
    "Ogre Power",
    "Girdle",
    "Giant Strength",
    "Mirror",
    "Necklace",
    "Dragon",
    "vs Giants",
    "vorpal",
    "cold resistance",
    "Diamond",
    "Lightning",
    "Fireballs",
    "of",
    "Vulnerability",
    "Speed",
    "Silver",
    "Extra",
    "Healing",
    "Charming",
    "Fear",
    "Magic Missiles",
    "Missiles",
    "1 Spell",
    "2 Spells",
    "3 Spells",
    "Paralyzation",
    "Invisibility",
    "Cute Yellow Canary",
    "AC 10",
    "AC 6",
    "AC 4",
    "AC 3",
    "AC 2",
    "+1",
    "+2",
    "+3",
    "+4",
    "+5",
    "-1",
    "-2",
    "-3",
    "Electrical Immunity",
    "Gaze Resistance",
    "Spiritual"
};

char far *gem_text = "Gem";
char far *item_word_jewelry = "Jewelry";
char far *item_word_blinking = "blinking";
char far *item_word_from_evil = "from evil";
char far *item_word_hoopak = "Hoopak";
char far *item_word_dragonlance = "Dragonlance";
char far *item_word_striking = "Striking";
char far *item_word_disruption = "Disruption";
char far *item_word_dragon_slayer = "Dragon Slayer";
char far *item_word_foethumper = "Foethumper";
char far *item_word_solamnic = "Solamnic";
char far *item_word_petrification = "Petrification";
char far *item_word_free_action = "Free Action";
char far *item_word_footmans = "Footman's";
char far *item_word_red_mage = "Red Mage";
char far *item_word_white_mage = "White Mage";
char far *item_word_quarrel = "Quarrel";
char far *item_word_cleric = "Cler";
char far *item_word_mu_scroll = "MU Scroll";
char far *item_word_amulet = "Amulet";
char far *item_word_bag = "Bag";
char far *item_word_solamnic_plate = "Solamnic Plate";
char far *item_word_proof_poison = "Proof vs Poison";
char far *item_word_elixir_youth = "Elixir Of Youth";
char far *item_word_eyes_charming = "Eyes Of Charming";
char far *item_word_vs_reptiles = "vs Reptiles";
char far *item_word_frost_brand = "Frost Brand";
char far *item_word_berserker = "Berserker";
char far *item_word_olins = "Olin's";
char far *item_word_white = "White";
char far *item_word_red = "Red";
char far *item_word_blank = "Blank";
char far *item_word_generic = "Item";

char far *old_text = "who is looking very old";

char far *strain_text = "the strain of the last battle has worn heavily on ";

char far *breath_text = "breathes deeply one last time and crumples to the ground.";

char far *he_text = "he";

char far *she_text = "she";

char far *age_death_text = "dies from age";

char far *no_witness_text = "Sadly, no party member witnesses the passing of the champion.";

char far *one_witness_text = " weeps at the loss of such a venerable companion.";

char far *many_witness_text = " The remaining party members are overcome with grief as they search ";

char far *search_body_text = "'s body for valuables.";

