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
