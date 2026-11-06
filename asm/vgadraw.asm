;  DQK.EXE resident frame 10d7: three far VGA routines -- a DAC palette
;  load, a row blit that skips a transparent key byte, and a two-colour
;  8x8 character cell for the unchained 256-colour mode ("xvga",
;  video_mode 15).  EXE 0x005266..0x0053bb, 341 bytes,
;  10d7:20f6..10d7:224b.  pixrow.asm ends at 0x5266; lzcodec.asm starts at
;  0x53bb.
;
;  Why these routines are one file is a choice, not a measurement: the
;  frame has no far call between its own routines, relocation, pad byte or
;  change of house style at 20f6 or 224b.  They are grouped as the code
;  that writes VGA ports or the screen (draw_seg:screen_addr) directly,
;  between the glyph row reworkers and the decompressors.
;
;  House style: C far entry, bp frame, arguments from [bp+6], DS = SS =
;  DGROUP on entry, DGROUP read through SS once DS points elsewhere.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups: the range
;  holds no MZ relocation and the object makes none.  The DGROUP words are
;  externs the build resolves to their offsets.

                .8086

DGROUP          group   _DATA
_DATA           segment word public 'DATA'
                extrn   _draw_seg:word          ; ca33: vidmouse.c, the segment drawn into
                extrn   _text_bytes:word        ; cb6c: vidmouse.c, bytes per scan line
                extrn   _screen_addr:word       ; cb85: vidmouse.c, the first byte to draw
_DATA           ends

INPUT_STATUS    equ     3DAh                    ; VGA input status 1
VRETRACE        equ     8                       ; its vertical retrace bit
DAC_WRITE       equ     3C8h                    ; DAC write index; data at 3C9h
SEQ_INDEX       equ     3C4h                    ; sequencer index; data at 3C5h
MAP_MASK        equ     2                       ; sequencer: plane write enable

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT, ds:DGROUP, ss:DGROUP

                public  _dac_load, _keyed_blit, _xvga_char

;  void far dac_load(int first, int count, int wait, void far *rgb)
;                                                               10d7:20f6
;
;  Loads `count` DAC registers from `first` on with the red, green, blue
;  bytes at rgb (count = 0 loads 65536 triples; only first's low byte and
;  wait's low byte are read).  With wait nonzero it first waits for the
;  start of a vertical retrace: while the retrace bit is set, then until it
;  is.  Interrupts are off from before that wait to the end.
_dac_load       proc    far
                push    bp
                mov     bp,sp
                push    si
                mov     ah,[bp+6]               ; ah: first
                mov     cx,[bp+8]               ; cx: count
                mov     bl,[bp+0Ah]             ; bl: wait
                push    ds
                lds     si,dword ptr [bp+0Ch]
                cld
                mov     dx,INPUT_STATUS
                or      bl,bl
                cli
                je      dl_load
dl_in_retrace:
                in      al,dx
                test    al,VRETRACE
                jne     dl_in_retrace
dl_no_retrace:
                in      al,dx
                test    al,VRETRACE
                je      dl_no_retrace
dl_load:
                mov     al,ah
                mov     dx,DAC_WRITE
                out     dx,al
                inc     dx                      ; the DAC data port
dl_triple:
                lodsb
                out     dx,al                   ; red
                lodsb
                out     dx,al                   ; green
                lodsb
                out     dx,al                   ; blue
                loop    dl_triple
                sti
                pop     ds
                pop     si
                pop     bp
                ret
_dac_load       endp

;  void far keyed_blit(char far *src, int rows, int width, int skip,
;                      char key)                                10d7:212e
;
;  Copies `rows` rows of `width` bytes from src to screen_addr in draw_seg,
;  one scan line (text_bytes, no bank wrap) apart, leaving the screen byte
;  as it is wherever the source byte equals key.  src steps width + skip
;  bytes a row.  DS is not saved: it is reloaded from SS at the end.
;  glyph.c calls it with key 0FFh, once per plane.
_keyed_blit     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                mov     si,[bp+6]
                mov     ax,[bp+8]
                mov     ds,ax                   ; ds:si: src
                mov     dx,[bp+0Ch]             ; dx: width
                mov     bx,[bp+0Eh]             ; bx: skip
                mov     ah,[bp+10h]             ; ah: key
kb_row:
                mov     cx,dx
                push    di
kb_byte:
                lodsb
                cmp     ah,al
                je      kb_clear
                stosb
                loop    kb_byte
kb_row_end:
                pop     di
                add     si,bx
                add     di,ss:_text_bytes       ; one scan line down
                dec     word ptr [bp+0Ah]
                jne     kb_row
                mov     ax,ss
                mov     ds,ax                   ; DS = DGROUP again
                pop     di
                pop     si
                pop     bp
                ret
kb_clear:
                inc     di                      ; the key: keep the screen byte
                loop    kb_byte
                jmp     kb_row_end
_keyed_blit     endp

;  PIXEL plane -- one pixel of _xvga_char: enables `plane` alone for
;  writing, shifts the row's next bit out of ah and stores bh (the bit
;  set) or bl (clear) at es:di.  dx = SEQ_INDEX on entry and exit.
PIXEL           macro   plane
                local   px_set, px_done
                mov     al,MAP_MASK
                out     dx,al
                inc     dx
                mov     al,plane
                out     dx,al
                dec     dx
                shl     ah,1
                jb      px_set
                mov     es:[di],bl
                jmp     px_done
px_set:
                mov     es:[di],bh
px_done:
                endm

;  void far xvga_char(int fore, int back, unsigned char far *bits)
;                                                               10d7:2171
;
;  Draws an 8x8 cell at screen_addr in draw_seg in the unchained 256-colour
;  mode: the eight bytes at bits are its rows, high bit leftmost; a set bit
;  draws fore's low byte, a clear one back's.  Pixel x of a row is plane
;  x mod 4 of byte x/4, so a row is two bytes, each written four times
;  through the sequencer's map mask; rows are text_bytes apart.  The map
;  mask is left at plane 3.  vidmouse.c's character drawer calls it in
;  video_mode 15.
_xvga_char      proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                mov     bh,[bp+6]               ; bh: fore
                mov     bl,[bp+8]               ; bl: back
                push    ds
                lds     si,dword ptr [bp+0Ah]
                mov     dx,SEQ_INDEX
                mov     cx,8
xc_row:
                lodsb
                mov     ah,al                   ; ah: the row's bits
                PIXEL   1                       ; pixels 0..3: the first byte
                PIXEL   2
                PIXEL   4
                PIXEL   8
                inc     di
                PIXEL   1                       ; pixels 4..7: the second
                PIXEL   2
                PIXEL   4
                PIXEL   8
                dec     di
                add     di,ss:_text_bytes       ; one scan line down
                dec     cx
                je      xc_done
                jmp     xc_row                  ; out of short range
xc_done:
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_xvga_char      endp

_TEXT           ends
                end
