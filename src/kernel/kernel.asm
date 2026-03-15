[bits 16]
[org 0x0000]

INPUT_MAX equ 63
COMMAND_MAX equ 15
HISTORY_COUNT equ 8
HISTORY_ENTRY_SIZE equ INPUT_MAX + 1
DAY_TICKS_HI equ 0x0018
DAY_TICKS_LO equ 0x00B0
VIDEO_SEGMENT equ 0xB800
DESKTOP_APP_COUNT equ 7

APP_TERMINAL equ 0
APP_SYSTEM equ 1
APP_FILES equ 2
APP_NOTES equ 3
APP_CLOCK equ 4
APP_NETWORK equ 5
APP_POWER equ 6

BOOT_MODE_DESKTOP equ 0
BOOT_MODE_CONSOLE equ 1

start:
    cli
    cld
    xor ax, ax
    mov ss, ax
    mov sp, 0x9A00
    sti

    mov ax, cs
    mov ds, ax
    mov es, ax

    mov [boot_drive], dl
    mov [kernel_sectors], cl
    mov byte [desktop_selection], APP_TERMINAL
    mov byte [boot_mode], BOOT_MODE_DESKTOP

    call capture_boot_ticks
    call init_mouse
    call init_network_subsystem
    call boot_mode_menu

    cmp byte [boot_mode], BOOT_MODE_CONSOLE
    jne desktop_main
    call terminal_session
    jmp desktop_main

desktop_main:
    call set_text_mode
    call show_mouse_cursor_if_available

.desktop_loop:
    call render_desktop_home
    call wait_desktop_input

    cmp byte [event_type], 2
    je .open_selected

    cmp ah, 0x48
    je .move_up
    cmp ah, 0x50
    je .move_down
    cmp ah, 0x3B
    je .show_help

    cmp al, 13
    je .open_selected
    cmp al, 27
    je .open_power

    cmp al, '1'
    je .shortcut_terminal
    cmp al, '2'
    je .shortcut_system
    cmp al, '3'
    je .shortcut_files
    cmp al, '4'
    je .shortcut_notes
    cmp al, '5'
    je .shortcut_clock
    cmp al, '6'
    je .shortcut_network
    cmp al, '7'
    je .shortcut_power

    cmp al, 't'
    je .shortcut_terminal
    cmp al, 'T'
    je .shortcut_terminal
    cmp al, 's'
    je .shortcut_system
    cmp al, 'S'
    je .shortcut_system
    cmp al, 'f'
    je .shortcut_files
    cmp al, 'F'
    je .shortcut_files
    cmp al, 'n'
    je .shortcut_notes
    cmp al, 'N'
    je .shortcut_notes
    cmp al, 'c'
    je .shortcut_clock
    cmp al, 'C'
    je .shortcut_clock
    cmp al, 'w'
    je .shortcut_network
    cmp al, 'W'
    je .shortcut_network
    cmp al, 'p'
    je .shortcut_power
    cmp al, 'P'
    je .shortcut_power

    jmp .desktop_loop

.move_up:
    cmp byte [desktop_selection], APP_TERMINAL
    jne .dec_selection
    mov byte [desktop_selection], DESKTOP_APP_COUNT - 1
    jmp .desktop_loop

.dec_selection:
    dec byte [desktop_selection]
    jmp .desktop_loop

.move_down:
    cmp byte [desktop_selection], DESKTOP_APP_COUNT - 1
    jne .inc_selection
    mov byte [desktop_selection], APP_TERMINAL
    jmp .desktop_loop

.inc_selection:
    inc byte [desktop_selection]
    jmp .desktop_loop

.show_help:
    call hide_mouse_cursor_if_visible
    call desktop_help_app
    call show_mouse_cursor_if_available
    jmp .desktop_loop

.open_selected:
    call open_desktop_app
    jmp .desktop_loop

.open_power:
    mov byte [desktop_selection], APP_POWER
    call open_desktop_app
    jmp .desktop_loop

.shortcut_terminal:
    mov byte [desktop_selection], APP_TERMINAL
    call open_desktop_app
    jmp .desktop_loop

.shortcut_system:
    mov byte [desktop_selection], APP_SYSTEM
    call open_desktop_app
    jmp .desktop_loop

.shortcut_files:
    mov byte [desktop_selection], APP_FILES
    call open_desktop_app
    jmp .desktop_loop

.shortcut_notes:
    mov byte [desktop_selection], APP_NOTES
    call open_desktop_app
    jmp .desktop_loop

.shortcut_clock:
    mov byte [desktop_selection], APP_CLOCK
    call open_desktop_app
    jmp .desktop_loop

.shortcut_network:
    mov byte [desktop_selection], APP_NETWORK
    call open_desktop_app
    jmp .desktop_loop

.shortcut_power:
    mov byte [desktop_selection], APP_POWER
    call open_desktop_app
    jmp .desktop_loop

open_desktop_app:
    call hide_mouse_cursor_if_visible

    cmp byte [desktop_selection], APP_TERMINAL
    je .run_terminal
    cmp byte [desktop_selection], APP_SYSTEM
    je .run_system
    cmp byte [desktop_selection], APP_FILES
    je .run_files
    cmp byte [desktop_selection], APP_NOTES
    je .run_notes
    cmp byte [desktop_selection], APP_CLOCK
    je .run_clock
    cmp byte [desktop_selection], APP_NETWORK
    je .run_network
    jmp .run_power

.run_terminal:
    call terminal_session
    jmp .done

.run_system:
    call system_app
    jmp .done

.run_files:
    call files_app
    jmp .done

.run_notes:
    call notes_app
    jmp .done

.run_clock:
    call clock_app
    jmp .done

.run_network:
    call network_app
    jmp .done

.run_power:
    call power_app

.done:
    call show_mouse_cursor_if_available
    ret

desktop_help_app:
.loop:
    call render_help_app
    call wait_key
    cmp al, 27
    je .done
    cmp al, 13
    je .done
    cmp al, 't'
    je .terminal
    cmp al, 'T'
    je .terminal
    jmp .loop

.terminal:
    call terminal_session
.done:
    ret

system_app:
.loop:
    call render_system_app
    call wait_key
    cmp al, 27
    je .done
    cmp al, 13
    je .done
    cmp al, 'r'
    je .loop
    cmp al, 'R'
    je .loop
    cmp al, 't'
    je .terminal
    cmp al, 'T'
    je .terminal
    jmp .loop

.terminal:
    call terminal_session
.done:
    ret

files_app:
.loop:
    call render_files_app
    call wait_key
    cmp al, 27
    je .done
    cmp al, 13
    je .done
    cmp al, 't'
    je .terminal
    cmp al, 'T'
    je .terminal
    jmp .loop

.terminal:
    call terminal_session
.done:
    ret

notes_app:
.loop:
    call render_notes_app
    call wait_key
    cmp al, 27
    je .done
    cmp al, 13
    je .done
    cmp al, 't'
    je .terminal
    cmp al, 'T'
    je .terminal
    jmp .loop

.terminal:
    call terminal_session
.done:
    ret

clock_app:
.loop:
    call render_clock_app
    call wait_key
    cmp al, 27
    je .done
    cmp al, 13
    je .done
    cmp al, 'r'
    je .loop
    cmp al, 'R'
    je .loop
    cmp al, 't'
    je .terminal
    cmp al, 'T'
    je .terminal
    jmp .loop

.terminal:
    call terminal_session
.done:
    ret

network_app:
.loop:
    call render_network_app
    call wait_key
    cmp al, 27
    je .done
    cmp al, 13
    je .done
    cmp al, 'r'
    je .refresh
    cmp al, 'R'
    je .refresh
    cmp al, 't'
    je .terminal
    cmp al, 'T'
    je .terminal
    jmp .loop

.refresh:
    call init_network_subsystem
    jmp .loop

.terminal:
    call terminal_session
.done:
    ret

power_app:
.loop:
    call render_power_app
    call wait_key
    cmp al, 27
    je .done
    cmp al, 13
    je .done
    cmp al, 'd'
    je .done
    cmp al, 'D'
    je .done
    cmp al, 'h'
    je do_halt
    cmp al, 'H'
    je do_halt
    cmp al, 'r'
    je do_reboot
    cmp al, 'R'
    je do_reboot
    cmp al, 't'
    je .terminal
    cmp al, 'T'
    je .terminal
    jmp .loop

.terminal:
    call terminal_session
.done:
    ret

