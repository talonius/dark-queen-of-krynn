;  DQK.EXE resident frame 10d7: env_get, look a variable up in the DOS environment.
;  EXE 0x005d04..0x005d6e, 106 bytes, 10d7:2b94..10d7:2bfe; rest.c's module
;  (1396:000e) starts at the next byte.
;
;  int far env_get(char far *name, char far *value)
;
;  Walks the environment block whose segment is at PSP:2Ch (the PSP's segment
;  is the DGROUP word _psp, 0020) for a "NAME=value" string whose NAME matches
;  `name`, and copies the value, with its NUL, to `value`.  Returns 1 when
;  found; otherwise stores an empty string at `value` and returns 0.  The one
;  caller, pathfind.c, asks for "path".
;
;  The name compare lets a lower-case `name` match the upper-case names DOS
;  keeps: when the two characters differ and the environment's is 'A'
;  (41h) or above, 20h is added to it and they are compared again.  Every byte
;  from 41h up is shifted that way, not only the letters.  The `name` side is
;  used as given.
;
;  C far call, arguments at [bp+6] and [bp+0Ah]; si, di, ds and bp are saved.
;  Hand assembly (lds/les, lodsb), not Turbo C output.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups, and the MZ
;  relocation table has no entry in this range: the one DGROUP word, _psp, is
;  addressed through DS by its offset, which the build resolves.

                .8086

DGROUP          group   _DATA
_DATA           segment word public 'DATA'
                extrn   __psp:word              ; 0020: the PSP's segment
_DATA           ends

PSP_ENV_SEG     equ     2Ch                     ; PSP word: environment segment
CASE_BIT        equ     20h                     ; 'A' + 20h = 'a'

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT, ds:DGROUP

                public  _env_get

_env_get        proc    far                     ; 10d7:2b94
                push    bp
                mov     bp,sp
                push    si
                push    di
                push    ds
                mov     es,__psp
                mov     es,es:[PSP_ENV_SEG]
                mov     di,0                    ; es:di = the first string
next_var:
                mov     ah,es:[di]              ; its first character
                inc     di
                or      ah,ah
                jnz     compare_name
                les     di,dword ptr [bp+0Ah]   ; an empty string: the block's end
                mov     es:[di],ah              ; value = ""
                xor     ax,ax
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret

compare_name:
                lds     si,dword ptr [bp+6]     ; ds:si = name
name_char:
                lodsb                           ; al = name's, ah = the variable's
                cmp     ah,al
                je      chars_match
                cmp     ah,'A'
                jb      mismatch
                add     ah,CASE_BIT
                cmp     ah,al
                jne     mismatch
chars_match:
                mov     ah,es:[di]
                inc     di
                jmp     name_char

mismatch:
                or      al,al                   ; found when the name has ended
                jnz     skip_var                ; and the variable's has too,
                cmp     ah,'='                  ; at its '='
                je      found
skip_var:
                or      ah,ah                   ; on to the byte past this string's NUL
                jz      next_var
                mov     ah,es:[di]
                inc     di
                jmp     skip_var

found:
                lds     si,dword ptr [bp+0Ah]   ; ds:si = value, es:di = after '='
copy_value:
                mov     al,es:[di]
                inc     di
                mov     [si],al
                inc     si
                test    al,0FFh
                jnz     copy_value
                mov     ax,1
                pop     ds
                pop     di
                pop     si
                pop     bp
                ret
_env_get        endp

_TEXT           ends
                end
