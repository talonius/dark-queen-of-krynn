;  DQK.EXE resident frame 10d7: the long division helper Turbo C's code calls for
;  32-bit `/` and `%`.  EXE 0x005be2..0x005c69, 135 bytes, 10d7:2a72..10d7:2af9.
;
;  It keeps Borland's interface and four entry points (LDIV@, LUDIV@, LMOD@,
;  LUMOD@), and its 32-step shift-subtract loop and its negate-and-exit match
;  TC 2.0's LDIV instruction for instruction; the rest is a variant: bp is set
;  before si and di are saved, the selector lives in bx, there is no quick 32/16
;  path (every division runs the loop), and there is no separate remainder-sign
;  bit.  No library or toolchain on disk holds this body.
;
;  Calling convention (Borland's): far call, the divisor pushed first, then the
;  dividend, each a long (low word at the lower address); the helper pops both
;  (retf 8) and returns the result in dx:ax.  ax, bx, cx, dx are not preserved.
;
;  ⚠ Sign handling (the shipped bytes, run under emulation): one flag, bit 2 of
;  the selector, is the XOR of the two operands' signs, and it negates whichever
;  result is returned.  So the signed remainder takes the quotient's sign, not
;  the dividend's: 7 % -2 gives -1 where C gives 1, and -7 % 2 gives -1, -7 % -2
;  gives 1.  A zero divisor is not trapped (there is no div instruction): every
;  step subtracts zero, so LUDIV@ returns all ones and LUMOD@ the dividend; the
;  signed entries then apply the sign step, so LDIV@(5,0) is -1, LDIV@(-5,0) is
;  1 and LMOD@(-5,0) is -5.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups, and the MZ
;  relocation table has no entry in this range.

                .8086

SEL_UNSIGNED    equ     1                       ; selector bit 0: unsigned operands
SEL_REMAINDER   equ     2                       ; selector bit 1: return the remainder
SEL_NEGATE      equ     4                       ; selector bit 2: negate the result
LONG_BITS       equ     32                      ; one loop step per dividend bit

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  LDIV@, LUDIV@, LMOD@, LUMOD@

;  The four entries load the selector and fall into one body.

LDIV@           proc    far                     ; 10d7:2a72  signed quotient
                xor     bx,bx
                jmp     short ldiv_body
LDIV@           endp

LUDIV@          proc    far                     ; 10d7:2a76  unsigned quotient
                mov     bx,SEL_UNSIGNED
                jmp     short ldiv_body
LUDIV@          endp

LMOD@           proc    far                     ; 10d7:2a7b  signed remainder
                mov     bx,SEL_REMAINDER
                jmp     short ldiv_body
LMOD@           endp

LUMOD@          proc    far                     ; 10d7:2a80  unsigned remainder
                mov     bx,SEL_UNSIGNED or SEL_REMAINDER
                                                ; falls into the body

ldiv_body:                                      ; 10d7:2a83
                push    bp
                mov     bp,sp
                push    si
                push    di
                mov     di,bx                   ; di = selector while signs are taken
                mov     ax,[bp+6]               ; dx:ax = dividend
                mov     dx,[bp+8]
                mov     bx,[bp+0Ah]             ; cx:bx = divisor
                mov     cx,[bp+0Ch]
                test    di,SEL_UNSIGNED
                jnz     magnitudes
                or      dx,dx                   ; signed: work on magnitudes
                jns     dividend_positive
                neg     dx                      ; negate dx:ax
                neg     ax
                sbb     dx,0
                or      di,SEL_NEGATE
dividend_positive:
                or      cx,cx
                jns     magnitudes
                neg     cx                      ; negate cx:bx
                neg     bx
                sbb     cx,0
                xor     di,SEL_NEGATE

;  Shift the dividend left through di:si, one bit a step; whenever the partial
;  remainder reaches the divisor (bp:bx) subtract it and set the quotient bit
;  that the shift just freed in ax.  After 32 steps dx:ax is the quotient and
;  di:si the remainder.

magnitudes:
                mov     bp,LONG_BITS            ; bp = divisor high, cx = step count
                xchg    bp,cx
                push    di                      ; keep the selector
                xor     di,di                   ; di:si = partial remainder
                xor     si,si
divide_step:
                shl     ax,1
                rcl     dx,1
                rcl     si,1
                rcl     di,1
                cmp     di,bp
                jb      next_step
                ja      subtract
                cmp     si,bx
                jb      next_step
subtract:
                sub     si,bx
                sbb     di,bp
                inc     ax
next_step:
                loop    divide_step

                pop     bx                      ; bx = selector
                test    bx,SEL_REMAINDER
                jz      have_result
                mov     ax,si                   ; the remainder instead
                mov     dx,di
have_result:
                test    bx,SEL_NEGATE
                jz      done
                neg     dx
                neg     ax
                sbb     dx,0
done:
                pop     di
                pop     si
                pop     bp                      ; the frame's bp, not the divisor
                ret     8
LUMOD@          endp

_TEXT           ends
                end
