capture_boot_ticks:
    pusha
    mov ah, 0x00
    int 0x1A
    jc .done
    mov [boot_ticks_hi], cx
    mov [boot_ticks_lo], dx
.done:
    popa
    ret

build_system_stats:
    call build_date_string
    call build_time_string
    call build_memory_string
    call build_kernel_sectors_string
    call build_boot_drive_string
    call build_uptime_string
    call build_network_strings
    ret

build_date_string:
    push ax
    push cx
    push dx
    push si
    push di
    push es
    push ds
    pop es

    mov ah, 0x04
    int 0x1A
    jc .fallback

    mov di, date_buffer
    mov al, ch
    call store_bcd_ascii
    mov al, cl
    call store_bcd_ascii
    mov al, '-'
    stosb
    mov al, dh
    call store_bcd_ascii
    mov al, '-'
    stosb
    mov al, dl
    call store_bcd_ascii
    xor al, al
    stosb
    jmp .done

.fallback:
    mov si, unavailable_msg
    mov di, date_buffer
    call copy_zero_string

.done:
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop ax
    ret

build_time_string:
    push ax
    push cx
    push dx
    push si
    push di
    push es
    push ds
    pop es

    mov ah, 0x02
    int 0x1A
    jc .fallback

    mov di, time_buffer
    mov al, ch
    call store_bcd_ascii
    mov al, ':'
    stosb
    mov al, cl
    call store_bcd_ascii
    mov al, ':'
    stosb
    mov al, dh
    call store_bcd_ascii
    xor al, al
    stosb
    jmp .done

.fallback:
    mov si, unavailable_msg
    mov di, time_buffer
    call copy_zero_string

.done:
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop ax
    ret

build_memory_string:
    push ax
    push di
    int 0x12
    mov di, memory_buffer
    call word_to_decimal_string
    pop di
    pop ax
    ret

build_kernel_sectors_string:
    push ax
    push di
    xor ax, ax
    mov al, [kernel_sectors]
    mov di, kernel_sector_buffer
    call word_to_decimal_string
    pop di
    pop ax
    ret

build_boot_drive_string:
    push ax
    push di
    mov al, [boot_drive]
    mov di, boot_drive_buffer
    call byte_to_hex_string
    pop di
    pop ax
    ret

build_uptime_string:
    push ax
    push bx
    push cx
    push dx
    push si
    push di

    call get_elapsed_ticks
    jc .fallback
    mov bx, 18
    call divide_dword_by_word
    mov dx, cx
    mov di, uptime_buffer
    call dword_to_decimal_string
    jmp .done

.fallback:
    mov si, unavailable_msg
    mov di, uptime_buffer
    push es
    push ds
    pop es
    call copy_zero_string
    pop es

.done:
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

store_bcd_ascii:
    push ax
    mov ah, al
    shr al, 4
    and al, 0x0F
    add al, '0'
    stosb
    mov al, ah
    and al, 0x0F
    add al, '0'
    stosb
    pop ax
    ret

print_date_inline:
    push ax
    push cx
    push dx

    mov ah, 0x04
    int 0x1A
    jc .error
    mov al, ch
    call print_bcd_byte
    mov al, cl
    call print_bcd_byte
    mov al, '-'
    call print_char
    mov al, dh
    call print_bcd_byte
    mov al, '-'
    call print_char
    mov al, dl
    call print_bcd_byte
    clc
    jmp .done

.error:
    stc

.done:
    pop dx
    pop cx
    pop ax
    ret

print_time_inline:
    push ax
    push cx
    push dx

    mov ah, 0x02
    int 0x1A
    jc .error
    mov al, ch
    call print_bcd_byte
    mov al, ':'
    call print_char
    mov al, cl
    call print_bcd_byte
    mov al, ':'
    call print_char
    mov al, dh
    call print_bcd_byte
    clc
    jmp .done

.error:
    stc

.done:
    pop dx
    pop cx
    pop ax
    ret

print_uptime_inline:
    push ax
    push bx
    push cx
    push dx
    push si

    call get_elapsed_ticks
    jc .error
    mov bx, 18
    call divide_dword_by_word
    mov dx, cx
    call print_dword_decimal
    mov si, seconds_suffix_msg
    call print_string
    clc
    jmp .done

.error:
    stc

.done:
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

get_elapsed_ticks:
    push bx
    push cx

    mov ah, 0x00
    int 0x1A
    jc .error

    mov ax, dx
    mov dx, cx
    sub ax, [boot_ticks_lo]
    sbb dx, [boot_ticks_hi]
    jnc .done
    add ax, DAY_TICKS_LO
    adc dx, DAY_TICKS_HI

.done:
    clc
    pop cx
    pop bx
    ret

.error:
    stc
    pop cx
    pop bx
    ret
