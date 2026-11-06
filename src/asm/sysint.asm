;  DQK.EXE resident frame 10d7: interrupts and ports -- a software
;  interrupt with its number patched into the code, port input and output
;  for bytes and words, a bare INT 21h, and the critical-error and
;  Ctrl-Break handlers with the routines that install and remove them.
;  EXE 0x005a25..0x005b02, 221 bytes, 10d7:28b5..10d7:2992.  farmem.asm
;  ends at 0x5a25; bios.asm starts at 0x5b02.
;
;  Why these routines are one file is a choice, not a measurement: the
;  frame has no far call into or out of this range, nor a relocation or pad
;  byte at 28b5 or 2992.  crit_install is kept with the handler whose two
;  offsets it loads, crit_restore with crit_install, and the rest with
;  them by role.
;  int_call keeps a bp frame, the port routines read through bx = sp.
;
;  int_call writes the code segment: it patches the number byte of its own
;  INT instruction.  The code is in the resident image, so the write lands
;  in the copy DOS loaded.
;
;  Assembled by TASM 3.1, /ml, single pass: each forward jmp takes three
;  bytes, EB xx 90, as it shipped.  No segment fixups, and the MZ
;  relocation table has no entry in this range: the handlers' offsets are
;  frame-relative words the build resolves, and the DGROUP word __psp is
;  addressed through DS by its offset.

                .8086

DGROUP          group   _DATA
_DATA           segment word public 'DATA'
                extrn   __psp:word              ; 0020: the PSP's segment
_DATA           ends

DOS             equ     21h
SET_CRIT_VEC    equ     2524h                   ; DOS 25h: set the int 24h vector to ds:dx
SET_BREAK_VEC   equ     2523h                   ;   and int 23h's
SET_BREAK_FLAG  equ     3301h                   ; DOS 33h al=1: BREAK off (dl=0) or on (dl=1)
PSP_INT23       equ     0Eh                     ; PSP: the int 23h vector at start-up
PSP_INT24       equ     12h                     ; PSP: the int 24h vector at start-up
CRIT_TO_DOSERR  equ     13h                     ; critical-error code + 13h = DOS error code
IRET_FLAGS      equ     14h                     ; crit_handler: the program's flags,
CARRY           equ     01h                     ;   above its saved bx..es and ip, cs

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT, ds:DGROUP

                public  _int_call, _port_in, _port_out, _port_inw, _port_outw
                public  _int21_raw, _crit_install, _crit_restore
                public  crit_handler, break_handler

;  int far int_call(int n, int ah, int al, int bx, int cx, int dx, int si,
;                   int di)                                     10d7:28b5
;
;  Executes INT n with those registers (ah and al from the low bytes of
;  theirs) and es = ds; returns the ax the interrupt leaves.  The INT
;  instruction's number byte (10d7:28f2) is written first; eight register
;  loads, two `jmp $+3` (EB 01 90) and 13 more NOPs separate the write
;  from the INT, apparently so that no
;  copy of the old byte fetched ahead of the write is executed.  diskfile.c
;  calls it with n = 11h, the BIOS equipment list.  si and di are saved.
_int_call       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,ds
                mov     es,ax
                mov     ax,[bp+6]               ; al = n
                mov     di,offset int_insn+1
                mov     byte ptr cs:[di],al
                jmp     flush_1
flush_1:
                jmp     flush_2
flush_2:
                rept    13
                nop
                endm
                mov     ax,[bp+0Ah]             ; al
                mov     cx,[bp+8]
                mov     ah,cl                   ; ah
                mov     cx,[bp+0Eh]
                mov     dx,[bp+10h]
                mov     si,[bp+12h]
                mov     di,[bp+14h]
                mov     bx,[bp+0Ch]
int_insn:
                int     0                       ; 10d7:28f1, the number patched
                pop     di
                pop     si
                pop     bp
                ret
_int_call       endp

;  int far port_in(int port)                                    10d7:28f7
;
;  The byte at the I/O port, zero-extended.  The four port routines read
;  their arguments through bx = sp; nothing in DQK.EXE calls them.
_port_in        proc    far
                mov     bx,sp
                mov     dx,word ptr ss:[bx+4]
                in      al,dx
                xor     ah,ah
                ret
