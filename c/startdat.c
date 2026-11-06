/* DQK.EXE data-only module, first in DGROUP: the start-up's data, DGROUP 0000..002c.
 * start.asm stores the PSP's segment and its stack pointer here; heap_brk is the
 * runtime's heap break, set to the end of the region start-up zeroes. No routines;
 * its empty _TEXT sits at start.asm's start.
 */

#define DQK_DEFINES_STARTDAT
#include "dqk.h"

char null_area[32] = { 0 };             /* 0000  zeros a stray null write lands on */
unsigned _psp = 0;                      /* 0020  the PSP's segment                 */
char far *far *heap_brk = bss_end;      /* 0022                                    */
void far *start_sp = null_area;         /* 0026  start.asm stores SP in its low word */
int joy_last_ail = 0;                   /* 002a                                    */
