;  DQK.EXE resident frame 2eef, its second source file: RTLink/Plus run-time
;  code that finds and opens the overlay file -- an environment lookup, the
;  search along the manager's option string ("C,P,EPATH,A" in DQK.EXE, at
;  manager offset a78), the program's own path from the environment, and
;  the prompt that asks the user for a directory.
;  EXE 0x02156f..0x0218d5, 870 bytes, 2eef:027f..2eef:05e5.  rtlerr.asm
;  ends at 0x2156f; 0x218d5..0x218df is the alignment before the overlay
;  sections' relocation tables.
;
;  Hand assembly with a register interface, as rtlerr.asm.  The manager's
;  rtlink_open_file far-calls rtl_prog_path (for an overlay file whose
;  name ends in .exe) and rtl_find_file, stored 1eef:0421 and 1eef:02db;
;  nothing in DQK.EXE calls rtl_getenv but rtl_find_file.
;
;  Why this is its own file, and starts at 027f, is a choice the bytes
;  support but do not force: nothing here refers to 0009..027e (rtlerr.asm)
;  nor the reverse, and it ends with its own table of message offsets and
;  the messages, as rtlerr.asm does.  Inside it, rtl_find_file reaches every
;  other routine through its try_ routines.
;
;  Assembled by TASM 3.1, /ml, single pass.  No segment fixups, and the MZ
;  relocation table has no entry in this range; every address is an offset
;  in this segment.

                .8086

DOS             equ     21h
DOS_PUTS        equ     09h                     ; ds:dx, ended by '$'
DOS_GETS        equ     0Ah                     ; buffered input to ds:dx
DOS_CURDRIVE    equ     19h                     ; al = default drive, 0 = A
DOS_VERSION     equ     3000h                   ; al = major version
DOS_OPEN        equ     3Dh                     ; ds:dx, al = mode; ax = handle
DOS_CLOSE       equ     3Eh
DOS_DUP         equ     45h                     ; ax = a new copy of handle bx
DOS_FORCEDUP    equ     46h                     ; handle cx becomes a copy of handle bx
DOS_CURDIR      equ     47h                     ; drive dl (0 = default) to ds:si
STDIN           equ     0
STDOUT          equ     1
STDERR          equ     2
PSP_ENV_SEG     equ     2Ch                     ; PSP: the environment's segment
ENV_MAX         equ     8000h                   ; rtl_prog_path's scan limit
CURDIR_MAX      equ     40h                     ; put_cur_dir's scan limit
LIST_SEP        equ     ','                     ; between the options
PATH_SEP        equ     ';'                     ; between an env variable's directories
CASE_BIT        equ     20h

;  rtl_find_file's frame: the registers it saved, by role
F_NAME_SEG      equ     0                       ; es: the file name's segment
F_SPEC_SEG      equ     2                       ; ds: the option string's
F_NAME_OFF      equ     8                       ; di: the file name
F_BUF_SEG       equ     0Ch                     ; cx: the work buffer's segment
F_MODE          equ     0Eh                     ; bl: the DOS open mode
F_AX            equ     10h                     ; ax: the PSP in, the handle out
BUF_PATH        equ     0                       ; work buffer: the path built
BUF_SCRATCH     equ     50h                     ;   an env value, or DOS_GETS's
                                                ;   max, count and text (50h..)

RTL_TEXT        segment byte public 'CODE'
                assume  cs:RTL_TEXT

                public  rtl_getenv, rtl_find_file, rtl_prog_path

;  rtl_getenv                                                    2eef:027f
;
;  ax = the PSP, ds:si = a NUL-ended variable name, es:di = where to put
;  its value.  Copies the value of the first environment string that is
;  the name (compared case for case) followed by '=', NUL-ended.  Carry
;  set when the PSP has no environment or the name is not in it.  Every
;  register is kept.
G_DEST_SEG      equ     2                       ; es
G_NAME          equ     6                       ; si
G_DEST          equ     8                       ; di

rtl_getenv      proc    far
                push    ax
                push    cx
                push    di
                push    si
                push    ds
                push    es
                push    bp
                mov     bp,sp
                mov     es,ax
                mov     cx,es:[PSP_ENV_SEG]
                jcxz    env_missing
                mov     es,cx
                xor     di,di