_port_in        endp

;  void far port_out(int port, int b)                           10d7:2901
_port_out       proc    far
                mov     bx,sp
                mov     dx,word ptr ss:[bx+4]
                mov     ax,word ptr ss:[bx+6]
                out     dx,al
                ret
_port_out       endp

;  int far port_inw(int port)                                   10d7:290d
_port_inw       proc    far
                mov     bx,sp
                mov     dx,word ptr ss:[bx+4]
                in      ax,dx
                ret
_port_inw       endp

;  void far port_outw(int port, int w)                          10d7:2915
_port_outw      proc    far
                mov     bx,sp
                mov     dx,word ptr ss:[bx+4]
                mov     ax,word ptr ss:[bx+6]
                out     dx,ax
                ret
_port_outw      endp

;  int21_raw                                                    10d7:2921
;
;  INT 21h with the registers as the caller left them, far-callable from
;  assembly.  Nothing in DQK.EXE calls it.
_int21_raw      proc    far
                int     DOS
                ret
_int21_raw      endp

;  void far crit_install(void)                                  10d7:2924
;
;  Points int 24h (critical error) at crit_handler and int 23h (Ctrl-Break)
;  at break_handler, crit_handler's closing iret, so Ctrl-Break is
;  ignored, and turns DOS's BREAK checking off.  Called once, from
;  dispinit.c.  Leaves ds = ss (DGROUP).
_crit_install   proc    far
                mov     ax,cs
                mov     ds,ax
                mov     dx,offset crit_handler
                mov     ax,SET_CRIT_VEC
                int     DOS
                mov     dx,offset break_handler
                mov     ax,SET_BREAK_VEC
                int     DOS
                mov     ax,ss
                mov     ds,ax
                mov     ax,SET_BREAK_FLAG
                xor     dl,dl                   ; off
                int     DOS
                ret
_crit_install   endp

;  void far crit_restore(void)                                  10d7:2944
;
;  Puts back the int 24h and int 23h vectors DOS recorded in the PSP at
;  start-up and turns BREAK checking on.  es is saved; leaves ds = ss.
;  Nothing in DQK.EXE calls it.
_crit_restore   proc    far
                push    es
                mov     es,__psp
                mov     ds,word ptr es:[PSP_INT24+2]
                mov     dx,word ptr es:[PSP_INT24]
                mov     ax,SET_CRIT_VEC
                int     DOS
                mov     ds,word ptr es:[PSP_INT23+2]
                mov     dx,word ptr es:[PSP_INT23]
                mov     ax,SET_BREAK_VEC
                int     DOS
                mov     ax,ss
                mov     ds,ax
                pop     es
                mov     ax,SET_BREAK_FLAG
                mov     dl,1                    ; on
                int     DOS
                ret
_crit_restore   endp

;  crit_handler, the int 24h handler                            10d7:2974
;
;  Fails the DOS call that met the error by returning straight to the
;  program that made it: drops DOS's int 24h return (ip, cs, flags) and the
;  program's ax, sets carry in the program's saved flags, makes ax the
;  critical-error code (di's low byte) + 13h, the matching DOS error code,
;  restores the program's other registers as DOS saved them, and irets
;  past its INT 21h.  Its last instruction, the iret, is also the int 23h
;  (Ctrl-Break) handler, break_handler: crit_install points int 23h there.
crit_handler    proc    far
                pop     ax                      ; int 24h's ip
                pop     ax                      ;   cs
                pop     ax                      ;   flags
                pop     ax                      ; the program's ax
                mov     bx,sp
                mov     ax,ss
                mov     ds,ax
                or      byte ptr [bx+IRET_FLAGS],CARRY
                mov     ax,di
                add     ax,CRIT_TO_DOSERR
                xor     ah,ah
                pop     bx
                pop     cx
                pop     dx
                pop     si
                pop     di
                pop     bp
                pop     ds
                pop     es
break_handler   label   far                     ; 10d7:2991, the int 23h handler:
                iret                            ;   Ctrl-Break is ignored
crit_handler    endp

_TEXT           ends
                end
