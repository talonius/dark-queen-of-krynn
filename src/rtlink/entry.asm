; RTLink startup wrapper. See docs/loader-entry.md for a C#-style walkthrough.
; Link-time address 1D99:076C; original EXE file offset 0x204FC.
; Four explicit byte pairs preserve alternate encodings NASM does not select.

bits 16
cpu 8086
org 0x076c

%include "symbols.inc"

rtlink_entry:
    ; DOS supplies the PSP (process startup information) through DS and ES.
    mov [cs:saved_dos_es], es
    mov [cs:saved_dos_ds], ds
    mov bx, manager_segment         ; DOS relocates this segment immediate.
    mov ds, bx                      ; Manager variables now use DS as their base.

    push ds
    call rtlink_init_workspace     
    pop ds

    mov bx, [private_stack_segment]
    db 0x0b, 0xdb                   ; or bx, bx (original encoding)
    jz short .save_registers

    ; Keep the application's initial stack location in CX:DX while switching.
    mov cx, ss
    db 0x8b, 0xd4                   ; mov dx, sp (original encoding)
    cli
    mov ss, bx
    mov sp, private_stack_top
    sti

.save_registers:
    push bx                         ; Whether a private stack was selected.
    push cx                         ; Original SS, if a switch occurred.
    push dx                         ; Original SP, if a switch occurred.
    push ds
    call rtlink_init_sections       ; Loads the startup sections, including data.
    pop ds
    pop dx
    pop cx
    pop bx

    db 0x0b, 0xdb                   ; or bx, bx (original encoding)
    jz short .restore_dos_segments
    cli
    mov ss, cx
    db 0x8b, 0xe2                   ; mov sp, dx (original encoding)
    sti

.restore_dos_segments:
    mov es, [saved_dos_es]
    mov ds, [saved_dos_ds]
    jmp 0x0000:0x0000               ; DOS relocates the segment to the image base.

%if ($ - $$) != 0x49
    %error "RTLink entry must occupy exactly 73 bytes"
%endif