render_desktop_home:
    call build_desktop_stats

    mov dh, 0
    mov dl, 0
    mov ch, 25
    mov cl, 80
    mov bl, 0x1F
    mov al, ' '
    call fill_rect

    mov dh, 0
    mov dl, 0
    mov ch, 1
    mov cl, 80
    mov bl, 0x70
    mov al, ' '
    call fill_rect

    mov dh, 2
    mov dl, 2
    mov ch, 20
    mov cl, 20
    mov bl, 0x17
    mov al, ' '
    call fill_rect

    mov dh, 2
    mov dl, 24
    mov ch, 20
    mov cl, 54
    mov bl, 0x1F
    mov al, ' '
    call fill_rect

    mov dh, 23
    mov dl, 0
    mov ch, 2
    mov cl, 80
    mov bl, 0x70
    mov al, ' '
    call fill_rect

    mov dh, 0
    mov dl, 2
    mov bl, 0x70
    mov si, desktop_top_title_msg
    call write_string_at

    mov dh, 0
    mov dl, 46
    mov bl, 0x70
    mov si, date_buffer
    call write_string_at

    mov dh, 0
    mov dl, 60
    mov bl, 0x70
    mov si, time_buffer
    call write_string_at

    mov dh, 3
    mov dl, 5
    mov bl, 0x1E
    mov si, desktop_apps_header_msg
    call write_string_at

    mov al, APP_TERMINAL
    mov dh, 5
    mov si, desktop_app_terminal_msg
    call render_app_entry

    mov al, APP_SYSTEM
    mov dh, 7
    mov si, desktop_app_system_msg
    call render_app_entry

    mov al, APP_FILES
    mov dh, 9
    mov si, desktop_app_files_msg
    call render_app_entry

    mov al, APP_NOTES
    mov dh, 11
    mov si, desktop_app_notes_msg
    call render_app_entry

    mov al, APP_CLOCK
    mov dh, 13
    mov si, desktop_app_clock_msg
    call render_app_entry

    mov al, APP_NETWORK
    mov dh, 15
    mov si, desktop_app_network_msg
    call render_app_entry

    mov al, APP_POWER
    mov dh, 17
    mov si, desktop_app_power_msg
    call render_app_entry

    mov dh, 4
    mov dl, 27
    mov bl, 0x1F
    mov si, desktop_workspace_header_msg
    call write_string_at

    call render_desktop_preview

    mov dh, 23
    mov dl, 2
    mov bl, 0x70
    mov si, desktop_footer_line1_msg
    call write_string_at

    mov dh, 24
    mov dl, 2
    mov bl, 0x70
    mov si, desktop_footer_line2_msg
    call write_string_at

    mov dh, 24
    mov dl, 62
    mov bl, 0x70
    cmp byte [mouse_available], 1
    jne .mouse_off
    mov si, desktop_mouse_on_msg
    jmp .mouse_done
.mouse_off:
    mov si, desktop_mouse_off_msg
.mouse_done:
    call write_string_at
    ret

render_app_entry:
    push ax
    push bx
    push cx
    push dx
    push si

    mov bl, 0x17
    cmp al, [desktop_selection]
    jne .draw
    mov bl, 0x70

.draw:
    mov dl, 4
    mov ch, 1
    mov cl, 16
    mov al, ' '
    call fill_rect

    mov dl, 6
    call write_string_at

    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

render_desktop_preview:
    cmp byte [desktop_selection], APP_TERMINAL
    je .terminal
    cmp byte [desktop_selection], APP_SYSTEM
    je .system
    cmp byte [desktop_selection], APP_FILES
    je .files
    cmp byte [desktop_selection], APP_NOTES
    je .notes
    cmp byte [desktop_selection], APP_CLOCK
    je .clock
    cmp byte [desktop_selection], APP_NETWORK
    je .network
    jmp .power

.terminal:
    mov dh, 6
    mov dl, 27
    mov bl, 0x1F
    mov si, preview_terminal_title_msg
    call write_string_at
    mov dh, 8
    mov si, preview_terminal_line1_msg
    call write_string_at
    mov dh, 9
    mov si, preview_terminal_line2_msg
    call write_string_at
    mov dh, 10
    mov si, preview_terminal_line3_msg
    call write_string_at
    jmp .common

.system:
    mov dh, 6
    mov dl, 27
    mov bl, 0x1F
    mov si, preview_system_title_msg
    call write_string_at
    mov dh, 8
    mov si, preview_system_line1_msg
    call write_string_at
    mov dh, 9
    mov si, preview_system_line2_msg
    call write_string_at
    mov dh, 10
    mov si, preview_system_line3_msg
    call write_string_at
    jmp .common

.files:
    mov dh, 6
    mov dl, 27
    mov bl, 0x1F
    mov si, preview_files_title_msg
    call write_string_at
    mov dh, 8
    mov si, preview_files_line1_msg
    call write_string_at
    mov dh, 9
    mov si, preview_files_line2_msg
    call write_string_at
    mov dh, 10
    mov si, preview_files_line3_msg
    call write_string_at
    jmp .common

.notes:
    mov dh, 6
    mov dl, 27
    mov bl, 0x1F
    mov si, preview_notes_title_msg
    call write_string_at
    mov dh, 8
    mov si, preview_notes_line1_msg
    call write_string_at
    mov dh, 9
    mov si, preview_notes_line2_msg
    call write_string_at
    mov dh, 10
    mov si, preview_notes_line3_msg
    call write_string_at
    jmp .common

.clock:
    mov dh, 6
    mov dl, 27
    mov bl, 0x1F
    mov si, preview_clock_title_msg
    call write_string_at
    mov dh, 8
    mov si, preview_clock_line1_msg
    call write_string_at
    mov dh, 9
    mov si, preview_clock_line2_msg
    call write_string_at
    mov dh, 10
    mov si, preview_clock_line3_msg
    call write_string_at
    jmp .common

.network:
    mov dh, 6
    mov dl, 27
    mov bl, 0x1F
    mov si, preview_network_title_msg
    call write_string_at
    mov dh, 8
    mov si, preview_network_line1_msg
    call write_string_at
    mov dh, 9
    mov si, preview_network_line2_msg
    call write_string_at
    mov dh, 10
    mov si, preview_network_line3_msg
    call write_string_at
    jmp .common

.power:
    mov dh, 6
    mov dl, 27
    mov bl, 0x1F
    mov si, preview_power_title_msg
    call write_string_at
    mov dh, 8
    mov si, preview_power_line1_msg
    call write_string_at
    mov dh, 9
    mov si, preview_power_line2_msg
    call write_string_at
    mov dh, 10
    mov si, preview_power_line3_msg
    call write_string_at

.common:
    mov dh, 13
    mov dl, 27
    mov bl, 0x1E
    mov si, desktop_quick_header_msg
    call write_string_at

    mov dh, 15
    mov dl, 27
    mov bl, 0x1F
    mov si, desktop_memory_label_msg
    call write_string_at
    mov dl, 48
    mov si, memory_buffer
    call write_string_at
    mov dl, 54
    mov si, desktop_kb_msg
    call write_string_at

    mov dh, 16
    mov dl, 27
    mov si, desktop_kernel_label_msg
    call write_string_at
    mov dl, 48
    mov si, kernel_sector_buffer
    call write_string_at

    mov dh, 17
    mov dl, 27
    mov si, desktop_boot_label_msg
    call write_string_at
    mov dl, 48
    mov si, boot_drive_buffer
    call write_string_at

    mov dh, 18
    mov dl, 27
    mov si, desktop_clock_label_msg
    call write_string_at
    mov dl, 48
    mov si, date_buffer
    call write_string_at
    mov dl, 60
    mov si, time_buffer
    call write_string_at

    mov dh, 19
    mov dl, 27
    mov si, desktop_network_label_msg
    call write_string_at
    mov dl, 48
    mov si, network_status_buffer
    call write_string_at

    mov dh, 20
    mov dl, 27
    mov si, desktop_launch_hint_msg
    call write_string_at
    ret

render_system_app:
    call build_desktop_stats
    mov si, app_system_title_msg
    call render_standard_app_layout

    mov dh, 4
    mov dl, 8
    mov bl, 0x17
    mov si, system_heading_msg
    call write_string_at

    mov dh, 6
    mov si, system_version_label_msg
    call write_string_at
    mov dl, 28
    mov si, version_name_msg
    call write_string_at

    mov dh, 7
    mov dl, 8
    mov si, system_boot_label_msg
    call write_string_at
    mov dl, 28
    mov si, boot_drive_buffer
    call write_string_at

    mov dh, 8
    mov dl, 8
    mov si, system_kernel_label_msg
    call write_string_at
    mov dl, 28
    mov si, kernel_sector_buffer
    call write_string_at

    mov dh, 9
    mov dl, 8
    mov si, system_memory_label_msg
    call write_string_at
    mov dl, 28
    mov si, memory_buffer
    call write_string_at
    mov dl, 34
    mov si, desktop_kb_msg
    call write_string_at

    mov dh, 10
    mov dl, 8
    mov si, system_date_label_msg
    call write_string_at
    mov dl, 28
    mov si, date_buffer
    call write_string_at

    mov dh, 11
    mov dl, 8
    mov si, system_time_label_msg
    call write_string_at
    mov dl, 28
    mov si, time_buffer
    call write_string_at

    mov dh, 12
    mov dl, 8
    mov si, system_uptime_label_msg
    call write_string_at
    mov dl, 28
    mov si, uptime_buffer
    call write_string_at
    mov dl, 40
    mov si, seconds_suffix_inline_msg
    call write_string_at

    mov dh, 14
    mov dl, 8
    mov si, system_note_line1_msg
    call write_string_at
    mov dh, 15
    mov si, system_note_line2_msg
    call write_string_at
    mov dh, 16
    mov si, system_note_line3_msg
    call write_string_at
    ret

render_files_app:
    mov si, app_files_title_msg
    call render_standard_app_layout

    mov dh, 4
    mov dl, 8
    mov bl, 0x17
    mov si, files_heading_msg
    call write_string_at
    mov dh, 6
    mov si, files_line1_msg
    call write_string_at
    mov dh, 7
    mov si, files_line2_msg
    call write_string_at
    mov dh, 8
    mov si, files_line3_msg
    call write_string_at
    mov dh, 9
    mov si, files_line4_msg
    call write_string_at
    mov dh, 10
    mov si, files_line5_msg
    call write_string_at
    mov dh, 12
    mov si, files_note_line1_msg
    call write_string_at
    mov dh, 13
    mov si, files_note_line2_msg
    call write_string_at
    mov dh, 14
    mov si, files_note_line3_msg
    call write_string_at
    ret

