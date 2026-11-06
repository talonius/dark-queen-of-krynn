;  DQK.EXE resident frame 10d7: the PC-speaker sample player.  EXE
;  0x00420f..0x0050a7, 3736 bytes, 10d7:109f..10d7:1f37: a 32-word jump
;  table, then one far routine whose body is 32 unrolled cases.
;  dissolve.asm ends at 0x420f; the routine glyphfx.c calls at 10d7:1f37
;  starts at 0x50a7.
;
;  Why this is a file of its own is a choice, not a measurement (see
;  gfxutil.asm's header): nothing else in the frame touches the table, the
;  speaker or the timer, and the table sits right before its routine as the
;  dissolves' tables do before theirs.
;
;  How it plays, as far as the bytes show it.  Samples are 4 bits, two to a
;  byte, low nibble first.  PIT channel 2 runs in mode 2 (rate generator)
;  with the caller's divisor -- only its low byte reaches the PIT, as the
;  channel is programmed LSB-only -- so its count falls from the divisor to
;  1 and reloads once per sample period.  Port 61h has the timer 2 gate on
;  throughout; its bit 1, the speaker data enable, is what the cases
;  switch: dl holds port 61h with it clear (SPEAKER_OFF), dh with it set
;  (SPEAKER_ON).  Each nibble value 0..15 has a case that drives the
;  speaker for one period: 0 keeps it off, 15 keeps it on, and 1..14 switch
;  it on and off inside the period, for a share that by the cases' shape
;  (below) grows with the value; the pulse widths and gaps are timed by
;  instruction counts (NOPs, dead `mov al,dh/ah` fillers, JMP_NEXT).  A case polls the latched count with
;  PIT_READ: above 28h it is early in the period, at or below 28h late, and
;  a count above 28h after a late reading means the counter reloaded --
;  the period is over and the case dispatches the next nibble.  The cycle
;  counts of the paths are not analysed here.
;
;  Each value has two copies of its case: lo<n> plays a low nibble and then
;  indexes the same byte's high nibble (FETCH_HIGH, table entries 16..31),
;  hi<n> plays a high nibble and then loads the next byte and indexes its
;  low nibble (FETCH_LOW, entries 0..15).  The two copies differ only in
;  that fetch, so both are one DUTY<n> macro below.  The cases also come in
;  complementary pairs, the same code with the speaker's two states
;  exchanged: 15 is 0 reversed and 9..14 are 7..2 reversed, so DUTY0..DUTY8
;  take the state to pulse to and the state to rest in as arguments.  1 and
;  8 have no partner.
;
;  Interrupts are off for the whole call (cli at entry, sti at the end).
;  bx indexes the jump table, bh = 0.  jcxz cannot reach play_again from
;  most cases, so each such case has a `jmp play_again` just before it
;  (<case>_again).  cx counts bytes off as they are loaded, and the
;  low-nibble case tests it after its first speaker write, so a pass ends
;  there on the last byte: that byte's nibbles are not played out.
;
;  Assembled by TASM 3.1, /ml, single pass: a forward jmp is EB xx 90, as
;  shipped, and JMP_NEXT is such a jmp to the next instruction.  No segment
;  fixups: the range holds no MZ relocation and the object makes none (the
;  jump table's words are offsets within the segment).

                .8086

PIT_CH2         equ     42h                     ; 8253/8254 channel 2 counter
PIT_CMD         equ     43h                     ; 8253/8254 mode/command port
PIT_CH2_MODE2   equ     94h                     ; ch 2, LSB only, mode 2, binary
PIT_CH2_LATCH   equ     80h                     ; latch channel 2's count
PORT_B          equ     61h                     ; 8255 port B: bit 0 timer 2 gate,
                                                ;   bit 1 speaker data enable
LATE            equ     28h                     ; a count at or below it is late
                                                ;   in the period

;  The speaker on (bit 1 set) or off, gate on in both: dh and dl hold the
;  two port B values.
SPEAKER_ON      macro
                mov     al,dh
                out     PORT_B,al
                endm

SPEAKER_OFF     macro
                mov     al,dl
                out     PORT_B,al
                endm

;  Latch and read the low byte of channel 2's count and compare it with
;  LATE; the caller branches on ja (early) or jbe (late).
PIT_READ        macro
                mov     al,PIT_CH2_LATCH
                out     PIT_CMD,al
                in      al,PIT_CH2
                cmp     al,LATE
                endm

;  Load the next byte into ah and index its low nibble (bl = 2 * nibble,
;  table entries 0..15); count the byte off cx.
FETCH_LOW       macro
                lodsb
                mov     ah,al
                and     al,0Fh
                shl     al,1
                mov     bl,al
                dec     cx
                endm

;  Index the high nibble of the byte in ah (bl = 20h + 2 * nibble, table
;  entries 16..31), then two dead fillers.
FETCH_HIGH      macro
                mov     al,ah
                and     al,0F0h
                clc
                rcr     al,1
                sar     al,1
                sar     al,1
                add     al,20h
                mov     bl,al
                IDLE    ah,2
                endm

;  Go to the case for the nibble bl indexes.
DISPATCH        macro
                jmp     cs:nibble_jump[bx]
                endm

;  Delays: n NOPs; n dead `mov al,reg`; a jmp to the next instruction (TASM
;  makes it EB 01 90).
PAD             macro   n
                rept    n
                nop
                endm
                endm

IDLE            macro   reg,n
                rept    n
                mov     al,reg
                endm
                endm

JMP_NEXT        macro
                local   next
                jmp     next
next:
                endm

;  DUTY0: the whole period in one state: `rest` (value 0 off, 15 on).
DUTY0           macro   fetch, again, pulse, rest
                local   t1, t2
                rest
                jcxz    again                   ; the pass is over
                fetch
t1:
                PIT_READ
                ja      t1                      ; wait for the late part ...
t2:
                PIT_READ
                jbe     t2                      ; ... and for the reload
                DISPATCH
                endm

;  DUTY1: the narrowest pulse, `pulse` and `rest` back to back (one out apart),
;  on every other poll: one loop while the count is early, one while it is
;  late, until the reload.
DUTY1           macro   fetch, again, pulse, rest
                local   t1, t2, t3, t4, t5
                pulse
                rest
                jcxz    again
                rest
                fetch
                PAD     4
                mov     al,ah
                PIT_READ
                jbe     t3                      ; already late
t1:
                JMP_NEXT                        ; the early loop
                pulse
                rest
                PIT_READ
                jbe     t4
                IDLE    dh,6
                PIT_READ
                ja      t1
                PAD     4
t2:
                PAD     4                       ; the late loop
t3:
                nop
                pulse
                rest
                PIT_READ
                ja      t5                      ; reloaded
                IDLE    dh,6
t4:
                PIT_READ
                jbe     t2
                DISPATCH                        ; reloaded
t5:
                JMP_NEXT
                JMP_NEXT
                JMP_NEXT
                IDLE    ah,2
                DISPATCH
                endm

;  DUTY2: DUTY1 with two NOPs inside each pulse, and other fillers changed
;  to keep the period: JMP_NEXT and a nop after the fetch, PAD 4 for each
;  IDLE dh,6, and a different tail.
DUTY2           macro   fetch, again, pulse, rest
                local   t1, t2, t3, t4, t5
                pulse
                PAD     2
                rest
                jcxz    again
                rest
                fetch
                JMP_NEXT
                nop
                mov     al,ah
                PIT_READ
                jbe     t3
t1:
                JMP_NEXT
                pulse
                PAD     2
                rest
                PIT_READ
                jbe     t4
                PAD     4
                PIT_READ
                ja      t1
                PAD     4
t2:
                PAD     4
t3:
                nop
                pulse
                PAD     2
                rest
                PIT_READ
                ja      t5
                PAD     4
t4:
                PIT_READ
                jbe     t2
                DISPATCH
t5:
                JMP_NEXT
                JMP_NEXT
                IDLE    ah,2
                nop
                DISPATCH
                endm

;  DUTY3: DUTY2's pulse before every poll.
DUTY3           macro   fetch, again, pulse, rest
                local   t1, t2
                pulse
                jcxz    again
                rest
                fetch
                JMP_NEXT
                PAD     3
                IDLE    ah,2
t1:
                JMP_NEXT
                pulse
                PAD     2
                rest
                mov     al,ah
                PIT_READ
                ja      t1
                PAD     4
t2:
                JMP_NEXT
                pulse
                PAD     2
                rest
                mov     al,ah
                PIT_READ
                jbe     t2
                DISPATCH
                endm

;  DUTY4: a pulse of four NOPs and a filler before every poll.
DUTY4           macro   fetch, again, pulse, rest
                local   t1, t2
                pulse
                jcxz    again
                PAD     2
                mov     al,ah
                rest
                fetch
                JMP_NEXT
                PAD     4
t1:
                JMP_NEXT
                pulse
                PAD     4
                mov     al,dl
                rest
                mov     al,ah
                PIT_READ
                ja      t1
                PAD     4
t2:
                JMP_NEXT
                pulse
                PAD     4
                mov     al,ah
                rest
                mov     al,ah
                PIT_READ
                jbe     t2
                DISPATCH
                endm

;  DUTY5: `pulse` and `rest` alternate on each pass of a loop, `pulse` held
;  across a jmp, `rest` across a poll and fillers; an early and a late loop
;  as in DUTY1.
DUTY5           macro   fetch, again, pulse, rest
                local   t1, t2, t3, t4, t5
                rest
                jcxz    again
                fetch
                jmp     t2
t1:
                rest
                PIT_READ
                jbe     t4
                IDLE    dh,2
                nop
t2:
                nop
                mov     al,dh
                pulse
                jmp     t1
t3:
                rest
                PIT_READ
                ja      t5
                PAD     4
t4:
                pulse
                jmp     t3
t5:
                pulse
                JMP_NEXT
                nop
                IDLE    ah,2
                DISPATCH
                endm

;  DUTY6: DUTY5 with a JMP_NEXT added to the `pulse` state.
DUTY6           macro   fetch, again, pulse, rest
                local   t1, t2, t3, t4, t5
                rest
                jcxz    again
                fetch
                jmp     t2
t1:
                JMP_NEXT
                rest
                PIT_READ
                jbe     t4
                IDLE    ah,2
                nop
t2:
                nop
                mov     al,ah
                pulse
                jmp     t1
t3:
                JMP_NEXT
                rest
                PIT_READ
                ja      t5
                PAD     4
t4:
                pulse
                jmp     t3
t5:
                pulse
                JMP_NEXT
                nop
                IDLE    ah,2
                DISPATCH
                endm

;  DUTY7: a poll in both states: `pulse` held across a poll and fillers,
;  `rest` across a JMP_NEXT and a poll.
DUTY7           macro   fetch, again, pulse, rest
                local   t1, t2, t3, t4, t5
                pulse
                jcxz    again
                fetch
                jmp     t2
t1:
                JMP_NEXT
                pulse
                PIT_READ
                jbe     t4
                IDLE    ah,2
                nop
t2:
                nop
                mov     al,ah
                rest
                PIT_READ
                ja      t1
                PAD     4
t3:
                JMP_NEXT
                pulse
                PIT_READ
                ja      t5
                IDLE    ah,6
t4:
                rest
                PIT_READ
                jbe     t3
                DISPATCH
t5:
                rest
                JMP_NEXT
                JMP_NEXT
                nop
                IDLE    ah,2
                DISPATCH
                endm

;  DUTY8: DUTY7 with a JMP_NEXT added to the `pulse` state.
DUTY8           macro   fetch, again, pulse, rest
                local   t1, t2, t3, t4, t5
                pulse
                jcxz    again
                fetch
                jmp     t2
t1:
                JMP_NEXT
                pulse
                PIT_READ
                jbe     t4
                IDLE    ah,2
                nop
t2:
                nop
                mov     al,ah
                JMP_NEXT
                rest
                PIT_READ
                ja      t1
                PAD     4
t3:
                JMP_NEXT
                pulse
                PIT_READ
                ja      t5
                IDLE    ah,6
t4:
                JMP_NEXT
                rest
                PIT_READ
                jbe     t3
                DISPATCH
t5:
                JMP_NEXT
                rest
                JMP_NEXT
                JMP_NEXT
                nop
                IDLE    ah,2
                DISPATCH
                endm
_TEXT           segment byte public 'CODE'
                assume  cs:_TEXT

                public  _speaker_play, nibble_jump

;  The case for each nibble: entries 0..15 play a byte's low nibble, 16..31
;  its high nibble.
nibble_jump     dw      offset lo0, offset lo1, offset lo2, offset lo3       ; 10d7:109f
                dw      offset lo4, offset lo5, offset lo6, offset lo7
                dw      offset lo8, offset lo9, offset lo10, offset lo11
                dw      offset lo12, offset lo13, offset lo14, offset lo15
                dw      offset hi0, offset hi1, offset hi2, offset hi3       ; 10d7:10bf
                dw      offset hi4, offset hi5, offset hi6, offset hi7
                dw      offset hi8, offset hi9, offset hi10, offset hi11
                dw      offset hi12, offset hi13, offset hi14, offset hi15

;  void far speaker_play(char far *samples, unsigned len, unsigned divisor,
;                        int times)                             10d7:10df
;
;  Plays len bytes of 4-bit samples on the PC speaker `times` times (its low
;  byte; 0 wraps to 65536 passes), one nibble per PIT period -- the last
;  byte is not played out (see the header).  The period is divisor & 0FFh
;  input clocks (0 is 256), since only the low byte is programmed;
;  digisnd.c passes 1193181 / rate, rounded, which exceeds 255 below about
;  4.7 kHz.  The samples must not cross their segment: si wraps.  With
;  len = 0 the first pass ends at once, before dl and dh are set, and the
;  next ones play 65535 bytes (cx counts down from 0) writing the caller's
;  dx to port 61h.
;  Returns with the gate and the speaker off, DS = SS.
_speaker_play   proc    far
                push    bp
                mov     bp,sp
                push    si
                mov     ax,[bp+0Eh]
                mov     ah,0
                push    ax                      ; [bp-4]: passes left
                mov     si,[bp+6]
                mov     ax,[bp+8]
                mov     ds,ax                   ; ds:si: the samples
                mov     bh,0
                mov     cx,[bp+0Ah]             ; cx: bytes left
                cli
                mov     ax,[bp+0Ch]
                mov     ah,al
                mov     al,PIT_CH2_MODE2
                out     PIT_CMD,al
                in      al,PORT_B
                and     al,0FCh
                or      al,1                    ; gate on, speaker off
                out     PORT_B,al
                mov     al,ah
                out     PIT_CH2,al              ; the divisor's low byte
                jcxz    play_again
                FETCH_LOW
                in      al,PORT_B
                and     al,0FCh
                or      al,1
                mov     dl,al                   ; dl: speaker off
                out     PORT_B,al
                or      al,2
                mov     dh,al                   ; dh: speaker on
                DISPATCH
play_again:
                mov     si,[bp+6]               ; one pass done: rewind
                mov     bh,0
                pop     cx
                dec     cx
                jcxz    play_done
                push    cx
                mov     cx,[bp+0Ah]
                FETCH_LOW
                DISPATCH
play_done:
                in      al,PORT_B
                and     al,0FCh                 ; gate and speaker off
                out     PORT_B,al
                mov     ax,ss
                mov     ds,ax
                pop     si
                pop     bp
                sti
                ret

;  The cases, in shipped order: the high nibbles 0..15, then the low ones.
;  DUTYn fetch, again, pulse, rest
hi0:
                DUTY0   FETCH_LOW, play_again, SPEAKER_ON, SPEAKER_OFF
hi1:
                DUTY1   FETCH_LOW, play_again, SPEAKER_ON, SPEAKER_OFF
hi2_again:      jmp     play_again
hi2:
                DUTY2   FETCH_LOW, hi2_again, SPEAKER_ON, SPEAKER_OFF
hi3_again:      jmp     play_again
hi3:
                DUTY3   FETCH_LOW, hi3_again, SPEAKER_ON, SPEAKER_OFF
hi4_again:      jmp     play_again
hi4:
                DUTY4   FETCH_LOW, hi4_again, SPEAKER_ON, SPEAKER_OFF
hi5_again:      jmp     play_again
hi5:
                DUTY5   FETCH_LOW, hi5_again, SPEAKER_ON, SPEAKER_OFF
hi6_again:      jmp     play_again
hi6:
                DUTY6   FETCH_LOW, hi6_again, SPEAKER_ON, SPEAKER_OFF
hi7_again:      jmp     play_again
hi7:
                DUTY7   FETCH_LOW, hi7_again, SPEAKER_ON, SPEAKER_OFF
hi8_again:      jmp     play_again
hi8:
                DUTY8   FETCH_LOW, hi8_again, SPEAKER_ON, SPEAKER_OFF
hi9_again:      jmp     play_again
hi9:
                DUTY7   FETCH_LOW, hi9_again, SPEAKER_OFF, SPEAKER_ON
hi10_again:     jmp     play_again
hi10:
                DUTY6   FETCH_LOW, hi10_again, SPEAKER_OFF, SPEAKER_ON
hi11_again:     jmp     play_again
hi11:
                DUTY5   FETCH_LOW, hi11_again, SPEAKER_OFF, SPEAKER_ON
hi12_again:     jmp     play_again
hi12:
                DUTY4   FETCH_LOW, hi12_again, SPEAKER_OFF, SPEAKER_ON
hi13_again:     jmp     play_again
hi13:
                DUTY3   FETCH_LOW, hi13_again, SPEAKER_OFF, SPEAKER_ON
hi14_again:     jmp     play_again
hi14:
                DUTY2   FETCH_LOW, hi14_again, SPEAKER_OFF, SPEAKER_ON
hi15_again:     jmp     play_again
hi15:
                DUTY0   FETCH_LOW, hi15_again, SPEAKER_OFF, SPEAKER_ON
lo0_again:      jmp     play_again
lo0:
                DUTY0   FETCH_HIGH, lo0_again, SPEAKER_ON, SPEAKER_OFF
lo1_again:      jmp     play_again
lo1:
                DUTY1   FETCH_HIGH, lo1_again, SPEAKER_ON, SPEAKER_OFF
lo2_again:      jmp     play_again
lo2:
                DUTY2   FETCH_HIGH, lo2_again, SPEAKER_ON, SPEAKER_OFF
lo3_again:      jmp     play_again
lo3:
                DUTY3   FETCH_HIGH, lo3_again, SPEAKER_ON, SPEAKER_OFF
lo4_again:      jmp     play_again
lo4:
                DUTY4   FETCH_HIGH, lo4_again, SPEAKER_ON, SPEAKER_OFF
lo5_again:      jmp     play_again
lo5:
                DUTY5   FETCH_HIGH, lo5_again, SPEAKER_ON, SPEAKER_OFF
lo6_again:      jmp     play_again
lo6:
                DUTY6   FETCH_HIGH, lo6_again, SPEAKER_ON, SPEAKER_OFF
lo7_again:      jmp     play_again
lo7:
                DUTY7   FETCH_HIGH, lo7_again, SPEAKER_ON, SPEAKER_OFF
lo8_again:      jmp     play_again
lo8:
                DUTY8   FETCH_HIGH, lo8_again, SPEAKER_ON, SPEAKER_OFF
lo9_again:      jmp     play_again
lo9:
                DUTY7   FETCH_HIGH, lo9_again, SPEAKER_OFF, SPEAKER_ON
lo10_again:     jmp     play_again
lo10:
                DUTY6   FETCH_HIGH, lo10_again, SPEAKER_OFF, SPEAKER_ON
lo11_again:     jmp     play_again
lo11:
                DUTY5   FETCH_HIGH, lo11_again, SPEAKER_OFF, SPEAKER_ON
lo12_again:     jmp     play_again
lo12:
                DUTY4   FETCH_HIGH, lo12_again, SPEAKER_OFF, SPEAKER_ON
lo13_again:     jmp     play_again
lo13:
                DUTY3   FETCH_HIGH, lo13_again, SPEAKER_OFF, SPEAKER_ON
lo14_again:     jmp     play_again
lo14:
                DUTY2   FETCH_HIGH, lo14_again, SPEAKER_OFF, SPEAKER_ON
lo15_again:     jmp     play_again
lo15:
                DUTY0   FETCH_HIGH, lo15_again, SPEAKER_OFF, SPEAKER_ON
_speaker_play   endp

_TEXT           ends
                end
