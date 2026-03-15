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