render_notes_app:
    mov si, app_notes_title_msg
    call render_standard_app_layout

    mov dh, 4
    mov dl, 8
    mov bl, 0x17
    mov si, notes_heading_msg
    call write_string_at
    mov dh, 6
    mov si, notes_line1_msg
    call write_string_at
    mov dh, 7
    mov si, notes_line2_msg
    call write_string_at
    mov dh, 8
    mov si, notes_line3_msg
    call write_string_at
    mov dh, 9
    mov si, notes_line4_msg
    call write_string_at
    mov dh, 10
    mov si, notes_line5_msg
    call write_string_at
    mov dh, 12
    mov si, notes_footer_line1_msg
    call write_string_at
    mov dh, 13
    mov si, notes_footer_line2_msg
    call write_string_at
    ret

render_clock_app:
    call build_desktop_stats
    mov si, app_clock_title_msg
    call render_standard_app_layout

    mov dh, 4
    mov dl, 8
    mov bl, 0x17
    mov si, clock_heading_msg
    call write_string_at

    mov dh, 7
    mov dl, 8
    mov si, clock_date_label_msg
    call write_string_at
    mov dl, 24
    mov si, date_buffer
    call write_string_at

    mov dh, 9
    mov dl, 8
    mov si, clock_time_label_msg
    call write_string_at
    mov dl, 24
    mov si, time_buffer
    call write_string_at

    mov dh, 11
    mov dl, 8
    mov si, clock_uptime_label_msg
    call write_string_at
    mov dl, 24
    mov si, uptime_buffer
    call write_string_at
    mov dl, 36
    mov si, seconds_suffix_inline_msg
    call write_string_at

    mov dh, 14
    mov dl, 8
    mov si, clock_note_line1_msg
    call write_string_at
    mov dh, 15
    mov si, clock_note_line2_msg
    call write_string_at
    ret

render_network_app:
    call build_network_strings
    mov si, app_network_title_msg
    call render_standard_app_layout

    mov dh, 4
    mov dl, 8
    mov bl, 0x17
    mov si, network_heading_msg
    call write_string_at

    mov dh, 6
    mov dl, 8
    mov si, network_status_label_msg
    call write_string_at
    mov dl, 28
    mov si, network_status_buffer
    call write_string_at

    mov dh, 7
    mov dl, 8
    mov si, network_slot_label_msg
    call write_string_at
    mov dl, 28
    mov si, net_slot_buffer
    call write_string_at

    mov dh, 8
    mov dl, 8
    mov si, network_vendor_label_msg
    call write_string_at
    mov dl, 28
    mov si, net_vendor_buffer
    call write_string_at

    mov dh, 9
    mov dl, 8
    mov si, network_device_label_msg
    call write_string_at
    mov dl, 28
    mov si, net_device_buffer
    call write_string_at

    mov dh, 10
    mov dl, 8
    mov si, network_class_label_msg
    call write_string_at
    mov dl, 28
    mov si, net_class_buffer
    call write_string_at
    mov dl, 31
    mov si, slash_msg
    call write_string_at
    mov dl, 32
    mov si, net_subclass_buffer
    call write_string_at

    mov dh, 12
    mov dl, 8
    mov si, network_note_line1_msg
    call write_string_at
    mov dh, 13
    mov si, network_note_line2_msg
    call write_string_at
    mov dh, 14
    mov si, network_note_line3_msg
    call write_string_at
    mov dh, 16
    mov si, network_note_line4_msg
    call write_string_at
    ret

render_power_app:
    mov dh, 0
    mov dl, 0
    mov ch, 25
    mov cl, 80
    mov bl, 0x4F
    mov al, ' '
    call fill_rect

    mov dh, 0
    mov dl, 0
    mov ch, 1
    mov cl, 80
    mov bl, 0x70
    mov al, ' '
    call fill_rect

    mov dh, 5
    mov dl, 14
    mov ch, 13
    mov cl, 52
    mov bl, 0x4F
    mov al, ' '
    call fill_rect

    mov dh, 0
    mov dl, 2
    mov bl, 0x70
    mov si, app_power_title_msg
    call write_string_at

    mov dh, 7
    mov dl, 20
    mov bl, 0x4F
    mov si, power_heading_msg
    call write_string_at
    mov dh, 10
    mov si, power_line1_msg
    call write_string_at
    mov dh, 11
    mov si, power_line2_msg
    call write_string_at
    mov dh, 12
    mov si, power_line3_msg
    call write_string_at
    mov dh, 13
    mov si, power_line4_msg
    call write_string_at
    mov dh, 16
    mov si, power_line5_msg
    call write_string_at
    mov dh, 17
    mov si, power_line6_msg
    call write_string_at
    ret

render_help_app:
    mov si, app_help_title_msg
    call render_standard_app_layout

    mov dh, 4
    mov dl, 8
    mov bl, 0x17
    mov si, help_heading_msg
    call write_string_at
    mov dh, 6
    mov si, help_line1_msg
    call write_string_at
    mov dh, 7
    mov si, help_line2_msg
    call write_string_at
    mov dh, 8
    mov si, help_line3_msg
    call write_string_at
    mov dh, 9
    mov si, help_line4_msg
    call write_string_at
    mov dh, 10
    mov si, help_line5_msg
    call write_string_at
    mov dh, 12
    mov si, help_line6_msg
    call write_string_at
    mov dh, 13
    mov si, help_line7_msg
    call write_string_at
    mov dh, 14
    mov si, help_line8_msg
    call write_string_at
    ret

render_standard_app_layout:
    push si

    mov dh, 0
    mov dl, 0
    mov ch, 25
    mov cl, 80
    mov bl, 0x1F
    mov al, ' '
    call fill_rect

    mov dh, 0
    mov dl, 0
    mov ch, 1
    mov cl, 80
    mov bl, 0x70
    mov al, ' '
    call fill_rect

    mov dh, 2
    mov dl, 4
    mov ch, 20
    mov cl, 72
    mov bl, 0x17
    mov al, ' '
    call fill_rect

    mov dh, 23
    mov dl, 0
    mov ch, 2
    mov cl, 80
    mov bl, 0x70
    mov al, ' '
    call fill_rect

    pop si
    mov dh, 0
    mov dl, 2
    mov bl, 0x70
    call write_string_at

    mov dh, 23
    mov dl, 2
    mov bl, 0x70
    mov si, app_footer_line1_msg
    call write_string_at

    mov dh, 24
    mov dl, 2
    mov bl, 0x70
    mov si, app_footer_line2_msg
    call write_string_at
    ret

terminal_session:
    call hide_mouse_cursor_if_visible
    call clear_screen
    call print_banner
    mov si, terminal_ready_msg
    call print_string
    call print_boot_summary_line
    mov si, terminal_hint_msg
    call print_string

shell_loop:
    mov si, prompt_msg
    call print_string

    mov di, input_buffer
    mov cx, INPUT_MAX
    call read_line

process_input:
    call trim_input_buffer
    cmp byte [input_buffer], 0
    je shell_loop

    mov si, input_buffer
    call save_history
    call parse_command

    mov si, command_buffer
    mov di, cmd_repeat
    call string_equals
    cmp al, 1
    je dispatch_command

    mov si, input_buffer
    mov di, last_runnable_buffer
    mov cx, HISTORY_ENTRY_SIZE
    call copy_string

dispatch_command:
    mov si, command_buffer
    mov di, cmd_help
    call string_equals
    cmp al, 1
    je show_help

    mov si, command_buffer
    mov di, cmd_clear
    call string_equals
    cmp al, 1
    je do_clear

    mov si, command_buffer
    mov di, cmd_cls
    call string_equals
    cmp al, 1
    je do_clear

    mov si, command_buffer
    mov di, cmd_about
    call string_equals
    cmp al, 1
    je show_about

    mov si, command_buffer
    mov di, cmd_version
    call string_equals
    cmp al, 1
    je show_version

    mov si, command_buffer
    mov di, cmd_ver
    call string_equals
    cmp al, 1
    je show_version

    mov si, command_buffer
    mov di, cmd_echo
    call string_equals
    cmp al, 1
    je do_echo

    mov si, command_buffer
    mov di, cmd_banner
    call string_equals
    cmp al, 1
    je show_banner

    mov si, command_buffer
    mov di, cmd_beep
    call string_equals
    cmp al, 1
    je do_beep

    mov si, command_buffer
    mov di, cmd_date
    call string_equals
    cmp al, 1
    je show_date

    mov si, command_buffer
    mov di, cmd_time
    call string_equals
    cmp al, 1
    je show_time

    mov si, command_buffer
    mov di, cmd_uptime
    call string_equals
    cmp al, 1
    je show_uptime

    mov si, command_buffer
    mov di, cmd_mem
    call string_equals
    cmp al, 1
    je show_mem

    mov si, command_buffer
    mov di, cmd_boot
    call string_equals
    cmp al, 1
    je show_boot

    mov si, command_buffer
    mov di, cmd_status
    call string_equals
    cmp al, 1
    je show_status

    mov si, command_buffer
    mov di, cmd_history
    call string_equals
    cmp al, 1
    je show_history

    mov si, command_buffer
    mov di, cmd_repeat
    call string_equals
    cmp al, 1
    je do_repeat

    mov si, command_buffer
    mov di, cmd_net
    call string_equals
    cmp al, 1
    je show_net

    mov si, command_buffer
    mov di, cmd_ping
    call string_equals
    cmp al, 1
    je do_ping

    mov si, command_buffer
    mov di, cmd_arch
    call string_equals
    cmp al, 1
    je show_arch

    mov si, command_buffer
    mov di, cmd_apps
    call string_equals
    cmp al, 1
    je show_apps

    mov si, command_buffer
    mov di, cmd_desktop
    call string_equals
    cmp al, 1
    je leave_to_desktop

    mov si, command_buffer
    mov di, cmd_gui
    call string_equals
    cmp al, 1
    je leave_to_desktop

    mov si, command_buffer
    mov di, cmd_popeye
    call string_equals
    cmp al, 1
    je leave_to_desktop

    mov si, command_buffer
    mov di, cmd_halt
    call string_equals
    cmp al, 1
    je do_halt

    mov si, command_buffer
    mov di, cmd_shutdown
    call string_equals
    cmp al, 1
    je do_halt

    mov si, command_buffer
    mov di, cmd_reboot
    call string_equals
    cmp al, 1
    je do_reboot

    mov si, command_buffer
    mov di, cmd_restart
    call string_equals
    cmp al, 1
    je do_reboot

    mov si, unknown_msg
    call print_string
    jmp shell_loop

