;  DQK.EXE resident frame 10d7, its first source file: the tick handlers, the
;  BIOS keyboard, video-adapter and mouse calls, the scan-line blitters under
;  the screen code, the pixel-format converters, far memory moves, the byte
;  swaps, and the PackBits and prefix-code expanders the resource library
;  uses.  EXE 0x003179..0x003ea1, 3368 bytes, 10d7:0009..10d7:0d31.
;  10d7:0000..0008 is the tail of AIL (frame 100c, ail.asm, which ends at
;  0x3179).
;
;  Hand assembly, not Turbo C output: a register-interface near helper, bp
;  reused as a counter, data in the code segment.  The routines are C far
;  calls, arguments from [bp+6] (swap16 and swap32 read them through
;  bx = sp), result in ax or dx:ax; C code far-calls all of them but these:
;  row_advance (near, di in and out), timer_tick (AIL calls it through the
;  pointer sound.c registers), timer_isr (an interrupt handler),
;  mouse_handler (the mouse driver's far callback, registers in), and four
;  that nothing in the image calls -- planar_to_cga, planar_to_chunky,
;  planar_to_nibbles, joy_sample.
;
;  Why one file: three words live in the code segment, at 10d7:014d..0152:
;  isr_ds, the DGROUP segment the interrupt-time code loads into DS (written
;  by timer_install and mouse_handler_set, read by timer_tick, timer_isr and
;  mouse_handler at 0b8b), and old_int8, the int 8 vector timer_install
;  replaces.  The near helper row_advance (007d) is called from column_2bpp,
;  column_1bpp, blit_bytes, blit_masked_b, pattern_rows (041c) and rle_draw
;  (06f5).  Where the file ends, 0d31, is a cut chosen at a routine start:
;  nothing after it in the frame reaches into this range, and nothing here
;  reaches past it, so the bytes neither force nor forbid a longer file.
;
;  Assembled by TASM 3.1, /ml, single pass: a forward jmp takes three bytes,
;  EB xx 90, as it shipped.  The four segment fixups are the range's four MZ
;  relocations, all far calls into vidmouse.c (stored 12b7): 0x31c1 and
;  0x3368 (pointer_follow), 0x32ba (pointer_erase), 0x3d24 (mouse_event).
;  _DATA owns the initialized joystick state byte at a246; a247 is the
;  following alignment byte. Other DGROUP operands are externs the build
;  resolves to their offsets. The code
;  takes DS = SS = DGROUP on entry; the handlers load DS from isr_ds, and the
;  routines that point DS at a source read DGROUP through SS (ss: overrides,
;  hence `assume ss:DGROUP`) or through a segment register holding a saved
;  DGROUP.

                .8086

DGROUP          group   _DATA
_DATA           segment word public 'DATA'
                extrn   _ticks:word             ; cda6: lowio.c's long tick count
                extrn   _fire_latch:byte        ; cdb2: lowio.c, joystick buttons newly pressed
                extrn   _hide_count:word        ; cb57: vidmouse.c
                extrn   _show_count:word        ; cb70: vidmouse.c
                extrn   _draw_seg:word          ; ca33: vidmouse.c, the segment drawn into
                extrn   _text_bytes:word        ; cb6c: vidmouse.c, bytes per scan line
                extrn   _base_offset:word       ; cb80: vidmouse.c, added at a bank's end
                extrn   _extent_text:word       ; cb83: vidmouse.c, a bank's end offset
                extrn   _screen_addr:word       ; cb85: vidmouse.c, the first byte to draw
                public  _joy_last
_joy_last       db      0                      ; a246: port 201h as timer_isr last read it
                extrn   _joy_last_ail:byte      ; 002a: the same for timer_tick
                extrn   _column_mouse:word      ; cb55: vidmouse.c, the pointer's text column
                extrn   _mousey_now:word        ; cb77: vidmouse.c, the pointer's text row
                extrn   _buttons_now:word       ; cb89: vidmouse.c, the mouse buttons
_DATA           ends

                extrn   _pointer_follow:far     ; 12b7:1871, vidmouse.c
                extrn   _pointer_erase:far      ; 12b7:2271, vidmouse.c
                extrn   _mouse_event:far        ; 12b7:178c, vidmouse.c

GAME_PORT       equ     201h                    ; joystick: buttons in bits 4..7, active low
PIC_CMD         equ     20h                     ; 8259 command port
EOI             equ     20h                     ; non-specific end of interrupt
INT8_VECTOR     equ     8*4                     ; int 8 (IRQ 0, timer) in the vector table
KBD_READY       equ     01h                     ; int 16h: is a key waiting
KBD_READY_EXT   equ     11h                     ; int 16h: the same, enhanced keyboard
MOUSE_SET_HANDLER equ   0Ch                     ; int 33h: install a user event handler
EGA_INFO        equ     12h                     ; int 10h ah: alternate select
EGA_INFO_BL     equ     10h                     ;   bl: return EGA information
VGA_DISPLAY     equ     1A00h                   ; int 10h: read the display combination code
MOUSE_INT       equ     33h                     ; the mouse driver
PATTERN_MASK    equ     1Eh                     ; pattern_rows: 16 words, as a byte offset

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT, ds:DGROUP, ss:DGROUP

                public  _timer_tick, _bios_key, row_advance
                public  _column_2bpp, _column_1bpp, _pointer_hide
                public  isr_ds, old_int8
                public  _timer_install, _timer_isr, _mouse_handler_set
                public  _ega_query, _display_code
                public  _blit_words, _blit_bytes, _blit_masked_w, _blit_masked_b
                public  _fill_bytes, _fill_words, _mask_words
                public  _pattern_rows, _planar_to_cga, _planar_to_chunky
                public  _rle_draw, _expand_1to2, _expand_1to4, _expand_1to8
                public  _planar_to_nibbles, _fill_far_words, _huge_move, _far_move
                public  _cga_copy, _swap16, _swap32, _joy_sample, _grab_words
                public  _packbits_expand, _split_nibbles, _nibbles_to_planar
                public  _mouse_call, _mouse_call_regs, _mouse_handler
                public  _blit_shifted, _put_bytes, _grab_bytes, _huff_expand

;  void far timer_tick(void)                                    10d7:0009
;
;  The callback sound.c registers with AIL's timer (ail_register_timer, then
;  divisor 4000h).  AIL far-calls it, so it ends in retf, not iret, and leaves
;  the BIOS chain and the EOI to AIL.  It counts the tick in lowio.c's long
;  `ticks` and ORs the joystick buttons pressed since the last tick into
;  fire_latch.  On even ticks, unless pointer updates are held (hide_count
;  non-zero), it calls pointer_follow.  The test before the odd-tick retf sets flags that
;  AIL's caller (ail.asm, `call timer_callback[si]`) does not read; it is
;  timer_isr's chain test without the jump.  The previous port reading is
;  kept at DGROUP 002a, not at a246 as in timer_isr; who owns 002a is not
;  established.
_timer_tick     proc    far
                push    ds
                mov     ds,cs:isr_ds
                inc     _ticks
                jnz     tt_counted
                inc     _ticks+2
tt_counted:
                push    ax
                push    dx
                mov     dx,GAME_PORT
                in      al,dx
                mov     ah,al
                not     al                      ; buttons down now (active low) ...
                and     al,_joy_last_ail        ; ... that were up last time
                mov     _joy_last_ail,ah
                or      _fire_latch,al
                test    _ticks,1
                jz      tt_even
                pop     dx
                pop     ax
                test    _ticks,2
                pop     ds
                ret
