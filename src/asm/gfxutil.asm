;  DQK.EXE resident frame 10d7, after lowlevel.asm: five small far routines
;  the screen and resource code call -- a masked pattern column, the huge
;  pointer add, an indexed video register write, a run-length row drawer
;  with its near row skipper, and a two-buffer byte swap.  EXE
;  0x003ea1..0x003fa5, 260 bytes, 10d7:0d31..10d7:0e35.  lowlevel.asm ends
;  at 0x3ea1; dissolve.asm starts at 0x3fa5.
;
;  Hand assembly in the house style of lowlevel.asm (C far entry, arguments
;  from [bp+6], DS = SS = DGROUP on entry, DGROUP read through SS once DS
;  points elsewhere).  Why these routines are one file is a choice, not a
;  measurement: between 10d7:0009 and 10d7:1f37 the frame has no far call
;  within the frame, pad byte or change of style to mark a source-file
;  boundary.
;  They are grouped as the small drawing helpers ahead of the full-screen
;  dissolves; the only tie the bytes prove is runs_draw's near calls to
;  runs_skip.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups: the range
;  holds no MZ relocation and the object makes none.  The DGROUP words are
;  externs the build resolves to their offsets.

                .8086

DGROUP          group   _DATA
_DATA           segment word public 'DATA'
                extrn   _draw_seg:word          ; ca33: vidmouse.c, the segment drawn into
                extrn   _text_bytes:word        ; cb6c: vidmouse.c, bytes per scan line
                extrn   _base_offset:word       ; cb80: vidmouse.c, added at a bank's end
                extrn   _extent_text:word       ; cb83: vidmouse.c, a bank's end offset
                extrn   _screen_addr:word       ; cb85: vidmouse.c, the first byte to draw
_DATA           ends

PATTERN_MASK    equ     1Eh                     ; 16 pattern words, as a byte offset

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT, ds:DGROUP, ss:DGROUP

                public  _pattern_column, _huge_add, _reg_write
                public  _runs_draw, runs_skip, _mem_swap

;  void far pattern_column(int rows, int mask, int far *pattern, int phase)
;                                                               10d7:0d31
;
;  Merges one word column of a 16-word pattern into the screen, `rows` rows
;  down from screen_addr in draw_seg: each screen word keeps its bits outside
;  `mask` and takes the pattern word's bits inside it.  The pattern word is
;  pattern[phase], phase counting on modulo 16 row by row.  The scan-line step
;  is lowlevel.asm's row_advance written inline (text_bytes, plus base_offset
;  at a bank's end).  vidmouse.c calls it for the left and right edge words
;  of a pattern fill.
_pattern_column proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                mov     dx,[bp+8]               ; dx: the mask
                push    ds
                lds     si,dword ptr [bp+0Ah]   ; ds:si: the pattern
                mov     bx,[bp+0Eh]
                shl     bx,1
                and     bx,PATTERN_MASK         ; bx: the phase, as a byte offset
pc_row:
                mov     ax,dx
                not     ax
                and     ax,es:[di]              ; the screen outside the mask ...
                mov     cx,[bx+si]
                and     cx,dx                   ; ... and the pattern inside it
                or      ax,cx
                mov     es:[di],ax
                inc     bx
                inc     bx
                and     bl,PATTERN_MASK
                add     di,ss:_text_bytes       ; one scan line down
                cmp     di,ss:_extent_text
                jb      pc_next
                add     di,ss:_base_offset
pc_next:
                dec     word ptr [bp+6]
                jne     pc_row
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_pattern_column endp

;  void far *far huge_add(void far *p, long n)                  10d7:0d7d
;
;  p + n as a normalised huge pointer: the 20-bit sum of p's offset and n is
;  split into an offset 0..15 (ax) and a paragraph count added to p's
;  segment (dx).  Bits 20..31 of the sum are dropped by the shl.
_huge_add       proc    far
                push    bp
                mov     bp,sp
                mov     dx,[bp+0Ch]
                mov     ax,[bp+0Ah]
                add     ax,[bp+6]               ; dx:ax = n + offset
                adc     dx,0
                mov     bx,ax
                and     ax,0Fh                  ; the new offset
                mov     cl,4
                shr     bx,cl
                mov     cl,12
                shl     dx,cl
                add     dx,bx                   ; the sum in paragraphs ...
                add     dx,[bp+8]               ; ... plus the segment
                pop     bp
                ret
