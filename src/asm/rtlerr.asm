;  DQK.EXE resident frame 2eef, its first source file: RTLink/Plus run-time
;  code beside the overlay manager (frame 2d99) -- the hex formatter the
;  manager's error messages use, the fatal-error exit and the vector it is
;  reached through, a table of exit calls and the hook on the PSP's terminate
;  address that runs them, and three two-instruction stubs.
;  EXE 0x0212f7..0x02156f, 632 bytes, 2eef:0007..2eef:027f.  rtlfind.asm
;  starts at 0x2156f.
;
;  What precedes it: 2eef:0000..0006 (0x212f0..0x212f6) is the tail of the
;  last of the manager's 182 vector stubs.  The file opens with
;  rtl_cache_none at 2eef:0007, `stc; retf`: the manager's three far calls
;  to it (from rtlink_evict_section, rtlink_load_section and
;  rtlink_init_workspace) are stored 1eef:0007, the frame of this segment,
;  so it is this segment's first routine, not manager code; that it is this
;  file's rather than a two-byte file of its own is a choice.
;
;  Hand assembly with a register interface: arguments and results in
;  registers, the others kept by the routines that return (rtl_fatal and
;  rtl_internal_error do not).  The manager reaches rtl_hex_word
;  (from rtlink_load_section and rtlink_check_vectors), rtl_save_term and
;  rtl_on_exit (rtlink_init_workspace) and rtl_stub_0202 (rtlink_close_file)
;  by far calls stored 1eef:OFF, and fatal_vector and exe_ovl_offset through
;  far pointers it holds at manager offsets a6b and bf9.  rtl_set_fatal,
;  rtl_exit_cleanup, rtl_stub_0206 and rtl_stub_020a are called by nothing
;  in DQK.EXE.
;
;  Why these routines are one file, and why the file ends at 027f, is a
;  choice the bytes support but do not force: the message table at 020f,
;  which rtl_fatal and rtl_internal_error read, follows the stubs, so the
;  code, the stubs and the messages go together; nothing in 0009..027e
;  refers to 027f..05e4, nor the reverse, and each half ends with its own
;  table of message offsets and the messages.  The hex formatter at 0009
;  shares no data and no call with the rest and is kept with it by role:
;  the manager uses it to write numbers into its error messages.
;
;  Assembled by TASM 3.1, /ml, single pass.  One segment fixup: fatal_vector's
;  segment word at 2eef:0050 (0x21340), the range's one MZ relocation (stored
;  1eef).  Every other address is an offset in this segment.

                .8086

DOS             equ     21h
VIDEO           equ     10h
DOS_PUTCHAR     equ     02h                     ; dl
DOS_PUTS        equ     09h                     ; ds:dx, ended by '$'
DOS_GETCH       equ     07h                     ; no echo
DOS_FORCEDUP    equ     46h                     ; handle cx becomes a copy of handle bx
DOS_EXIT_1      equ     4C01h                   ; terminate with code 1
VID_GETMODE     equ     0Fh                     ; al = the video mode
STDOUT          equ     1
STDERR          equ     2
PSP_TERM_IP     equ     0Ah                     ; PSP: the terminate address (int 22h)
PSP_TERM_CS     equ     0Ch
EXIT_SLOTS      equ     8                       ; exit_table: far routine, mask byte,
EXIT_ENTRY      equ     6                       ;   a pad byte
ERR_INTERNAL    equ     6                       ; ah for rtl_internal_error

RTL_TEXT        segment byte public 'CODE'
                assume  cs:RTL_TEXT

                public  rtl_cache_none
                public  rtl_hex_word, rtl_set_fatal, rtl_fatal, rtl_exit_cleanup
                public  rtl_save_term, rtl_on_exit, rtl_internal_error
                public  rtl_stub_0202, rtl_stub_0206, rtl_stub_020a
                public  exe_ovl_offset, fatal_vector

