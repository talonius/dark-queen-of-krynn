;  DQK.EXE resident frame 10d7: con_puts, write a DGROUP string to standard
;  output a character at a time through conio.asm's con_putc_dl.  EXE
;  0x005610..0x005627, 23 bytes, 10d7:24a0..10d7:24b7.  conio.asm ends at
;  0x5610; strcmp.asm starts at 0x5627.
;
;  Why a file of its own: the far call to con_putc_dl (10d7:2483, inside
;  conio.asm's con_putc) is relocated, 9A 83 24 D7 00; TASM 3.1 (SMART, the
;  default) would have made it `push cs; call near` (0E E8) within one file
;  (measured, given SMART assembly; see conio.asm).  Where it ends, 24b7, is a choice: str_cat
;  after it keeps a bp frame, a different house style, and is a string
;  routine, not console output; nothing in the bytes ties them either way.
;
;  Register style: no bp frame, the argument read through bx = sp.
;
;  Assembled by TASM 3.1, /ml, single pass.  One segment fixup, the far
;  call at 0x005621 (10d7:24b1), the range's one MZ relocation;
;  _con_putc_dl is an extern resolved to 10d7:2483.

                .8086

                extrn   _con_putc_dl:far        ; 10d7:2483, conio.asm

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _con_puts

;  void far con_puts(char *s)                                   10d7:24a0
;
;  s is a NEAR pointer, read through ds (DGROUP); its characters up to the
;  NUL go to con_putc_dl, '\n' as CR LF.  si is saved.  Nothing in DQK.EXE
;  calls it.
_con_puts       proc    far
                mov     bx,sp
                push    si
                mov     si,word ptr ss:[bx+4]
next_char:
                lodsb
                or      al,al
                je      done
                mov     dl,al
                call    _con_putc_dl
                jmp     next_char
done:
                pop     si
                ret
_con_puts       endp

_TEXT           ends
                end
