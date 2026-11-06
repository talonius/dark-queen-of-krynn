;  DQK.EXE resident frame 10d7: the signed long right-shift helper Turbo C's code
;  calls for `>>` on a long.  EXE 0x005c9c..0x005cba, 30 bytes, 10d7:2b2c..2b4a.
;
;  These bytes are byte-identical to the LRSH module of Turbo C 2.0's and
;  2.01's CL.LIB (wiki games/dqk/runtime-library).  This source is written from
;  DQK's bytes, not copied from Borland's.
;
;  LXRSH@: dx:ax >>= cl, arithmetic (the sign fills from the left), far call,
;  result in dx:ax; bx and cl are not preserved.  A count of 16 or more shifts the
;  high word into the low word by cl - 16 and fills dx with the sign.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups, and the MZ
;  relocation table has no entry in this range.

                .8086

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  LXRSH@

LXRSH@          proc    far                     ; 10d7:2b2c
                cmp     cl,16
                jae     word_or_more
                mov     bx,dx                   ; the high word's low bits ...
                shr     ax,cl
                sar     dx,cl
                neg     cl
                add     cl,16
                shl     bx,cl                   ; ... moved up by 16 - cl
                or      ax,bx                   ; ... into the low word
                ret
word_or_more:
                sub     cl,16
                mov     ax,dx
                cwd                             ; dx = the sign
                sar     ax,cl
                ret
LXRSH@          endp

_TEXT           ends
                end
