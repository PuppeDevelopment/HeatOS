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
