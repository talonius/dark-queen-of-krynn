/* DQK.EXE resident module 2c41: the state around module 25ff's speaker
 * sound.  Five 14-byte slots whose first four bytes are a far pointer, a
 * word set to -1 alongside them, a flag and a far hook called with the
 * timer tick count.  Starting up calls 25ff's start-up, zeroes the slot
 * pointers and the hook, sets the word to -1 and the flag to 1; stopping
 * clears the slots and silences 25ff (port 0x61, and port 0xc0 when 25ff's
 * flag at cdc2 is set).  sfx_update reads the tick count from 251e and
 * passes it to sfx_tick, which hands it to the hook when one is set; code
 * at 0x01680e calls sfx_tick the same way.  No other code in the
 * image names DGROUP dad2..db1e in a direct operand, so nothing found sets
 * the hook, reads the flag or touches a slot's last ten bytes.
 *
 * EXE 0x01e815..0x01e8c2, one Turbo C large-model _TEXT segment starting at
 * byte 5 of paragraph 2c41.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * It starts the byte after module 2aa6's last retf (2aa6:19b4); module 2c4c
 * starts at 0x01e8c2 with no stub between.  All seven MZ relocation sites
 * that store segment 1c41 are far calls to this module's five routines.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  The mod1c41_ names are sysutil.c's and diskfile.c's; the rest are
 * placeholders chosen from use.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined: sfx_slots_done far-calls sfx_reset and sfx_update far-calls
 * sfx_tick, so those two carry prototypes.
 */

#include "dqk.h"

struct SfxSlot {
    void far *data;             /* +0, the only field this module touches */
    char rest[10];              /* +4                                     */
};

void far sfx_reset(void);
void far sfx_tick(long ticks);

static int sfx_cur;                                              /* dad2 */
static void (far *sfx_hook)(long ticks);                         /* dad4 */
static char sfx_armed;                                           /* dad8 */
static struct SfxSlot sfx_slot[5];                               /* dad9 */

void far sfx_slots_init(int argc, char far * far *argv)            /* 2c41:0005 */
{
    register int slot_index;

    speaker_init();
    for (slot_index = 0; slot_index < 5; slot_index++) {
        sfx_slot[slot_index].data = 0;
    }
    sfx_cur = -1;
    sfx_armed = 1;
    sfx_hook = 0;
}

void far sfx_slots_done(void)                                      /* 2c41:004c */
{
    sfx_reset();
    speaker_done();
}

void far sfx_reset(void)                                      /* 2c41:0057 */
{
    register int slot_index;

    sfx_cur = -1;
    for (slot_index = 0; slot_index < 5; slot_index++) {
        sfx_slot[slot_index].data = 0;
    }
    speaker_silence();
}

void far sfx_update(void)                                      /* 2c41:0089 */
{
    sfx_tick(sound_ticks());
}

void far sfx_tick(long ticks)                                    /* 2c41:0098 */
{
    if (sfx_hook)
        sfx_hook(ticks);
}
