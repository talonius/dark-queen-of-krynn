/* DQK.EXE resident module 2c52: the C side of start-up.  The start-up code
 * at 1000:0000 copies the command tail onto the stack, puts the program's
 * path and a space ahead of it under DOS 3 or later ("C " under DOS 2; DOS
 * 1 exits), and far-calls start_main with that string.  start_main splits
 * it into words at the heap break, a word being a quoted run or a run of
 * characters above ' ' (a signed compare, so a byte of 0x80 or more ends a
 * word), cuts each with a NUL, ends the table with a null pointer, moves
 * the heap break past the table, and calls 1d99:114d with the word count
 * and the table.  1d99:114d is an RTLink vector into overlay section 3
 * (stored 1fe9:00ef), and argc, argv is the shape a main takes.
 *
 * EXE 0x01e922..0x01ea00, one Turbo C large-model _TEXT segment starting at
 * byte 2 of paragraph 2c52.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Module 2c4c's last routine ends at 0x01e922, and module 2c60 starts at
 * 0x01ea00 (2c60:0000) with no stub between.  Its one far caller, at
 * 0x0024b9, stores 1c52:0002, and both MZ relocation sites inside it have
 * frame 1c52.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  A name another module gives an address is reused from there; the
 * rest are placeholders chosen from use.
 *
 * This module has no _DATA or _BSS.  heap_brk (0022) is the runtime's heap
 * break, initialized to DGROUP:db60, the end of the region start-up zeroes;
 * 10d7:266f allocates from its offset word.
 */

#include "dqk.h"

void far start_main(char far *command_cursor)                                 /* 2c52:0002 */
{
    char far *far *argument_slot;
    char far *far *argv;

    argument_slot = heap_brk;
    while (*command_cursor) {
        if (*command_cursor == '"') {
            command_cursor++;
            *argument_slot = command_cursor;
            argument_slot++;
            while (*command_cursor) {
                if (*command_cursor == '"')
                    goto cut;
                command_cursor++;
            }
        } else if (*command_cursor > ' ') {
            *argument_slot = command_cursor;
            argument_slot++;
            do
                command_cursor++;
            while (*command_cursor > ' ');
        } else {
cut:
            *command_cursor = 0;
            command_cursor++;
        }
    }
    *argument_slot = 0;
    argv = heap_brk;
    heap_brk = argument_slot + 1;
    game_main(argument_slot - argv, argv);
}