tt_even:
                cmp     _hide_count,0
                jnz     tt_done
                push    bx
                push    cx
                push    si
                push    di
                push    es
                cld
                call    _pointer_follow
                pop     es
                pop     di
                pop     si
                pop     cx
                pop     bx
tt_done:
                pop     dx
                pop     ax
                pop     ds
                ret
_timer_tick     endp

;  int far bios_key(int fn)                                     10d7:005c
;
;  int 16h with ah = fn.  For the "is a key waiting" functions, 01h and 11h,
;  it returns -1 when one is (ZF clear) and 0 when none is; for any other
;  function it returns int 16h's ax.  lowio.c calls it with 0, 1 and 2.
_bios_key       proc    far
                push    bp
                mov     bp,sp
                mov     ah,[bp+6]
                cmp     ah,KBD_READY
                je      bk_ready
                cmp     ah,KBD_READY_EXT
                je      bk_ready
                int     16h
                pop     bp
                ret
bk_ready:
                int     16h
                jz      bk_none
                mov     ax,-1
                pop     bp
                ret
bk_none:
                xor     ax,ax
                pop     bp
                ret
_bios_key       endp

;  row_advance                                                  10d7:007d
;
;  Near, register interface: di = an offset in the screen; returns di moved
;  one scan line down.  It adds text_bytes, and once di reaches extent_text
;  (the end of an interleave bank) adds base_offset too.  Reads DGROUP through
;  SS, so callers may have DS elsewhere.  pattern_rows and rle_draw call it
;  too.
row_advance     proc    near
                add     di,ss:_text_bytes
                cmp     di,ss:_extent_text
                jb      ra_done
                add     di,ss:_base_offset
ra_done:
                ret
row_advance     endp