;  rtl_cache_none                                           2eef:0007
;
;  Returns carry set and nothing else.  The manager far-calls it while
;  evicting, loading and initialising sections; goldbox.toml's manager_names
;  called it rtlink_unavailable_cache_hook, reading it as the answer of an
;  overlay cache that is not linked in.  That role is a reading of the
;  callers, not shown by these two bytes.
rtl_cache_none  proc    far
                stc
                ret
rtl_cache_none  endp

;  rtl_hex_word                                                  2eef:0009
;
;  cx as four upper-case hex digits at ds:dx.  ax, di and es are kept.  The
;  manager writes section numbers and vector addresses into its error
;  messages with it.
rtl_hex_word    proc    far
                push    ax
                push    di
                push    es
                mov     ax,ds
                mov     es,ax
                mov     ax,cx
                mov     di,dx
                call    hex_ax
                pop     es
                pop     di
                pop     ax
                ret
rtl_hex_word    endp

;  hex_ax, hex_al, hex_digit                          2eef:001b, 0022, 002f
;
;  ax, al, or al's low nibble as hex digits stored at es:di++.  Each of the
;  first two does its high half by a call and falls into the next for the
;  low half.  The digit: 0..9 + 90h, daa, adc 40h, daa gives '0'..'9' and
;  'A'..'F'.
hex_ax          proc    near
                push    ax
                mov     al,ah
                call    hex_al
                pop     ax
hex_ax          endp                            ; falls into hex_al

hex_al          proc    near
                push    ax
                shr     al,1
                shr     al,1
                shr     al,1
                shr     al,1
                call    hex_digit
                pop     ax
hex_al          endp                            ; falls into hex_digit

hex_digit       proc    near
                and     al,0Fh
                add     al,90h
                daa
                adc     al,40h
                daa
                stosb
                ret
hex_digit       endp

;  exe_ovl_offset                                                2eef:0039
;
;  A long the manager adds to its seek position when it reads a section from
;  a file it found as an .exe (rtlink_load_section, through its far pointer
;  at manager offset bf9).  Zero here: the sections' positions in DQK.EXE are
;  absolute.
exe_ovl_offset  dd      0

;  rtl_set_fatal                                                 2eef:003d
;
;  Installs ds:si as the fatal-error routine and returns the one it replaces
;  in ds:si.  ax is kept.
rtl_set_fatal   proc    far
                push    ax
                xchg    cs:fatal_off,si
                mov     ax,ds
                xchg    cs:fatal_seg,ax
                mov     ds,ax
                pop     ax
                ret
rtl_set_fatal   endp

;  fatal_vector, the routine the manager's rtlink_error and rtl_internal_error
;  jump to (rtl_fatal unless rtl_set_fatal replaced it); saved_term, the
;  terminate address rtl_save_term found in the PSP; psp_seg, that PSP.
fatal_vector    label   dword                   ; 2eef:004e
fatal_off       dw      offset rtl_fatal
fatal_seg       dw      seg rtl_fatal           ; 2eef:0050, the MZ relocation
saved_term      label   dword                   ; 2eef:0052
saved_term_off  dw      0
saved_term_seg  dw      0
psp_seg         dw      0                       ; 2eef:0056

;  rtl_fatal                                                     2eef:0058
;
;  The fatal-error exit, entered by a far jump with ds:bx the address of a
;  list of NUL-ended strings, ended by a zero word (its callers also set ax
;  and cx, which this routine does not read).  It
;  makes standard output a copy of standard error, prints "Fatal Error "
;  and the strings, and, when the screen is in a graphics mode (not 0..3
;  and not 7), "Press any key to terminate." and waits for a key.  Exits to
;  DOS with code 1.
rtl_fatal       proc    far
                push    bx
                mov     bx,STDERR
                mov     cx,STDOUT
                mov     ah,DOS_FORCEDUP
                int     DOS
                pop     bx
                push    ds
                mov     ax,cs
                mov     ds,ax
                assume  ds:RTL_TEXT
                mov     dx,msg_fatal
                mov     ah,DOS_PUTS
                int     DOS
                pop     ds
                assume  ds:nothing
next_string:
                mov     si,[bx]
                or      si,si
                je      strings_done
