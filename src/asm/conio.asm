;  DQK.EXE resident frame 10d7: console character I/O through DOS -- read a
;  key with and without echo, two tests on standard input's state, write a
;  character (with '\n' as CR LF), and DOS's direct console call.  EXE
;  0x0055b9..0x005610, 87 bytes, 10d7:2449..10d7:24a0.  lzcodec.asm ends at
;  0x55b9; cputs.asm starts at 0x5610.
;
;  Where the file ends is measured: cputs.asm's con_puts (10d7:24a0) reaches
;  con_putc_dl (10d7:2483, the second entry of con_putc) by a relocated far
;  call, 9A 83 24 D7 00 at 10d7:24ae, MZ relocation 0x5621.  TASM 3.1 in its
;  default SMART mode assembles a far call to a far label in its own segment
;  as `push cs; call near` (0E E8; measured), so con_puts is in another
;  file.  That rests on the source having been assembled as the other
;  files of the frame are: under NOSMART, or by MASM, one file would emit
;  9A too (measured for NOSMART), and the bytes cannot tell those apart.
;  Where the file starts, 2449, is the first byte
;  after lzcodec.asm's last retf.  stdin_not_tty shares stdin_idle's
;  `return 1` (a jump from 10d7:2473 to 2462), so those two are one file.
;
;  Register style: no bp frame; the one argument read through bx = sp,
;  ss:[bx+4], the first word past the far return address.  The result is in
;  ax.  bx and dx are not preserved.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups, and the MZ
;  relocation table has no entry in this range.

                .8086

DOS             equ     21h
DOS_GETCHE      equ     01h                     ; read stdin with echo, al = char
DOS_PUTCH       equ     02h                     ; write dl to stdout
DOS_DIRECT      equ     06h                     ; direct console I/O: dl = FFh reads
DOS_GETCH       equ     07h                     ; read stdin, no echo, no ^C check
IOCTL_INFO      equ     4400h                   ; dx = the handle's device information
IOCTL_IN_READY  equ     4406h                   ; al = FFh when input is ready
STDIN           equ     0
DEV_CHAR        equ     80h                     ; device information: a character device
DEV_NUL_CLOCK   equ     0Ch                     ;   bit 2 the NUL device, bit 3 the clock
LF              equ     0Ah
CR              equ     0Dh

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _con_getche, _con_getch, _stdin_idle, _stdin_not_tty
                public  _con_putc, _con_putc_dl, _con_direct

;  int far con_getche(void)                                     10d7:2449
;
;  Reads a character from standard input with echo (DOS 01h); returns it
;  zero-extended.  Nothing in DQK.EXE calls it.
_con_getche     proc    far
                mov     ah,DOS_GETCHE
                int     DOS
                xor     ah,ah
                ret
_con_getche     endp

;  int far con_getch(void)                                      10d7:2450
;
;  Reads a character from standard input without echo (DOS 07h); returns it
;  zero-extended.
_con_getch      proc    far
                mov     ah,DOS_GETCH
                int     DOS
                xor     ah,ah
                ret
_con_getch      endp

;  int far stdin_idle(void)                                     10d7:2457
;
;  Asks DOS whether standard input has a character ready (IOCTL 4406h) and
;  returns 1 when it does NOT (al = 0), 0 when it does or when the call
;  fails (carry set, al = an error code).  Nothing in DQK.EXE calls it.
_stdin_idle     proc    far
                mov     ax,IOCTL_IN_READY
                xor     bx,bx                   ; STDIN
                int     DOS
                or      al,al
                jne     return_0
return_1:
                mov     ax,1
                ret
return_0:
                xor     ax,ax
                ret
_stdin_idle     endp

;  int far stdin_not_tty(void)                                  10d7:2469
;
;  Reads standard input's device information (IOCTL 4400h).  Returns 1
;  when it is a file, the NUL device or the clock device, 0 when it is
;  another character device (the console).  The carry flag is not tested.
;  Nothing in DQK.EXE calls it.
_stdin_not_tty  proc    far
                mov     ax,IOCTL_INFO
                xor     bx,bx                   ; STDIN
                int     DOS
                test    dl,DEV_CHAR
                je      return_1
                test    dl,DEV_NUL_CLOCK
                jne     return_1
                xor     ax,ax
                ret
_stdin_not_tty  endp

;  void far con_putc(char c)                                    10d7:247d
;
;  Writes c to standard output (DOS 02h), '\n' as CR LF.  fmtout.c hands it
;  to text_format as the character sink.  The second entry, con_putc_dl
;  (10d7:2483), takes the character in dl instead; cputs.asm's con_puts
;  far-calls it.
_con_putc       proc    far
                mov     bx,sp
                mov     dx,word ptr ss:[bx+4]
_con_putc_dl    label   far                     ; 10d7:2483, dl = the character
                cmp     dl,LF
                jne     put_dl
                mov     dl,CR
                mov     ah,DOS_PUTCH
                int     DOS
                mov     dl,LF
put_dl:
                mov     ah,DOS_PUTCH
                int     DOS
                ret
_con_putc       endp

;  int far con_direct(int c)                                    10d7:2495
;
;  DOS 06h with dl = c: writes c to standard output without a ^C check, or,
;  for c = FFh, reads a character if one is ready (al; zero flag set when
;  none is).  ah is left 06h, so a C caller's int is 06xxh.
_con_direct     proc    far
                mov     bx,sp
                mov     dx,word ptr ss:[bx+4]
                mov     ah,DOS_DIRECT
                int     DOS
                ret
_con_direct     endp

_TEXT           ends
                end
