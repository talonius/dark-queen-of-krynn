/* DQK.EXE overlay 04 module 25a4: the configuration file, the memory and
 * video checks made against it.  (v1.0 has no arg_hex: v1.1 added the
 * hex-valued command options.)
 *
 * EXE 0x037910..0x037b71, one Turbo C large-model _TEXT segment starting at
 * byte 0 of paragraph 25a4, the first module of RTLink section 4 (payload
 * 0x37910, link segment 25a4).  Its final retf is byte 0 of paragraph 25ca,
 * where the next module begins.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  Callee names are placeholders chosen from call shapes; a modNNNN_
 * name says only which segment the callee lives in, and display_options_set is
 * another module of this overlay section.  The 00d7 DOS wrappers load
 * AX, DX, BX and CX from however many arguments a caller pushes.
 *
 * _DATA is the six literals, DGROUP 608e..6156.  config is the whole of
 * _BSS, acd6..ad65; other modules read its fields at their own addresses.
 */

#include "dqk.h"

struct Config {
    int video;                  /* +0  0 VGA, 1 EGA, 2 Tandy 16-colour  */
    int sound_card;             /* +2  sound.c's sound_card             */
    unsigned char rest[0x8c];
};

struct Config config;                                            /* acd6 */

void far config_load(void)                                       /* 25a4:0000 */
{
    unsigned long avail;
    int file_handle;

    if ((file_handle = dos_open(0x3d00, "disk1\\dqk.cfg")) >= 0) {
        if (dos_io(0x3f00, &config, file_handle, sizeof config) == sizeof config) {
            dos_call(0x3e00, 0, file_handle);
            avail = (unsigned long)(peek_word(_psp, 2) - _psp) << 4;
            if (avail < 540000L) {
                con_printf("Not enough memory to run!\n"
                           "%l bytes Required, %l bytes available.\n",
                           540000L, avail);
                dos_call(0x4c01);               /* terminate, exit code 1 */
            }
            if (config.video == 0)
                display_options_set(5);
            return;
        }
        dos_call(0x3e00, 0, file_handle);
    }
    con_printf("Error reading configuration file!\n");
    dos_call(0x4c01);
}

void far video_check(void)                                       /* 25a4:00e2 */
{
    char far *msg;

    msg = 0;
    switch (config.video) {
    case 0:
        if (!is_xvga())
            msg = "Installed for VGA only!\n";
        break;
    case 1:
        if (pixel_shift() != 3 || video_planes() != 4)
            msg = "Installed for EGA only!\n";
        break;
    case 2:
        if (pixel_shift() != 1)
            msg = "Installed for TANDY 16 color only!\n";
        break;
    }
    if (msg) {
        sys_shutdown();
        con_printf(msg);
        dos_call(0x4c01);
    }
}
