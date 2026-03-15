dword_to_decimal_string:
    push bx
    push cx
    push si
    push es
    mov [number_work_lo], ax
    mov [number_work_hi], dx
    mov si, decimal_buffer + 10
    mov byte [si], 0

    cmp dx, 0
    jne .convert
    cmp ax, 0
    jne .convert
    dec si
    mov byte [si], '0'
    jmp .copy

.convert:
.next_digit:
    mov ax, [number_work_lo]
    mov dx, [number_work_hi]
    mov bx, 10
    call divide_dword_by_word
    mov [number_work_lo], ax
    mov [number_work_hi], cx
    add dl, '0'
    dec si
    mov [si], dl
    cmp cx, 0
    jne .next_digit
    cmp ax, 0
    jne .next_digit

.copy:
    push ds
    pop es
    call copy_zero_string
    pop es
    pop si
    pop cx
    pop bx
    ret

byte_to_hex_string:
    push ax
    push es
    push ds
    pop es
    mov ah, al
    shr al, 4
    call nibble_to_hex_ascii
    stosb
    mov al, ah
    and al, 0x0F
    call nibble_to_hex_ascii
    stosb
    xor al, al
    stosb
    pop es
    pop ax
    ret

word_to_hex_string:
    push ax
    push bx
    push es
    push ds
    pop es

    mov bx, ax

    mov al, bh
    shr al, 4
    call nibble_to_hex_ascii
    stosb

    mov al, bh
    and al, 0x0F
    call nibble_to_hex_ascii
    stosb

    mov al, bl
    shr al, 4
    call nibble_to_hex_ascii
    stosb

    mov al, bl
    and al, 0x0F
    call nibble_to_hex_ascii
    stosb

    xor al, al
    stosb

    pop es
    pop bx
    pop ax
    ret

nibble_to_hex_ascii:
    and al, 0x0F
    cmp al, 9
    jbe .digit
    add al, 'A' - 10
    ret
.digit:
    add al, '0'
    ret

print_word_decimal:
    push dx
    xor dx, dx
    call print_dword_decimal
    pop dx
    ret

print_dword_decimal:
    pusha
    mov [number_work_lo], ax
    mov [number_work_hi], dx
    mov byte [decimal_buffer + 10], 0

    cmp dx, 0
    jne .convert
    cmp ax, 0
    jne .convert
    mov al, '0'
    call print_char
    jmp .done

.convert:
    mov di, decimal_buffer + 10

.next_digit:
    mov ax, [number_work_lo]
    mov dx, [number_work_hi]
    mov bx, 10
    call divide_dword_by_word
    mov [number_work_lo], ax
    mov [number_work_hi], cx
    add dl, '0'
    dec di
    mov [di], dl
    cmp cx, 0
    jne .next_digit
    cmp ax, 0
    jne .next_digit

    mov si, di
    call print_string

.done:
    popa
    ret

; IN: DX:AX dividend, BX divisor
; OUT: CX:AX quotient, DX remainder
divide_dword_by_word:
    push si
    mov si, ax
    mov ax, dx
    xor dx, dx
    div bx
    mov cx, ax
    mov ax, si
    div bx
    pop si
    ret

print_hex_byte:
    pusha
    mov ah, al
    shr al, 4
    call print_hex_nibble
    mov al, ah
    and al, 0x0F
    call print_hex_nibble
    popa
    ret

print_hex_nibble:
    and al, 0x0F
    cmp al, 9
    jbe .digit
    add al, 'A' - 10
    jmp .out
.digit:
    add al, '0'
.out:
    call print_char
    ret

print_bcd_byte:
    pusha
    mov ah, al
    shr al, 4
    and al, 0x0F
    add al, '0'
    call print_char
    mov al, ah
    and al, 0x0F
    add al, '0'
    call print_char
    popa
    ret

format_mac_address:
    push ax
    push cx
    push si
    push di
    push es
    push ds
    pop es
    
    mov cx, 5
.loop:
    lodsb
    call byte_to_hex_string_inline
    mov al, ':'
    stosb
    loop .loop
    
    lodsb
    call byte_to_hex_string_inline
    xor al, al
    stosb
    
    pop es
    pop di
    pop si
    pop cx
    pop ax
    ret

byte_to_hex_string_inline:
    push ax
    mov ah, al
    shr al, 4
    call nibble_to_hex_ascii
    stosb
    mov al, ah
    and al, 0x0F
    call nibble_to_hex_ascii
    stosb
    pop ax
    ret

