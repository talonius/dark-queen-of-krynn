; Link-time offsets in the RTLink manager segment, unless noted.
%ifndef RTLINK_SYMBOLS
%define RTLINK_SYMBOLS
manager_segment            equ 0x1d99
saved_dos_es               equ 0x0a86
saved_dos_ds               equ 0x0a88
private_stack_segment      equ 0x0c0f
private_stack_top          equ 0x0200
rtlink_init_workspace      equ 0x07b5
rtlink_init_sections       equ 0x0826
rtlink_load_section        equ 0x021c
rtlink_open_file           equ 0x0479
rtlink_section_record      equ 0x0610
rtlink_zero_paragraphs     equ 0x0620
rtlink_init_vector_check   equ 0x0658
section_count              equ 0x0c19
section_table              equ 0x0c1f
section_record_size        equ 0x0012
cache_slot_count           equ 0x0c1b
reload_stack_bytes         equ 0x0c1d
workspace_start            equ 0x0a90
reload_stack_used          equ 0x0a70
reload_stack_offset        equ 0x0a72
reload_stack_capacity      equ 0x0a76
cache_available            equ 0x0a84
file_open_mode             equ 0x0a85
open_file_token            equ 0x0a8c
manager_flags              equ 0x0c0d
overlay_start_segment      equ 0x0bf3
overlay_end_segment        equ 0x0bf5
start_output_offset        equ 0x0c11
start_output_segment       equ 0x0c13
end_output_offset          equ 0x0c15
end_output_segment         equ 0x0c17
%endif
