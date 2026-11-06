;  DQK.EXE resident frame 10d7: the near-heap allocator and the DOS call
;  wrappers -- mem_alloc, four INT 21h wrappers that return a negated error
;  code on carry, and file seek and rename.  EXE 0x0057df..0x0058e6, 263
;  bytes, 10d7:266f..10d7:2776.  strings.asm ends at 0x57df; memops.asm
;  starts at 0x58e6.
;
;  Boundaries: mem_alloc (266f) far-calls far_alloc (10d7:2820), MZ
;  relocation 0x5808; TASM 3.1 (SMART, the default) would emit `push cs;
;  call near` for that call within one file (measured; see conio.asm), so,
;  given SMART assembly, a file boundary lies in 266f..2820.  Where this file starts and ends
;  inside that, 266f and 2776, is a choice by role: system calls here,
;  memory moves and scans in memops.asm, the far heap in farmem.asm.
;  dos_open, between dos_call and dos_io, is held here by a measurement:
;  dos_io's error path jumps back into dos_call's (10d7:2708 to 26bc).
;
;  Two register styles: mem_alloc, file_seek and file_rename read their
;  arguments through bx = sp (ss:[bx+4] on); the INT 21h wrappers keep a bp
;  frame (arguments from [bp+6]) and save si and di.  DS = SS = DGROUP on
;  entry.
;
;  Assembled by TASM 3.1, /ml, single pass.  One segment fixup, the far
;  call at 0x005808 (10d7:2698), the range's one MZ relocation; _far_alloc
;  is an extern resolved to 10d7:2820.  The DGROUP word _heap_brk is
;  addressed through DS by its offset, which the build resolves.

                .8086

DGROUP          group   _DATA
_DATA           segment word public 'DATA'
                extrn   _heap_brk:word          ; 0022: the heap break's offset (a far
_DATA           ends                            ;   pointer, cmdargs.c; segment at 0024)

                extrn   _far_alloc:far          ; 10d7:2820, farmem.asm

DOS             equ     21h
DOS_SEEK        equ     42h                     ; al = the origin, cx:dx the offset
DOS_RENAME      equ     56h                     ; ds:dx the old name, es:di the new
STACK_MARGIN    equ     800h                    ; mem_alloc keeps the heap this far below sp
ERR_MASK        equ     7FFFh

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT, ds:DGROUP

                public  _mem_alloc, _dos_call, _dos_open, _dos_io, _dos_regs
                public  _file_seek, _file_rename

;  void far *far mem_alloc(unsigned n)                          10d7:266f
;
;  Takes n bytes at the heap break in DGROUP when the new break neither
;  wraps past FFFFh nor rises above sp - 800h, and returns DGROUP:old
;  break; otherwise returns far_alloc(n), memory from DOS (0:0 when DOS has
;  none).  Only the break's offset word moves.  Nothing is ever given back.
_mem_alloc      proc    far
                mov     bx,sp
                mov     ax,_heap_brk
                mov     cx,ax
                add     cx,word ptr ss:[bx+4]   ; the new break
                jb      from_dos                ; wrapped
                mov     dx,sp
                sub     dx,STACK_MARGIN
                cmp     cx,dx
                ja      from_dos
                mov     _heap_brk,cx
                mov     dx,ds                   ; dx:ax = DGROUP:old break
                ret
from_dos:
                xor     dx,dx
                mov     ax,word ptr ss:[bx+4]
                push    dx                      ; far_alloc((long)n)
                push    ax
                call    _far_alloc
                pop     bx
                pop     bx
                ret
_mem_alloc      endp

;  int far dos_call(int ax, int dx, int bx, int cx)             10d7:269d
;
;  INT 21h with those registers and si = bx.  Returns ax & 7FFFh, or, when
;  DOS sets carry, -(ax & 7FFFh): a negated DOS error code.
_dos_call       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                mov     dx,[bp+8]
                mov     bx,[bp+0Ah]
                mov     si,bx
                mov     cx,[bp+0Ch]
                clc
                int     DOS
                pop     di
                pop     si
                pop     bp
                jb      dos_failed
                and     ax,ERR_MASK
                ret