;  void far column_2bpp(char far *bits, int rows, int and_mask, int xor_mask,
;                       int odd_mask, int even_mask)            10d7:008f
;
;  Draws one byte of 1-bit pixels per row, `rows` rows down from screen_addr
;  in draw_seg, at two bits per pixel: each source bit is doubled into a
;  16-bit word (bit 7 lands in the word's top two bits).  The word is ANDed
;  with the low byte of odd_mask or even_mask by the parity of the remaining
;  row count (both halves), then with and_mask, XORed with xor_mask, and
;  stored high byte first.
_column_2bpp    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                push    ds
                lds     si,dword ptr [bp+6]
                mov     bh,[bp+10h]
                mov     dh,[bp+12h]
                mov     cx,[bp+0Ah]
c2_row:
                mov     bl,[si]
                inc     si
                rept    8                       ; each bit, twice, into ax from the top
                ror     bl,1
                rcr     ax,1
                sar     ax,1
                endm
                test    cx,1
                jz      c2_even
                and     al,bh
                and     ah,bh
                jmp     c2_masked
c2_even:
                and     al,dh
                and     ah,dh
c2_masked:
                and     ax,[bp+0Ch]
                xor     ax,[bp+0Eh]
                xchg    al,ah
                mov     es:[di],ax
                call    row_advance
                loop    c2_row
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_column_2bpp    endp

;  void far column_1bpp(char far *bits, int rows, int and_mask, int xor_mask,
;                       int odd_mask, int even_mask)            10d7:0102
;
;  column_2bpp at one bit per pixel: each source byte is ANDed with odd_mask
;  or even_mask by row parity, then with and_mask, XORed with xor_mask (low
;  bytes only) and stored.
_column_1bpp    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                push    ds
                lds     si,dword ptr [bp+6]
                mov     ah,[bp+0Ch]
                mov     bl,[bp+0Eh]
                mov     bh,[bp+10h]
                mov     dh,[bp+12h]
                mov     cx,[bp+0Ah]
c1_row:
                lodsb
                test    cx,1
                jz      c1_even
                and     al,bh
                jmp     c1_masked
c1_even:
                and     al,dh
c1_masked:
                and     al,ah
                xor     al,bl
                mov     es:[di],al
                call    row_advance
                loop    c1_row
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_column_1bpp    endp

;  void far pointer_hide(void)                                  10d7:0141
;
;  Counts one hide of the mouse pointer: decrements vidmouse.c's show_count
;  and erases the pointer (pointer_erase) when it reaches zero.  vidmouse.c's
;  22b7:2122 is the matching show.
_pointer_hide   proc    far
                dec     _show_count
                jnz     ph_done
                call    _pointer_erase
ph_done:
                ret
_pointer_hide   endp

;  Code-segment data, 10d7:014d..0152.
isr_ds          dw      0                       ; DGROUP, for timer_tick, timer_isr, mouse_handler
old_int8        label   dword                   ; the int 8 vector timer_install replaced
old_int8_off    dw      0
old_int8_seg    dw      0

;  void far timer_install(void interrupt (far *isr)(void))      10d7:0153
;
;  Records DS (DGROUP) in isr_ds for the handlers.  A non-zero offset: saves
;  the int 8 vector in old_int8 and installs `isr`.  A zero offset (only the
;  offset is tested): puts old_int8 back.  lowio.c's timer_rehook passes
;  timer_isr, timer_unhook passes 0.
_timer_install  proc    far
                push    bp
                mov     bp,sp
                push    si
                mov     cs:isr_ds,ds
                mov     bx,[bp+6]
                xor     ax,ax
                mov     es,ax
                mov     si,INT8_VECTOR
                or      bx,bx
                jnz     ti_install
                cli
                mov     ax,cs:old_int8_off
                mov     es:[si],ax
                inc     si
                inc     si
                mov     ax,cs:old_int8_seg
                mov     es:[si],ax
                jmp     ti_done
ti_install:
                mov     ax,es:[si]
                mov     cs:old_int8_off,ax
                inc     si
                inc     si
                mov     ax,es:[si]
                mov     cs:old_int8_seg,ax
                cli
                mov     ax,[bp+8]
                mov     es:[si],ax
                dec     si
                dec     si
                mov     es:[si],bx
ti_done:
                sti
                pop     si
                pop     bp
                ret
_timer_install  endp

;  void interrupt timer_isr(void)                               10d7:019e
;
;  The int 8 handler timer_install puts in while AIL's timer is not running
;  (lowio.c's timer_rehook sets timer 0 to four times the BIOS rate).  Counts
;  ticks and joystick presses as timer_tick does.  On ticks with bits 0 and 1
;  both set, one in four, it jumps to the old int 8 handler, which keeps the
;  BIOS clock and sends the EOI.  On the other odd ticks it sends the EOI and
;  returns.  On even ticks it sends the EOI, enables interrupts, and calls
;  pointer_follow unless pointer updates are held (hide_count).
_timer_isr      proc    far
                push    ds
                mov     ds,cs:isr_ds
                inc     _ticks
                jnz     ti_counted
                inc     _ticks+2
ti_counted:
                push    ax
                push    dx
                mov     dx,GAME_PORT
                in      al,dx
                mov     ah,al
                not     al
                and     al,_joy_last
                mov     _joy_last,ah
                or      _fire_latch,al
                test    _ticks,1
                jz      ti_even
                pop     dx
                pop     ax
                test    _ticks,2
                pop     ds
                jz      ti_eoi
                jmp     cs:old_int8             ; chain to the BIOS
ti_eoi:
                push    ax
                mov     al,EOI
                out     PIC_CMD,al
                pop     ax
                iret
ti_even:
                mov     al,EOI
                out     PIC_CMD,al
                sti
                cmp     _hide_count,0
                jnz     ti_return
                push    bx
                push    cx
                push    si
                push    di
                push    es
                cld
                call    _pointer_follow
                pop     es
                pop     di
                pop     si
                pop     cx
                pop     bx
ti_return:
                pop     dx
                pop     ax
                pop     ds
                iret
_timer_isr      endp

;  void far mouse_handler_set(void (far *fn)(void), int mask)  10d7:0203
;
;  Records DS (DGROUP) in isr_ds for the handler and installs `fn` as the
;  mouse driver's user event handler for the events in `mask` (int 33h, ax =
;  0Ch, es:dx = fn, cx = mask).  vidmouse.c installs mouse_handler with mask 7Fh.
;  es is left changed.
_mouse_handler_set proc far
                push    bp
                mov     bp,sp
                mov     cs:isr_ds,ds
                mov     ax,MOUSE_SET_HANDLER
                mov     cx,[bp+0Ah]
                mov     dx,[bp+6]
                mov     bx,[bp+8]
                push    bx
                pop     es
                int     33h
                pop     bp
                ret
_mouse_handler_set endp

;  int far ega_query(void)                                      10d7:021d
;
;  int 10h ah = 12h, bl = 10h (EGA information).  bh is preset to FFh; if the
;  BIOS leaves it there (no EGA BIOS) the result is 0.  Otherwise al = bh + 1
;  (1 colour, 2 monochrome) and ah = cl (the switch settings); dispinit.c
;  reads the low byte and ax & 0F00h.
_ega_query      proc    far
                mov     ah,EGA_INFO
                mov     bx,0FF00h or EGA_INFO_BL
                int     10h
                mov     al,bh
                inc     al
                jz      eq_none
                mov     ah,cl
                ret
eq_none:
                xor     ax,ax
                ret
_ega_query      endp

;  int far display_code(void)                                   10d7:0230
;
;  int 10h ax = 1A00h: returns the active display code (bl) when the BIOS
;  supports the call (al = 1Ah on return), else 0.  dispinit.c tests for 8,
;  VGA with a colour display.
_display_code   proc    far
                mov     ax,VGA_DISPLAY
                int     10h
                cmp     al,1Ah
                jne     dc_none
                mov     al,bl
                xor     ah,ah
                ret
dc_none:
                xor     ax,ax
                ret
_display_code   endp

;  void far blit_words(void far *src, int rows, int bytes, int skip)
;                                                               10d7:0241
;
;  Copies `rows` rows of `bytes` bytes (as bytes/2 words) from src to the
;  screen at screen_addr in draw_seg, stepping scan lines as row_advance
;  does; src steps `skip` bytes past each row's end.  DS is src's segment
;  inside and is set back from SS, which this code takes to be DGROUP.
_blit_words     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     si,[bp+6]
                mov     ax,[bp+8]
                mov     ds,ax
                mov     dx,[bp+0Ah]
                mov     ax,[bp+0Ch]
                shr     ax,1
                mov     bx,[bp+0Eh]
                mov     bp,ss:_screen_addr      ; bp: this row's screen offset
bw_row:
                mov     cx,ax
                mov     di,bp
                rep     movsw
                add     si,bx
                add     bp,ss:_text_bytes
                cmp     bp,ss:_extent_text
                jb      bw_next
                add     bp,ss:_base_offset
bw_next:
                dec     dx
                jnz     bw_row
                mov     ax,ss
                mov     ds,ax
                pop     di
                pop     si
                pop     bp
                ret
_blit_words     endp

;  void far blit_bytes(void far *src, int rows, int bytes, int skip)
;                                                               10d7:0286
;
;  blit_words copying bytes, with the scan-line step through row_advance.
_blit_bytes     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                mov     si,[bp+6]
                mov     ax,[bp+8]
                mov     ds,ax
                mov     dx,[bp+0Ah]
                mov     ax,[bp+0Ch]
                mov     bx,[bp+0Eh]
bb_row:
                mov     cx,ax
                push    di
                rep     movsb
                pop     di
                add     si,bx
                call    row_advance
                dec     dx
                jnz     bb_row
                mov     ax,ss
                mov     ds,ax
                pop     di
                pop     si
                pop     bp
                ret
_blit_bytes     endp

;  void far blit_masked_w(void far *image, void far *mask, int rows,
;                         int bytes, int skip)                  10d7:02ba
;
;  For `rows` rows of bytes/2 words at screen_addr in draw_seg: screen =
;  (screen AND mask) XOR image.  Both sources step `skip` past each row.
;  Only image's segment is loaded: mask is read at its offset in that same
;  segment.  `bytes` is halved in place on the stack.
_blit_masked_w  proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                mov     bx,[bp+6]               ; image
                mov     ax,[bp+8]
                mov     ds,ax
                mov     si,[bp+0Ah]             ; mask
                shr     word ptr [bp+10h],1
                mov     dx,[bp+12h]
mw_row:
                mov     cx,[bp+10h]
                push    di
mw_word:
                lodsw
                and     ax,es:[di]
                xor     ax,[bx]
                inc     bx
                inc     bx
                stosw
                loop    mw_word
                pop     di
                add     si,dx
                add     bx,dx
                add     di,ss:_text_bytes
                cmp     di,ss:_extent_text
                jb      mw_next
                add     di,ss:_base_offset
mw_next:
                dec     word ptr [bp+0Eh]
                jnz     mw_row
                mov     ax,ss
                mov     ds,ax
                pop     di
                pop     si
                pop     bp
                ret
_blit_masked_w  endp

;  void far blit_masked_b(void far *image, void far *mask, int rows,
;                         int bytes, int skip)                  10d7:030a
;
;  blit_masked_w by bytes, with the scan-line step through row_advance.
_blit_masked_b  proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                mov     bx,[bp+6]               ; image
                mov     ax,[bp+8]
                mov     ds,ax
                mov     si,[bp+0Ah]             ; mask
                mov     dx,[bp+12h]
mb_row:
                mov     cx,[bp+10h]
                push    di
mb_byte:
                lodsb
                and     al,es:[di]
                xor     al,[bx]
                inc     bx
                stosb
                loop    mb_byte
                pop     di
                add     si,dx
                add     bx,dx
                call    row_advance
                dec     word ptr [bp+0Eh]
                jnz     mb_row
                mov     ax,ss
                mov     ds,ax
                pop     di
                pop     si
                pop     bp
                ret
_blit_masked_b  endp

;  void far fill_bytes(int rows, int bytes, int value)          10d7:0348
;
;  Fills `rows` scan lines of `bytes` bytes from screen_addr in draw_seg
;  with value's low byte.  The line step is row_advance's, written inline
;  through DS.
_fill_bytes     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     si,_screen_addr         ; si: this row's screen offset
                mov     dx,[bp+6]
                mov     bx,[bp+8]
                mov     al,[bp+0Ah]
                mov     bp,_text_bytes
fb_row:
                mov     cx,bx
                mov     di,si
                rep     stosb
                add     si,bp
                cmp     si,_extent_text
                jb      fb_next
                add     si,_base_offset
fb_next:
                dec     dx
                jnz     fb_row
                pop     di
                pop     si
                pop     bp
                ret
_fill_bytes     endp

;  void far fill_words(int rows, int bytes, int value)          10d7:037b
;
;  fill_bytes by words: bytes/2 copies of `value` per row.
_fill_words     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     si,_screen_addr
                mov     dx,[bp+6]
                mov     bx,[bp+8]
                mov     ax,[bp+0Ah]
                shr     bx,1
                mov     bp,_text_bytes
fw_row:
                mov     cx,bx
                mov     di,si
                rep     stosw
                add     si,bp
                cmp     si,_extent_text
                jb      fw_next
                add     si,_base_offset
fw_next:
                dec     dx
                jnz     fw_row
                pop     di
                pop     si
                pop     bp
                ret
_fill_words     endp

;  void far mask_words(int xor_value, int and_value, int rows, int bytes)
;                                                               10d7:03b0
;
;  For `rows` scan lines of bytes/2 words from screen_addr in draw_seg:
;  screen = (screen AND and_value) XOR xor_value.
_mask_words     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                mov     dx,[bp+6]
                mov     bx,[bp+8]
                mov     si,[bp+0Ch]
                shr     si,1                    ; si: words per row
                mov     bp,[bp+0Ah]             ; bp: rows left
mk_row:
                mov     cx,si
                push    di
mk_word:
                mov     ax,es:[di]
                and     ax,bx
                xor     ax,dx
                stosw
                loop    mk_word
                pop     di
                add     di,_text_bytes
                cmp     di,_extent_text
                jb      mk_next
                add     di,_base_offset
mk_next:
                dec     bp
                jnz     mk_row
                pop     di
                pop     si
                pop     bp
                ret
_mask_words     endp


;  void far pattern_rows(int rows, int bytes, int far *pattern, int phase)
;                                                               10d7:03ee
;
;  Fills `rows` scan lines of bytes/2 words from screen_addr in draw_seg,
;  each line with one word of a 16-word pattern: line k takes
;  pattern[(phase + k) & 15].  The line step is row_advance.  vidmouse.c's
;  caller passes n + n as `bytes`.
_pattern_rows   proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                push    ds
                mov     dx,[bp+0Eh]
                shl     dx,1                    ; dx: the pattern word's byte offset
                lds     bx,[bp+0Ah]
                mov     ax,[bp+8]
                shr     ax,1
                mov     si,ax                   ; si: words per line
fp_row:
                mov     cx,si
                and     dx,PATTERN_MASK
                add     bx,dx
                mov     ax,[bx]
                sub     bx,dx
                push    di
                rep     stosw
                pop     di
                inc     dx
                inc     dx
                call    row_advance
                dec     word ptr [bp+6]
                jne     fp_row
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_pattern_rows   endp

;  Gathers the next pixel of four plane bytes into bl, the plane-3 bit
;  highest (shifting cl = plane 3, ch = 2, dl = 1, dh = 0 right), and doubles
;  it into a word index for planar_to_cga's table.
PIXEL_INDEX     macro
                shr     cl,1
                rcl     bl,1
                shr     ch,1
                rcl     bl,1
                shr     dl,1
                rcl     bl,1
                shr     dh,1
                rcl     bl,1
                shl     bl,1
                endm

;  void far planar_to_cga(int count, void far *planes, int plane_bytes,
;                         int far *table, void far *dst)        10d7:0429
;
;  Converts `count` bytes of 4-plane (EGA) pixels to CGA 2-bit pixels: for
;  each byte position it reads the four planes, `plane_bytes` apart (plane 0
;  first), and for each of the 8 pixels looks its 4-bit colour up in a table
;  of 16 words, keeping that pixel's two bits from the word (so the table
;  holds each colour's 2-bit value repeated across the word).  Stores one
;  word per source byte, high byte first.  The source offset advances in
;  place on the stack.  Nothing in the image calls this routine.
_planar_to_cga  proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                les     di,[bp+12h]
pc_byte:
                lds     si,[bp+8]
                mov     ax,[bp+0Ch]
                mov     dh,[si]                 ; plane 0
                add     si,ax
                mov     dl,[si]                 ; plane 1
                add     si,ax
                mov     ch,[si]                 ; plane 2
                add     si,ax
                mov     cl,[si]                 ; plane 3
                inc     word ptr [bp+8]
                lds     si,[bp+0Eh]             ; the colour table
                xor     bx,bx
                PIXEL_INDEX                     ; bit 0 of each plane: the rightmost pixel
                mov     ax,[bx+si]
                and     ax,0003h
                xor     bl,bl
                PIXEL_INDEX
                mov     bx,[bx+si]
                and     bx,000Ch
                or      ax,bx
                xor     bl,bl
                PIXEL_INDEX
                mov     bx,[bx+si]
                and     bx,0030h
                or      ax,bx
                xor     bl,bl
                PIXEL_INDEX
                mov     bx,[bx+si]
                and     bx,00C0h
                or      ax,bx
                xor     bl,bl
                PIXEL_INDEX
                mov     bx,[bx+si]
                and     bx,0300h
                or      ax,bx
                xor     bh,bh                   ; from here the masked bl is zero, bh not
                PIXEL_INDEX
                mov     bx,[bx+si]
                and     bx,0C00h
                or      ax,bx
                xor     bh,bh
                PIXEL_INDEX
                mov     bx,[bx+si]
                and     bx,3000h
                or      ax,bx
                xor     bh,bh
                PIXEL_INDEX                     ; bit 7: the leftmost pixel
                mov     bx,[bx+si]
                and     bx,0C000h
                or      ax,bx
                xchg    al,ah
                stosw
                dec     word ptr [bp+6]
                je      pc_done
                jmp     pc_byte
pc_done:
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_planar_to_cga  endp

;  void far planar_to_chunky(int count, void far *planes, int plane_bytes,
;                            void far *dst)                     10d7:0537
;
;  Converts `count` bytes of 4-plane pixels to one byte per pixel: for each
;  byte position it reads the four planes, `plane_bytes` apart, and stores 8
;  bytes, leftmost pixel first, plane k in bit k.  The source offset
;  advances in place on the stack.  Nothing in the image calls this routine.
_planar_to_chunky proc  far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                les     di,[bp+0Eh]
                mov     ax,[bp+0Ah]
                mov     ds,ax
pk_byte:
                mov     si,[bp+8]
                mov     ax,[bp+0Ch]
                mov     dh,[si]                 ; plane 0
                add     si,ax
                mov     dl,[si]                 ; plane 1
                add     si,ax
                mov     bh,[si]                 ; plane 2
                add     si,ax
                mov     bl,[si]                 ; plane 3
                inc     word ptr [bp+8]
                mov     cx,8
pk_pixel:
                xor     al,al
                shl     bl,1
                rcl     al,1
                shl     bh,1
                rcl     al,1
                shl     dl,1
                rcl     al,1
                shl     dh,1
                rcl     al,1
                stosb
                loop    pk_pixel
                dec     word ptr [bp+6]
                jne     pk_byte
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_planar_to_chunky endp

;  void far rle_draw(int skip_rows, int rows, int skip_left, int width,
;                    void far *codes, void far *pixels)         10d7:057e
;
;  Draws a run-length coded image at screen_addr in draw_seg, one byte per
;  pixel, clipped: the first `skip_rows` coded rows are passed over, then
;  `rows` rows are drawn, each from its `skip_left`th pixel for `width`
;  pixels, the screen line stepped by row_advance.  glyph.c's caller.
;
;  The code stream (`codes`) holds a byte per run: bits 0..5 the length,
;  bits 6..7 its kind; `pixels` is a second stream, of pixel bytes, of which
;  only the offset is used: it must lie in codes' segment.
;    11  literal: `length` pixel bytes, copied
;    10  fill:    one pixel byte, repeated `length` times
;    01  blend:   `length` mask bytes follow in the code stream, and as
;                 many pixel bytes: screen = (screen & ~mask) | (pixel & mask)
;    00  transparent: `length` pixels skipped; a 0 byte ends the row
;  A run that straddles the left clip has its length reduced in al (the
;  kind bits survive: the length is larger than the amount taken off) and
;  is dispatched again without reading a new code.  Returns with DS = SS.
RUN_LENGTH      equ     3Fh
RUN_FILLED      equ     80h                     ; 1x: literal or fill
RUN_SECOND      equ     40h                     ; 11 literal, 01 blend

_rle_draw       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                mov     si,[bp+0Eh]
                mov     ax,[bp+10h]
                mov     ds,ax
                mov     bx,[bp+12h]
                mov     dx,[bp+6]               ; dx: rows to pass over
                xor     ah,ah
                mov     ch,ah
                or      dx,dx
                je      rd_row
rd_top:                                         ; pass over a whole row's runs
                lodsb
                mov     cl,al
                and     cl,RUN_LENGTH
                test    al,RUN_FILLED
                je      rd_top_0x
                test    al,RUN_SECOND
                je      rd_top_fill
                add     bx,cx                   ; literal
                jmp     rd_top
rd_top_0x:
                test    al,RUN_SECOND
                je      rd_top_clear
                add     si,cx                   ; blend
                add     bx,cx
                jmp     rd_top
rd_top_fill:
                inc     bx
                jmp     rd_top
rd_top_clear:
                or      al,al
                jne     rd_top
                dec     dx                      ; end of a row
                jne     rd_top

rd_row:                                         ; each drawn row
                mov     dx,[bp+0Ah]             ; dx: pixels left of the clip
                lodsb
                push    di
                or      dx,dx
                je      rd_draw_code
                jmp     rd_left_code
rd_left:
                lodsb
rd_left_code:
                mov     cl,al
                and     cl,RUN_LENGTH
                test    al,RUN_FILLED
                je      rd_left_0x
                test    al,RUN_SECOND
                je      rd_left_fill
                cmp     cx,dx                   ; literal
                jle     rd_left_lit_all
                add     bx,dx
                sub     ax,dx
                jmp     rd_draw_code
rd_left_lit_all:
                sub     dx,cx
                add     bx,cx
                jmp     rd_left
rd_left_0x:
                test    al,RUN_SECOND
                je      rd_left_clear
                cmp     cx,dx                   ; blend
                jle     rd_left_blend_all
                add     si,dx
                add     bx,dx
                sub     ax,dx
                jmp     rd_draw_code
rd_left_blend_all:
                sub     dx,cx
                add     si,cx
                add     bx,cx
                jmp     rd_left
rd_left_fill:
                cmp     cx,dx
                jle     rd_left_fill_all
                sub     ax,dx
                jmp     rd_draw_code
rd_left_fill_all:
                sub     dx,cx
                inc     bx
                jmp     rd_left
rd_left_clear:
                or      al,al
                jne     rd_left_skip
                jmp     rd_row_end
rd_left_skip:
                cmp     cx,dx
                jle     rd_left_skip_all
                sub     ax,dx
                jmp     rd_draw_code
rd_left_skip_all:
                sub     dx,cx
                jmp     rd_left

rd_draw_code:                                   ; draw `width` pixels
                mov     dx,[bp+0Ch]
                jmp     rd_draw_run
rd_draw:
                lodsb
rd_draw_run:
                mov     cl,al
                and     cl,RUN_LENGTH
                test    al,RUN_FILLED
                je      rd_draw_0x
                test    al,RUN_SECOND
                je      rd_draw_fill
                cmp     cx,dx                   ; literal
                jl      rd_lit_inside
                or      dx,dx
                jne     rd_lit_clip
                jmp     rd_rest
rd_lit_clip:
                sub     cx,dx
                xchg    cx,dx                   ; cx: the part drawn, dx: the rest
                xchg    bx,si
                rep     movsb
                xchg    bx,si
                add     bx,dx
                jmp     rd_rest
rd_lit_inside:
                sub     dx,cx
                xchg    bx,si
                rep     movsb
                xchg    bx,si
                jmp     rd_draw
rd_draw_0x:
                test    al,RUN_SECOND
                je      rd_draw_clear
                cmp     cx,dx                   ; blend
                jl      rd_blend_inside
                or      dx,dx
                je      rd_rest
                sub     cx,dx
                xchg    cx,dx
rd_blend_clip:
                lodsb
                mov     ah,al
                not     al
                and     ah,[bx]
                inc     bx
                and     al,es:[di]
                or      al,ah
                stosb
                loop    rd_blend_clip
                xor     ah,ah
                add     bx,dx
                add     si,dx
                jmp     rd_rest
rd_blend_inside:
                sub     dx,cx
rd_blend_pixel:
                lodsb
                mov     ah,al
                not     al
                and     ah,[bx]
                inc     bx
                and     al,es:[di]
                or      al,ah
                stosb
                loop    rd_blend_pixel
                xor     ah,ah
                jmp     rd_draw
rd_draw_fill:
                mov     al,[bx]
                inc     bx
                cmp     cx,dx
                jl      rd_fill_inside
                or      dx,dx
                je      rd_rest
                mov     cx,dx
                rep     stosb
                jmp     rd_rest
rd_fill_inside:
                sub     dx,cx
                rep     stosb
                jmp     rd_draw
rd_draw_clear:
                or      al,al
                je      rd_row_end
                cmp     cx,dx
                jl      rd_clear_inside
                add     di,dx
                jmp     rd_rest
rd_clear_inside:
                sub     dx,cx
                add     di,cx
                jmp     rd_draw

rd_rest:                                        ; pass over the row's remaining runs
                lodsb
                mov     cl,al
                and     cl,RUN_LENGTH
                test    al,RUN_FILLED
                je      rd_rest_0x
                test    al,RUN_SECOND
                je      rd_rest_fill
                add     bx,cx
                jmp     rd_rest
rd_rest_0x:
                test    al,RUN_SECOND
                je      rd_rest_clear
                add     si,cx
                add     bx,cx
                jmp     rd_rest
rd_rest_fill:
                inc     bx
                jmp     rd_rest
rd_rest_clear:
                or      al,al
                jne     rd_rest
rd_row_end:
                pop     di
                call    row_advance
                dec     word ptr [bp+8]
                je      rd_done
                jmp     rd_row
rd_done:
                mov     ax,ss
                mov     ds,ax
                pop     di
                pop     si
                pop     bp
                ret
_rle_draw       endp

;  void far expand_1to2(void far *src, void far *dst, int count)
;                                                               10d7:0708
;
;  Widens `count` bytes of 1-bit pixels to 2 bits per pixel, each bit
;  doubled (column_2bpp's expansion without the masks), one word per source
;  byte stored high byte first.  vidmouse.c converts the pointer shapes.
_expand_1to2    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     si,[bp+6]
                les     di,[bp+0Ah]
                mov     cx,[bp+0Eh]
e2_byte:
                lodsb
                mov     bl,al
                rept    8                       ; each bit, twice, into ax from the top
                ror     bl,1
                rcr     ax,1
                sar     ax,1
                endm
                xchg    al,ah
                stosw
                loop    e2_byte
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_expand_1to2    endp

;  void far expand_1to4(void far *src, void far *dst, int count)
;                                                               10d7:0754
;
;  Widens `count` bytes of 1-bit pixels to 4 bits per pixel, each bit
;  repeated four times: two words per source byte, high bytes first.  The
;  low four bits are built first and stored second.
_expand_1to4    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     si,[bp+6]
                les     di,[bp+0Ah]
                mov     cx,[bp+0Eh]
e4_byte:
                lodsb
                mov     bl,al
                rept    4                       ; bits 0..3, each four times
                ror     bl,1
                rcr     ax,1
                sar     ax,1
                sar     ax,1
                sar     ax,1
                endm
                xchg    al,ah
                inc     di
                inc     di
                stosw                           ; the right-hand word
                dec     di
                dec     di
                dec     di
                dec     di
                rept    4                       ; bits 4..7
                ror     bl,1
                rcr     ax,1
                sar     ax,1
                sar     ax,1
                sar     ax,1
                endm
                xchg    al,ah
                stosw                           ; the left-hand word
                inc     di
                inc     di
                loop    e4_byte
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_expand_1to4    endp

;  void far expand_1to8(void far *src, void far *dst, int count)
;                                                               10d7:07cb
;
;  Widens `count` bytes of 1-bit pixels to a byte per pixel, 0 or FFh,
;  leftmost first.
_expand_1to8    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     si,[bp+6]
                les     di,[bp+0Ah]
                mov     dx,[bp+0Eh]
e8_byte:
                lodsb
                mov     cx,8
e8_pixel:
                cbw                             ; ah: bit 7 of al, spread
                shl     al,1
                mov     es:[di],ah
                inc     di
                loop    e8_pixel
                dec     dx
                jne     e8_byte
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_expand_1to8    endp

;  void far planar_to_nibbles(void far *planes, int plane_bytes,
;                             void far *dst, int count)         10d7:07ef
;
;  Converts `count` (a byte, 1..256) bytes of 4-plane pixels to packed
;  4-bit pixels, two to a byte, the leftmost in the high nibble, plane k in
;  bit k: four bytes out per source byte position.  Nothing in the image
;  calls this routine.
_planar_to_nibbles proc far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     si,[bp+6]
                mov     dx,[bp+0Ah]
                les     di,[bp+0Ch]
                mov     ah,[bp+10h]
pn_byte:
                push    si
                mov     bl,[si]                 ; plane 0
                add     si,dx
                mov     bh,[si]                 ; plane 1
                add     si,dx
                mov     cl,[si]                 ; plane 2
                add     si,dx
                mov     ch,[si]                 ; plane 3
                pop     si
                inc     si
                rept    4                       ; four output bytes ...
                rept    2                       ; ... of two pixels
                rol     ch,1
                rcl     al,1
                rol     cl,1
                rcl     al,1
                rol     bh,1
                rcl     al,1
                rol     bl,1
                rcl     al,1
                endm
                stosb
                endm
                dec     ah
                je      pn_done
                jmp     pn_byte
pn_done:
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_planar_to_nibbles endp

;  void far fill_far_words(void far *dst, unsigned bytes, int value)
;                                                               10d7:08a2
;
;  Stores bytes/2 words of value's low byte, repeated, at dst.  Keeps di in
;  bx rather than on the stack.  dispinit.c clears its off-screen buffer
;  (and video memory) with it.
_fill_far_words proc    far
                push    bp
                mov     bp,sp
                mov     bx,di
                mov     cx,[bp+0Ah]
                shr     cx,1
                or      cx,cx
                je      ff_done
                mov     es,[bp+8]
                mov     di,[bp+6]
                mov     al,[bp+0Ch]
                mov     ah,al
                cld
                rep     stosw
ff_done:
                mov     di,bx
                pop     bp
                ret
_fill_far_words endp

;  void far huge_move(void far *src, void far *dst, unsigned n)  10d7:08c2
;
;  Copies n bytes from src to dst, by words with an odd byte, and copes
;  with overlap: it copies downward when src is below dst, comparing the
;  segments first and the offsets only when the segments are equal (the
;  linear addresses are not computed).  The most-called routine here.
_huge_move      proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     bx,ds
                mov     ax,[bp+8]
                mov     ds,ax
                mov     dx,[bp+0Ch]
                mov     es,dx
                mov     si,[bp+6]
                mov     di,[bp+0Ah]
                mov     cx,[bp+0Eh]
                or      cx,cx
                je      hm_done
                cmp     ax,dx
                jb      hm_down
                jne     hm_up
                cmp     si,di
                jb      hm_down
hm_up:
                cld
                shr     cx,1
                jae     hm_up_words
                movsb
                or      cx,cx
                je      hm_done
hm_up_words:
                rep     movsw
                jmp     hm_done
hm_down:
                add     si,cx                   ; the last word
                dec     si
                dec     si
                add     di,cx
                dec     di
                dec     di
                std
                shr     cx,1
                jae     hm_down_words
                inc     si                      ; the odd last byte first
                inc     di
                movsb
                or      cx,cx
                je      hm_down_done
                dec     si
                dec     di
hm_down_words:
                rep     movsw
hm_down_done:
                cld
hm_done:
                pop     di
                pop     si
                mov     ds,bx
                pop     bp
                ret
_huge_move      endp

;  void far far_move(unsigned src, unsigned src_seg, unsigned dst,
;                    unsigned dst_seg, unsigned n)              10d7:0918
;
;  huge_move by bytes.  vidmouse.c and scrfont.c move screen memory with it.
_far_move       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     bx,ds
                mov     ax,[bp+8]
                mov     ds,ax
                mov     dx,[bp+0Ch]
                mov     es,dx
                mov     si,[bp+6]
                mov     di,[bp+0Ah]
                mov     cx,[bp+0Eh]
                or      cx,cx
                je      fm_done
                cmp     ax,dx
                jb      fm_down
                jne     fm_up
                cmp     si,di
                jb      fm_down
fm_up:
                cld
                rep     movsb
                jmp     fm_done
fm_down:
                add     si,cx
                dec     si
                add     di,cx
                dec     di
                std
                rep     movsb
                cld
fm_done:
                pop     di
                pop     si
                mov     ds,bx
                pop     bp
                ret
_far_move       endp

;  void far cga_copy(unsigned from_seg, unsigned to_seg)        10d7:0956
;
;  Copies a 320x200 CGA screen image between two segments in the CGA
;  layout: 100 line pairs of 80 bytes, the even line at offset 80*k, the
;  odd one 2000h above it.  The 192 bytes past each bank's 8000 are not
;  copied.  vidmouse.c's caller.
CGA_BANK        equ     2000h                   ; the odd lines' bank
CGA_LINE        equ     80                      ; bytes per line
CGA_PAIRS       equ     100

_cga_copy       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     bx,ds
                mov     ds,[bp+6]
                mov     es,[bp+8]
                xor     si,si
                mov     di,si
                mov     dx,CGA_PAIRS
                mov     ax,CGA_LINE/2
                cld
cc_pair:
                mov     cx,ax
                rep     movsw                   ; the even line
                add     si,CGA_BANK-CGA_LINE
                mov     di,si
                mov     cx,ax
                rep     movsw                   ; the odd line
                sub     si,CGA_BANK
                mov     di,si
                dec     dx
                jne     cc_pair
                pop     di
                pop     si
                mov     ds,bx
                pop     bp
                ret
_cga_copy       endp

;  int far swap16(int v)                                        10d7:098b
;
;  v with its bytes swapped: a big-endian word from a file.  Leaf style:
;  the argument read through bx = sp.  digisnd.c's caller.
_swap16         proc    far
                mov     bx,sp
                mov     ax,ss:[bx+4]
                xchg    ah,al
                ret
_swap16         endp

;  long far swap32(long v)                                      10d7:0994
;
;  v with its four bytes reversed: a big-endian long.  digisnd.c's caller.
_swap32         proc    far
                mov     bx,sp
                mov     ax,ss:[bx+6]            ; the high word, to the low
                mov     dx,ss:[bx+4]
                xchg    ah,al
                xchg    dh,dl
                ret
_swap32         endp

;  int far joy_sample(void far *log)                            10d7:09a3
;
;  Times the game port's four one-shots: reads their bits (0..3); if all
;  four are set it returns at once (a sample still running).  Otherwise,
;  interrupts off, it fires them (any write to the port), and each time
;  the four bits change it appends a 3-byte record to `log`: the new bits
;  and a word count of polls so far.  It stops when the bits are back to
;  their state before the trigger, or the count wraps.  Returns the port's
;  last reading, the buttons in bits 4..7 (active low).  Nothing in the
;  image calls this routine.
AXIS_BITS       equ     0Fh

_joy_sample     proc    far
                push    bp
                mov     bp,sp
                push    di
                les     di,[bp+6]
                xor     cx,cx
                mov     dx,GAME_PORT
                in      al,dx
                and     al,AXIS_BITS
                mov     bl,al                   ; bl: the bits at rest
                cmp     bl,AXIS_BITS
                je      js_done
                cli
                out     dx,al                   ; fire the one-shots
                nop
                nop
                nop
                in      al,dx
                and     al,AXIS_BITS
                cmp     al,AXIS_BITS
                jne     js_record
                mov     ah,al
js_poll:
                in      al,dx
                and     al,ah
                inc     cx
                je      js_done
                cmp     al,ah
                je      js_poll
js_record:
                stosb
                mov     es:[di],cx
                inc     di
                inc     di
                mov     ah,al
                cmp     al,bl
                jne     js_poll
js_done:
                in      al,dx
                sti
                xor     ah,ah
                pop     di
                pop     bp
                ret
_joy_sample     endp

;  void far grab_words(void far *dst, int rows, int bytes)      10d7:09e4
;
;  blit_words the other way: copies `rows` lines of bytes/2 words from
;  screen_addr in draw_seg to dst, packed.  DS is the screen inside, so
;  DGROUP is read through SS.  tiles.c's caller.
_grab_words     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                mov     si,_screen_addr
                mov     ds,_draw_seg
                les     di,[bp+6]
                mov     dx,[bp+0Ah]
                mov     ax,[bp+0Ch]
                shr     ax,1
gw_row:
                mov     cx,ax
                push    si
                rep     movsw
                pop     si
                add     si,ss:_text_bytes
                cmp     si,ss:_extent_text
                jb      gw_next
                add     si,ss:_base_offset
gw_next:
                dec     dx
                jne     gw_row
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_grab_words     endp

;  char far *far packbits_expand(char far *in, char far *out, int n)
;                                                               10d7:0a1c
;
;  Expands PackBits run-length data until n bytes are out: a code c of
;  0..127 copies the next c+1 bytes, -1..-127 repeats the next byte 1-c
;  times, -128 is skipped.  Returns where the input stopped.  library.c,
;  glyph.c and saveload.c call it.
_packbits_expand proc   far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     si,[bp+6]
                les     di,[bp+0Ah]
                mov     dx,[bp+0Eh]
                xor     ch,ch
                cld
pb_code:
                lodsb
                or      al,al
                jl      pb_run
                mov     cl,al                   ; a literal
                inc     cx
                sub     dx,cx
                rep     movsb
                or      dx,dx
                jg      pb_code
                jmp     pb_done
pb_run:
                cmp     al,80h
                je      pb_code
                neg     al                      ; a repeat
                mov     cl,al
                inc     cx
                lodsb
                sub     dx,cx
                rep     stosb
                or      dx,dx
                jg      pb_code
pb_done:
                mov     dx,ds
                mov     ax,si
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_packbits_expand endp

;  void far split_nibbles(char far *in, char far *out, int n)   10d7:0a5c
;
;  Writes each of n input bytes as two bytes, its high nibble then its
;  low.  glyph.c's caller.
_split_nibbles  proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     si,[bp+6]
                les     di,[bp+0Ah]
                mov     dx,[bp+0Eh]
                mov     cl,4
sn_byte:
                lodsb
                mov     ah,al
                shr     al,cl
                stosb
                mov     al,ah
                and     al,0Fh
                stosb
                dec     dx
                jne     sn_byte
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_split_nibbles  endp

;  void far nibbles_to_planar(char far *in, char far *out, int plane_bytes,
;                             int n)                            10d7:0a80
;
;  planar_to_nibbles reversed: reads packed 4-bit pixels four bytes (eight
;  pixels) at a time and writes one byte to each of four planes,
;  `plane_bytes` apart, plane k taking bit k.  n counts input bytes; n/4
;  groups are done (the count is a byte: 1..256 groups).  glyph.c's caller.
_nibbles_to_planar proc far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     si,[bp+6]
                les     di,[bp+0Ah]
                mov     dx,[bp+0Eh]
                mov     ax,[bp+10h]
                sar     ax,1
                sar     ax,1
                mov     ah,al
np_group:
                rept    4                       ; four input bytes ...
                lodsb
                rept    2                       ; ... of two pixels
                rol     al,1
                rcl     bl,1                    ; bit 3
                rol     al,1
                rcl     bh,1                    ; bit 2
                rol     al,1
                rcl     cl,1                    ; bit 1
                rol     al,1
                rcl     ch,1                    ; bit 0
                endm
                endm
                push    di
                mov     es:[di],ch              ; plane 0
                add     di,dx
                mov     es:[di],cl
                add     di,dx
                mov     es:[di],bh
                add     di,dx
                mov     es:[di],bl              ; plane 3
                pop     di
                inc     di
                dec     ah
                je      np_done
                jmp     np_group
np_done:
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_nibbles_to_planar endp

;  int far mouse_call(int ax, int bx, int cx, int dx, int si, int di)
;                                                               10d7:0b3d
;
;  int 33h with the six registers from the arguments; returns ax.  Callers
;  pass only as many as the function needs (vidmouse.c).
_mouse_call     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     ax,[bp+6]
                mov     bx,[bp+8]
                mov     cx,[bp+0Ah]
                mov     dx,[bp+0Ch]
                mov     si,[bp+0Eh]
                mov     di,[bp+10h]
                int     MOUSE_INT
                pop     di
                pop     si
                pop     bp
                ret
_mouse_call     endp

;  void far mouse_call_regs(int far *ax, int far *bx, int far *cx,
;                           int far *dx)                        10d7:0b5a
;
;  int 33h with ax..dx loaded from the four pointers and stored back.
_mouse_call_regs proc   far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     si,[bp+6]
                mov     ax,[si]
                lds     si,[bp+0Ah]
                mov     bx,[si]
                lds     si,[bp+0Eh]
                mov     cx,[si]
                les     di,[bp+12h]
                mov     dx,es:[di]
                int     MOUSE_INT
                mov     es:[di],dx
                mov     [si],cx
                lds     si,[bp+0Ah]
                mov     [si],bx
                lds     si,[bp+6]
                mov     [si],ax
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_mouse_call_regs endp

;  mouse_handler                                                10d7:0b8b
;
;  The user event handler vidmouse.c installs through mouse_handler_set
;  (mask 7Fh).  The driver far-calls it with ax = the events, bx = the
;  buttons, cx, dx = the pointer's pixel position.  It loads DS from isr_ds,
;  records the position in character cells (column_mouse, mousey_now) and
;  the buttons (buttons_now), and for any button press or release (ax &
;  7Eh; bit 0 is movement) calls vidmouse.c's mouse_event(events).
MOUSE_BUTTONS   equ     7Eh                     ; event bits 1..6

_mouse_handler  proc    far
                push    ds
                mov     ds,cs:isr_ds
                push    ax
                mov     ax,cx
                mov     cl,3
                sar     ax,cl
                mov     _column_mouse,ax
                mov     ax,dx
                sar     ax,cl
                mov     _mousey_now,ax
                mov     _buttons_now,bx
                pop     ax
                test    ax,MOUSE_BUTTONS
                je      mh_done
                push    es
                push    si
                push    di
                push    ax
                cld
                call    _mouse_event
                pop     ax
                pop     di
                pop     si
                pop     es
mh_done:
                pop     ds
                ret
_mouse_handler  endp

;  void far blit_shifted(char far *image, void far *mask, int rows,
;                        int bytes, int skip, int first_mask, int last_mask,
;                        int shift)
;                                                               10d7:0bbc
;
;  Draws `rows` lines of a `bytes`-wide image shifted right by `shift`
;  bits at screen_addr in draw_seg, touching bytes+1 screen bytes a line:
;  for each source byte, m = NOT the mask byte, both shifted into a word,
;  and screen = (screen AND NOT m) XOR image.  Only mask's offset is used: it must lie in image's
;  segment.  The first byte of a line is ANDed (image and m) with
;  first_mask and the last with last_mask (a one-byte line with both).
;  Each source steps `skip` past a line's end; the line step is
;  row_advance's, read through DGROUP between lines.  vidmouse.c draws the
;  pointer with it.
_blit_shifted   proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                push    ds
                lds     si,[bp+6]
                mov     bx,[bp+0Ah]
                mov     cl,[bp+18h]             ; cl: the shift
bs_row:
                push    di
                mov     ch,[bp+10h]             ; ch: bytes left
                lodsb
                and     al,[bp+14h]
                mov     dh,[bx]
                inc     bx
                not     dh
                and     dh,[bp+14h]
                dec     ch
                jne     bs_store
                and     al,[bp+16h]             ; one byte: the last as well
                and     dh,[bp+16h]
                jmp     bs_store
bs_middle:
                lodsb
                mov     dh,[bx]
                inc     bx
                not     dh
bs_store:
                mov     ah,al
                xor     al,al
                shr     ax,cl
                xchg    al,ah
                xor     dl,dl
                shr     dx,cl
                not     dx
                xchg    dl,dh
                and     dx,es:[di]
                xor     ax,dx
                stosw
                dec     di
                dec     ch
                jg      bs_middle
                jne     bs_line_end
                lodsb                           ; the last byte
                and     al,[bp+16h]
                mov     dh,[bx]
                inc     bx
                not     dh
                and     dh,[bp+16h]
                jmp     bs_store
bs_line_end:
                add     si,[bp+12h]
                add     bx,[bp+12h]
                pop     di
                mov     ax,ds
                pop     ds
                add     di,_text_bytes
                cmp     di,_extent_text
                jb      bs_next
                add     di,_base_offset
bs_next:
                push    ds
                mov     ds,ax
                dec     word ptr [bp+0Eh]
                jne     bs_row
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_blit_shifted   endp

;  void far put_bytes(void far *src, int rows, int bytes)       10d7:0c46
;
;  Copies `rows` lines of `bytes` bytes from src, packed, to screen_addr
;  in draw_seg; the line step reads DGROUP between lines.  vidmouse.c puts
;  back what the pointer covered.
_put_bytes      proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     es,_draw_seg
                mov     di,_screen_addr
                push    ds
                lds     si,[bp+6]
                mov     dx,[bp+0Ah]
pu_row:
                mov     cx,[bp+0Ch]
                mov     bx,di
                rep     movsb
                mov     di,bx
                mov     ax,ds
                pop     ds
                add     di,_text_bytes
                cmp     di,_extent_text
                jb      pu_next
                add     di,_base_offset
pu_next:
                push    ds
                mov     ds,ax
                dec     dx
                jne     pu_row
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_put_bytes      endp

;  void far grab_bytes(void far *dst, int rows, int bytes)      10d7:0c7f
;
;  put_bytes the other way: copies `rows` lines from screen_addr in
;  draw_seg to dst, packed.  ES briefly holds DGROUP for the line step.
;  vidmouse.c saves what the pointer will cover.
_grab_bytes     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                mov     si,_screen_addr
                mov     ds,_draw_seg
                les     di,[bp+6]
                mov     dx,[bp+0Ah]
gb_row:
                mov     cx,[bp+0Ch]
                mov     bx,si
                rep     movsb
                mov     si,bx
                mov     ax,es
                pop     es                      ; DGROUP
                assume  es:DGROUP
                add     si,es:_text_bytes
                cmp     si,es:_extent_text
                jb      gb_next
                add     si,es:_base_offset
gb_next:
                assume  es:nothing
                push    es
                mov     es,ax
                dec     dx
                jne     gb_row
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_grab_bytes     endp

;  char far *far huff_expand(char far *in, char far *out, int n,
;                            void far *table)                   10d7:0cbb
;
;  Decodes n bytes of a prefix-coded bit stream, most significant bit
;  first, by table lookup on the next 8 bits.  The table is four 256-byte
;  pages: page 0 the decoded byte, page 1 its code length; a length with
;  bit 7 set is an escape (low nibble the length to drop), and the next
;  byte is decoded from pages 2 and 3 instead.  Only the table's offset is
;  used, read through SS, so it must lie in the stack segment.  Returns
;  in's segment and in + ceil(bits consumed / 8), the first byte none of
;  whose bits were consumed (si - 2 when the bits end on a byte boundary,
;  ch = 8; si - 1 otherwise).  bp is reused as the output offset.  library.c's caller.
ESCAPE_LENGTH   equ     0Fh

_huff_expand    proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                push    es
                mov     si,[bp+6]
                mov     ax,[bp+8]
                mov     ds,ax
                mov     ax,[bp+0Ch]
                mov     es,ax
                mov     ax,[bp+0Ah]
                mov     dx,[bp+0Eh]             ; dx: bytes still to decode
                mov     di,[bp+10h]             ; di: the table
                mov     bp,ax                   ; bp: the output
                lodsw
                xchg    ah,al                   ; ax: the next 16 bits, first in bit 15
                mov     ch,8                    ; ch: bits left in al
                xor     bh,bh                   ; bh: the table page pair
he_code:
                mov     bl,ah
                mov     cl,ss:[bx+di]           ; the byte ...
                mov     es:[bp],cl
                inc     bp
                inc     bh
                mov     cl,ss:[bx+di]           ; ... and its length
                or      cl,cl
                jge     he_plain
                dec     bp                      ; an escape: no byte out
                inc     bh
                inc     dx
                and     cl,ESCAPE_LENGTH
                jmp     he_drop
he_plain:
                xor     bh,bh
he_drop:
                cmp     cl,ch
                jl      he_shift
                xchg    cl,ch                   ; more than al holds:
                shl     ax,cl                   ; take what it has,
                lodsb                           ; load the next byte,
                sub     ch,cl
                mov     cl,ch
                shl     ax,cl                   ; and drop the rest
                mov     ch,8
                sub     ch,cl
                dec     dx
                jne     he_code
                jmp     he_end
he_shift:
                shl     ax,cl
                sub     ch,cl
                dec     dx
                jne     he_code
he_end:
                mov     dx,ds
                mov     ax,si
                dec     ax
                cmp     ch,8
                jne     he_return
                dec     ax
he_return:
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_huff_expand    endp

_TEXT           ends
                end
