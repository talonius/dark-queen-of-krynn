/* DQK.EXE resident module 2023: sound effects and the sound switches.
 * sfx_play takes module 1fd3's event numbers: it drops any above 0x29
 * but 0xff, and turns effect 11 into 14 or 15 by game_state[STATE_REALM].
 * Events 0 and 1 mute and unmute effects through sound_toggle; effects
 * 3..0x0f go to sound_event when digital is set, and otherwise sound a
 * two-tick tone on the speaker; with sound_card 6 effects and songs
 * (0x20..0x27) are skipped.  sfx_init asks slot_used(-1) for the far
 * memory left: under 0x33c00 bytes it clears digital (v1.0 prints no
 * message), otherwise it sets digital and passes
 * its address to sound_open, which stores through it; either way music_on
 * starts set.  game_sound_stop shuts sound down (misc.c's game_exit and
 * gamemain.c call it).  sound_toggle flips the speaker's on flag and sends
 * event 1 or 0, the Ins key (0x152) in misc.c, menu.c, listbox.c, combat.c,
 * titleseq.c and party.c; music_toggle flips music_on and sends 0x28 or
 * 0x29, the Del key.
 *
 * EXE 0x012636..0x0128c6, one Turbo C large-model _TEXT segment starting at
 * byte 6 of paragraph 2023.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * Module 1fd3's last routine ends with the retf at 0x012635.  The lone
 * retf at 0x0128c6 is module 204c's (charpic_unit_init).  Every routine here is
 * far-called; the one far call found to sfx_setup is at 0x037d90, and the
 * one to sfx_init at 0x037e1a.
 *
 * v1.0 has no _DATA (v1.1's is sfx_init's four literals); _BSS is
 * a846..a847.  No code outside this module was found naming either variable.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  A name another module
 * gives an address is reused from there, this module's own mod1023_ names
 * included; the rest are placeholders chosen from use.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined: sfx_play far-calls sound_toggle, so that one carries a
 * prototype.
 */

#include "dqk.h"

static char digital;                                             /* a846 */
static char music_on;                                            /* a847 */

void far sound_toggle(void);

void far sfx_play(unsigned short sound_id)                              /* 2023:0006 */
{
    long until;

    if (sound_id >= 0 && sound_id < 0x2a || sound_id == 0xff) {      /* n >= 0 is always true, as shipped */
        if (sound_id == 11) {
            if (game_state[STATE_REALM] == 3)
                sound_id = 14;
            else if (game_state[STATE_REALM] == 6)
                sound_id = 15;
        }
        switch (sound_id) {    /* 2e 3b 07 at 0053: Turbo C's sparse-switch value search */
        case 0:
            if (speaker_enabled())
                sound_toggle();
            break;
        case 1:
            if (!speaker_enabled())
                sound_toggle();
            break;
        case 2:
            sound_event(sound_id);
            speaker_silence();
            break;
        case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:
        case 11: case 12: case 13: case 14: case 15:
            if (sound_card == 6)
                break;
            if (digital) {
                sound_event(sound_id);
                break;
            }
            until = timer_ticks() + 2;
            voice_set(0, 0xe24, 0x7f);
            voice_set(1, 2, 0x7f);
            while (timer_ticks() < until)
                ;
            speaker_silence();
            break;
        case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:
        case 0x26: case 0x27:
            if (sound_card == 6)
                break;
        case 0x28: case 0x29: case 0xff:
            sound_event(sound_id);
            break;
        }
    }
}

/* Empty; game_setup calls it among the allocations (plausible). */
void far sfx_setup(void)                                      /* 2023:016e */
{
}

void far sfx_init(void)                                          /* 2023:016f */
{
    long avail;

    avail = slot_used(-1);
    if (avail < 0x33c00L)
        digital = 0;            /* v1.0: silently; v1.1 says how much more it needs */
    else {
        digital = 1;
        sound_open(&digital);
    }
    music_on = 1;
}

void far game_sound_stop(void)                                      /* 2023:0241 */
{
    sound_close();
}

void far sound_toggle(void)                                      /* 2023:0247 */
{
    speaker_enable(!speaker_enabled());
    sound_event(speaker_enabled() ? 1 : 0);
}

void far music_toggle(void)                                      /* 2023:0272 */
{
    music_on = !music_on;
    sound_event(music_on ? 0x28 : 0x29);
}