put_char:
                lodsb
                or      al,al
                je      string_done
                mov     dl,al
                mov     ah,DOS_PUTCHAR
                int     DOS
                jmp     put_char
string_done:
                add     bx,2
                jmp     next_string
strings_done:
                mov     ax,cs
                mov     ds,ax
                assume  ds:RTL_TEXT
                mov     ah,VID_GETMODE
                int     VIDEO
                cmp     al,3
                jbe     terminate               ; a colour text mode
                cmp     al,7
                je      terminate               ; monochrome text
                mov     dx,msg_press
                mov     ah,DOS_PUTS
                int     DOS
                mov     ah,DOS_GETCH
                int     DOS
                assume  ds:nothing
terminate:
                mov     ax,DOS_EXIT_1
                int     DOS
rtl_fatal       endp

;  rtl_exit_cleanup                                              2eef:00ab
;
;  Calls the exit routines registered with mask bit 0, then puts the PSP's
;  terminate address back and clears term_hooked.  ax and es are kept.
rtl_exit_cleanup proc   far
                push    ax
                push    es
                mov     al,1
                call    run_exit_calls
                mov     es,cs:psp_seg
                cli
                mov     ax,cs:saved_term_off
                mov     es:[PSP_TERM_IP],ax
                mov     ax,cs:saved_term_seg
                mov     es:[PSP_TERM_CS],ax
                mov     cs:term_hooked,0
                sti
                pop     es
                pop     ax
                ret
rtl_exit_cleanup endp

;  rtl_save_term                                                 2eef:00d3
;
;  es = the PSP.  Records es in psp_seg and, the first time only (while
;  saved_term is zero), the PSP's terminate address in saved_term.  ax is
;  kept.  rtlink_init_workspace calls it first, with the PSP DOS passed.
rtl_save_term   proc    far
                push    ax
                mov     cs:psp_seg,es
                mov     ax,cs:saved_term_off
                or      ax,cs:saved_term_seg
                jne     term_saved
                cli
                mov     ax,es:[PSP_TERM_IP]
                mov     cs:saved_term_off,ax
                mov     ax,es:[PSP_TERM_CS]
                mov     cs:saved_term_seg,ax
                sti
term_saved:
                pop     ax
                ret
rtl_save_term   endp

;  rtl_on_exit                                                   2eef:00f8
;
;  Registers the far routine ds:dx in the first free slot of exit_table with
;  the mask al: bit 0 has rtl_exit_cleanup call it, bit 1 term_handler.  The
;  first registration with al other than 1 points the PSP's terminate
;  address at term_handler.  With all eight slots taken it ends in
;  rtl_internal_error.  ax and bx are kept.  rtlink_init_workspace registers
;  rtlink_close_file (1d99:09f4) with al = 1.
rtl_on_exit     proc    far
                push    bx
                push    ax
                mov     bx,offset exit_table
find_free:
                mov     ax,cs:[bx]
                or      ax,cs:[bx+2]
                je      slot_found
                add     bx,EXIT_ENTRY
                jmp     find_free

term_hooked     dw      0                       ; 2eef:010b, 0FFh while term_handler is hooked

slot_found:
                test    cs:term_hooked,0FFh
                jne     store_entry
                pop     ax
                push    ax
                cmp     al,1
                je      store_entry
                mov     cs:term_hooked,0FFh
                push    es
                mov     es,cs:psp_seg
                cli
                mov     word ptr es:[PSP_TERM_IP],offset term_handler
                mov     es:[PSP_TERM_CS],cs
                sti
                pop     es
store_entry:
                cmp     bx,offset exit_table_end
                je      table_full
                mov     cs:[bx],dx
                mov     cs:[bx+2],ds
                pop     ax
                mov     cs:[bx+4],al
                pop     bx
                ret
table_full:
                push    cs                      ; a far call within this segment;
                call    near ptr rtl_internal_error ; it does not return
rtl_on_exit     endp