env_next:
                mov     al,es:[di]
                or      al,al
                je      env_missing             ; the empty string ends the block
                mov     si,[bp+G_NAME]
env_compare:
                lodsb
                or      al,al
                jne     env_char
                cmp     byte ptr es:[di],'='
                je      env_found
                jmp     short env_skip
env_char:
                mov     ah,es:[di]
                inc     di
                cmp     al,ah
                je      env_compare
env_skip:
                xor     al,al                   ; to the next string
                mov     cx,0FFFFh
                repne   scasb
                jmp     env_next
env_found:
                mov     ax,es
                mov     ds,ax
                mov     si,di
                inc     si                      ; past the '='
                mov     es,[bp+G_DEST_SEG]
                mov     di,[bp+G_DEST]
env_copy:
                lodsb
                stosb
                or      al,al
                jne     env_copy
                jmp     short env_return        ; carry clear from the `or`
env_missing:
                stc
env_return:
                pop     bp
                pop     es
                pop     ds
                pop     si
                pop     di
                pop     cx
                pop     ax
                ret
rtl_getenv      endp

;  rtl_find_file                                                 2eef:02db
;
;  Finds and opens a file along a list of places: ax = the PSP, bl = the
;  DOS open mode, cx = the segment of a work buffer, es:di = the file name,
;  ds:si = the options, NUL-ended, separated by ',':
;    C       the current directory of the default drive   (try_current)
;    Ename   each directory in the variable name          (try_env_path)
;    P       the program's own directory, DOS 3 and up    (try_prog_dir)
;    A       a directory the user types                   (try_ask)
;  Any other character is skipped.  Each place is tried as the path of the
;  name at cx:0000.  Carry clear when one opens: ax = the handle, the path
;  is left at cx:0000, and ZF is clear when the name ends in ".exe" (any
;  case).  Carry set when none does.  Other registers are kept.
;  Each try_ routine is entered by a jump with option_done pushed as its
;  return address, bp still rtl_find_file's frame, and returns carry.
;
;  An option string that is empty, ends in ',', or whose last option holds
;  none of C, E, P and A ("Z", "C,Z"; run under emulation) reaches find_failed
;  from the test at 02ee with the two words pushed at 02e7 and 02eb still
;  on the stack, so the pops and the retf take the wrong words.  The
;  manager's string does neither.
rtl_find_file   proc    far
                push    ax
                push    bx
                push    cx
                push    dx
                push    di
                push    si
                push    bp
                push    ds
                push    es
                mov     bp,sp
next_option:
                lodsb
                push    si
                mov     bx,offset option_done
                push    bx
                or      al,al
                je      find_failed             ; see the note above
                cmp     al,'C'
                jne     not_c
                jmp     short try_current
not_c:
                cmp     al,'E'
                jne     not_e
                jmp     short try_env_path
not_e:
                cmp     al,'P'
                jne     not_p
                jmp     try_prog_dir
not_p:
                cmp     al,'A'
                jne     not_option
                jmp     try_ask
not_option:
                add     sp,2
                pop     si
                jmp     next_option
option_done:
                pop     si
                jb      option_failed
                mov     [bp+F_AX],ax            ; the handle
                test    byte ptr cs:name_is_exe,0FFh ; ZF; carry clear
                jmp     short find_return
option_failed:
                mov     ds,[bp+F_SPEC_SEG]      ; to the next ','
skip_option:
                lodsb
                or      al,al
                je      find_failed
                cmp     al,LIST_SEP
                je      next_option
                jmp     skip_option
find_failed:
                stc
find_return:
                pop     es
                pop     ds
                pop     bp
                pop     si
                pop     di
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                ret
rtl_find_file   endp

;  try_current                                                   2eef:0337
;
;  "D:\dir\" of the default drive, then the name.
try_current     proc    near
                mov     es,[bp+F_BUF_SEG]
                xor     di,di
                xor     al,al
                call    put_cur_dir
                call    open_in_dir
                ret
try_current     endp

