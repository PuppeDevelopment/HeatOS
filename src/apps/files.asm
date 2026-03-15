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
