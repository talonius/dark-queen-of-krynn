;  DQK.EXE resident frame 10d7: six far routines glyphfx.c calls to
;  rework the pixel rows of a glyph in place -- four horizontal mirrors,
;  one per pixel size (1, 2, 4 and 8 bits), and two colour translations,
;  one for byte-per-pixel rows and one for four-plane 16-colour rows.  EXE
;  0x0050a7..0x005266, 447 bytes, 10d7:1f37..10d7:20f6.  pcmplay.asm ends
;  at 0x50a7; vgadraw.asm starts at 0x5266.
;
;  Why these routines are one file is a choice, not a measurement: the
;  frame has no far call between its own routines, relocation, pad byte or
;  change of house style at 1f37 or 20f6.  They are grouped because one C
;  module calls all six and no other module calls any of them, and the four
;  mirrors share their first pass byte for byte (ROW_REVERSE below).
;
;  House style: C far entry, bp frame, arguments from [bp+6], si/di/ds
;  saved.  glyphfx.c picks the mirror by mouse_idle()'s 3, 2, 1, 0 (the
;  1-, 2-, 4- and 8-bit routines) and calls the 8-bit one alone on is_xvga.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups: the range
;  holds no MZ relocation and the object makes none.

                .8086

PAIR_LOW        equ     33h                     ; bits 0-1 and 4-5 of a byte
PAIR_HIGH       equ     0CCh                    ; bits 2-3 and 6-7 of a byte

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _row_mirror1, _row_mirror2, _row_mirror4, _row_mirror8
                public  _row_recolor, _plane_recolor

;  ROW_REVERSE -- the first pass of every mirror, after `push ds`.  Loads
;  ds = es = p's segment, bx = n, and reverses the order of the n bytes at
;  p by swapping them from both ends, n/2 swaps (none for n < 2).  Leaves
;  the direction flag clear.
ROW_REVERSE     macro
                local   rr_swap, rr_done
                mov     si,[bp+6]
                mov     di,si
                mov     ax,[bp+8]
                mov     ds,ax
                mov     es,ax
                mov     bx,[bp+0Ah]             ; bx: n
                mov     cx,bx
                add     si,cx
                dec     si                      ; si: the last byte
                cld
                sar     cx,1
                je      rr_done
rr_swap:
                mov     al,[si]
                mov     ah,[di]
                mov     [si],ah
                dec     si
                stosb
                loop    rr_swap
rr_done:
                endm

;  void far row_mirror1(char far *p, int n)                     10d7:1f37
;
;  Mirrors a row of n bytes of 1-bit pixels: reverses the bytes, then the
;  eight bits of each byte (rotated out of ah low bit first, into al).
_row_mirror1    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                ROW_REVERSE
                mov     cx,bx
                mov     di,[bp+6]
m1_byte:
                mov     ah,[di]
                rept    8
                ror     ah,1
                rcl     al,1
                endm
                stosb
                loop    m1_byte
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_row_mirror1    endp

;  void far row_mirror2(char far *p, int n)                     10d7:1f8f
;
;  Mirrors a row of n bytes of 2-bit pixels: reverses the bytes, then the
;  four bit pairs of each byte (rotate by two each way and merge, which
;  exchanges pairs 0/3 and 1/2).
_row_mirror2    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                ROW_REVERSE
                mov     cx,bx
                mov     di,[bp+6]
m2_byte:
                mov     al,[di]
                mov     ah,al
                rol     al,1
                rol     al,1
                and     al,PAIR_LOW
                ror     ah,1
                ror     ah,1
                and     ah,PAIR_HIGH
                or      al,ah
                stosb
                loop    m2_byte
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_row_mirror2    endp

;  void far row_mirror4(char far *p, int n)                     10d7:1fd8
;
;  Meant to mirror a row of 4-bit pixels, but as shipped the second pass
;  rotates ah, not the al it stores, so it writes every byte back
;  unchanged: the row comes out byte-reversed with each byte's two pixels
;  in their old order.  Four `ror al,1` (a nibble swap) would complete the
;  mirror.  Whether the game ever takes this path is not measured here.
_row_mirror4    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                ROW_REVERSE
                mov     cx,bx
                mov     di,[bp+6]
