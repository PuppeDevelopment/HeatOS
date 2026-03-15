kickoff_menu_app:
    mov al, [desktop_selection]
    mov [kickoff_selection], al

.loop:
    call render_kickoff_menu
    call wait_key

    cmp ah, 0x48
    je .up
    cmp ah, 0x50
    je .down

    cmp al, 27
    je .done
    cmp al, 13
    je .launch

    cmp al, '1'
    je .set_terminal
    cmp al, '2'
    je .set_system
    cmp al, '3'
    je .set_files
    cmp al, '4'
    je .set_notes
    cmp al, '5'
    je .set_clock
    cmp al, '6'
    je .set_network
    cmp al, '7'
    je .set_power

    jmp .loop

.up:
    cmp byte [kickoff_selection], APP_TERMINAL
    jne .dec
    mov byte [kickoff_selection], DESKTOP_APP_COUNT - 1
    jmp .loop

.dec:
    dec byte [kickoff_selection]
    jmp .loop

.down:
    cmp byte [kickoff_selection], DESKTOP_APP_COUNT - 1
    jne .inc
    mov byte [kickoff_selection], APP_TERMINAL
    jmp .loop

.inc:
    inc byte [kickoff_selection]
    jmp .loop

.set_terminal:
    mov byte [kickoff_selection], APP_TERMINAL
    jmp .launch

.set_system:
    mov byte [kickoff_selection], APP_SYSTEM
    jmp .launch

.set_files:
    mov byte [kickoff_selection], APP_FILES
    jmp .launch

.set_notes:
    mov byte [kickoff_selection], APP_NOTES
    jmp .launch

.set_clock:
    mov byte [kickoff_selection], APP_CLOCK
    jmp .launch

.set_network:
    mov byte [kickoff_selection], APP_NETWORK
    jmp .launch

.set_power:
    mov byte [kickoff_selection], APP_POWER

.launch:
    mov al, [kickoff_selection]
    mov [desktop_selection], al
    call open_desktop_app

.done:
    ret

render_kickoff_menu:
    call render_desktop_home

    mov dh, 4
    mov dl, 20
    mov ch, 17
    mov cl, 40
    mov bl, 0x17
    mov al, ' '
    call fill_rect

    mov dh, 4
    mov dl, 20
    mov ch, 1
    mov cl, 40
    mov bl, 0x70
    mov al, ' '
    call fill_rect

    mov dh, 4
    mov dl, 22
    mov bl, 0x70
    mov si, kickoff_title_msg
    call write_string_at

    mov al, APP_TERMINAL
    mov dh, 7
    mov si, kickoff_terminal_msg
    call render_kickoff_entry

    mov al, APP_SYSTEM
    mov dh, 9
    mov si, kickoff_system_msg
    call render_kickoff_entry

    mov al, APP_FILES
    mov dh, 11
    mov si, kickoff_files_msg
    call render_kickoff_entry

    mov al, APP_NOTES
    mov dh, 13
    mov si, kickoff_notes_msg
    call render_kickoff_entry

    mov al, APP_CLOCK
    mov dh, 15
    mov si, kickoff_clock_msg
    call render_kickoff_entry

    mov al, APP_NETWORK
    mov dh, 17
    mov si, kickoff_network_msg
    call render_kickoff_entry

    mov al, APP_POWER
    mov dh, 19
    mov si, kickoff_power_msg
    call render_kickoff_entry

    mov dh, 21
    mov dl, 22
    mov bl, 0x17
    mov si, kickoff_hint_msg
    call write_string_at
    ret

render_kickoff_entry:
    push ax
    push bx
    push cx
    push dx
    push si

    mov bl, 0x17
    cmp al, [kickoff_selection]
    jne .draw
    mov bl, 0x70

.draw:
    mov dl, 22
    mov ch, 1
    mov cl, 36
    mov al, ' '
    call fill_rect

    mov dl, 24
    call write_string_at

    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret
