;  DQK.EXE resident frame 10d7: the structure copy helper Turbo C's code calls for
;  struct assignment.  EXE 0x005cd9..0x005d04, 43 bytes, 10d7:2b69..10d7:2b94.
;
;  Borland's interface: far call with cx = the byte count and two far pointers
;  on the stack, the source at [bp+6] (loaded as ds:si) and the destination at
;  [bp+0Ah] (es:di); the helper pops both (retf 8).  ax, cx, dx and es are not
;  preserved.
;
;  Not Borland's body: Borland's copies forward by words; this one copies by
;  bytes and picks a direction.  It copies BACKWARD when the source's segment
;  is above the destination's, or, failing that, when the source's offset is
;  above the destination's; otherwise forward.  Run under emulation, that is
;  the reverse of the direction an overlapping copy needs: with the source
;  above, the backward copy overwrites source bytes before it reads them, and
;  with the source below, the forward copy does the same, so an overlapping
;  copy is corrupted either way; for blocks that do not overlap the direction
;  makes no difference.  The forward path relies on the direction
;  flag being clear on entry; both paths clear it on the way out.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups, and the MZ
;  relocation table has no entry in this range.

                .8086

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  SCOPY@

SCOPY@          proc    far                     ; 10d7:2b69
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                lds     si,dword ptr [bp+6]     ; ds:si = source
                les     di,dword ptr [bp+0Ah]   ; es:di = destination
                mov     ax,ds
                mov     dx,es
                cmp     ax,dx
                ja      backward
                cmp     si,di
                ja      backward
copy:
                rep     movsb
                cld
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret     8
backward:
                add     si,cx                   ; start at the last byte of each
                add     di,cx
                dec     si
                dec     di
                std
                jmp     copy
SCOPY@          endp

_TEXT           ends
                end
