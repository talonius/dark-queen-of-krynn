;  DQK.EXE resident frame 10d7: the inner loops of the resource
;  decompressors and one compressor search -- the 10-bit LZW decoder's
;  string emitter and main loop (library.c's lzw_expand calls both), a
;  byte-coded LZ77 decoder (library.c's lz_expand), and a longest-match
;  search for the LZ77 side that nothing in DQK.EXE calls.  EXE
;  0x0053bb..0x0055b9, 510 bytes, 10d7:224b..10d7:2449.  vgadraw.asm ends
;  at 0x53bb.  10d7:2449 is not in this file: it is the first of the tiny
;  INT 21h wrappers (2449 AH=01h, 2450 AH=07h, 2457 AX=4406h, ...), whose
;  register style (no bp frame) is theirs, not this file's.
;
;  Why these routines are one file is a choice, not a measurement: the
;  frame has no far call between its own routines, relocation or pad byte
;  at 224b or 2449.  They are grouped by role, the LZW pair sharing the
;  dictionary layout.  lz_match differs in style -- it saves di before si
;  and es before ds, where every routine before it saves si first -- which
;  is weak evidence that it came from another source file; it stays here
;  as the LZ77 decoder's counterpart.
;
;  House style: C far entry, bp frame, arguments from [bp+6], si/di/ds
;  saved.
;
;  Assembled by TASM 3.1, /ml, single pass.  One segment fixup, the far
;  call to library.c's lzw_prune at 0x005461 (10d7:22f1), whose segment
;  word at 0x005464 is the range's one MZ relocation; _lzw_prune is an extern resolved to 293f:0e39.

                .8086

                extrn   _lzw_prune:far          ; 293f:0e39, library.c

;  library.c's struct Lzw, the dictionary for codes 0..3FFh.
LZW_USED        equ     0                       ; char used[400h]: 0 free
LZW_SUFFIX      equ     402h                    ; char suffix[400h]
LZW_PREFIX      equ     802h                    ; int prefix[400h]
LZW_NEXT        equ     1002h                   ; int next: the next free code
LZW_FULL        equ     400h                    ; next has run past the last code
ROOT_CODES      equ     100h                    ; codes below are single bytes
CODE_BITS       equ     10

LZ_END          equ     0FFh                    ; lz_decode's end-of-data byte

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _lzw_emit, _lzw_decode, _lz_decode, _lz_match

;  int far lzw_emit(char far * far *out, int code, struct Lzw far *t)
;                                                               10d7:224b
;
;  Writes code's string at *out and advances *out's offset past it (its
;  segment is not written back); returns the string's first byte in ax
;  (library.c declares it void).  The string is found backwards, suffix
;  bytes pushed while the code is not a root (bh != 0) and the prefix
;  chain followed, then popped into place; a root code is its own byte.
_lzw_emit       proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     di,dword ptr [bp+6]
                les     di,dword ptr [di]       ; es:di: *out
                mov     bx,[bp+0Ah]             ; bx: code
                lds     si,dword ptr [bp+0Ch]
                mov     cx,1                    ; cx: the string's length
                add     si,LZW_SUFFIX           ; ds:si: t->suffix
                jmp     le_test
le_link:
                push    word ptr [bx+si]        ; suffix[code], low byte
                inc     cx
                add     bx,bx
                add     bx,LZW_PREFIX-LZW_SUFFIX
                mov     bx,[bx+si]              ; code = prefix[code]
le_test:
                or      bh,bh
                jne     le_link
                push    bx                      ; the root byte
le_store:
                pop     ax
                stosb
                loop    le_store
                mov     ax,bx                   ; returns the first byte
                lds     si,dword ptr [bp+6]
                mov     [si],di                 ; *out's offset
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_lzw_emit       endp

;  char far *far lzw_decode(unsigned w, int code, unsigned char far *in,
;                           int next, char far *end, struct Lzw far *t,
;                           char far * far *out)                10d7:2286
;
;  The LZW decoder's main loop, after lzw_expand has read the first code
;  from the first two bytes w and emitted it.  Codes are 10 bits, high bit
;  first: ch keeps the last byte read, cl how many of its low bits are
;  still unread (6 after w).  Per code: its string is emitted at *out (as
;  lzw_emit does, no special case for a code not yet defined); if next is
;  400h, library.c's lzw_prune(previous, t, 400h) frees codes and returns
;  the new next; then the entry `next` gets the previous code as prefix
;  and the string's first byte as suffix, used[previous] = 1 and
;  used[next] = 0FFh, and next moves to the first code above it whose used
;  byte is 0.  That scan stops at 400h at the latest because the word
;  after used[], which lzw_reset clears, reads as used[400h] = 0.  Loops
;  while *out's offset is below end's offset (a signed compare; end's
;  segment is not read).  Stores t->next and *out's offset, and returns in
;  past the codes read.
_lzw_decode     proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     di,dword ptr [bp+18h]
                les     di,dword ptr [di]       ; es:di: *out
                mov     cl,16-CODE_BITS         ; bits of ch unread
                mov     ch,[bp+6]               ; w's low byte
                mov     dx,[bp+0Eh]             ; dx: next
                jmp     ld_test                 ; out of short range
ld_code:
                lds     si,dword ptr [bp+0Ah]   ; ds:si: in
                or      cl,cl
                jne     ld_one_byte
                lodsw                           ; no bits left: two bytes,
                xchg    al,ah                   ; high byte first
                mov     cl,16-CODE_BITS
                jmp     ld_got
ld_one_byte:
                mov     ah,ch                   ; the leftover bits and
                lodsb                           ; one more byte
                dec     cl
                dec     cl
ld_got:
                mov     ch,al
                mov     [bp+0Ah],si
                shr     ax,cl
                and     ah,3                    ; ax: the 10-bit code
                push    cx
                mov     bx,ax
                xchg    ax,[bp+8]               ; ax: the previous code
                push    ax
                lds     si,dword ptr [bp+14h]   ; ds:si: t
                mov     cx,1
                jmp     ld_test_root
ld_link:
                add     bx,LZW_SUFFIX
                push    word ptr [bx+si]        ; suffix[code], low byte
                inc     cx
                add     bx,bx
                mov     bx,[bx+si-2]            ; 2*LZW_SUFFIX-2 = LZW_PREFIX:
                                                ; code = prefix[code]
ld_test_root:
                or      bh,bh
                jne     ld_link
                push    bx                      ; the root byte
ld_store:
                pop     ax
                stosb
                loop    ld_store
                mov     ax,bx                   ; al: the first byte
                test    dx,LZW_FULL
                je      ld_add
                pop     bx                      ; the previous code
                push    bx
                push    ax
                push    es
                push    dx                      ; lzw_prune(previous, t, next)
                push    ds
                push    si
                push    bx
                call    _lzw_prune
                add     sp,8
                pop     es
                mov     dx,ax                   ; dx: the new next
                pop     ax
ld_add:
                pop     cx                      ; cx: the previous code
                mov     bx,cx
                mov     byte ptr [bx+si],1      ; used[previous] = 1
                mov     bx,dx
                mov     byte ptr [bx+si],0FFh   ; used[next] = 0FFh
                add     bx,LZW_SUFFIX
                mov     [bx+si],al              ; suffix[next]
                add     bx,bx
                mov     [bx+si-2],cx            ; prefix[next] = previous
                mov     bx,dx
ld_find_free:
                inc     bx
                mov     al,[bx+si]
                or      al,al
                jne     ld_find_free
                mov     dx,bx                   ; next: the first free above
                pop     cx
ld_test:
                cmp     di,[bp+10h]
                jge     ld_end
                jmp     ld_code                 ; out of short range
ld_end:
                lds     si,dword ptr [bp+14h]
                mov     [si+LZW_NEXT],dx
                lds     si,dword ptr [bp+18h]
                mov     [si],di                 ; *out's offset
                les     ax,dword ptr [bp+0Ah]
                mov     dx,es                   ; returns in
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_lzw_decode     endp

;  char far *far lz_decode(char far *in, char far *out, int n)  10d7:233d
;
;  Decodes LZ77 byte codes from in to out until the byte 0FFh and returns
;  in past it; n is not read.  Each code starts with a byte b0:
;    b0 < 80h:         b0 b1.  (b1 >> 3) & 3 literal bytes follow; then a
;                      match of (b1 & 7) + 3 bytes at distance
;                      ((b1 >> 5) << 7 | b0) + 1.
;    b0 = 80h..0BFh:   b0 b1 b2.  b2 & 3 literals follow; then a match of
;                      (b1 & 1Fh) + 3 bytes at distance
;                      ((b2 >> 2) << 9 | (b1 >> 5) << 6 | (b0 & 3Fh)) + 1.
;    b0 = 0C0h..0CFh:  ((b0 & 0Fh) + 1) * 4 literals follow.
;    b0 = 0D0h..0FEh:  b0 & 0Fh literals follow.
;    b0 = 0FFh:        the end.
;  A match is copied forwards byte by byte from out - distance, so it may
;  overlap its own output.  The match source is in out's segment.
_lz_decode      proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                push    es
                lds     si,dword ptr [bp+6]     ; ds:si: in
                les     di,dword ptr [bp+0Ah]   ; es:di: out
                mov     ch,0
lz_code:
                lodsb
                or      al,al
                jl      lz_high
                mov     dl,al                   ; b0 < 80h: two bytes
                lodsb
                mov     ah,al
                and     al,18h
                je      lz_short_match
                shr     al,1
                shr     al,1
                shr     al,1
                mov     cl,al
                rep     movsb                   ; the literals
lz_short_match:
                mov     cl,ah
                and     cl,7
                inc     cl
                inc     cl
                inc     cl                      ; cl: the length
                mov     al,ah
                shl     ax,1
                shl     ax,1
                and     ax,380h
                or      al,dl
                inc     ax                      ; ax: the distance
                push    si
                push    ds
                push    es
                pop     ds
                mov     si,di
                sub     si,ax
                rep     movsb                   ; the match
                pop     ds
                pop     si
                jmp     lz_code
lz_high:
                test    al,40h
                jne     lz_literals
                mov     dl,al                   ; 80h..0BFh: three bytes
                lodsb
                mov     ah,al
                lodsb
                mov     dh,al
                and     al,3
                je      lz_long_match
                mov     cl,al
                rep     movsb                   ; the literals
lz_long_match:
                mov     cl,ah
                and     cl,1Fh
                inc     cl
                inc     cl
                inc     cl                      ; cl: the length
                shr     dh,1
                and     dh,7Eh
                and     dl,3Fh
                mov     al,ah
                shl     ax,1
                and     ax,1C0h
                or      ax,dx
                inc     ax                      ; ax: the distance
                push    si
                push    ds
                push    es
                pop     ds
                mov     si,di
                sub     si,ax
                rep     movsb                   ; the match
                pop     ds
                pop     si
                jmp     lz_code
lz_literals:
                test    al,30h
                jne     lz_few_literals
                and     al,0Fh                  ; 0C0h..0CFh: a long run
                inc     al
                shl     al,1
                shl     al,1
                mov     cl,al
                rep     movsb
                jmp     lz_code                 ; out of short range
lz_few_literals:
                cmp     al,LZ_END
                je      lz_end
                and     al,0Fh                  ; 0D0h..0FEh
                mov     cl,al
                rep     movsb
                jmp     lz_code                 ; out of short range
lz_end:
                mov     ax,si
                mov     dx,ds                   ; returns in
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_lz_decode      endp

;  int far lz_match(char far *start, char far *cur, int max, int far *dist)
;                                                               10d7:23f2
;
;  The longest match for the bytes at cur among the earlier bytes from
;  start on, for the encoder of lz_decode's format: scans backwards from
;  cur - 1 for cur's first byte (repne scasb, direction flag set), then
;  counts how many of up to `max` bytes agree (repe cmpsb).  A match
;  replaces the best so far only when longer (a signed byte compare), so
;  among equal lengths the nearest wins; for each new best,
;  *dist = cur - match - 1, the distance as lz_decode stores it.  Returns
;  the best length in ax, 0 if cur's first byte occurs nowhere before it
;  (*dist is then not written).  start and cur are taken to share a
;  segment: cur's offset is read through start's.  No far call to it
;  exists in DQK.EXE.
_lz_match       proc    far
                push    bp
                mov     bp,sp
                push    di
                push    si
                push    es
                push    ds
                lds     bx,dword ptr [bp+6]     ; ds:bx: start
                les     di,dword ptr [bp+0Ah]   ; es:di: cur
                mov     si,di
                mov     dx,[bp+0Eh]             ; dx: max
                mov     ah,0                    ; ah: the best length
                mov     al,[di]                 ; al: cur's first byte
lm_scan:
                mov     cx,di
                dec     di
                sub     cx,bx                   ; the bytes left before
                jbe     lm_done
                std
                repne   scasb
                cld
                jne     lm_done
                inc     di                      ; di: a candidate
                push    bx
                mov     bx,di
                mov     cx,dx
                repe    cmpsb
                mov     si,[bp+0Ah]
                xchg    di,bx                   ; bx: where the compare stopped
                jne     lm_measure
                inc     bx                      ; all `max` bytes agreed
lm_measure:
                sub     bx,di
                dec     bl                      ; bl: the length
                cmp     bl,ah
                jle     lm_next
                mov     ah,bl                   ; a new best
                mov     cx,si
                sub     cx,di
                dec     cx
                push    es
                les     bx,dword ptr [bp+10h]
                mov     es:[bx],cx              ; *dist
                pop     es
lm_next:
                pop     bx
                jmp     lm_scan
lm_done:
                mov     al,ah
                mov     ah,0
                pop     ds
                pop     es
                pop     si
                pop     di
                pop     bp
                ret
_lz_match       endp

_TEXT           ends
                end