show_help:
    mov si, help_msg
    call print_string
    jmp shell_loop

do_clear:
    call clear_screen
    jmp shell_loop

show_about:
    mov si, about_msg
    call print_string
    jmp shell_loop

show_version:
    mov si, version_msg
    call print_string
    jmp shell_loop

do_echo:
    mov si, [args_ptr]
    cmp byte [si], 0
    jne .print
    mov si, echo_usage_msg
    call print_string
    jmp shell_loop
.print:
    call print_string
    call print_newline
    jmp shell_loop

show_banner:
    call print_banner
    jmp shell_loop

do_beep:
    mov al, 7
    call print_char
    mov si, beep_msg
    call print_string
    jmp shell_loop

show_date:
    mov si, date_prefix_msg
    call print_string
    call print_date_inline
    jc .error
    call print_newline
    jmp shell_loop
.error:
    mov si, unavailable_msg
    call print_string
    call print_newline
    jmp shell_loop

show_time:
    mov si, time_prefix_msg
    call print_string
    call print_time_inline
    jc .error
    call print_newline
    jmp shell_loop
.error:
    mov si, unavailable_msg
    call print_string
    call print_newline
    jmp shell_loop

show_uptime:
    mov si, uptime_prefix_msg
    call print_string
    call print_uptime_inline
    jc .error
    call print_newline
    jmp shell_loop
.error:
    mov si, unavailable_msg
    call print_string
    call print_newline
    jmp shell_loop

show_mem:
    mov si, mem_prefix_msg
    call print_string
    int 0x12
    call print_word_decimal
    mov si, kb_suffix_msg
    call print_string
    call print_newline
    jmp shell_loop

show_boot:
    mov si, boot_prefix_msg
    call print_string
    call print_boot_info_inline
    call print_newline
    jmp shell_loop

show_status:
    mov si, status_header_msg
    call print_string

    mov si, status_version_label
    call print_string
    mov si, version_name_msg
    call print_string
    call print_newline

    mov si, status_boot_label
    call print_string
    call print_boot_info_inline
    call print_newline

    mov si, status_mem_label
    call print_string
    int 0x12
    call print_word_decimal
    mov si, kb_suffix_msg
    call print_string
    call print_newline

    mov si, status_date_label
    call print_string
    call print_date_inline
    jc .date_error
    call print_newline
    jmp .time
.date_error:
    mov si, unavailable_msg
    call print_string
    call print_newline

.time:
    mov si, status_time_label
    call print_string
    call print_time_inline
    jc .time_error
    call print_newline
    jmp .uptime
.time_error:
    mov si, unavailable_msg
    call print_string
    call print_newline

.uptime:
    mov si, status_uptime_label
    call print_string
    call print_uptime_inline
    jc .uptime_error
    call print_newline
    jmp .history
.uptime_error:
    mov si, unavailable_msg
    call print_string
    call print_newline

.history:
    mov si, status_history_label
    call print_string
    xor ax, ax
    mov al, [history_used]
    call print_word_decimal
    mov si, entries_suffix_msg
    call print_string
    call print_newline
    call print_newline
    jmp shell_loop

show_history:
    cmp byte [history_used], 0
    jne .have_entries
    mov si, no_history_msg
    call print_string
    jmp shell_loop

.have_entries:
    mov si, history_header_msg
    call print_string

    xor cx, cx
    mov cl, [history_used]
    mov bl, [history_next]
    cmp byte [history_used], HISTORY_COUNT
    je .loop_ready
    xor bl, bl

.loop_ready:
    xor dx, dx
    mov dl, 1

.print_entry:
    mov ax, dx
    call print_word_decimal
    mov al, '.'
    call print_char
    mov al, ' '
    call print_char

    xor bh, bh
    mov ax, bx
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov si, history_buffer
    add si, ax
    call print_string
    call print_newline

    inc bl
    cmp bl, HISTORY_COUNT
    jb .wrapped
    xor bl, bl
.wrapped:
    inc dx
    loop .print_entry

    call print_newline
    jmp shell_loop

do_repeat:
    cmp byte [last_runnable_buffer], 0
    jne .have_command
    mov si, no_repeat_msg
    call print_string
    jmp shell_loop

.have_command:
    mov si, repeat_prefix_msg
    call print_string
    mov si, last_runnable_buffer
    call print_string
    call print_newline

    mov si, last_runnable_buffer
    mov di, input_buffer
    mov cx, HISTORY_ENTRY_SIZE
    call copy_string
    call parse_command
    jmp dispatch_command

show_apps:
    mov si, apps_msg
    call print_string
    jmp shell_loop

show_net:
    call build_network_strings
    mov si, net_console_header_msg
    call print_string
    mov si, net_console_state_label_msg
    call print_string
    mov si, network_status_buffer
    call print_string
    call print_newline

    cmp byte [net_present], 1
    jne .missing

    mov si, net_console_slot_label_msg
    call print_string
    mov si, net_slot_buffer
    call print_string
    call print_newline

    mov si, net_console_vendor_label_msg
    call print_string
    mov si, net_vendor_buffer
    call print_string
    mov si, net_console_device_mid_msg
    call print_string
    mov si, net_device_buffer
    call print_string
    call print_newline

    mov si, net_console_class_label_msg
    call print_string
    mov si, net_class_buffer
    call print_string
    mov si, slash_msg
    call print_string
    mov si, net_subclass_buffer
    call print_string
    call print_newline

    mov si, net_console_hint_msg
    call print_string
    jmp shell_loop

.missing:
    mov si, net_console_missing_msg
    call print_string
    jmp shell_loop

do_ping:
    mov si, [args_ptr]
    cmp byte [si], 0
    jne .has_target
    mov si, ping_usage_msg
    call print_string
    jmp shell_loop

.has_target:
    mov di, ping_loopback_target_msg
    call string_equals
    cmp al, 1
    je .loopback

    mov si, [args_ptr]
    mov di, ping_localhost_target_msg
    call string_equals
    cmp al, 1
    je .loopback

    mov si, ping_stub_msg
    call print_string
    jmp shell_loop

.loopback:
    mov si, ping_reply_msg
    call print_string
    jmp shell_loop

show_arch:
    mov si, arch_msg
    call print_string
    jmp shell_loop

leave_to_desktop:
    mov si, returning_to_desktop_msg
    call print_string
    ret

do_halt:
    mov si, halt_msg
    call print_string
.halt_forever:
    cli
    hlt
    jmp .halt_forever

do_reboot:
    mov si, reboot_msg
    call print_string
    int 0x19
    jmp do_halt

capture_boot_ticks:
    pusha
    mov ah, 0x00
    int 0x1A
    jc .done
    mov [boot_ticks_hi], cx
    mov [boot_ticks_lo], dx
.done:
    popa
    ret

boot_mode_menu:
    call clear_screen
    mov si, banner_msg
    call print_string
    mov si, boot_menu_title_msg
    call print_string
    mov si, boot_menu_line1_msg
    call print_string
    mov si, boot_menu_line2_msg
    call print_string
    mov si, boot_menu_line3_msg
    call print_string

.wait:
    mov si, boot_menu_prompt_msg
    call print_string
    call wait_key
    call print_newline

    cmp al, 13
    je .desktop
    cmp al, 'd'
    je .desktop
    cmp al, 'D'
    je .desktop
    cmp al, 'c'
    je .console
    cmp al, 'C'
    je .console

    mov si, boot_menu_invalid_msg
    call print_string
    jmp .wait

.desktop:
    mov byte [boot_mode], BOOT_MODE_DESKTOP
    ret

.console:
    mov byte [boot_mode], BOOT_MODE_CONSOLE
    ret

init_mouse:
    mov byte [mouse_available], 0
    mov byte [mouse_visible], 0
    mov byte [mouse_prev_buttons], 0

    xor ax, ax
    mov es, ax
    mov bx, 0x00CC
    mov ax, [es:bx]
    or ax, [es:bx + 2]
    jz .done

    mov ax, 0x0000
    int 0x33
    cmp ax, 0
    je .done

    mov byte [mouse_available], 1
