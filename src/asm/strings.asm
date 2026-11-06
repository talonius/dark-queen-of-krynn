;  DQK.EXE resident frame 10d7: the small string and integer helpers --
;  three wrappers on strcmp.asm's case-blind compare, string copy, length
;  and character search, case conversion of a character and of a string in
;  place, and abs, max and min.  EXE 0x0056ac..0x0057df, 307 bytes,
;  10d7:253c..10d7:266f.  strcmp.asm ends at 0x56ac; dosio.asm starts at
;  0x57df.
;
;  Where the file starts is measured: str_icmp, str_same and str_after
;  reach str_cmp_bx (10d7:24de, inside strcmp.asm's str_cmp) by relocated
;  far transfers, which TASM 3.1 (SMART, the default) does not emit within
;  one file (measured; the boundary assumes SMART assembly, see conio.asm).  Where it ends, 266f, is a choice
;  bounded by a measurement: dosio.asm's mem_alloc (266f) far-calls
;  farmem.asm's far_alloc (2820), so a file boundary lies somewhere in
;  266f..2820; this file stops where the string and integer helpers stop.
;  Inside it, str_after shares str_same's `return 0` (10d7:255e jumps back
;  to 2552) and str_upr shares str_lwr's exit (260c to 25f6).
;
;  Register style, every routine: no bp frame; arguments read through
;  bx = sp, from ss:[bx+4]; result in ax or dx:ax.  si, di, ds and es are
;  saved where used; bx and dx are not.  The string routines rely on the
;  direction flag being clear.
;
;  Assembled by TASM 3.1, /ml, single pass.  Three segment fixups, the far
;  jmp at 0x0056b1 (10d7:2541) and the far calls at 0x0056b8 (2548) and
;  0x0056ca (255a), the range's three MZ relocations; _str_cmp_bx is an
;  extern resolved to 10d7:24de.

                .8086

                extrn   _str_cmp_bx:far         ; 10d7:24de, strcmp.asm

CASE_BIT        equ     20h                     ; 'A' + 20h = 'a'

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _str_icmp, _str_same, _str_after
                public  _str_copy, _str_len, _str_find
                public  _to_lower, _to_upper, _str_lwr, _str_upr
                public  _int_abs, _int_max, _int_min, _uint_max, _uint_min

;  int far str_icmp(char far *a, char far *b)                   10d7:253c
;
;  str_cmp under a second name: sets bx = sp and jumps (far) to str_cmp's
;  body, which returns to this routine's caller.  Nothing in DQK.EXE calls
;  it.
_str_icmp       proc    far
                mov     bx,sp
                jmp     _str_cmp_bx
_str_icmp       endp

;  int far str_same(char far *a, char far *b)                   10d7:2543
;
;  1 when str_cmp finds a and b equal (case ignored), else 0.  bx = sp
;  before the far call, so str_cmp_bx reads this routine's arguments.
_str_same       proc    far
                mov     bx,sp
                call    _str_cmp_bx
                or      ax,ax
                jne     return_0
                mov     ax,1
                ret
return_0:
                xor     ax,ax
                ret
_str_same       endp

;  int far str_after(char far *a, char far *b)                  10d7:2555
;
;  1 when str_cmp orders a after b, else 0.  Nothing in DQK.EXE calls it.
_str_after      proc    far
                mov     bx,sp
                call    _str_cmp_bx
                or      ax,ax
                jle     return_0
                mov     ax,1
                ret
_str_after      endp

;  void far str_copy(char far *dst, char far *src)              10d7:2564
;
;  Copies src, with its NUL, to dst.  Note the order: dst first.
_str_copy       proc    far
                mov     bx,sp
                push    si
                push    di
                push    ds
                push    es
                les     di,dword ptr ss:[bx+4]  ; es:di = dst
                lds     si,dword ptr ss:[bx+8]  ; ds:si = src
copy_char:
                lodsb
                stosb
                or      al,al
                jne     copy_char
                pop     es
                pop     ds
                pop     di
                pop     si
                ret
_str_copy       endp

;  int far str_len(char far *s)                                 10d7:257d
;
;  The number of characters before the NUL.
_str_len        proc    far
                mov     bx,sp
                push    si
                push    ds
                lds     si,dword ptr ss:[bx+4]
                mov     bx,si
find_nul:
                lodsb
                or      al,al
                jne     find_nul
                mov     ax,si                   ; one past the NUL
                sub     ax,bx
                dec     ax
                pop     ds
                pop     si
                ret
_str_len        endp

