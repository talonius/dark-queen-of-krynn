;  DQK.EXE resident frame 10d7: far memory blocks -- move, fill, swap, and
;  scans for a byte or a word.  EXE 0x0058e6..0x005990, 170 bytes,
;  10d7:2776..10d7:2820.  dosio.asm ends at 0x58e6; farmem.asm starts at
;  0x5990.
;
;  Why these routines are one file is a choice, not a measurement: they
;  share no data, near call or jump with each other or with their
;  neighbours, and the frame has no relocation or pad byte at 2776 or 2820.
;  They are grouped by role and by style: all five keep a bp frame
;  (arguments from [bp+6]), where the routines on either side use bx = sp.
;  A file boundary is measured somewhere in 266f..2820 (see dosio.asm).
;
;  Assembled by TASM 3.1, /ml, single pass: the forward jmp in mem_move
;  takes three bytes, EB xx 90, as it shipped.  No segment fixups, and the
;  MZ relocation table has no entry in this range.

                .8086

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _mem_move, _mem_fill, _mem_exchange, _mem_scan, _mem_scan_w

;  void far mem_move(void far *src, void far *dst, unsigned n)  10d7:2776
;
;  Copies n bytes from src to dst, backward when src's OFFSET is below
;  dst's, so overlapping blocks in one segment move correctly; the
;  segments are not compared.  Does nothing when n is 0 or the offsets are
;  equal (whatever the segments).  Leaves the direction flag clear.  si,
;  di, ds and es are saved.
_mem_move       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                push    es
                lds     si,dword ptr [bp+6]     ; ds:si = src
                les     di,dword ptr [bp+0Ah]   ; es:di = dst
                mov     cx,[bp+0Eh]
                or      cx,cx
                je      move_done
                cmp     si,di
                je      move_done
                jb      move_down
                cld
                rep     movsb
                jmp     move_done
move_down:
                add     si,cx                   ; from the last byte of each
                dec     si
                add     di,cx
                dec     di
                std
                rep     movsb
                cld
move_done:
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_mem_move       endp

;  void far mem_fill(void far *p, unsigned n, int c)            10d7:27a6
;
;  Stores c in n bytes at p (none when n is 0).  di is saved.
_mem_fill       proc    far
                push    bp
                mov     bp,sp
                push    di
                les     di,dword ptr [bp+6]
                mov     cx,[bp+0Ah]
                mov     al,[bp+0Ch]
                or      cx,cx
                je      fill_done
                cld
                rep     stosb
fill_done:
                pop     di
                pop     bp
                ret
_mem_fill       endp

;  void far mem_exchange(void far *a, void far *b, unsigned n)  10d7:27bd
;
;  Exchanges n bytes between a and b.  n = 0 swaps 65536 bytes (loop).
;  Relies on the direction flag being clear.  Nothing in DQK.EXE calls it.
;  Its 31 bytes are the same as gfxutil.asm's mem_swap (10d7:0e16), which
;  glyphfx.c calls; two names keep the link's publics distinct.
_mem_exchange   proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                push    es
                lds     si,dword ptr [bp+6]     ; ds:si = a
                les     di,dword ptr [bp+0Ah]   ; es:di = b
                mov     cx,[bp+0Eh]
swap_byte:
                mov     al,byte ptr es:[di]
                xchg    al,byte ptr [si]
                inc     si
                stosb
                loop    swap_byte
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_mem_exchange   endp

;  unsigned far mem_scan(void far *p, unsigned n, int c)        10d7:27dc
;
;  The index of the first c among n bytes at p, or n when there is none.
;  di and es are saved.
_mem_scan       proc    far
                push    bp
                mov     bp,sp
                push    di
                push    es
                les     di,dword ptr [bp+6]
                mov     cx,[bp+0Ah]
                mov     al,[bp+0Ch]
                cmp     cx,0
                je      scan_done
                cld
                repne   scasb
                jne     scan_done               ; not found: cx = 0
                inc     cx                      ; found: count it back
scan_done:
                mov     ax,[bp+0Ah]
                pop     es
                sub     ax,cx                   ; n - the bytes not scanned
                pop     di
                pop     bp
                ret
_mem_scan       endp

;  unsigned far mem_scan_w(int far *p, unsigned n, int w)       10d7:27fe
;
;  mem_scan for words: the index of the first w among n words at p, or n.
;  Nothing in DQK.EXE calls it.
_mem_scan_w     proc    far
                push    bp
                mov     bp,sp
                push    di
                push    es
                les     di,dword ptr [bp+6]
                mov     cx,[bp+0Ah]
                mov     ax,[bp+0Ch]
                cmp     cx,0
                je      scanw_done
                cld
                repne   scasw
                jne     scanw_done
                inc     cx
scanw_done:
                mov     ax,[bp+0Ah]
                pop     es
                sub     ax,cx
                pop     di
                pop     bp
                ret
_mem_scan_w     endp

_TEXT           ends
                end