.done:
    ret

show_mouse_cursor_if_available:
    cmp byte [mouse_available], 1
    jne .done
    cmp byte [mouse_visible], 1
    je .done
    mov ax, 0x0001
    int 0x33
    mov byte [mouse_visible], 1
.done:
    ret

hide_mouse_cursor_if_visible:
    cmp byte [mouse_available], 1
    jne .done
    cmp byte [mouse_visible], 1
    jne .done
    mov ax, 0x0002
    int 0x33
    mov byte [mouse_visible], 0
.done:
    ret

wait_desktop_input:
.loop:
    call poll_mouse_event
    cmp al, 1
    je .mouse_event

    mov ah, 0x01
    int 0x16
    jz .loop

    xor ah, ah
    int 0x16
    mov byte [event_type], 1
    ret

.mouse_event:
    mov byte [event_type], 2
    ret

poll_mouse_event:
    push bx
    push cx
    push dx

    xor al, al
    cmp byte [mouse_available], 1
    jne .done

    mov ax, 0x0003
    int 0x33

    mov ax, cx
    shr ax, 3
    cmp ax, 79
    jbe .col_ok
    mov ax, 79
.col_ok:
    mov [mouse_col], al

    mov ax, dx
    shr ax, 3
    cmp ax, 24
    jbe .row_ok
    mov ax, 24
.row_ok:
    mov [mouse_row], al

    mov byte [mouse_click_app], 0xFF

    mov dl, [mouse_col]
    cmp dl, 4
    jb .check_click
    cmp dl, 19
    ja .check_click

    mov al, [mouse_row]
    call map_mouse_row_to_app
    cmp al, 0xFF
    je .check_click
    mov [desktop_selection], al
    mov [mouse_click_app], al

.check_click:
    mov al, [mouse_prev_buttons]
    and al, 1
    mov ah, bl
    and ah, 1
    mov [mouse_prev_buttons], bl

    cmp ah, 1
    jne .no_click
    cmp al, 0
    jne .no_click
    cmp byte [mouse_click_app], 0xFF
    je .no_click
    mov al, 1
    jmp .done

.no_click:
    xor al, al

.done:
    pop dx
    pop cx
    pop bx
    ret

map_mouse_row_to_app:
    cmp al, 5
    je .terminal
    cmp al, 7
    je .system
    cmp al, 9
    je .files
    cmp al, 11
    je .notes
    cmp al, 13
    je .clock
    cmp al, 15
    je .network
    cmp al, 17
    je .power
    mov al, 0xFF
    ret

.terminal:
    mov al, APP_TERMINAL
    ret
.system:
    mov al, APP_SYSTEM
    ret
.files:
    mov al, APP_FILES
    ret
.notes:
    mov al, APP_NOTES
    ret
.clock:
    mov al, APP_CLOCK
    ret
.network:
    mov al, APP_NETWORK
    ret
.power:
    mov al, APP_POWER
    ret

init_network_subsystem:
    call detect_network_adapter
    call build_network_strings
    ret

detect_network_adapter:
    push ax
    push bx
    push cx
    push dx
    push eax
    push ecx
    push edx

    mov byte [net_present], 0
    mov byte [net_device_slot], 0
    mov word [net_vendor_id], 0
    mov word [net_device_id], 0
    mov byte [net_class], 0
    mov byte [net_subclass], 0

    xor bl, bl

.scan_device:
    cmp bl, 32
    jae .done

    mov cl, 0x00
    call pci_read_dword_bus0
    mov edx, eax
    cmp dx, 0xFFFF
    je .next_device

    mov cl, 0x08
    call pci_read_dword_bus0
    mov edx, eax
    shr edx, 24
    cmp dl, 0x02
    jne .next_device

    mov byte [net_present], 1
    mov [net_device_slot], bl

    mov cl, 0x00
    call pci_read_dword_bus0
    mov edx, eax
    mov [net_vendor_id], dx
    shr edx, 16
    mov [net_device_id], dx

    mov cl, 0x08
    call pci_read_dword_bus0
    mov edx, eax
    shr edx, 16
    mov [net_subclass], dl
    mov [net_class], dh
    jmp .done

.next_device:
    inc bl
    jmp .scan_device

.done:
    pop edx
    pop ecx
    pop eax
    pop dx
    pop cx
    pop bx
    pop ax
    ret

pci_read_dword_bus0:
    push bx
    push cx
    push dx
    push edx

    movzx eax, bl
    shl eax, 11
    movzx edx, cl
    and edx, 0xFC
    or eax, edx
    or eax, 0x80000000

    mov dx, 0xCF8
    out dx, eax
    mov dx, 0xCFC
    in eax, dx

    pop edx
    pop dx
    pop cx
    pop bx
    ret

build_network_strings:
    push ax
    push di
    push si
    push es

    cmp byte [net_present], 1
    jne .offline

    push ds
    pop es

    mov si, net_state_online_msg
    mov di, network_status_buffer
    call copy_zero_string

    xor ax, ax
    mov al, [net_device_slot]
    mov di, net_slot_buffer
    call word_to_decimal_string

    mov ax, [net_vendor_id]
    mov di, net_vendor_buffer
    call word_to_hex_string

    mov ax, [net_device_id]
    mov di, net_device_buffer
    call word_to_hex_string

    mov al, [net_class]
    mov di, net_class_buffer
    call byte_to_hex_string

    mov al, [net_subclass]
    mov di, net_subclass_buffer
    call byte_to_hex_string
    jmp .done

.offline:
    push ds
    pop es

    mov si, net_state_offline_msg
    mov di, network_status_buffer
    call copy_zero_string

    mov si, net_none_short_msg
    mov di, net_slot_buffer
    call copy_zero_string

    mov si, net_none_word_msg
    mov di, net_vendor_buffer
    call copy_zero_string

    mov si, net_none_word_msg
    mov di, net_device_buffer
    call copy_zero_string

    mov si, net_none_byte_msg
    mov di, net_class_buffer
    call copy_zero_string

    mov si, net_none_byte_msg
    mov di, net_subclass_buffer
    call copy_zero_string

.done:
    pop es
    pop si
    pop di
    pop ax
    ret

build_desktop_stats:
    call build_date_string
    call build_time_string
    call build_memory_string
    call build_kernel_sectors_string
    call build_boot_drive_string
    call build_uptime_string
    call build_network_strings
    ret

build_date_string:
    push ax
    push cx
    push dx
    push si
    push di
    push es
    push ds
    pop es

    mov ah, 0x04
    int 0x1A
    jc .fallback

    mov di, date_buffer
    mov al, ch
    call store_bcd_ascii
    mov al, cl
    call store_bcd_ascii
    mov al, '-'
    stosb
    mov al, dh
    call store_bcd_ascii
    mov al, '-'
    stosb
    mov al, dl
    call store_bcd_ascii
    xor al, al
    stosb
    jmp .done

.fallback:
    mov si, unavailable_msg
    mov di, date_buffer
    call copy_zero_string

.done:
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop ax
    ret

build_time_string:
    push ax
    push cx
    push dx
    push si
    push di
    push es
    push ds
    pop es

    mov ah, 0x02
    int 0x1A
    jc .fallback

    mov di, time_buffer
    mov al, ch
    call store_bcd_ascii
    mov al, ':'
    stosb
    mov al, cl
    call store_bcd_ascii
    mov al, ':'
    stosb
    mov al, dh
    call store_bcd_ascii
    xor al, al
    stosb
    jmp .done

.fallback:
    mov si, unavailable_msg
    mov di, time_buffer
    call copy_zero_string

.done:
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop ax
    ret

build_memory_string:
    push ax
    push di
    int 0x12
    mov di, memory_buffer
    call word_to_decimal_string
    pop di
    pop ax
    ret

build_kernel_sectors_string:
    push ax
    push di
    xor ax, ax
    mov al, [kernel_sectors]
    mov di, kernel_sector_buffer
    call word_to_decimal_string
    pop di
    pop ax
    ret

build_boot_drive_string:
    push ax
    push di
    mov al, [boot_drive]
    mov di, boot_drive_buffer
    call byte_to_hex_string
    pop di
    pop ax
    ret

build_uptime_string:
    push ax
    push bx
    push cx
    push dx
    push si
    push di

    call get_elapsed_ticks
    jc .fallback
    mov bx, 18
    call divide_dword_by_word
    mov dx, cx
    mov di, uptime_buffer
    call dword_to_decimal_string
    jmp .done

.fallback:
    mov si, unavailable_msg
    mov di, uptime_buffer
    push es
    push ds
    pop es
    call copy_zero_string
    pop es

.done:
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

store_bcd_ascii:
    push ax
    mov ah, al
    shr al, 4
    and al, 0x0F
    add al, '0'
    stosb
    mov al, ah
    and al, 0x0F
    add al, '0'
    stosb
    pop ax
    ret

copy_zero_string:
    push ax
.loop:
    lodsb
    stosb
    test al, al
    jnz .loop
    pop ax
    ret

word_to_decimal_string:
    xor dx, dx
    jmp dword_to_decimal_string

dword_to_decimal_string:
    push bx
    push cx
    push si
    push es
    mov [number_work_lo], ax
    mov [number_work_hi], dx
    mov si, decimal_buffer + 10
    mov byte [si], 0

    cmp dx, 0
    jne .convert
    cmp ax, 0
    jne .convert
    dec si
    mov byte [si], '0'
    jmp .copy

