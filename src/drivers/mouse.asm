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
