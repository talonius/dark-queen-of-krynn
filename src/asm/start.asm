;  DQK.EXE resident frame 1000: the start-up.  It is not Turbo C 2.0's or 2.01's
;  C0L; who wrote it is not established.  EXE 0x002400..0x0024c3, 195 bytes from
;  1000:0000; its last two instructions overlap the first paragraph of frame 100c,
;  whose 100c:0003 is the alignment pad ahead of AIL.  The image's entry point is
;  RTLink's overlay manager (1d99:076c), which jumps here with ES = the PSP.
;
;  It makes DS and SS DGROUP, copies the command tail onto the stack with the
;  program's path and a space ahead of it ("C " instead under DOS 2; under DOS 1
;  it takes int 20h), zeroes DGROUP a260..db5f (the _BSS of every module), and
;  far-calls start_main (cmdargs.c, 2c52:0002) with that string.  When start_main
;  returns it exits to DOS with code 0.
;
;  Assembled by TASM 3.1, /ml, single pass: a forward jmp takes three bytes, EB xx
;  90, as it shipped.  The two segment fixups are the range's two MZ relocations,
;  0x2401 (DGROUP) and 0x24bc (start_main's frame, stored 1c52).  The two DGROUP
;  words are externs the build resolves to their offsets: _psp (0020) and
;  _start_sp (0026).

                .8086

ifdef LINKED
;  The RTLink/Plus build (build/DQK.LNK assembles this file with /dLINKED).
;  As the first object linked, this file fixes the segment order: code, then
;  DGROUP's _DATA, _BSS and the _BSSEND marker, then a 1K stack segment, as in
;  the shipped image (its SS:SP, 3fe4:0400, is DGROUP + db60h, the end of the
;  zeroed region).  The zeroed region is bss_start.._bss_end, resolved by the
;  linker, instead of the shipped a260..db5f; _bss_end is where startdat.c's
;  heap_brk starts.
START_TEXT      segment byte public 'CODE'
START_TEXT      ends
_DATA           segment word public 'DATA'
_DATA           ends
_BSS            segment word public 'BSS'
bss_start       label   byte
_BSS            ends
_BSSEND         segment byte public 'BSSEND'
                public  _bss_end
_bss_end        label   byte
_BSSEND         ends
_STACK          segment para stack 'STACK'
                db      400h dup (?)
_STACK          ends
DGROUP          group   _DATA, _BSS, _BSSEND
else
DGROUP          group   _DATA
endif
_DATA           segment word public 'DATA'
                extrn   __psp:word              ; 0020: the PSP's segment
                extrn   _start_sp:word          ; 0026: SP after the stack is set up
_DATA           ends

                extrn   _start_main:far         ; 1c52:0002, cmdargs.c

DGROUP_PARAS    equ     1000h                   ; DGROUP's 64K, data and stack
BSS_START       equ     0A260h                  ; DGROUP's _BSS, zeroed at start-up
BSS_BYTES       equ     3901h                   ; halved to 1c80h words: a260..db5f

START_TEXT      segment byte public 'CODE'
                assume  cs:START_TEXT, ds:DGROUP

start:                                          ; 1000:0000
                mov     ax,DGROUP
                mov     ds,ax
                mov     __psp,es
                mov     bx,es:[2]               ; PSP: top of the memory block
                sub     bx,ax                   ; paragraphs from DGROUP to the top
                test    bx,0F000h
                jnz     shrink_block            ; 64K or more: stack at DGROUP:fffe
                mov     cl,4
                shl     bx,cl
                mov     ss,ax                   ; less: stack at the block's top
                mov     sp,bx
                jmp     stack_ready
shrink_block:
                mov     ss,ax
                mov     sp,0FFFEh
                mov     bx,ds                   ; keep the PSP through DGROUP + 64K
                sub     bx,__psp
                add     bx,DGROUP_PARAS
                mov     ah,4Ah                  ; DOS: resize the block at ES
                int     21h
stack_ready:
                mov     _start_sp,sp

                ; The command tail, NUL-ended, into a word-rounded hole on the stack.
                mov     si,80h
                mov     cl,es:[si]              ; its length byte
                inc     si
                xor     ch,ch
                mov     bx,cx
                shr     bx,1
                inc     bx
                shl     bx,1
                sub     sp,bx
                mov     di,sp
                jcxz    tail_copied
copy_tail:
                lods    byte ptr es:[si]
                mov     [di],al
                inc     di
                loop    copy_tail
tail_copied:
                xor     al,al
                mov     [di],al

                mov     ah,30h                  ; DOS: version
                int     21h
                cmp     al,3
                jnb     find_path
                or      al,al
                jnz     dos2_name
                int     20h                     ; DOS 1
dos2_name:
                mov     ax,2043h                ; DOS 2: no path; "C " ahead of the tail
                push    ax
                jmp     call_main

                ; DOS 3 or later: the program's path follows the environment's
                ; double NUL and a word count.
find_path:
                mov     es,es:[2Ch]             ; PSP: environment segment
                xor     si,si
skip_vars:
                lods    byte ptr es:[si]
                or      al,al
                jnz     skip_vars
                lods    byte ptr es:[si]
                or      al,al
                jnz     skip_vars
                inc     si
                inc     si
                mov     cx,si
find_path_end:
                lods    byte ptr es:[si]
                or      al,al
                jnz     find_path_end
                xchg    cx,si
                sub     cx,si                   ; the path's length with its NUL
                sub     sp,cx
                dec     cx
                mov     di,sp
copy_path:
                lods    byte ptr es:[si]
                mov     [di],al
                inc     di
                loop    copy_path
                mov     byte ptr [di],' '       ; where the NUL would go: path, space, tail

call_main:
                mov     ax,sp                   ; the string's offset
                test    al,1
                jz      sp_even
                dec     sp
sp_even:
                push    ds                      ; start_main's far pointer
                push    ax
                mov     ax,ds
                mov     es,ax
ifdef LINKED
                ; The same ten bytes as the shipped sequence: no jz, since
                ; rep stosw with cx = 0 stores nothing; an odd last byte is
                ; left, as the shipped 3901h was.
                mov     cx,offset DGROUP:_bss_end
                mov     di,offset DGROUP:bss_start
                sub     cx,di
                shr     cx,1
else
                mov     cx,BSS_BYTES
                shr     cx,1
                jz      bss_zeroed
                mov     di,BSS_START
endif
                xor     ax,ax
                rep     stosw
bss_zeroed:
                call    _start_main
                mov     ax,4C00h                ; DOS: exit, code 0
                int     21h
START_TEXT      ends
ifdef LINKED
                end     start                   ; the entry RTLink's manager reaches
else
                end
endif