dos_failed:                                     ; dos_io's error path too
                and     ax,ERR_MASK
                neg     ax
                ret
_dos_call       endp

;  int far dos_open(int ax, char far *name, int bx, int cx)     10d7:26c2
;
;  INT 21h with ds:dx = es:dx = name, bx, cx and si = bx: the shape of
;  the calls that take a name in ds:dx.  Returns as dos_call does.  ds and
;  es are saved.
_dos_open       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                push    ds
                push    es
                lds     dx,dword ptr [bp+8]
                mov     bx,ds
                mov     es,bx
                mov     bx,[bp+0Ch]
                mov     cx,[bp+0Eh]
                mov     si,bx
                clc
                int     DOS
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                jb      open_failed
                and     ax,ERR_MASK
                ret
open_failed:
                and     ax,ERR_MASK
                neg     ax
                ret
_dos_open       endp

;  int far dos_io(int ax, void far *buf, int bx, int cx)        10d7:26ef
;
;  INT 21h with ds:dx = buf, bx and cx (read, write).  Unlike dos_call it
;  returns ax as DOS left it on success, unmasked, so a count of 8000h or
;  more reads as negative; on carry it returns through dos_call's error
;  path, -(ax & 7FFFh).  ds is saved.
_dos_io         proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                push    ds
                lds     dx,dword ptr [bp+8]
                mov     bx,[bp+0Ch]
                mov     cx,[bp+0Eh]
                clc
                int     DOS
                pop     ds
                pop     di
                pop     si
                pop     bp
                jb      dos_failed
                ret
_dos_io         endp

;  int far dos_regs(int ah, int dx, int cx, int bx, int al, int si, int di)
;                                                               10d7:270b
;
;  INT 21h with every general register from the arguments (ah and al from
;  the low bytes of theirs) and es = ds.  Returns as dos_call does.
;  dispinit.c calls it with ah = 30h, the DOS version.  The carry flag is
;  not cleared first.
_dos_regs       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,ds
                mov     es,ax
                mov     ah,[bp+6]
                mov     dx,[bp+8]
                mov     cx,[bp+0Ah]
                mov     bx,[bp+0Ch]
                mov     al,[bp+0Eh]
                mov     si,[bp+10h]
                mov     di,[bp+12h]
                int     DOS
                pop     di
                pop     si
                pop     bp
                jb      regs_failed
                and     ax,ERR_MASK
                ret
regs_failed:
                and     ax,ERR_MASK
                neg     ax
                ret
_dos_regs       endp

;  long far file_seek(int handle, long offset, int origin)      10d7:273a
;
;  DOS 42h.  Returns the new position in dx:ax, or dx = FFFFh and ax = the
;  negated error code.
_file_seek      proc    far
                mov     bx,sp
                mov     dx,word ptr ss:[bx+6]   ; offset, low word
                mov     cx,word ptr ss:[bx+8]   ;   high word
                mov     ax,word ptr ss:[bx+0Ah] ; al = origin
                mov     ah,DOS_SEEK
                mov     bx,word ptr ss:[bx+4]   ; handle
                int     DOS
                jae     seek_done
                mov     dx,-1
                neg     ax
seek_done:
                ret
_file_seek      endp

;  int far file_rename(char far *old, char far *new)            10d7:2758
;
;  DOS 56h; returns 0, or the negated error code.  As shipped it loads
;  es:di from ss:[bx+6], not ss:[bx+8] where the second far pointer lies:
;  di gets old's segment and es new's offset, so es:di is not the new
;  name.  Nothing in DQK.EXE calls it.  si, di, ds and es are saved.
_file_rename    proc    far
                mov     bx,sp
                push    si
                push    di
                push    ds
                push    es
                lds     dx,dword ptr ss:[bx+4]  ; ds:dx = old
                les     di,dword ptr ss:[bx+6]  ; (sic) see above
                mov     ah,DOS_RENAME
                int     DOS
                pop     es
                pop     ds
                pop     di
                pop     si
                jb      rename_failed
                xor     ax,ax
                ret
rename_failed:
                neg     ax
                ret
_file_rename    endp

_TEXT           ends
                end
