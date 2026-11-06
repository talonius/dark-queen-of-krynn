/* DQK.EXE resident module 2c4c: seven far wrappers, over the keyboard in
 * module 251e and the mouse in module 22b7.  Three pass their arguments to
 * a 22b7 routine that stores a position through the two pointers (a button
 * press, chosen by bits 0 and 1 of mouse_press's mask; a click not yet
 * fetched; the pointer itself); inp_pending returns mouse_pending, 1 when
 * either button flag (cb49, cb67) is set; two forward to key_poll and
 * key_get; and the last returns 0.
 * None has a local, and none reads DGROUP.
 *
 * EXE 0x01e8c2..0x01e922, one Turbo C large-model _TEXT segment starting at
 * byte 2 of paragraph 2c4c.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * It starts the byte after module 2c41's last retf (2c41:00b1).  All six MZ
 * relocation sites inside it lie in frame 1c4c, and every far call into it
 * stores segment 1c4c.  Module 2c52 starts at 0x01e922 with no stub between;
 * its one far caller (0x0024b9) stores 1c52, and the two relocation sites
 * inside it have frame 1c52.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  kbd_waiting, kbd_read and kbd_state_5f are sysutil.c's names.
 */

#include "dqk.h"

int far inp_press(char mask, int far *x, int far *y)             /* 2c4c:0002 */
{
    return mouse_press(mask, x, y);
}

char far inp_clicked(int far *x, int far *y)                     /* 2c4c:001d */
{
    return mouse_clicked(x, y);
}

void far inp_pos(int far *x, int far *y)                         /* 2c4c:0035 */
{
    mouse_pos(x, y);
}

char far inp_pending(void)                                       /* 2c4c:004d */
{
    return mouse_pending();
}

char far kbd_waiting(void)                                       /* 2c4c:0053 */
{
    return key_poll();
}

int far kbd_read(void)                                           /* 2c4c:0059 */
{
    return key_get();
}

char far kbd_state_5f(void)                                      /* 2c4c:005f */
{
    return 0;
}