;  try_env_path                                                  2eef:0345
;
;  ds:si = the variable's name, up to the next ',' or the end, which is
;  NUL-ended in place while rtl_getenv looks it up and then put back.  Its
;  value goes to buffer:BUF_SCRATCH; each ';'-separated directory in it is
;  copied to buffer:0000, given a '\' unless it ends in '\' or ':', and
;  tried.  An empty element goes on to the next one when bl is not zero:
;  bl is the separator that ended the last non-empty element, or, before
;  one, the low byte of the pointer popped at 0365.  So a value that ends
;  in ';' goes on reading past its NUL.
try_env_path    proc    near
                push    si
find_name_end:
                lodsb
                or      al,al
                je      name_end
                cmp     al,LIST_SEP
                jne     find_name_end
name_end:
                mov     di,si
                dec     di
                mov     byte ptr [di],0
                pop     si
                push    ax                      ; the separator and where it was
                push    di
                mov     es,[bp+F_BUF_SEG]
                mov     di,BUF_SCRATCH
                mov     ax,[bp+F_AX]            ; the PSP
                push    cs                      ; a far call within this segment
                call    near ptr rtl_getenv
                pop     bx
                pop     ax
                mov     [bx],al
                jae     have_value
                ret
have_value:
                mov     ax,es
                mov     ds,ax
                mov     si,di
                mov     es,[bp+F_BUF_SEG]
next_dir:
                xor     di,di
copy_dir:
                lodsb
                or      al,al
                je      dir_end
                cmp     al,PATH_SEP
                je      dir_end
                stosb
                mov     ah,al                   ; the last character copied
                jmp     copy_dir
dir_end:
                or      di,di
                je      dir_failed              ; an empty element
                mov     bl,al
                cmp     ah,'\'
                je      dir_ready
                cmp     ah,':'
                je      dir_ready
                mov     al,'\'
                stosb
dir_ready:
                call    open_in_dir
                jb      dir_failed
                clc
                ret
dir_failed:
                or      bl,bl
                jne     next_dir
                stc
                ret
try_env_path    endp

;  try_prog_dir                                                  2eef:03a5
;
;  The directory part of the program's own path (rtl_prog_path), then the
;  name.  Carry set under DOS 2, when rtl_prog_path fails, or when the
;  path has no directory part.
try_prog_dir    proc    near
                mov     ax,DOS_VERSION
                int     DOS
                cmp     al,3
                jae     dos3
                stc
prog_dir_done:
                ret

s_path          db      'PATH',0                ; 2eef:03b0, nothing refers to it

dos3:
                mov     ax,[bp+F_AX]            ; the PSP
                push    cs                      ; a far call within this segment
                call    near ptr rtl_prog_path
                jb      prog_dir_done
                cmp     bx,-1
                jne     have_dir
                stc
                jmp     prog_dir_done
have_dir:
                mov     si,dx
                mov     cx,bx
                sub     cx,dx
                mov     es,[bp+F_BUF_SEG]
                xor     di,di
                rep     movsb
                call    open_in_dir
                ret
try_prog_dir    endp

;  try_ask                                                       2eef:03d7
;
;  Asks for a directory (ask_dir) until one holds the file; carry set when
;  the user enters an empty line.
try_ask         proc    near
                call    ask_dir
                jae     answered
                ret
answered:
                call    open_in_dir
                jb      try_ask
                ret
try_ask         endp

;  put_cur_dir                                                   2eef:03e3
;
;  al = a drive, 1 = A (0: the default drive).  Stores "D:\", the current
;  directory of that drive, and a '\' unless it already ends in one, at
;  es:di, NUL-ended; di is left at the NUL.  ax, cx, dx, si and ds are
;  kept.
put_cur_dir     proc    near
                push    ax
                push    cx
                push    dx
                push    si
                push    ds
                or      al,al
                jne     have_drive
                mov     ah,DOS_CURDRIVE
                int     DOS
                inc     al
have_drive:
                mov     dl,al
                add     al,'A'-1
                stosb
                mov     ax,5C3Ah                ; ':', '\'
                stosw
                mov     si,di
                mov     ax,es
                mov     ds,ax
                mov     ah,DOS_CURDIR
                int     DOS
                xor     al,al                   ; to its NUL
                mov     cx,CURDIR_MAX
                repne   scasb
                dec     di
                cmp     byte ptr es:[di-1],'\'
                je      cur_dir_done            ; the root
                mov     al,'\'
                stosb