;  exit_table: eight entries of a far routine, its mask byte and a pad byte;
;  a zero routine is a free slot.  The zero dword after them ends the search.
exit_table      label   byte                    ; 2eef:0150
                rept    EXIT_SLOTS              ; (not `dup`: the build splices no
                dd      0                       ;  iterated data)
                db      0,0
                endm
exit_table_end  dd      0                               ; 2eef:0180

;  rtl_internal_error                                            2eef:0184
;
;  "Fatal Error ege0001: Internal error in .RTLink(R)/Plus run-time code."
;  Builds the string list in ierr_list and jumps through fatal_vector with
;  ds:bx at it, ah = 6, al = 1, cx = 0.  Does not return.
rtl_internal_error proc far
                mov     ax,cs
                mov     ds,ax
                assume  ds:RTL_TEXT
                mov     bx,msg_ierr_code
                mov     cs:ierr_list,bx
                mov     bx,msg_ierr_text
                mov     cs:ierr_list+2,bx
                mov     cs:ierr_list+4,0
                assume  ds:nothing
                mov     bx,offset ierr_list
                mov     ah,ERR_INTERNAL
                mov     al,1
                xor     cx,cx
                jmp     cs:fatal_vector
rtl_internal_error endp

ierr_list       dw      0,0,0                   ; 2eef:01af

;  term_handler                                                  2eef:01b5
;
;  Where the PSP's terminate address points once rtl_on_exit has hooked it:
;  DOS jumps here as the program ends.  Calls the exit routines registered
;  with mask bit 1, puts the terminate address back, and jumps to it.
term_handler    proc    far
                push    ax
                push    es
                mov     al,2
                call    run_exit_calls
                mov     es,cs:psp_seg
                cli
                mov     ax,cs:saved_term_off
                mov     es:[PSP_TERM_IP],ax
                mov     ax,cs:saved_term_seg
                mov     es:[PSP_TERM_CS],ax
                mov     cs:term_hooked,0
                sti
                pop     es
                pop     ax
                jmp     cs:saved_term
term_handler    endp

;  run_exit_calls                                                2eef:01e1
;
;  Far-calls, in table order, every exit_table routine whose mask shares a
;  bit with al, each with the flags saved around it.  bx and dx are kept.
run_exit_calls  proc    near
                push    bx
                push    dx
                mov     bx,offset exit_table
next_entry:
                mov     dx,cs:[bx]
                or      dx,cs:[bx+2]
                je      entries_done
                test    cs:[bx+4],al
                je      skip_entry
                pushf
                call    dword ptr cs:[bx]
                popf
skip_entry:
                add     bx,EXIT_ENTRY
                jmp     next_entry
entries_done:
                pop     dx
                pop     bx
                ret
run_exit_calls  endp

;  rtl_stub_0202, rtl_stub_0206, rtl_stub_020a              2eef:0202, 0206, 020a
;
;  ax = -1; cx = -1; dx:ax = 0.  rtlink_close_file calls the first before it
;  closes the overlay file and discards its ax; nothing calls the other two.
;  What component they stand in for is not established.
rtl_stub_0202   proc    far
                mov     ax,0FFFFh
                ret
rtl_stub_0202   endp

rtl_stub_0206   proc    far
                mov     cx,0FFFFh
                ret
rtl_stub_0206   endp

rtl_stub_020a   proc    far
                xor     ax,ax
                mov     dx,ax
                ret
rtl_stub_020a   endp

;  The messages, through a table of their offsets (2eef:020f): rtl_fatal's
;  two are printed by DOS function 9 and end in '$'; rtl_internal_error's
;  two are strings for rtl_fatal's list and end in NUL.
msg_fatal       dw      offset s_fatal
msg_press       dw      offset s_press
msg_ierr_code   dw      offset s_ierr_code
msg_ierr_text   dw      offset s_ierr_text
s_fatal         db      'Fatal Error $'
s_press         db      0Dh,0Ah,'Press any key to terminate.$'
s_ierr_code     db      'ege0001: ',0
s_ierr_text     db      'Internal error in .RTLink(R)/Plus run-time code.',0Dh,0Ah,0

RTL_TEXT        ends
                end
