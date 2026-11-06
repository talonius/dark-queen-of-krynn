;  DQK.EXE resident frame 10d7: the dissolves -- six far routines that copy
;  one screen page onto another a pixel (or byte) at a time in a scrambled
;  order, one per screen format.  EXE 0x003fa5..0x00420f, 618 bytes,
;  10d7:0e35..10d7:109f.  gfxutil.asm ends at 0x3fa5; pcmplay.asm starts at
;  0x420f with its jump table.
;
;  vidmouse.c's page_copy calls each with (mem_top_seg, display_seg) and
;  picks one by video_mode: 0e35 for 14, 0e64 for 15, 0ebf for 12 and 13,
;  0f27 for 5..7, 0fdc for 1..4, 1034 for 8 and 11.  Each loops 65536 times
;  over a position x that starts at 0 and steps x = (x + 1) * K mod 65536,
;  K = 313 (0e35, 0e64, 0ebf) or 321 (0f27, 0fdc, 1034).  Both are full-
;  period linear congruential steps (K - 1 divisible by 4, K odd), so x
;  takes each of the 65536 values once, in an order that looks random, and
;  every unit is copied once -- except in dissolve_planes, which spends one
;  count placing x = 0 and so never copies unit FFFFh (see it).  The routines split x into a byte offset
;  and a pixel or plane in different ways (see each).
;
;  Hand assembly: C far entry, arguments from [bp+6], bp reused as a
;  register, DS and ES pointed at the two pages and restored from SS
;  (DGROUP) on the way out.  Two mask tables live in the code segment
;  before the routines that read them: bit_masks (0f1f) before
;  dissolve_bits, pixel2_masks (0fd8) before dissolve_2bpp, which
;  dissolve_2bank reads too.  Why these six are one file is a choice, not
;  a measurement (see gfxutil.asm's header); what ties them is the shared
;  table and the shared shape.
;
;  Assembled by TASM 3.1, /ml, single pass: a forward jmp is EB xx 90, as
;  shipped.  No segment fixups: the range holds no MZ relocation and the
;  object makes none.

                .8086

SEQ_INDEX       equ     3C4h                    ; VGA/EGA sequencer, index port
SEQ_MAP_MASK    equ     2                       ;   index 2: planes written
SEQ_DATA_LO     equ     0C5h                    ;   dl for 3C5h, its data port
GC_INDEX        equ     3CEh                    ; graphics controller, index port
GC_READ_MAP     equ     4                       ;   index 4: plane read
GC_BIT_MASK     equ     8                       ;   index 8: bits written
GC_DATA_LO      equ     0CFh                    ;   dl for 3CFh, its data port
STEP_313        equ     139h                    ; the multiplier K of 0e35..0ebf
STEP_321        equ     141h                    ; the multiplier K of 0f27..1034

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _dissolve_bytes, _dissolve_planes, _dissolve_nibbles
                public  _dissolve_bits, _dissolve_2bpp, _dissolve_2bank
                public  bit_masks, pixel2_masks

;  void far dissolve_bytes(unsigned from_seg, unsigned to_seg)  10d7:0e35
;
;  Copies the 65536 bytes of from_seg to to_seg, byte x at a time (K 313).
_dissolve_bytes proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                mov     ds,ax
                mov     ax,[bp+8]
                mov     es,ax
                xor     ax,ax
                mov     cx,ax                   ; 65536 bytes
                mov     si,ax
                mov     di,ax                   ; x = 0
                mov     bx,STEP_313
db_byte:
                movsb
                mov     ax,si                   ; x + 1 ...
                mul     bx                      ; ... times K
                mov     si,ax
                mov     di,ax
                loop    db_byte
                mov     ax,ss
                mov     ds,ax
                mov     es,ax
                pop     di
                pop     si
                pop     bp
                ret
_dissolve_bytes endp

;  void far dissolve_planes(unsigned from_seg, unsigned to_seg) 10d7:0e64
;
;  The same over four planes of 16K (K 313): x's low two bits pick the plane
;  -- read map select and map mask both set to it -- and x >> 2 the byte
;  offset.  Interrupts are off while the registers are set, per byte.
;  The first `loop` count goes to placing x = 0 before any byte moves, so
;  65535 units are copied: unit FFFFh (plane 3, offset 3FFFh) never is.
_dissolve_planes proc   far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                mov     ds,ax
                mov     ax,[bp+8]
                mov     es,ax
                xor     ax,ax
                mov     cx,ax                   ; 65536 bytes
                push    ax
                push    ax
                mov     bx,STEP_313
                jmp     dp_place                ; x = 0
dp_byte:
                movsb
                inc     ax
                mul     bx                      ; x = (x + 1) * K
                push    ax
                push    ax
                shr     ax,1
                shr     ax,1
dp_place:
                mov     si,ax                   ; the offset, x >> 2
                mov     di,ax
                pop     ax
                and     al,3
                mov     ah,al                   ; ah: the plane, x & 3
                mov     dx,GC_INDEX
                mov     al,GC_READ_MAP
                cli
                out     dx,al
                inc     dx
                mov     al,ah
                out     dx,al                   ; read that plane ...
                push    cx
                mov     cl,ah
                mov     al,1
                shl     al,cl
                mov     ah,al
                pop     cx
                mov     dx,SEQ_INDEX
                mov     al,SEQ_MAP_MASK
                out     dx,al
                inc     dx
                mov     al,ah
                out     dx,al                   ; ... and write it
                sti
                pop     ax                      ; ax: x again
                loop    dp_byte
                mov     ax,ss
                mov     ds,ax
                mov     es,ax
                pop     di
                pop     si
                pop     bp
                ret
_dissolve_planes endp

;  void far dissolve_nibbles(unsigned from_seg, unsigned to_seg)
;                                                               10d7:0ebf
;
;  The same over the 65536 4-bit pixels of 32K, two to a byte (K 313): x
;  read as signed, x >= 0 is the high nibble of byte x, x < 0 the low nibble
;  of byte x & 7FFFh.  Each nibble is merged into the destination byte, the
;  other nibble kept.  The sign test picks the next pixel's half, so each
;  half has its own copy step and its own loop.
_dissolve_nibbles proc  far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                mov     ds,ax
                mov     ax,[bp+8]
                mov     es,ax
                xor     ax,ax
                mov     cx,ax                   ; 65536 pixels
                mov     si,ax
                mov     di,ax
                mov     bp,ax                   ; bp: x = 0
                mov     bx,STEP_313
dn_high:
                lodsb
                and     al,0F0h
                mov     ah,al
                mov     al,es:[di]
                and     al,0Fh
                or      al,ah                   ; the source's high nibble in
dn_store:
                stosb
                mov     ax,bp
                inc     ax
                mul     bx                      ; x = (x + 1) * K
                or      ax,ax
                jl      dn_to_low
                mov     si,ax
                mov     di,ax
                mov     bp,ax
                loop    dn_high
                jmp     dn_done
dn_low:
                lodsb
                and     al,0Fh
                mov     ah,al
                mov     al,es:[di]
                and     al,0F0h
                or      al,ah                   ; the source's low nibble in
                jmp     dn_store
dn_to_low:
                mov     bp,ax
                and     ah,7Fh
                mov     si,ax
                mov     di,ax
                loop    dn_low
dn_done:
                mov     ax,ss
                mov     ds,ax
                mov     es,ax
                pop     di
                pop     si
                pop     bp
                ret
_dissolve_nibbles endp

;  The graphics controller's bit mask for pixel x & 7, leftmost first.
bit_masks       db      80h,40h,20h,10h,08h,04h,02h,01h ; 10d7:0f1f

;  dissolve_bits: copy the latched byte of plane n with only the bit mask's
;  pixel written.  Reading es:[di] loads the latches, so the store changes
;  only the masked bit; stosb's di step is undone.
PLANE_COPY      macro
                mov     al,[si]
                mov     ah,es:[di]
                stosb
                dec     di
                endm

;  dissolve_bits: read plane n, write plane n (the GC index is left at the
;  read map select, the sequencer's at the map mask).
NEXT_PLANE      macro   n
                mov     dl,GC_DATA_LO
                mov     al,n
                out     dx,al
                mov     dl,SEQ_DATA_LO
                mov     al,1 shl n
                out     dx,al
                endm

;  void far dissolve_bits(unsigned from_seg, unsigned to_seg)   10d7:0f27
;
;  The same over the 65536 pixels of four 8K planes (K 321): x & 7 picks the
;  pixel's bit through bit_masks, x >> 3 the byte; each pixel is copied on
;  all four planes in turn.  It counts with dec, jcxz and a backward short
;  jmp rather than `loop`, which would have reached.  On the way out the bit
;  mask is set back to FFh.
_dissolve_bits  proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                mov     ds,ax
                mov     ax,[bp+8]
                mov     es,ax
                xor     ax,ax
                mov     cx,ax                   ; 65536 pixels
                mov     si,ax
                mov     di,ax
                mov     bp,ax                   ; bp: x = 0
                mov     bx,80h                  ; bl: pixel 0's mask
                mov     dx,SEQ_INDEX
                mov     al,SEQ_MAP_MASK
                out     dx,al
dbi_pixel:
                mov     dx,GC_INDEX
                mov     al,GC_BIT_MASK
                cli
                out     dx,al
                inc     dx
                mov     al,bl
                out     dx,al                   ; write only this pixel
                dec     dx
                mov     al,GC_READ_MAP
                out     dx,al
                inc     dx
                mov     al,0
                out     dx,al                   ; read plane 0 ...
                mov     dl,SEQ_DATA_LO
                mov     al,1
                out     dx,al                   ; ... write plane 0
                PLANE_COPY
                NEXT_PLANE 1
                PLANE_COPY
                NEXT_PLANE 2
                PLANE_COPY
                NEXT_PLANE 3
                sti
                PLANE_COPY
                mov     ax,bp
                inc     ax
                mov     dx,STEP_321
                mul     dx                      ; x = (x + 1) * K
                mov     bp,ax
                mov     bl,al
                and     bl,7
                mov     bl,cs:bit_masks[bx]
                mov     ax,bp
                clc
                rcr     ax,1
                sar     ax,1
                sar     ax,1                    ; x >> 3, unsigned
                mov     si,ax
                mov     di,ax
                dec     cx
                jcxz    dbi_done
                jmp     dbi_pixel
dbi_done:
                cli
                mov     dx,GC_INDEX
                mov     al,GC_BIT_MASK
                out     dx,al
                inc     dx
                mov     al,0FFh
                out     dx,al
                sti
                mov     ax,ss
                mov     ds,ax
                mov     es,ax
                pop     di
                pop     si
                pop     bp
                ret
_dissolve_bits  endp

;  The masks of 2-bit pixel x & 3 in a byte, leftmost first.
pixel2_masks    db      0C0h,30h,0Ch,03h        ; 10d7:0fd8

;  void far dissolve_2bpp(unsigned from_seg, unsigned to_seg)   10d7:0fdc
;
;  The same over the 65536 2-bit pixels of 16K (K 321): x & 3 picks the
;  pixel's mask through pixel2_masks, x >> 2 the byte; the pixel is merged
;  into the destination byte.  dl holds the mask, dh its complement.
_dissolve_2bpp  proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                mov     ds,ax
                mov     ax,[bp+8]
                mov     es,ax
                xor     ax,ax
                mov     cx,ax                   ; 65536 pixels
                mov     si,ax
                mov     di,ax
                mov     bp,ax                   ; bp: x = 0
                mov     bx,ax
                mov     dx,3FC0h                ; pixel 0: mask C0h, keep 3Fh
d2_pixel:
                lodsb
                and     al,dl
                mov     ah,es:[di]
                and     ah,dh
                or      al,ah
                stosb
                mov     ax,bp
                inc     ax
                mov     dx,STEP_321
                mul     dx                      ; x = (x + 1) * K
                mov     bp,ax
                mov     bl,al
                and     bl,3
                mov     dl,cs:pixel2_masks[bx]
                mov     dh,dl
                not     dh
                mov     ax,bp
                clc
                rcr     ax,1
                sar     ax,1                    ; x >> 2, unsigned
                mov     si,ax
                mov     di,ax
                loop    d2_pixel
                mov     ax,ss
                mov     ds,ax
                mov     es,ax
                pop     di
                pop     si
                pop     bp
                ret
_dissolve_2bpp  endp

;  void far dissolve_2bank(unsigned from_seg, unsigned to_seg)  10d7:1034
;
;  dissolve_2bpp done on two bytes per pixel position: byte x >> 2 and the
;  byte 4000h above it, the same mask in both, so it covers 32K.
_dissolve_2bank proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                mov     ds,ax
                mov     ax,[bp+8]
                mov     es,ax
                xor     ax,ax
                mov     cx,ax                   ; 65536 pixel positions
                mov     si,ax
                mov     di,ax
                mov     bp,ax                   ; bp: x = 0
                mov     bx,ax
                mov     dx,3FC0h                ; pixel 0: mask C0h, keep 3Fh
d2b_pixel:
                lodsb
                and     al,dl
                mov     ah,es:[di]
                and     ah,dh
                or      al,ah
                stosb
                add     si,3FFFh                ; the byte 4000h above
                add     di,3FFFh
                lodsb
                and     al,dl
                mov     ah,es:[di]
                and     ah,dh
                or      al,ah
                stosb
                mov     ax,bp
                inc     ax
                mov     dx,STEP_321
                mul     dx                      ; x = (x + 1) * K
                mov     bp,ax
                mov     bl,al
                and     bl,3
                mov     dl,cs:pixel2_masks[bx]
                mov     dh,dl
                not     dh
                mov     ax,bp
                clc
                rcr     ax,1
                sar     ax,1                    ; x >> 2, unsigned
                mov     si,ax
                mov     di,ax
                loop    d2b_pixel
                mov     ax,ss
                mov     ds,ax
                mov     es,ax
                pop     di
                pop     si
                pop     bp
                ret
_dissolve_2bank endp

_TEXT           ends
                end
