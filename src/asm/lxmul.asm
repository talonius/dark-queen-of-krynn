;  DQK.EXE resident frame 10d7: the long multiply helper Turbo C's code calls for
;  32-bit `*`.  EXE 0x005c69..0x005c7d, 20 bytes, 10d7:2af9..10d7:2b0d.
;
;  Borland's interface, and a Borland body, though not CL.LIB's: the same 20
;  bytes are in TC 2.0's and 2.01's own TC.EXE, TCC.EXE and CPP.EXE (TC-2-01
;  TCC.EXE at 0x26a6e).  dx:ax * cx:bx, the low 32 bits of the
;  product returned in dx:ax.  The same for signed and unsigned operands.  It
;  keeps the low multiplicand in bp (saved) rather than testing for zero high
;  words, so it always does three MULs.  cx is not preserved; bx is.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups, and the MZ
;  relocation table has no entry in this range.

                .8086

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  LXMUL@

;  With a = dx:ax and b = cx:bx: a*b mod 2^32 = lo(a)*lo(b) + ((hi(a)*lo(b) +
;  lo(a)*hi(b)) << 16).  The two cross terms reach the high word only, so only
;  their low halves are kept; hi(a)*hi(b) lies wholly above bit 31.

LXMUL@          proc    far                     ; 10d7:2af9
                push    bp
                mov     bp,ax                   ; bp = multiplicand low
                mov     ax,dx
                mul     bx                      ; multiplicand high * multiplier low
                xchg    ax,cx                   ; cx = that, ax = multiplier high
                mul     bp                      ; multiplier high * multiplicand low
                add     cx,ax                   ; cx = the cross terms
                mov     ax,bp
                mul     bx                      ; dx:ax = low * low
                add     dx,cx
                pop     bp
                ret
LXMUL@          endp

_TEXT           ends
                end
