copy_zero_string:
    push ax
.loop:
    lodsb
    stosb
    test al, al
    jnz .loop
    pop ax
    ret

word_to_decimal_string:
    xor dx, dx
    jmp dword_to_decimal_string

skip_spaces:
.skip:
    cmp byte [si], ' '
    jne .done
    inc si
    jmp .skip
.done:
    ret

to_lower:
    cmp al, 'A'
    jb .done
    cmp al, 'Z'
    ja .done
    add al, 32
.done:
    ret

trim_input_buffer:
    push ax
    push bx
    push si
    push di
    push es
    push ds
    pop es

    mov si, input_buffer
    call skip_spaces
    mov di, input_buffer

.copy:
    lodsb
    stosb
    test al, al
    jnz .copy

    mov bx, di
    dec bx
    cmp bx, input_buffer
    je .done
    dec bx

.trim_tail:
    cmp byte [bx], ' '
    jne .done
    mov byte [bx], 0
    cmp bx, input_buffer
    je .done
    dec bx
    jmp .trim_tail

.done:
    pop es
    pop di
    pop si
    pop bx
    pop ax
    ret

parse_command:
    push ax
    push cx
    push si
    push di

    mov si, input_buffer
    call skip_spaces
    mov di, command_buffer
    mov cx, COMMAND_MAX

.copy_char:
    mov al, [si]
    cmp al, 0
    je .finish
    cmp al, ' '
    je .finish
    cmp cx, 0
    je .skip_store
    call to_lower
    mov [di], al
    inc di
    dec cx
.skip_store:
    inc si
    jmp .copy_char

.finish:
    mov byte [di], 0
    call skip_spaces
    mov [args_ptr], si

    pop di
    pop si
    pop cx
    pop ax
    ret

string_equals:
    push si
    push di
.compare:
    mov al, [si]
    mov ah, [di]
    cmp al, ah
    jne .not_equal
    cmp al, 0
    je .equal
    inc si
    inc di
    jmp .compare
.equal:
    mov al, 1
    jmp .finish
.not_equal:
    xor al, al
.finish:
    pop di
    pop si
    ret

copy_string:
    push ax
    push es
    push ds
    pop es
.loop:
    cmp cx, 1
    jne .room
    mov byte [di], 0
    jmp .done
.room:
    lodsb
    stosb
    dec cx
    test al, al
    jnz .loop
.done:
    pop es
    pop ax
    ret