cur_dir_done:
                mov     byte ptr es:[di],0
                pop     ds
                pop     si
                pop     dx
                pop     cx
                pop     ax
                ret
put_cur_dir     endp

;  rtl_prog_path                                                 2eef:0421
;
;  ax = the PSP.  Under DOS 3 and up, finds the program's pathname after
;  the environment's strings (past the empty string, where the count word
;  must be 1): ds:dx = the pathname, bx = the offset just past its last
;  '\' or ':' (-1 when it has neither), carry clear.  Carry set, ds, dx
;  and bx kept, under DOS 2, with no environment, or when the count is not
;  1.  ax, cx, di and es are kept.
P_DX            equ     6
P_BX            equ     0Ah

rtl_prog_path   proc    far
                push    ax
                push    bx
                push    cx
                push    dx
                push    di
                push    es
                push    bp
                mov     bp,sp
                mov     di,ax
                mov     ax,DOS_VERSION
                int     DOS
                cmp     al,3
                jae     prog_dos3
no_prog_path:
                stc
prog_path_return:
                pop     bp
                pop     es
                pop     di
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                ret
prog_dos3:
                mov     es,di
                mov     cx,es:[PSP_ENV_SEG]
                jcxz    no_prog_path
                mov     es,cx
                mov     cx,ENV_MAX
                xor     di,di
                xor     al,al
skip_strings:
                cmp     byte ptr es:[di],0
                je      strings_end
                repne   scasb
                jmp     skip_strings
strings_end:
                inc     di
                cmp     word ptr es:[di],1
                jne     no_prog_path
                add     di,2
                mov     dx,di
                mov     bx,-1
scan_path:
                mov     al,es:[di]
                inc     di
                or      al,al
                je      path_scanned
                cmp     al,'\'
                je      mark_dir
                cmp     al,':'
                jne     scan_path
mark_dir:
                mov     bx,di
                jmp     scan_path
path_scanned:
                mov     ax,es
                mov     ds,ax
                mov     [bp+P_DX],dx
                mov     [bp+P_BX],bx
                clc
                jmp     prog_path_return
rtl_prog_path   endp

;  open_in_dir                                                   2eef:048a
;
;  Appends the name to the directory at buffer:di (append_name) and opens
;  buffer:0000 with the caller's mode: carry, and ax = the handle or the
;  DOS error.  dx and ds are kept.  Uses rtl_find_file's frame.
open_in_dir     proc    near
                call    append_name
                push    dx
                push    ds
                mov     ds,[bp+F_BUF_SEG]
                xor     dx,dx
                mov     al,[bp+F_MODE]
                mov     ah,DOS_OPEN
                int     DOS
                pop     ds
                pop     dx
                ret
open_in_dir     endp

;  append_name                                                   2eef:049e
;
;  Copies the NUL-ended file name to buffer:di (di left past the NUL) and
;  sets name_is_exe to 1 when it ends in ".exe" in any case, else 0.  ax,
;  si, ds and es are kept.  Uses rtl_find_file's frame.
append_name     proc    near
                push    ax
                push    si
                push    ds
                push    es
                mov     es,[bp+F_BUF_SEG]
                mov     ds,[bp+F_NAME_SEG]
                mov     si,[bp+F_NAME_OFF]
copy_name:
                lodsb
                stosb
                or      al,al
                jne     copy_name
                mov     byte ptr cs:name_is_exe,0
                mov     ax,es:[di-5]
                or      ah,CASE_BIT
                cmp     ax,652Eh                ; '.', 'e'
                jne     name_checked
                mov     ax,es:[di-3]
                or      ax,CASE_BIT*101h
                cmp     ax,6578h                ; 'x', 'e'
                jne     name_checked
                mov     byte ptr cs:name_is_exe,1
name_checked:
                pop     es
                pop     ds
                pop     si
                pop     ax
                ret
append_name     endp

name_is_exe     db      0                       ; 2eef:04da

