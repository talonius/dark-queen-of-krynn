/* DQK.EXE data-only module between config.c and approach.c: the compass names, DGROUP
 * 6158..6170, by facing (0 north, clockwise); frame.c and roster.c print their first
 * letters. No routines; its empty _TEXT sits at approach.c's start in section 2.
 */

#define DQK_DEFINES_DIRNAMES
#include "dqk.h"

char dir_names[8][3] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
