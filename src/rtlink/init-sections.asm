; Reconstructed from DQK.EXE. See docs/loader-startup.md.
bits 16
cpu 8086
%include "symbols.inc"
org 0x0826

rtlink_sections_body:
    ; Preserve flags, and establish the DOS-dependent file-open mode.
    pushf
    cld
    mov ax,0x3000
    int byte 0x21
    cmp al,0x3
    jc .check_vector_option
    ; DOS 3+: read-only access with deny-write sharing (20h).
    mov byte [file_open_mode],0x20
.check_vector_option:
    ; Optional vector-integrity bookkeeping; not enabled in the shipped flags.
    test word [manager_flags],0x100
    jz .check_bounds_options
    call rtlink_init_vector_check
.check_bounds_options:
    ; Compute overlay bounds only if clearing or output pointers request them.
    test word [manager_flags],0x2
    jnz .find_bounds
    mov ax,[start_output_offset]
    or ax,[start_output_segment]
    or ax,[end_output_offset]
    or ax,[end_output_segment]
    jz .initialize_records
.find_bounds:
    ; Find the lowest nonzero load segment and highest exclusive end segment.
    mov cx,[section_count]
    db 0x33, 0xc0 ; xor ax,ax (original encoding)
    mov bx, section_table
    mov ax,0xffff
    db 0x33, 0xd2 ; xor dx,dx (original encoding)
.scan_bounds:
    ; CX is reused for each segment while its loop count is on the stack.
    push cx
    mov cx,[bx]
    db 0x0b, 0xc9 ; or cx,cx (original encoding)
    jz .next_bound
    db 0x3b, 0xc1 ; cmp ax,cx (original encoding)
    jna .check_upper_bound
    db 0x8b, 0xc1 ; mov ax,cx (original encoding)
.check_upper_bound:
    add cx,[bx+0x8]
    db 0x3b, 0xd1 ; cmp dx,cx (original encoding)
    jnc .next_bound
    db 0x8b, 0xd1 ; mov dx,cx (original encoding)
.next_bound:
    pop cx
    add bx, section_record_size
    loop .scan_bounds
    ; Publish computed bounds inside the manager.
    mov [overlay_start_segment],ax
    mov [overlay_end_segment],dx
    ; Optionally zero the entire overlay address span before loading sections.
    test word [manager_flags],0x2
    jz .initialize_records
    call rtlink_zero_paragraphs
.initialize_records:
    ; Scan all records: load flag-01 sections and remember a filename descriptor.
    mov cx,[cs:section_count]
    db 0x33, 0xc0 ; xor ax,ax (original encoding)
    db 0x33, 0xf6 ; xor si,si (original encoding)
    mov bx, section_table
.scan_records:
    db 0x0b, 0xf6 ; or si,si (original encoding)
    jnz .check_startup_flag
    mov si,[bx+0x2]
    db 0x0b, 0xf6 ; or si,si (original encoding)
    jz .check_startup_flag
    ; Flag 08: filename descriptor is indirect.
    test byte [bx+0x7],0x8
    jz .check_startup_flag
    mov si,[si]
.check_startup_flag:
    ; The shipped table marks only section 12 (shared data) for startup loading.
    test byte [bx+0x7],0x1
    jz .next_record
    ; PUSH CS + near CALL supplies the far return expected by the loader.
    push cs
    call rtlink_load_section
.next_record:
    add bx, section_record_size
    inc ax
    loop .scan_records
    ; If startup loads did not open a file, open the remembered descriptor.
    cmp word [open_file_token],0x0
    jnz .write_start_bound
    db 0x0b, 0xf6 ; or si,si (original encoding)
    jz .write_start_bound
    call rtlink_open_file
.write_start_bound:
    ; Optional far output pointers receive the computed segment bounds.
    mov ax,[start_output_offset]
    or ax,[start_output_segment]
    jz .write_end_bound
    les bx,word [start_output_offset]
    mov ax,[overlay_start_segment]
    mov [es:bx],ax
.write_end_bound:
    mov ax,[end_output_offset]
    or ax,[end_output_segment]
    jz .done
    les bx,word [end_output_offset]
    mov ax,[overlay_end_segment]
    mov [es:bx],ax
.done:
    ; Restore caller flags and return to the entry wrapper.
    popf
    ret

%if ($ - $$) != 0xd5
    %error "init-sections must occupy exactly 213 bytes"
%endif