_huge_add       endp

;  void far reg_write(int port, int index, int value)           10d7:0da0
;
;  Writes `index` to `port` and `value` to port+1 with interrupts off (the
;  flags, and so the interrupt state, are restored after): one register of
;  an indexed video register file, the sequencer (3C4h), graphics controller
;  (3CEh) or CRT controller (3D4h, 3B4h) at the C callers.  Two NOPs between
;  the outs.
_reg_write      proc    far
                push    bp
                mov     bp,sp
                mov     dx,[bp+6]
                mov     al,[bp+8]
                pushf
                cli
                out     dx,al                   ; the index ...
                mov     al,[bp+0Ah]
                inc     dx
                nop
                nop
                out     dx,al                   ; ... and the value
                popf
                pop     bp
                ret
_reg_write      endp

;  char far *far runs_draw(int top, int bottom, int rows, int left,
;                          int width, char far *p)              10d7:0db6
;
;  Draws `rows` rows of run-coded bytes at screen_addr in draw_seg, one scan
;  line (text_bytes, no bank wrap) apart, after skipping `top` coded rows and
;  before skipping `bottom` more; returns p past everything consumed.  A
;  coded row is a list of signed counts ended by 0: n > 0 copies the n bytes
;  that follow, n < 0 steps -n bytes over the screen.  `left` is not read
;  and `width` is loaded into bx and not used (glyph.c's blit_runs passes
;  both).
_runs_draw      proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                mov     es,_draw_seg
                mov     di,_screen_addr
                lds     si,dword ptr [bp+10h]
                mov     bx,[bp+0Eh]             ; width: not used
                xor     ah,ah
                xor     ch,ch
                mov     dx,[bp+6]
                call    runs_skip               ; past `top` rows
                mov     dx,[bp+0Ah]
rd_row:
                push    di
rd_count:
                lodsb
                or      al,al
                je      rd_row_end
                jle     rd_skip
                mov     cl,al                   ; n > 0: copy n bytes
                rep     movsb
                jmp     rd_count
rd_skip:
                neg     al                      ; n < 0: step over -n
                add     di,ax
                jmp     rd_count
rd_row_end:
                pop     di
                add     di,ss:_text_bytes
                dec     dx
                jne     rd_row
                mov     dx,[bp+8]
                call    runs_skip               ; past `bottom` rows
                mov     ax,si
                mov     dx,ds
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_runs_draw      endp

;  runs_skip                                                    10d7:0e03
;
;  Near, register interface: ds:si = coded rows, dx = how many to skip,
;  ah = 0; returns si past them.  A positive count skips its data bytes, a
;  negative one has none.  Clobbers al and dx.
runs_skip       proc    near
                or      dx,dx
                je      rs_done
rs_count:
                lodsb
                or      al,al
                je      rs_row_end
                jle     rs_count
                add     si,ax
                jmp     rs_count
rs_row_end:
                dec     dx
                jne     rs_count
rs_done:
                ret
runs_skip       endp

;  void far mem_swap(void far *a, void far *b, unsigned n)      10d7:0e16
;
;  Exchanges n bytes between a and b (n = 0 swaps 65536).  glyphfx.c uses
;  it to swap plane blocks of a glyph it mirrors.
_mem_swap       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                push    es
                lds     si,dword ptr [bp+6]
                les     di,dword ptr [bp+0Ah]
                mov     cx,[bp+0Eh]
ms_byte:
                mov     al,es:[di]
                xchg    al,[si]
                inc     si
                stosb
                loop    ms_byte
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_mem_swap       endp

_TEXT           ends
                end