.convert:
.next_digit:
    mov ax, [number_work_lo]
    mov dx, [number_work_hi]
    mov bx, 10
    call divide_dword_by_word
    mov [number_work_lo], ax
    mov [number_work_hi], cx
    add dl, '0'
    dec si
    mov [si], dl
    cmp cx, 0
    jne .next_digit
    cmp ax, 0
    jne .next_digit

.copy:
    push ds
    pop es
    call copy_zero_string
    pop es
    pop si
    pop cx
    pop bx
    ret

byte_to_hex_string:
    push ax
    push es
    push ds
    pop es
    mov ah, al
    shr al, 4
    call nibble_to_hex_ascii
    stosb
    mov al, ah
    and al, 0x0F
    call nibble_to_hex_ascii
    stosb
    xor al, al
    stosb
    pop es
    pop ax
    ret

word_to_hex_string:
    push ax
    push bx
    push es
    push ds
    pop es

    mov bx, ax

    mov al, bh
    shr al, 4
    call nibble_to_hex_ascii
    stosb

    mov al, bh
    and al, 0x0F
    call nibble_to_hex_ascii
    stosb

    mov al, bl
    shr al, 4
    call nibble_to_hex_ascii
    stosb

    mov al, bl
    and al, 0x0F
    call nibble_to_hex_ascii
    stosb

    xor al, al
    stosb

    pop es
    pop bx
    pop ax
    ret

nibble_to_hex_ascii:
    and al, 0x0F
    cmp al, 9
    jbe .digit
    add al, 'A' - 10
    ret
.digit:
    add al, '0'
    ret

wait_key:
    xor ah, ah
    int 0x16
    ret

set_text_mode:
    push ax
    mov ax, 0x0003
    int 0x10
    pop ax
    ret

fill_rect:
    push ax
    push bx
    push cx
    push dx
    push di

    mov [rect_char], al
    mov [rect_attr], bl
    mov [rect_top], dh
    mov [rect_left], dl
    mov [rect_height], ch
    mov [rect_width], cl

    mov dh, [rect_top]
    xor di, di
    xor ax, ax
    mov al, [rect_height]
    mov di, ax

.row_loop:
    mov dl, [rect_left]
    xor cx, cx
    mov cl, [rect_width]

.col_loop:
    mov al, [rect_char]
    mov bl, [rect_attr]
    call put_char_at
    inc dl
    loop .col_loop

    inc dh
    dec di
    jnz .row_loop

    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

write_string_at:
    push ax
    push bx
    push dx
    push si

.next_char:
    lodsb
    test al, al
    jz .done
    call put_char_at
    inc dl
    jmp .next_char

.done:
    pop si
    pop dx
    pop bx
    pop ax
    ret

put_char_at:
    push ax
    push bx
    push cx
    push dx
    push es
    push di

    cmp dh, 25
    jae .done
    cmp dl, 80
    jae .done

    mov ch, bl
    mov cl, al

    xor ax, ax
    mov al, dh
    mov di, ax
    shl di, 6
    shl ax, 4
    add di, ax
    xor ax, ax
    mov al, dl
    add di, ax
    shl di, 1

    mov ax, VIDEO_SEGMENT
    mov es, ax
    mov ax, cx
    mov [es:di], ax

.done:
    pop di
    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    ret

skip_spaces:
.skip:
    cmp byte [si], ' '
    jne .done
    inc si
    jmp .skip
.done:
    ret

to_lower:
    cmp al, 'A'
    jb .done
    cmp al, 'Z'
    ja .done
    add al, 32
.done:
    ret

trim_input_buffer:
    push ax
    push bx
    push si
    push di
    push es
    push ds
    pop es

    mov si, input_buffer
    call skip_spaces
    mov di, input_buffer

.copy:
    lodsb
    stosb
    test al, al
    jnz .copy

    mov bx, di
    dec bx
    cmp bx, input_buffer
    je .done
    dec bx

.trim_tail:
    cmp byte [bx], ' '
    jne .done
    mov byte [bx], 0
    cmp bx, input_buffer
    je .done
    dec bx
    jmp .trim_tail

.done:
    pop es
    pop di
    pop si
    pop bx
    pop ax
    ret

parse_command:
    push ax
    push cx
    push si
    push di

    mov si, input_buffer
    call skip_spaces
    mov di, command_buffer
    mov cx, COMMAND_MAX

.copy_char:
    mov al, [si]
    cmp al, 0
    je .finish
    cmp al, ' '
    je .finish
    cmp cx, 0
    je .skip_store
    call to_lower
    mov [di], al
    inc di
    dec cx
.skip_store:
    inc si
    jmp .copy_char

.finish:
    mov byte [di], 0
    call skip_spaces
    mov [args_ptr], si

    pop di
    pop si
    pop cx
    pop ax
    ret

string_equals:
    push si
    push di
.compare:
    mov al, [si]
    mov ah, [di]
    cmp al, ah
    jne .not_equal
    cmp al, 0
    je .equal
    inc si
    inc di
    jmp .compare
.equal:
    mov al, 1
    jmp .finish
.not_equal:
    xor al, al
.finish:
    pop di
    pop si
    ret

copy_string:
    push ax
    push es
    push ds
    pop es
.loop:
    cmp cx, 1
    jne .room
    mov byte [di], 0
    jmp .done
.room:
    lodsb
    stosb
    dec cx
    test al, al
    jnz .loop
.done:
    pop es
    pop ax
    ret

save_history:
    push ax
    push bx
    push cx
    push di
    push si

    xor bx, bx
    mov bl, [history_next]
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1

    mov di, history_buffer
    add di, bx
    mov cx, HISTORY_ENTRY_SIZE
    call copy_string

    inc byte [history_next]
    cmp byte [history_next], HISTORY_COUNT
    jb .counted
    mov byte [history_next], 0

.counted:
    cmp byte [history_used], HISTORY_COUNT
    jae .done
    inc byte [history_used]

.done:
    pop si
    pop di
    pop cx
    pop bx
    pop ax
    ret

clear_screen:
    jmp set_text_mode

print_banner:
    mov si, banner_msg
    call print_string
    ret

print_char:
    push ax
    push bx
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0F
    int 0x10
    pop bx
    pop ax
    ret

print_string:
    pusha
.next_char:
    lodsb
    test al, al
    jz .done
    call print_char
    jmp .next_char
.done:
    popa
    ret

print_newline:
    pusha
    mov al, 13
    call print_char
    mov al, 10
    call print_char
    popa
    ret

read_line:
    pusha
    xor bx, bx

.read_key:
    xor ah, ah
    int 0x16

    cmp al, 13
    je .enter
    cmp al, 27
    je .clear_current
    cmp al, 8
    je .backspace
    cmp al, 32
    jb .read_key
    cmp al, 126
    ja .read_key
    cmp bx, cx
    jae .read_key

    mov [di + bx], al
    inc bx
    call print_char
    jmp .read_key

.backspace:
    cmp bx, 0
    je .read_key
    dec bx
    mov ah, 0x0E
    mov al, 8
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 8
    int 0x10
    jmp .read_key

.clear_current:
    cmp bx, 0
    je .read_key
.clear_loop:
    dec bx
    mov ah, 0x0E
    mov al, 8
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 8
    int 0x10
    cmp bx, 0
    jne .clear_loop
    jmp .read_key

.enter:
    mov byte [di + bx], 0
    call print_newline
    popa
    ret

print_boot_summary_line:
    mov si, boot_summary_a
    call print_string
    mov al, [boot_drive]
    call print_hex_byte
    mov si, boot_summary_b
    call print_string
    xor ax, ax
    mov al, [kernel_sectors]
    call print_word_decimal
    mov si, boot_summary_c
    call print_string
    ret

print_boot_info_inline:
    push ax
    push bx
    push dx
    push si

    mov si, boot_inline_a
    call print_string
    mov al, [boot_drive]
    call print_hex_byte

    mov si, boot_inline_b
    call print_string
    xor ax, ax
    mov al, [kernel_sectors]
    call print_word_decimal

    mov si, boot_inline_c
    call print_string
    xor ax, ax
    mov al, [kernel_sectors]
    mov bx, 512
    mul bx
    call print_dword_decimal

    mov si, boot_inline_d
    call print_string

    pop si
    pop dx
    pop bx
    pop ax
    ret

print_date_inline:
    push ax
    push cx
    push dx

    mov ah, 0x04
    int 0x1A
    jc .error
    mov al, ch
    call print_bcd_byte
    mov al, cl
    call print_bcd_byte
    mov al, '-'
    call print_char
    mov al, dh
    call print_bcd_byte
    mov al, '-'
    call print_char
    mov al, dl
    call print_bcd_byte
    clc
    jmp .done

.error:
    stc

.done:
    pop dx
    pop cx
    pop ax
    ret

print_time_inline:
    push ax
    push cx
    push dx

    mov ah, 0x02
    int 0x1A
    jc .error
    mov al, ch
    call print_bcd_byte
    mov al, ':'
    call print_char
    mov al, cl
    call print_bcd_byte
    mov al, ':'
    call print_char
    mov al, dh
    call print_bcd_byte
    clc
    jmp .done

.error:
    stc

.done:
    pop dx
    pop cx
    pop ax
    ret

print_uptime_inline:
    push ax
    push bx
    push cx
    push dx
    push si

    call get_elapsed_ticks
    jc .error
    mov bx, 18
    call divide_dword_by_word
    mov dx, cx
    call print_dword_decimal
    mov si, seconds_suffix_msg
    call print_string
    clc
    jmp .done

