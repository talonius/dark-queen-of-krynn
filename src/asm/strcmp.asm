;  DQK.EXE resident frame 10d7: str_cat, and str_cmp, the case-blind string
;  compare.  EXE 0x005627..0x0056ac, 133 bytes, 10d7:24b7..10d7:253c.
;  cputs.asm ends at 0x5627; strings.asm starts at 0x56ac.
;
;  Where the file ends is measured: str_cmp has a second entry, str_cmp_bx
;  (10d7:24de, past its `mov bx,sp`), which the next three routines reach by
;  relocated far transfers -- a far jmp at 10d7:253e and far calls at 2545
;  and 2557 (MZ relocations 0x56b1, 0x56b8, 0x56ca).  TASM 3.1 in its
;  default SMART mode assembles a far call or jmp to a far label in its own
;  segment as `push cs; call near` or a short jmp (measured), so, given the
;  frame was assembled that way (see conio.asm), those routines, from 253c
;  on, are in another file.  Where it starts, 24b7, is
;  a choice (see cputs.asm).  str_cat and str_cmp are kept together as
;  string routines; their styles differ (str_cat: bp frame, saves si, di,
;  ds, es; str_cmp: bx = sp, saves ds, es, si, di), so they may as well have
;  been two files.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups, and the MZ
;  relocation table has no entry in this range.

                .8086

CASE_BIT        equ     20h                     ; 'A' + 20h = 'a'

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _str_cat, _str_cmp, _str_cmp_bx

;  void far str_cat(char far *dst, char far *src)               10d7:24b7
;
;  Appends src, with its NUL, at dst's NUL.  C far call, arguments from
;  [bp+6]; si, di, ds and es are saved.  Relies on the direction flag being
;  clear.
_str_cat        proc    far
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                push    es
                lds     si,dword ptr [bp+6]     ; ds:si = dst
find_end:
                lodsb
                or      al,al
                jne     find_end
                dec     si                      ; back onto the NUL
                mov     di,si
                mov     ax,ds
                mov     es,ax                   ; es:di = dst's NUL
                lds     si,dword ptr [bp+0Ah]   ; ds:si = src
append:
                lodsb
                stosb
                or      al,al
                jne     append
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_str_cat        endp

;  int far str_cmp(char far *a, char far *b)                    10d7:24dc
;
;  Compares a and b as unsigned bytes with the case of 'A'..'Z' ignored;
;  returns -1, 0 or 1.  When the two characters differ and the larger is
;  above 'Z' while the smaller is a capital, the capital is lowered and the
;  pair compared again; any other difference decides.  That orders the
;  strings as if every capital were lowered first: "ABC" equals "abc", and
;  '[' .. '`' (5Bh..60h) sort below every letter of either case.
;
;  Arguments through bx = sp, at ss:[bx+4] and ss:[bx+8].  ds, es, si and di
;  are saved; bx is not.  Clears the direction flag.  The second entry,
;  str_cmp_bx (10d7:24de), takes bx already pointing at a caller's frame:
;  strings.asm's str_icmp, str_same and str_after set bx = sp and reach it
;  by far jmp or far call, so the arguments read are their callers'.
_str_cmp        proc    far
                mov     bx,sp
_str_cmp_bx     label   far                     ; 10d7:24de, bx = the frame
                push    ds
                push    es
                push    si
                push    di
                lds     si,dword ptr ss:[bx+4]  ; ds:si = a
                les     di,dword ptr ss:[bx+8]  ; es:di = b
                xor     ax,ax                   ; ah stays 0
                cld
next_pair:
                lodsb                           ; al = *a++
                mov     bl,byte ptr es:[di]     ; bl = *b++
                inc     di
                cmp     al,bl
                ja      a_above
                jb      a_below
                or      al,al
                jne     next_pair
                pop     di                      ; both ended: ax = 0
                pop     si
                pop     es
                pop     ds
                ret

a_above:                                        ; al > bl
                cmp     bl,'Z'
                ja      return_1
                cmp     al,'Z'
                jbe     return_1
                cmp     bl,'A'
                jb      return_1
                add     bl,CASE_BIT             ; b's capital, lowered
                cmp     al,bl
                je      next_pair
                jb      return_minus_1
return_1:
                mov     al,1
                pop     di
                pop     si
                pop     es
                pop     ds
                ret

a_below:                                        ; al < bl
                cmp     al,'Z'
                ja      return_minus_1
                cmp     bl,'Z'
                jbe     return_minus_1
                cmp     al,'A'
                jb      return_minus_1
                add     al,CASE_BIT             ; a's capital, lowered
                cmp     al,bl
                je      next_pair
                ja      return_1
return_minus_1:
                mov     ax,-1
                pop     di
                pop     si
                pop     es
                pop     ds
                ret
_str_cmp        endp

_TEXT           ends
                end
