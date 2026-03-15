render_desktop_home:
    call build_desktop_stats

    mov dh, 0
    mov dl, 0
    mov ch, 25
    mov cl, 80
    mov bl, 0x1B
    mov al, ' '
    call fill_rect

    mov dh, 0
    mov dl, 0
    mov ch, 1
    mov cl, 80
    mov bl, 0x30
    mov al, ' '
    call fill_rect

    mov dh, 2
    mov dl, 2
    mov ch, 20
    mov cl, 20
    mov bl, 0x13
    mov al, ' '
    call fill_rect

    mov dh, 2
    mov dl, 24
    mov ch, 20
    mov cl, 54
    mov bl, 0x1E
    mov al, ' '
    call fill_rect

    mov dh, 22
    mov dl, 0
    mov ch, 3
    mov cl, 80
    mov bl, 0x70
    mov al, ' '
    call fill_rect

    mov dh, 0
    mov dl, 2
    mov bl, 0x30
    mov si, desktop_top_title_msg
    call write_string_at

    mov dh, 0
    mov dl, 46
    mov bl, 0x30
    mov si, date_buffer
    call write_string_at

    mov dh, 0
    mov dl, 60
    mov bl, 0x30
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

    mov dh, 22
    mov dl, 2
    mov bl, 0x70
    mov si, desktop_footer_line1_msg
    call write_string_at

    mov dh, 23
    mov dl, 2
    mov bl, 0x70
    mov si, desktop_footer_line2_msg
    call write_string_at

    mov dh, 24
    mov dl, 2
    mov bl, 0x70
    mov si, desktop_footer_line3_msg
    call write_string_at

    mov dh, 24
    mov dl, 66
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
    mov bl, 0x1B
    mov al, ' '
    call fill_rect

    mov dh, 0
    mov dl, 0
    mov ch, 1
    mov cl, 80
    mov bl, 0x30
    mov al, ' '
    call fill_rect

    mov dh, 2
    mov dl, 6
    mov ch, 1
    mov cl, 68
    mov bl, 0x70
    mov al, ' '
    call fill_rect

    mov dh, 3
    mov dl, 6
    mov ch, 20
    mov cl, 68
    mov bl, 0x17
    mov al, ' '
    call fill_rect

    mov dh, 22
    mov dl, 0
    mov ch, 3
    mov cl, 80
    mov bl, 0x70
    mov al, ' '
    call fill_rect

    pop si
    mov dh, 0
    mov dl, 2
    mov bl, 0x30
    call write_string_at

    mov dh, 22
    mov dl, 2
    mov bl, 0x70
    mov si, app_footer_line1_msg
    call write_string_at

    mov dh, 23
    mov dl, 2
    mov bl, 0x70
    mov si, app_footer_line2_msg
    call write_string_at

    mov dh, 24
    mov dl, 2
    mov bl, 0x70
    mov si, app_footer_line3_msg
    call write_string_at
    ret
