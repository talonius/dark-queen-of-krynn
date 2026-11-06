/* DQK.EXE resident module 1fd3: sound events over sound.c's layer.
 * sound_open starts sound for the configured card: its music and digital
 * drivers, the song set and the effect set; when a driver fails it returns
 * -2 (v1.0 has no fallback to card 0's effect mode).  sound_close shuts it
 * down.
 * sound_event takes the event numbers the game plays by: 0 and 1 clear and
 * set sfx_enabled, 2 waits for the song, 3..0x0f play an effect, 0x20..0x27
 * play a song, 0x28 and 0x29 set the music loud or quiet, and 0xff stops
 * the song and waits for the effect.
 *
 * EXE 0x01213b..0x012636, one Turbo C large-model _TEXT segment starting at
 * byte b of paragraph 1fd3, the byte after sound.c's last retf.  Built with
 * Turbo C 2.01, tcc -ml -O -Z -d.  Functions are defined in shipped order;
 * bake consumes them in that order.  Its last routine, music_volume, runs
 * past paragraph 2023 to 2023:0005; module 2023 begins at 2023:0006.
 * Ghidra's index cuts sound_event at 042a and 04d9, inside its body.
 *
 * _DATA is DGROUP 1e5c..24c0: the sound state, the per-card driver and
 * mode tables, the song and effect file names, and two literals.  There is
 * no _BSS.  No decoded instruction outside the module in the recovered
 * listings (v11/src/generated/) names this _DATA; their db runs were not
 * searched.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  Callee names are
 * placeholders chosen from use; a modNNNN_ name says only which segment the
 * callee lives in.  music_volume is static: sound_event near-calls it ahead
 * of its definition.
 */

#include "dqk.h"

extern void far lib_open(char mode, char far *name, int slot);   /* 193f:00d5 */

static char music_ok = 0;                                        /* 1e5c */
static char sfx_ok = 0;                                          /* 1e5d */
static char sound_live = 0;                                      /* 1e5e */
static char music_drv[0x50] = "";                                /* 1e5f */
static char voc_drv[0x50] = "";                                  /* 1eaf */
static int music_mode = 0;                                       /* 1eff */
static int sfx_mode = 0;                                         /* 1f01 */
static char sfx_file[13] = "";                                   /* 1f03 */
static int sfx_index = -999;                                     /* 1f10 */
static char music_file[13] = "";                                 /* 1f12 */
static int music_index = -999;                                   /* 1f1f */
static int song_part[8] = { 1, 2, 2, 2, 2, 2, 2, 3 };            /* 1f21 */
static int song_index[8] = { 0, 0, 1, 2, 3, 4, 5, 0 };           /* 1f31 */
static int card_music_mode[8] = { 3, 2, 1, 1, 0, 0, 0, 0 };      /* 1f41 */
static int card_sfx_mode[8] = { 1, 1, 1, 0, 1, 0, 0, 0 };        /* 1f51 */
static char card_music_drv[8][0x50] = {                          /* 1f61 */
    "PCSPKR.ADV", "TANDY.ADV", "ADLIB.ADV", "ADLIB.ADV",
    "MT32MPU.ADV", "MT32MPU.ADV", "", ""
};
static char card_voc_drv[8][0x50] = {                            /* 21e1 */
    "", "", "", "SBDIG.ADV", "", "SBDIG.ADV", "", ""
};
static char music_names[4][13] = { "RODQ", "ADDQ", "TYDQ", "PCDQ" }; /* 2461 */
static char sfx_names[2][13] = { "SFXDQ.VOC", "" };              /* 2495 */
static char sfx_enabled = 1;                                     /* 24af */
static char music_loud = 1;                                      /* 24b0 */

static int music_volume(char loud, int mode);

int far sound_open(char far *digital)                            /* 1fd3:000b */
{
    short want;
    register int card;
    register int return_code;

    if (sound_card == 6)
        goto off;
    sound_live = 1;
    card = sound_card;
    str_copy(music_drv, card_music_drv[card]);
    str_copy(voc_drv, card_voc_drv[card]);
    want = (*music_drv ? 1 : 0) + (*voc_drv ? 1 : 0);
    music_mode = card_music_mode[card];
    sfx_mode = card_sfx_mode[card];
    if ((return_code = sound_init(music_drv, voc_drv)) == want)
        goto on;
    if (return_code >= 0)
        goto off;
    return_code = return_code * -1;                       /* mul by 0xffff, not neg */
    music_ok = return_code & 1 ? 0 : 1;
    *digital = sfx_ok = return_code & 2 ? 0 : 1;
    return -2;                  /* v1.0: no fallback to card 0's effect mode */
off:
    *digital = sfx_ok = music_ok = 0;
    return 0;
on:
    *digital = sfx_ok = music_ok = 1;
    if (!*voc_drv) {
        sample_library_set(0x17);
        lib_open('1', "sounds", 0x17);
    }
    return 0;
}