.error:
    stc

.done:
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

get_elapsed_ticks:
    push bx
    push cx

    mov ah, 0x00
    int 0x1A
    jc .error

    mov ax, dx
    mov dx, cx
    sub ax, [boot_ticks_lo]
    sbb dx, [boot_ticks_hi]
    jnc .done
    add ax, DAY_TICKS_LO
    adc dx, DAY_TICKS_HI

.done:
    clc
    pop cx
    pop bx
    ret

.error:
    stc
    pop cx
    pop bx
    ret

print_word_decimal:
    push dx
    xor dx, dx
    call print_dword_decimal
    pop dx
    ret

print_dword_decimal:
    pusha
    mov [number_work_lo], ax
    mov [number_work_hi], dx
    mov byte [decimal_buffer + 10], 0

    cmp dx, 0
    jne .convert
    cmp ax, 0
    jne .convert
    mov al, '0'
    call print_char
    jmp .done

.convert:
    mov di, decimal_buffer + 10

.next_digit:
    mov ax, [number_work_lo]
    mov dx, [number_work_hi]
    mov bx, 10
    call divide_dword_by_word
    mov [number_work_lo], ax
    mov [number_work_hi], cx
    add dl, '0'
    dec di
    mov [di], dl
    cmp cx, 0
    jne .next_digit
    cmp ax, 0
    jne .next_digit

    mov si, di
    call print_string

.done:
    popa
    ret

; IN: DX:AX dividend, BX divisor
; OUT: CX:AX quotient, DX remainder
divide_dword_by_word:
    push si
    mov si, ax
    mov ax, dx
    xor dx, dx
    div bx
    mov cx, ax
    mov ax, si
    div bx
    pop si
    ret

print_hex_byte:
    pusha
    mov ah, al
    shr al, 4
    call print_hex_nibble
    mov al, ah
    and al, 0x0F
    call print_hex_nibble
    popa
    ret

print_hex_nibble:
    and al, 0x0F
    cmp al, 9
    jbe .digit
    add al, 'A' - 10
    jmp .out
.digit:
    add al, '0'
.out:
    call print_char
    ret

print_bcd_byte:
    pusha
    mov ah, al
    shr al, 4
    and al, 0x0F
    add al, '0'
    call print_char
    mov al, ah
    and al, 0x0F
    add al, '0'
    call print_char
    popa
    ret

banner_msg db "================================", 13, 10
           db " HeatOS 0.5 + Popeye Desktop   ", 13, 10
           db "================================", 13, 10, 13, 10, 0
terminal_ready_msg db "Terminal opened from Popeye desktop.", 13, 10, 0
terminal_hint_msg db "Try: help, net, ping 127.0.0.1, desktop", 13, 10, 13, 10, 0
prompt_msg db "Heat> ", 0
unknown_msg db "Unknown command. Type 'help'.", 13, 10, 0
help_msg db "Commands:", 13, 10
         db "  help      clear/cls   about       version/ver", 13, 10
         db "  echo      banner      beep        mem", 13, 10
         db "  date      time        uptime      boot", 13, 10
         db "  status    history     repeat      apps", 13, 10
         db "  net       ping        arch", 13, 10
         db "  desktop   gui/popeye  halt/shutdown", 13, 10
         db "  reboot/restart", 13, 10
         db "Tip: press Esc while typing to clear the whole line.", 13, 10, 13, 10, 0
about_msg db "HeatOS kernel handles hardware/services; the OS layer", 13, 10
          db "provides Popeye desktop + terminal apps on top.", 13, 10
          db "Still real-mode BIOS based, but now split by role.", 13, 10, 13, 10, 0
version_msg db "HeatOS kernel v0.5", 13, 10
            db "Features: Popeye desktop, mouse hooks, net diagnostics.", 13, 10, 13, 10, 0
echo_usage_msg db "Usage: echo <text>", 13, 10, 0
date_prefix_msg db "Date: ", 0
time_prefix_msg db "Time: ", 0
uptime_prefix_msg db "Approx uptime: ", 0
mem_prefix_msg db "Conventional memory: ", 0
boot_prefix_msg db "Boot info: ", 0
status_header_msg db "System status:", 13, 10, 0
status_version_label db "  version: ", 0
version_name_msg db "HeatOS kernel v0.5", 0
status_boot_label db "  boot:    ", 0
status_mem_label db "  memory:  ", 0
status_date_label db "  date:    ", 0
status_time_label db "  time:    ", 0
status_uptime_label db "  uptime:  ", 0
status_history_label db "  history: ", 0
kb_suffix_msg db " KB", 0
entries_suffix_msg db " stored", 0
seconds_suffix_msg db " sec", 0
seconds_suffix_inline_msg db "sec", 0
unavailable_msg db "unavailable", 0
history_header_msg db "Command history:", 13, 10, 0
no_history_msg db "No history yet.", 13, 10, 0
repeat_prefix_msg db "Replaying: ", 0
no_repeat_msg db "Nothing to repeat yet.", 13, 10, 0
beep_msg db "Beep sent.", 13, 10, 0
apps_msg db "Desktop apps:", 13, 10
         db "  terminal - command shell", 13, 10
         db "  system   - live system summary", 13, 10
         db "  files    - placeholder file browser", 13, 10
         db "  notes    - roadmap and design notes", 13, 10
         db "  clock    - RTC date/time view", 13, 10
         db "  network  - PCI NIC diagnostics + ping stub", 13, 10
         db "  power    - halt or reboot", 13, 10, 13, 10, 0
returning_to_desktop_msg db "Returning to the desktop...", 13, 10, 0
halt_msg db "System halted.", 13, 10, 0
reboot_msg db "Rebooting...", 13, 10, 0
boot_summary_a db "Boot drive 0x", 0
boot_summary_b db ", kernel ", 0
boot_summary_c db " sectors loaded.", 13, 10, 0
boot_inline_a db "drive 0x", 0
boot_inline_b db ", kernel ", 0
boot_inline_c db " sectors / ", 0
boot_inline_d db " bytes", 0

desktop_top_title_msg db "Popeye Desktop Environment", 0
desktop_apps_header_msg db "Apps", 0
desktop_workspace_header_msg db "Workspace", 0
desktop_app_terminal_msg db "1  Terminal", 0
desktop_app_system_msg db "2  System", 0
desktop_app_files_msg db "3  Files", 0
desktop_app_notes_msg db "4  Notes", 0
desktop_app_clock_msg db "5  Clock", 0
desktop_app_network_msg db "6  Network", 0
desktop_app_power_msg db "7  Power", 0
desktop_footer_line1_msg db "Up/Down move  Enter launch  1-7 quick open  F1 help", 0
desktop_footer_line2_msg db "T/S/F/N/C/W/P jump  Esc power  click app names with mouse", 0
desktop_quick_header_msg db "Quick status", 0
desktop_memory_label_msg db "Memory", 0
desktop_kernel_label_msg db "Kernel sectors", 0
desktop_boot_label_msg db "Boot drive", 0
desktop_clock_label_msg db "Clock", 0
desktop_network_label_msg db "Network", 0
desktop_launch_hint_msg db "Press Enter to launch the selected app.", 0
desktop_kb_msg db "KB", 0
desktop_mouse_on_msg db "Mouse ON", 0
desktop_mouse_off_msg db "Mouse OFF", 0

preview_terminal_title_msg db "Terminal", 0
preview_terminal_line1_msg db "Open the integrated shell for commands, status, history, and uptime.", 0
preview_terminal_line2_msg db "Type 'desktop' from the shell to return here instantly.", 0
preview_terminal_line3_msg db "This is the fastest path to inspect live kernel state.", 0
preview_system_title_msg db "System", 0
preview_system_line1_msg db "View core operating-system metrics and current runtime state.", 0
preview_system_line2_msg db "The screen refreshes on demand and stays BIOS-only for now.", 0
preview_system_line3_msg db "Good place to validate boot, memory, time, and uptime signals.", 0
preview_files_title_msg db "Files", 0
preview_files_line1_msg db "This is the starting point for a future ramdisk or tiny filesystem.", 0
preview_files_line2_msg db "Right now it documents the planned layout instead of mounting disks.", 0
preview_files_line3_msg db "Next step is making these paths real and writable.", 0
preview_notes_title_msg db "Notes", 0
preview_notes_line1_msg db "Roadmap and desktop design notes live here until storage exists.", 0
preview_notes_line2_msg db "Use it as the design board for the next subsystems.", 0
preview_notes_line3_msg db "Protected mode, interrupts, storage, and actual graphics come next.", 0
preview_clock_title_msg db "Clock", 0
preview_clock_line1_msg db "Shows RTC date/time plus uptime from BIOS timer ticks.", 0
preview_clock_line2_msg db "Useful for checking that the kernel is keeping sane runtime state.", 0
preview_clock_line3_msg db "Refresh the screen whenever you want a fresh timestamp.", 0
preview_network_title_msg db "Network", 0
preview_network_line1_msg db "Scans PCI for NIC class devices and reports adapter identity.", 0
preview_network_line2_msg db "Run with QEMU -nic user,model=ne2k_pci for predictable testing.", 0
preview_network_line3_msg db "Terminal also includes net + ping diagnostics commands.", 0
preview_power_title_msg db "Power", 0
preview_power_line1_msg db "Shut the machine down or reboot directly from the desktop.", 0
preview_power_line2_msg db "This is still a BIOS reboot and CPU halt path, not ACPI power-off.", 0
preview_power_line3_msg db "Use it when you want fast control without dropping into the shell.", 0

