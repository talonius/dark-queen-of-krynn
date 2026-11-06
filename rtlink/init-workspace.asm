; Reconstructed from DQK.EXE. See docs/loader-startup.md.
bits 16
cpu 8086
%include "symbols.inc"
org 0x07b5

rtlink_workspace_body:
    ; Preserve flags, then use forward string operations during initialization.
    pushf
    cld
    ; Runtime helper saves the PSP/termination-vector context.
    mov es,word [cs:saved_dos_es]
    call 0x1eef:0xd3
    ; Register the manager callback at 09F4 with the runtime (AL=1).
    mov al,0x1
    mov dx, 0x09f4
    call 0x1eef:0xf8
    ; The first free workspace byte follows all 18-byte section records.
    mov ax,[section_count]
    call rtlink_section_record
    mov [workspace_start],bx
    ; Reserve 32 bytes per configured cache slot before the reload stack.
    mov ax,[cache_slot_count]
    db 0x8b, 0xd8 ; mov bx,ax (original encoding)
    shl bx,1
    shl bx,1
    shl bx,1
    shl bx,1
    shl bx,1
    add bx,[cs:workspace_start]
    ; Initialize the reload stack (six bytes per nested overlay call).
    mov word [reload_stack_used],0x0
    mov [reload_stack_offset],bx
    mov ax,[reload_stack_bytes]
    db 0x03, 0xd8 ; add bx,ax (original encoding)
    mov [reload_stack_capacity],ax
    ; Probe the optional cache backend with section count and saved PSP.
    push ds
    mov ax,cs
    mov es,ax
    db 0x33, 0xc0 ; xor ax,ax (original encoding)
    mov ds,ax
    db 0x8b, 0xd0 ; mov dx,ax (original encoding)
    mov ax,[cs:section_count]
    mov cx,[cs:saved_dos_es]
    ; In this EXE, 1EEF:0007 is STC / RETF: backend unavailable.
    call 0x1eef:0x7
    pop ds
    ; Enable caching if the probe succeeded OR cache slots were configured.
    jnc .enable_cache
    test word [cache_slot_count],0xffff
    jz .done
.enable_cache:
    mov byte [cache_available],0xff
.done:
    popf
    ret

%if ($ - $$) != 0x71
    %error "init-workspace must occupy exactly 113 bytes"
%endif