;  ask_dir                                                       2eef:04db
;
;  With standard input and output made copies of standard error, prints
;  "Enter directory for NAME (Enter to quit) : ", reads a line into
;  buffer:BUF_SCRATCH, and turns it into a directory at buffer:0000 with
;  di past it: "D:\..." as typed; "\..." on the default drive; "D:..."
;  and "..." after the current directory of that drive or the default;
;  each given a '\' unless it ends in '\' or ':'.  Carry set for an empty
;  line.  Standard input and output are put back and the copies closed.
;  Changes ax, bx, cx, dx, si, di, ds and es.  Uses rtl_find_file's frame.
ask_dir         proc    near
                xor     bx,bx                   ; STDIN
                mov     ah,DOS_DUP
                int     DOS
                push    ax
                mov     bx,STDOUT
                mov     ah,DOS_DUP
                int     DOS
                push    ax
                mov     bx,STDERR
                xor     cx,cx                   ; STDIN
                mov     ah,DOS_FORCEDUP
                int     DOS
                mov     bx,STDERR
                mov     cx,STDOUT
                mov     ah,DOS_FORCEDUP
                int     DOS
                mov     bx,cs
                mov     ds,bx
                assume  ds:RTL_TEXT
                mov     dx,msg_enter
                mov     ah,DOS_PUTS
                int     DOS
                assume  ds:nothing
                xor     di,di                   ; the name, then '$' past its NUL
                call    append_name
                mov     al,'$'
                mov     es,[bp+F_BUF_SEG]
                stosb
                mov     ds,[bp+F_BUF_SEG]
                xor     dx,dx
                mov     ah,DOS_PUTS
                int     DOS
                mov     ds,bx
                assume  ds:RTL_TEXT
                mov     dx,msg_quit
                mov     ah,DOS_PUTS
                int     DOS
                assume  ds:nothing
                mov     ds,[bp+F_BUF_SEG]
                mov     dx,BUF_SCRATCH
                mov     byte ptr ds:[BUF_SCRATCH],0FEh  ; the most it takes
                mov     ah,DOS_GETS
                int     DOS
                mov     cl,ds:[BUF_SCRATCH+1]           ; the count typed
                mov     ds,bx
                assume  ds:RTL_TEXT
                mov     dx,msg_crlf
                mov     ah,DOS_PUTS
                int     DOS
                assume  ds:nothing
                xor     ch,ch
                stc
                jcxz    restore_handles         ; an empty line
                mov     ds,[bp+F_BUF_SEG]
                mov     si,BUF_SCRATCH+2
                mov     es,[bp+F_BUF_SEG]
                xor     di,di
                cmp     byte ptr [si+1],':'
                jne     no_drive
                cmp     byte ptr [si+2],'\'
                je      copy_typed              ; "D:\..."
                mov     al,[si]                 ; "D:...": that drive's directory
                add     si,2
                sub     cx,2
                and     al,not CASE_BIT
                sub     al,'A'-1
                jmp     short drive_dir
no_drive:
                cmp     byte ptr [si],'\'
                jne     relative
                mov     ah,DOS_CURDRIVE         ; "\...": the default drive's letter
                int     DOS
                add     al,'A'
                mov     ah,':'
                stosw
                jmp     short copy_typed
relative:
                xor     al,al                   ; "...": the default drive's directory
drive_dir:
                call    put_cur_dir
copy_typed:
                rep     movsb
                mov     al,es:[di-1]
                cmp     al,'\'
                je      typed_done
                cmp     al,':'
                je      typed_done
                mov     al,'\'
                stosb
typed_done:
                clc
restore_handles:
                lahf
                mov     dx,ax
                pop     bx                      ; the copy of standard output
                mov     cx,STDOUT
                mov     ah,DOS_FORCEDUP
                int     DOS
                mov     ah,DOS_CLOSE
                int     DOS
                pop     bx                      ; the copy of standard input
                xor     cx,cx                   ; STDIN
                mov     ah,DOS_FORCEDUP
                int     DOS
                mov     ah,DOS_CLOSE
                int     DOS
                mov     ax,dx
                sahf
                ret
ask_dir         endp

;  The prompt's three parts, through a table of their offsets (2eef:05b3),
;  each ended by '$' for DOS function 9.
msg_enter       dw      offset s_enter
msg_quit        dw      offset s_quit
msg_crlf        dw      offset s_crlf
s_enter         db      'Enter directory for $'
s_quit          db      ' (Enter to quit) : $'
s_crlf          db      0Dh,0Ah,'$'

RTL_TEXT        ends
                end