void far sound_close(void)                                       /* 1fd3:0164 */
{
    if (sound_live) {
        sound_shutdown();
        sound_live = 0;
    }
}

int far sound_event(register int event_id)                              /* 1fd3:0176 */
{
    short vol;
    char far *name;
    char file[14];
    register int return_code;

    return_code = 0;
    switch (event_id) {        /* `2e 3b 07` at 018b: Turbo C's sparse-switch value search */
    case 0:
        sfx_enabled = 0;
        break;
    case 1:
        sfx_enabled = 1;
        break;
    case 0x29:
        music_loud = 0;
        if (music_index < 0)
            break;
        vol = music_volume(music_loud, music_mode);
        return_code = sound_volume(music_file, music_index, vol);
        if (music_mode != 3)
            break;
        return_code = sound_stop(music_file, music_index);
        goto stopped;   /* shipped keeps case 2's copy of this tail */
    case 0x28:
        music_loud = 1;
        if (music_index < 0)
            break;
        vol = music_volume(music_loud, music_mode);
        return_code = sound_volume(music_file, music_index, vol);
        break;
    case 2:
        if (music_index < 0)
            break;
        return_code = sound_wait(music_file, music_index, 0);
    stopped:
        if (return_code == 0)
            music_index = -1;
        break;
    case 0xff:
        if (music_index >= 0) {
            return_code = sound_stop(music_file, music_index);
            if (return_code == 0)
                music_index = -1;
        }
        if (return_code)
            break;
        if (sfx_index < 0)
            break;
        if (sfx_mode == 1)
            break;
        return_code = sound_wait(sfx_file, sfx_index, 0);
        if (return_code == 0)
            sfx_index = -1;
        break;
    default:
        if (event_id >= 3 && event_id < 0x10) {
            if (!sfx_ok)
                return 0;
            event_id -= 3;
            if (sfx_mode == 1) {
                if (music_index >= 0) {
                    return_code = sound_stop(music_file, music_index);
                    if (return_code == 0)
                        music_index = -1;
                }
                if (return_code)
                    break;
                sample_sequence_play(1, event_id, -1, -1, 1, -1);
                break;
            }
            if (!sfx_enabled)
                break;
            if (sfx_index >= 0) {
                return_code = sound_wait(sfx_file, sfx_index, 0);
                if (return_code == 0)
                    sfx_index = -1;
            }
            if (return_code)
                break;
            return_code = sound_play(sfx_names[sfx_mode], event_id, 100);
            if (return_code)
                break;
            if (sfx_index < -1)         /* -999 until the first effect */
                str_copy(sfx_file, sfx_names[sfx_mode]);
            sfx_index = event_id;
        } else if (event_id >= 0x20 && event_id < 0x28) {
            if (!music_ok)
                return 0;
            event_id -= 0x20;
            name = music_names[music_mode];
            str_format(file, "%s%d.XMI", name, song_part[event_id]);
            event_id = song_index[event_id];
            vol = music_volume(music_loud, music_mode);
            if (music_index >= 0) {
                return_code = sound_stop(music_file, music_index);
                if (return_code == 0)
                    music_index = -1;
            }
            if (return_code == 0 && sfx_index >= 0 && sfx_mode != 1) {
                return_code = sound_wait(sfx_file, sfx_index, 0);
                if (return_code == 0)
                    sfx_index = -1;
            }
            if (return_code)
                break;
            if (!music_loud && music_mode == 3)
                break;
            return_code = sound_play(file, event_id, vol);
            if (return_code)
                break;
            str_copy(music_file, file);
            music_index = event_id;
        } else {
            return_code = -5;
        }
        break;
    }
    return return_code;
}

static int music_volume(char loud, int mode)                     /* 1fd3:04e1 */
{
    register int volume;

    volume = loud ? 100 : 0;
    if (loud && mode == 0)
        volume = 90;
    return volume;
}
