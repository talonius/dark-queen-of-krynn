;  DQK.EXE resident frame 10d7: memory from DOS and words and bytes at an
;  absolute address -- far_alloc and far_free (INT 21h 48h, 49h), and
;  peek and poke for bytes and words.  EXE 0x005990..0x005a25, 149 bytes,
;  10d7:2820..10d7:28b5.  memops.asm ends at 0x5990; sysint.asm starts at
;  0x5a25.
;
;  Boundaries: dosio.asm's mem_alloc (266f) reaches far_alloc by a
;  relocated far call (MZ relocation 0x5808), which TASM 3.1 (SMART, the
;  default) does not emit within one file (measured; see conio.asm), so,
;  given SMART assembly, a file boundary lies in 266f..2820; this file starts at the latest place
;  it can, at far_alloc.  Where it ends, 28b5, is a choice by role and
;  style: every routine here reads its arguments through bx = sp, and
;  int_call after it keeps a bp frame.
;
;  Register style: no bp frame, arguments through bx = sp from ss:[bx+4];
;  result in ax or dx:ax; bx, cx and es are not preserved.
;
;  Assembled by TASM 3.1, /ml, single pass: the forward jmp in far_free
;  takes three bytes, EB xx 90, as it shipped.  No segment fixups, and the
;  MZ relocation table has no entry in this range.  The DGROUP long
;  _far_fallback, this file's _DATA and the last initialized DGROUP
;  data (a25c), is addressed through DS by its offset.

                .8086

DGROUP          group   _DATA
_DATA           segment word public 'DATA'
                public  _far_fallback
_far_fallback   dd      0                       ; a25c: the largest block DOS had,
_DATA           ends                            ;   in bytes, after a refusal

DOS             equ     21h
DOS_ALLOC       equ     48h                     ; bx = paragraphs; ax = the segment
DOS_FREE        equ     49h                     ; es = the segment

_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT, ds:DGROUP

                public  _far_alloc, _far_free
                public  _peek_byte, _poke_byte, _peek_word, _poke_word

;  void far *far far_alloc(long n)                              10d7:2820
;
;  Asks DOS for (n + 15) / 16 paragraphs (the count's low 16 bits) and
;  returns segment:0.  When DOS refuses, stores the largest block it has,
;  in bytes, in far_fallback and returns 0:0; fcache.c retries with that
;  size.
_far_alloc      proc    far
                mov     bx,sp
                mov     ax,word ptr ss:[bx+6]   ; n, high word
                mov     bx,word ptr ss:[bx+4]   ;   low word
                add     bx,15
                adc     ax,0
                mov     cl,4
                shr     bx,cl
                ror     ax,cl                   ; the high word's low nibble
                and     ah,0F0h                 ;   becomes the paragraphs'
                or      bh,ah                   ;   top four bits
                mov     ah,DOS_ALLOC
                int     DOS
                jae     alloc_done              ; ax = the segment
                mov     ax,bx                   ; bx = the largest block, paragraphs
                mov     cl,4
                shl     ax,cl
                mov     word ptr _far_fallback,ax
                mov     ax,bx
                rol     ax,cl
                and     ax,0Fh
                mov     word ptr _far_fallback+2,ax
                xor     ax,ax                   ; 0:0
alloc_done:
                mov     dx,ax
                xor     ax,ax
                ret
_far_alloc      endp

;  int far far_free(void far *p)                                10d7:285b
;
;  Gives p's segment back to DOS (p's offset is not read); returns 0, or
;  the negated error code.  Leaves es = ds.
_far_free       proc    far
                mov     bx,sp
                mov     es,word ptr ss:[bx+6]   ; p's segment
                mov     ah,DOS_FREE
                int     DOS
                jae     free_ok
                neg     ax
                jmp     free_done
free_ok:
                xor     ax,ax
free_done:
                mov     bx,ds
                mov     es,bx
                ret
_far_free       endp

;  int far peek_byte(unsigned seg, unsigned off)                10d7:2873
;
;  The byte at seg:off, zero-extended.  dispinit.c reads the BIOS model
;  byte (F000:FFFE) and the equipment word's low byte (0040:0010) with it.
_peek_byte      proc    far
                mov     bx,sp
                mov     es,word ptr ss:[bx+4]
                mov     bx,word ptr ss:[bx+6]
                mov     al,byte ptr es:[bx]
                xor     ah,ah
                ret
_peek_byte      endp

;  void far poke_byte(unsigned seg, unsigned off, int b)        10d7:2883
_poke_byte      proc    far
                mov     bx,sp
                mov     es,word ptr ss:[bx+4]
                mov     ax,word ptr ss:[bx+8]
                mov     bx,word ptr ss:[bx+6]
                mov     byte ptr es:[bx],al
                ret
_poke_byte      endp

;  int far peek_word(unsigned seg, unsigned off)                10d7:2895
_peek_word      proc    far
                mov     bx,sp
                mov     es,word ptr ss:[bx+4]
                mov     bx,word ptr ss:[bx+6]
                mov     ax,word ptr es:[bx]
                ret
_peek_word      endp

;  void far poke_word(unsigned seg, unsigned off, int w)        10d7:28a3
_poke_word      proc    far
                mov     bx,sp
                mov     es,word ptr ss:[bx+4]
                mov     ax,word ptr ss:[bx+8]
                mov     bx,word ptr ss:[bx+6]
                mov     word ptr es:[bx],ax
                ret
_poke_word      endp

_TEXT           ends
                end
