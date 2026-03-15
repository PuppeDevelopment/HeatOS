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

clear_screen:
    jmp set_text_mode