m4_byte:
                mov     al,[di]
                ror     ah,1                    ; ah, not al: no effect on
                ror     ah,1                    ; the byte stored
                ror     ah,1
                ror     ah,1
                stosb
                loop    m4_byte
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_row_mirror4    endp

;  void far row_mirror8(char far *p, int n)                     10d7:2018
;
;  Mirrors a row of n byte-per-pixel bytes: the byte reversal alone.
_row_mirror8    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                ROW_REVERSE
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_row_mirror8    endp

;  void far row_recolor(char far *p, int n, unsigned char far *tab)
;                                                               10d7:2046
;
;  Replaces each of the n bytes at p with tab[byte].  Nothing for n = 0.
_row_recolor    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     di,[bp+6]
                mov     ax,[bp+8]
                mov     es,ax
                mov     cx,[bp+0Ah]
                or      cx,cx
                je      rc_done
                push    ds
                lds     si,dword ptr [bp+0Ch]
                xor     bx,bx
                cld
rc_byte:
                mov     bl,es:[di]
                mov     al,[bx+si]
                stosb
                loop    rc_byte
                pop     ds
rc_done:
                pop     di
                pop     si
                pop     bp
                ret
_row_recolor    endp

;  void far plane_recolor(char far *p, int stride, int count,
;                         unsigned char far *tab)               10d7:206e
;
;  Translates the colours of 16-colour planar pixels: for each of `count`
;  byte columns from p on, the bytes at p, p+stride, p+2*stride and
;  p+3*stride are planes 0..3 of eight pixels, and every pixel's colour c
;  (plane k's bit as bit k of c) becomes the low four bits of tab[c].
;  Only tab's offset is read: the table is addressed through DS, so it must
;  lie in DGROUP.  glyphfx.c passes stride = count = one plane's size.
;
;  How: bl gathers pixel 0's colour by rotating each plane byte right
;  through carry; then eight times, the translated colour is rotated back
;  into the planes' top bits by rcr bl / rcr plane in turns, which also
;  pushes the next pixel's bits into bl, and the shr's bring them down to
;  index tab again.  After eight steps every byte has turned full circle
;  (the shipped bytes, run under emulation with full 8-bit tables, match a
;  per-pixel tab[c] & 0Fh reference).  The ninth
;  lookup's result is discarded.
_plane_recolor  proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+8]
                mov     es,ax
                mov     ax,[bp+0Ch]
                or      ax,ax
                je      pr_done
                push    ds
                mov     si,[bp+0Eh]             ; tab, in DGROUP
                cld
pr_column:
                mov     di,[bp+6]
                mov     bx,[bp+0Ah]             ; bx: the stride
                mov     al,es:[di]              ; al: plane 0
                add     di,bx
                mov     ah,es:[di]              ; ah: plane 1
                add     di,bx
                mov     dl,es:[di]              ; dl: plane 2
                add     di,bx
                mov     dh,es:[di]              ; dh: plane 3
                xor     bx,bx
                mov     cx,8
                rcr     dh,1                    ; pixel 0's colour into bl
                rcl     bl,1
                rcr     dl,1
                rcl     bl,1
                rcr     ah,1
                rcl     bl,1
                rcr     al,1
                rcl     bl,1
                mov     bl,[bx+si]
pr_pixel:
                rcr     bl,1                    ; new colour out, next
                rcr     al,1                    ; pixel's bits in
                rcr     bl,1
                rcr     ah,1
                rcr     bl,1
                rcr     dl,1
                rcr     bl,1
                rcr     dh,1
                rcr     bl,1
                rept    4
                shr     bl,1
                endm
                mov     bl,[bx+si]
                loop    pr_pixel
                mov     di,[bp+6]
                mov     bx,[bp+0Ah]
                mov     es:[di],al
                add     di,bx
                mov     es:[di],ah
                add     di,bx
                mov     es:[di],dl
                add     di,bx
                mov     es:[di],dh
                inc     word ptr [bp+6]         ; the next column
                dec     word ptr [bp+0Ch]
                jne     pr_column
                pop     ds
pr_done:
                pop     di
                pop     si
                pop     bp
                ret
_plane_recolor  endp

_TEXT           ends
                end