;  char far *far str_find(char far *s, char c)                  10d7:2594
;
;  A pointer to the first c in s; when s has none, a pointer to its NUL
;  (never a null pointer).  str_find(s, 0) finds the NUL.
_str_find       proc    far
                mov     bx,sp
                push    si
                push    ds
                lds     si,dword ptr ss:[bx+4]
                mov     dx,word ptr ss:[bx+8]   ; dl = c
next_char:
                lodsb
                cmp     al,dl
                je      found
                or      al,al
                jne     next_char
found:
                mov     ax,si
                dec     ax                      ; back onto the character
                mov     dx,ds
                pop     ds
                pop     si
                ret
_str_find       endp

;  int far to_lower(int c)                                      10d7:25b1
;
;  c's low byte lowered when it is 'A'..'Z' (signed byte compares, so
;  80h..FFh are left alone); ah is c's high byte, unchanged.
_to_lower       proc    far
                mov     bx,sp
                mov     ax,word ptr ss:[bx+4]
                cmp     al,'A'
                jl      lower_done
                cmp     al,'Z'
                jg      lower_done
                add     al,CASE_BIT
lower_done:
                ret
_to_lower       endp

;  int far to_upper(int c)                                      10d7:25c2
;
;  c's low byte raised when it is 'a'..'z'.
_to_upper       proc    far
                mov     bx,sp
                mov     ax,word ptr ss:[bx+4]
                cmp     al,'a'
                jl      upper_done
                cmp     al,'z'
                jg      upper_done
                sub     al,CASE_BIT
upper_done:
                ret
_to_upper       endp

;  void far str_lwr(char far *s)                                10d7:25d3
;
;  Lowers the capitals of s in place; only the bytes that change are
;  written.  Nothing in DQK.EXE calls it.
_str_lwr        proc    far
                mov     bx,sp
                push    si
                push    di
                push    ds
                push    es
                lds     si,dword ptr ss:[bx+4]
                mov     ax,ds
                mov     es,ax
lwr_next:
                lodsb
                or      al,al
                je      case_done
                cmp     al,'A'
                jl      lwr_next
                cmp     al,'Z'
                jg      lwr_next
                add     al,CASE_BIT
                mov     di,si
                dec     di                      ; the byte just read
                stosb
                jmp     lwr_next
case_done:
                pop     es
                pop     ds
                pop     di
                pop     si
                ret
_str_lwr        endp

;  void far str_upr(char far *s)                                10d7:25fb
;
;  Raises the small letters of s in place.  Leaves through str_lwr's exit.
_str_upr        proc    far
                mov     bx,sp
                push    si
                push    di
                push    ds
                push    es
                lds     si,dword ptr ss:[bx+4]
                mov     ax,ds
                mov     es,ax
upr_next:
                lodsb
                or      al,al
                je      case_done
                cmp     al,'a'
                jl      upr_next
                cmp     al,'z'
                jg      upr_next
                sub     al,CASE_BIT
                mov     di,si
                dec     di
                stosb
                jmp     upr_next
_str_upr        endp

;  int far int_abs(int n)                                       10d7:261e
;
;  Nothing in DQK.EXE calls it.
_int_abs        proc    far
                mov     bx,sp
                mov     ax,word ptr ss:[bx+4]
                or      ax,ax
                jge     abs_done
                neg     ax
abs_done:
                ret
_int_abs        endp

;  int far int_max(int a, int b)                                10d7:262b
_int_max        proc    far
                mov     bx,sp
                mov     ax,word ptr ss:[bx+4]
                mov     bx,word ptr ss:[bx+6]
                cmp     ax,bx
                jge     max_done
                mov     ax,bx
max_done:
                ret
_int_max        endp

;  int far int_min(int a, int b)                                10d7:263c
_int_min        proc    far
                mov     bx,sp
                mov     ax,word ptr ss:[bx+4]
                mov     bx,word ptr ss:[bx+6]
                cmp     ax,bx
                jle     min_done
                mov     ax,bx
min_done:
                ret
_int_min        endp

;  unsigned far uint_max(unsigned a, unsigned b)                10d7:264d
;
;  Nothing in DQK.EXE calls it.
_uint_max       proc    far
                mov     bx,sp
                mov     ax,word ptr ss:[bx+4]
                mov     bx,word ptr ss:[bx+6]
                cmp     ax,bx
                jae     umax_done
                mov     ax,bx
umax_done:
                ret
_uint_max       endp

;  unsigned far uint_min(unsigned a, unsigned b)                10d7:265e
_uint_min       proc    far
                mov     bx,sp
                mov     ax,word ptr ss:[bx+4]
                mov     bx,word ptr ss:[bx+6]
                cmp     ax,bx
                jbe     umin_done
                mov     ax,bx
umin_done:
                ret
_uint_min       endp

_TEXT           ends
                end