app_system_title_msg db "HeatOS System Center", 0
app_files_title_msg db "HeatOS Files", 0
app_notes_title_msg db "HeatOS Notes", 0
app_clock_title_msg db "HeatOS Clock", 0
app_network_title_msg db "HeatOS Network", 0
app_power_title_msg db "HeatOS Power", 0
app_help_title_msg db "HeatOS Desktop Help", 0
app_footer_line1_msg db "Esc desktop  Enter desktop  T terminal", 0
app_footer_line2_msg db "Press R to refresh where it makes sense", 0

system_heading_msg db "Live system summary", 0
system_version_label_msg db "Kernel version", 0
system_boot_label_msg db "Boot drive", 0
system_kernel_label_msg db "Kernel sectors", 0
system_memory_label_msg db "Conventional memory", 0
system_date_label_msg db "RTC date", 0
system_time_label_msg db "RTC time", 0
system_uptime_label_msg db "Approx uptime", 0
system_note_line1_msg db "This remains a single-task real-mode kernel with BIOS services.", 0
system_note_line2_msg db "Kernel core and OS shell are now separated by responsibility.", 0
system_note_line3_msg db "Next milestone is storage + protected mode + graphics pipeline.", 0

files_heading_msg db "Virtual file layout", 0
files_line1_msg db "/", 0
files_line2_msg db "  apps/     built-in programs and launchers", 0
files_line3_msg db "  docs/     notes, help, and system references", 0
files_line4_msg db "  home/     future per-user space", 0
files_line5_msg db "  system/   kernel data, drivers, and config", 0
files_note_line1_msg db "No persistent filesystem is mounted yet.", 0
files_note_line2_msg db "This screen is the contract for what the first ramdisk should expose.", 0
files_note_line3_msg db "Once storage exists, the desktop can become more than a launcher.", 0

notes_heading_msg db "Roadmap", 0
notes_line1_msg db "1. Add a tiny ramdisk and path lookup.", 0
notes_line2_msg db "2. Move keyboard and timer logic off direct BIOS polling.", 0
notes_line3_msg db "3. Add a real graphics mode and window compositor.", 0
notes_line4_msg db "4. Enter protected mode and build proper memory management.", 0
notes_line5_msg db "5. Replace placeholders with actual apps and saved data.", 0
notes_footer_line1_msg db "This screen is effectively the OS design notebook for now.", 0
notes_footer_line2_msg db "The terminal command set is still the quickest way to inspect behavior.", 0

clock_heading_msg db "Clock and uptime", 0
clock_date_label_msg db "RTC date", 0
clock_time_label_msg db "RTC time", 0
clock_uptime_label_msg db "Approx uptime", 0
clock_note_line1_msg db "Press R to redraw and fetch fresh BIOS clock values.", 0
clock_note_line2_msg db "Current uptime is derived from the BIOS tick counter since boot.", 0

network_heading_msg db "Network diagnostics", 0
network_status_label_msg db "Adapter status", 0
network_slot_label_msg db "PCI slot (bus0)", 0
network_vendor_label_msg db "Vendor ID", 0
network_device_label_msg db "Device ID", 0
network_class_label_msg db "Class/Subclass", 0
network_note_line1_msg db "This is an early networking layer: detect + inspect + ping stub.", 0
network_note_line2_msg db "Use run.cmd default NIC config to expose ne2k_pci in QEMU.", 0
network_note_line3_msg db "Press R to rescan PCI network class devices.", 0
network_note_line4_msg db "Next step: bring up a minimal packet TX/RX driver.", 0

power_heading_msg db "Power control", 0
power_line1_msg db "H  Halt the CPU", 0
power_line2_msg db "R  Reboot through BIOS", 0
power_line3_msg db "T  Open terminal instead", 0
power_line4_msg db "Esc or Enter returns to the desktop", 0
power_line5_msg db "This is an early OS control panel, not an ACPI shutdown path.", 0
power_line6_msg db "It is enough to control the VM cleanly while the platform grows.", 0

help_heading_msg db "Desktop controls", 0
help_line1_msg db "Up/Down moves between apps in the launcher rail.", 0
help_line2_msg db "Enter launches the selected app.", 0
help_line3_msg db "1-7 or T/S/F/N/C/W/P open apps directly.", 0
help_line4_msg db "Esc from the home screen jumps to the power panel.", 0
help_line5_msg db "Mouse can select and click app rows when available.", 0
help_line6_msg db "T inside app screens opens the integrated terminal.", 0
help_line7_msg db "Type 'desktop', 'gui', or 'popeye' in terminal to come back.", 0
help_line8_msg db "This is a starter desktop env; graphics mode comes next.", 0

boot_menu_title_msg db "Boot mode", 13, 10, 0
boot_menu_line1_msg db "Kernel and OS shell are separate layers now.", 13, 10, 0
boot_menu_line2_msg db "D/Enter = Popeye desktop   C = console first", 13, 10, 0
boot_menu_line3_msg db "You can switch later using 'desktop' from terminal.", 13, 10, 13, 10, 0
boot_menu_prompt_msg db "Select boot mode: ", 0
boot_menu_invalid_msg db "Invalid choice. Press D, C, or Enter.", 13, 10, 0

net_console_header_msg db "Network status:", 13, 10, 0
net_console_state_label_msg db "  state: ", 0
net_console_slot_label_msg db "  slot:  ", 0
net_console_vendor_label_msg db "  id:    vendor 0x", 0
net_console_device_mid_msg db "  device 0x", 0
net_console_class_label_msg db "  class: 0x", 0
net_console_hint_msg db "  qemu: run with NIC (run.cmd does this by default).", 13, 10, 13, 10, 0
net_console_missing_msg db "  no PCI network adapter found.", 13, 10
                      db "  try QEMU with -nic user,model=ne2k_pci", 13, 10, 13, 10, 0

ping_usage_msg db "Usage: ping <host>. Try: ping 127.0.0.1", 13, 10, 0
ping_stub_msg db "Only loopback ping is implemented in this stage.", 13, 10, 0
ping_reply_msg db "Reply from 127.0.0.1: bytes=32 time<1ms ttl=255", 13, 10, 0
ping_loopback_target_msg db "127.0.0.1", 0
ping_localhost_target_msg db "localhost", 0

arch_msg db "Architecture split:", 13, 10
         db "  kernel: hardware init, timing, memory, pci, services", 13, 10
         db "  os:     popeye desktop, apps, terminal command layer", 13, 10
         db "This separation keeps low-level and user-facing logic cleaner.", 13, 10, 13, 10, 0

slash_msg db "/", 0
net_state_online_msg db "detected", 0
net_state_offline_msg db "not-detected", 0
net_none_short_msg db "n/a", 0
net_none_word_msg db "----", 0
net_none_byte_msg db "--", 0

cmd_help db "help", 0
cmd_clear db "clear", 0
cmd_cls db "cls", 0
cmd_about db "about", 0
cmd_version db "version", 0
cmd_ver db "ver", 0
cmd_echo db "echo", 0
cmd_banner db "banner", 0
cmd_beep db "beep", 0
cmd_date db "date", 0
cmd_time db "time", 0
cmd_uptime db "uptime", 0
cmd_mem db "mem", 0
cmd_boot db "boot", 0
cmd_status db "status", 0
cmd_history db "history", 0
cmd_repeat db "repeat", 0
cmd_net db "net", 0
cmd_ping db "ping", 0
cmd_arch db "arch", 0
cmd_apps db "apps", 0
cmd_desktop db "desktop", 0
cmd_gui db "gui", 0
cmd_popeye db "popeye", 0
cmd_halt db "halt", 0
cmd_shutdown db "shutdown", 0
cmd_reboot db "reboot", 0
cmd_restart db "restart", 0

boot_drive db 0
kernel_sectors db 0
desktop_selection db 0
boot_mode db 0
event_type db 0
history_next db 0
history_used db 0
boot_ticks_hi dw 0
boot_ticks_lo dw 0
args_ptr dw 0
number_work_hi dw 0
number_work_lo dw 0

mouse_available db 0
mouse_visible db 0
mouse_prev_buttons db 0
mouse_col db 0
mouse_row db 0
mouse_click_app db 0

net_present db 0
net_device_slot db 0
net_class db 0
net_subclass db 0
net_vendor_id dw 0
net_device_id dw 0

rect_char db 0
rect_attr db 0
rect_top db 0
rect_left db 0
rect_height db 0
rect_width db 0

input_buffer times HISTORY_ENTRY_SIZE db 0
command_buffer times COMMAND_MAX + 1 db 0
last_runnable_buffer times HISTORY_ENTRY_SIZE db 0
history_buffer times HISTORY_COUNT * HISTORY_ENTRY_SIZE db 0
decimal_buffer times 11 db 0
date_buffer times 12 db 0
time_buffer times 12 db 0
memory_buffer times 8 db 0
kernel_sector_buffer times 8 db 0
boot_drive_buffer times 4 db 0
uptime_buffer times 12 db 0
network_status_buffer times 16 db 0
net_slot_buffer times 8 db 0
net_vendor_buffer times 8 db 0
net_device_buffer times 8 db 0
net_class_buffer times 4 db 0
net_subclass_buffer times 4 db 0
