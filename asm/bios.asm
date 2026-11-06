;  DQK.EXE resident frame 10d7: BIOS calls -- the serial port (INT 14h),
;  video (INT 10h), a character cell of the text screen read and written
;  directly, the clock (INT 1Ah) and the printer (INT 17h).  EXE
;  0x005b02..0x005be2, 224 bytes, 10d7:2992..10d7:2a72.  sysint.asm ends
;  at 0x5b02; ldiv.asm (LDIV@, 10d7:2a72) starts at 0x5be2.
;
;  Why these routines are one file is a choice, not a measurement: the
;  frame has no far call into or out of this range, nor a relocation or pad
;  byte at 2992.  The end, 2a72, is ldiv.asm's start.  They are grouped by
;  role and share a style: a bp frame, arguments from [bp+6] (a byte
;  argument read from its word's low byte).  v1.0 does not save si and di
;  around the BIOS call (v1.1 does); only the text-cell routines, which use
;  them, push and pop them.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups, and the MZ
;  relocation table has no entry in this range.

                .8086

BIOS_VIDEO      equ     10h
BIOS_SERIAL     equ     14h
BIOS_PRINTER    equ     17h
BIOS_CLOCK      equ     1Ah
COM1            equ     0
BIOS_DATA       equ     40h                     ; the BIOS data segment
EQUIPMENT       equ     10h                     ;   its equipment word
EQUIP_VIDEO     equ     30h                     ;   bits 4-5: the initial video mode,
EQUIP_MONO      equ     30h                     ;   30h = 80x25 monochrome
COLOR_TEXT_SEG  equ     0B800h
MONO_TEXT_SEG   equ     0B000h
TEXT_COLUMNS    equ     80                      ; row * 80 = row * 16 + row * 64

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _com_bios, _video_bios, _text_cell_put, _text_cell_get
                public  _clock_bios, _printer_bios

;  int far com_bios(int ah, int al)                             10d7:2992
;
;  INT 14h on COM1 (dx = 0); returns ax.  Nothing in DQK.EXE calls it.
_com_bios       proc    far
                push    bp
                mov     bp,sp
                mov     ah,[bp+6]
                mov     al,[bp+8]
                mov     dx,COM1
                int     BIOS_SERIAL
                pop     bp
                ret
_com_bios       endp

;  int far video_bios(int ah, int al, int bh, int bl, int cx, int dh,
;                     int dl)                                   10d7:29a6
;
;  INT 10h with es = ds and those registers; returns ax.  Among its
;  callers, dispinit.c sets video modes (ah = 0) and the EGA palette
;  (ah = 10h) with it and reads the mode (ah = 0Fh).
_video_bios     proc    far
                push    bp
                mov     bp,sp
                mov     ax,ds
                mov     es,ax
                mov     ah,[bp+6]
                mov     al,[bp+8]
                mov     bh,[bp+0Ah]
                mov     bl,[bp+0Ch]
                mov     cx,[bp+0Eh]
                mov     dh,[bp+10h]
                mov     dl,[bp+12h]
                int     BIOS_VIDEO
                pop     bp
                ret
_video_bios     endp

;  void far text_cell_put(int row, int col, char c, char attr)  10d7:29ca
;
;  Writes c and attr to the 80-column text screen's cell at row, col: at
;  B000h when the BIOS equipment word says the initial video mode is
;  monochrome, else at B800h; page 0.  Nothing in DQK.EXE calls it.
_text_cell_put  proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,BIOS_DATA
                mov     es,ax
                mov     si,EQUIPMENT
                mov     bx,word ptr es:[si]
                mov     ax,COLOR_TEXT_SEG
                and     bx,EQUIP_VIDEO
                cmp     bx,EQUIP_MONO
                jne     put_seg
                mov     ax,MONO_TEXT_SEG
put_seg:
                mov     es,ax
                mov     ax,[bp+6]               ; row
                mov     cl,4
                shl     ax,cl
                mov     di,ax                   ; row * 16
                mov     cl,2
                shl     ax,cl
                add     di,ax                   ; + row * 64
                add     di,[bp+8]               ; + col
                shl     di,1                    ; two bytes a cell
                mov     al,[bp+0Ah]
                mov     ah,[bp+0Ch]
                mov     word ptr es:[di],ax
                pop     di
                pop     si
                pop     bp
                ret
_text_cell_put  endp

;  int far text_cell_get(int row, int col)                      10d7:2a0b
;
;  The text screen's cell at row, col, found as text_cell_put finds it:
;  the character in al, the attribute in ah.  Nothing in DQK.EXE calls it.
_text_cell_get  proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,BIOS_DATA
                mov     es,ax
                mov     si,EQUIPMENT
                mov     bx,word ptr es:[si]
                mov     ax,COLOR_TEXT_SEG
                and     bx,EQUIP_VIDEO
                cmp     bx,EQUIP_MONO
                jne     get_seg
                mov     ax,MONO_TEXT_SEG
get_seg:
                mov     es,ax
                mov     ax,[bp+6]
                mov     cl,4
                shl     ax,cl
                mov     di,ax
                mov     cl,2
                shl     ax,cl
                add     di,ax
                add     di,[bp+8]
                shl     di,1
                mov     ax,word ptr es:[di]
                pop     di
                pop     si
                pop     bp
                ret
_text_cell_get  endp

;  long far clock_bios(int ah, int dx, int cx)                  10d7:2a46
;
;  INT 1Ah; returns cx:dx as a long (dx:ax), the tick count for ah = 0,
;  which is how dispinit.c calls it; lowio.c also calls it with ah = 1, to
;  set the count.
_clock_bios     proc    far
                push    bp
                mov     bp,sp
                mov     ah,[bp+6]
                mov     dx,[bp+8]
                mov     cx,[bp+0Ah]
                int     BIOS_CLOCK
                mov     ax,dx
                mov     dx,cx
                pop     bp
                ret
_clock_bios     endp

;  int far printer_bios(int ah, int dx, int al)                 10d7:2a5e
;
;  INT 17h, dx = the printer number; returns ax (the status in ah).
;  Nothing in DQK.EXE calls it.
_printer_bios   proc    far
                push    bp
                mov     bp,sp
                mov     ah,[bp+6]
                mov     dx,[bp+8]
                mov     al,[bp+0Ah]
                int     BIOS_PRINTER
                pop     bp
                ret
_printer_bios   endp

_TEXT           ends
                end
