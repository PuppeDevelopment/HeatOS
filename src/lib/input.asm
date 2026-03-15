wait_key:
    xor ah, ah
    int 0x16
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
