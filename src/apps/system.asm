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
