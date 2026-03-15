init_network_subsystem:
    call detect_network_adapter
    call init_ne2k
    call build_network_strings
    ret

detect_network_adapter:
    push ax
    push bx
    push cx
    push dx
    push eax
    push ecx
    push edx

    mov byte [net_present], 0
    mov byte [net_device_slot], 0
    mov word [net_vendor_id], 0
    mov word [net_device_id], 0
    mov byte [net_class], 0
    mov byte [net_subclass], 0

    xor bl, bl

.scan_device:
    cmp bl, 32
    jae .done

    mov cl, 0x00
    call pci_read_dword_bus0
    mov edx, eax
    cmp dx, 0xFFFF
    je .next_device

    mov cl, 0x08
    call pci_read_dword_bus0
    mov edx, eax
    shr edx, 24
    cmp dl, 0x02
    jne .next_device

    mov byte [net_present], 1
    mov [net_device_slot], bl

    mov cl, 0x00
    call pci_read_dword_bus0
    mov edx, eax
    mov [net_vendor_id], dx
    shr edx, 16
    mov [net_device_id], dx

    mov cl, 0x08
    call pci_read_dword_bus0
    mov edx, eax
    shr edx, 16
    mov [net_subclass], dl
    mov [net_class], dh

    mov cl, 0x10
    call pci_read_dword_bus0
    test al, 1
    jz .done ; Only support IO-based for now (NE2000)
    and ax, 0xFFFC
    mov [net_io_base], ax
    jmp .done

.next_device:
    inc bl
    jmp .scan_device

.done:
    pop edx
    pop ecx
    pop eax
    pop dx
    pop cx
    pop bx
    pop ax
    ret

pci_read_dword_bus0:
    push bx
    push cx
    push dx
    push edx

    movzx eax, bl
    shl eax, 11
    movzx edx, cl
    and edx, 0xFC
    or eax, edx
    or eax, 0x80000000

    mov dx, 0xCF8
    out dx, eax
    mov dx, 0xCFC
    in eax, dx

    pop edx
    pop dx
    pop cx
    pop bx
    ret

build_network_strings:
    push ax
    push di
    push si
    push es

    cmp byte [net_present], 1
    jne .offline

    push ds
    pop es

    mov si, net_state_online_msg
    mov di, network_status_buffer
    call copy_zero_string

    xor ax, ax
    mov al, [net_device_slot]
    mov di, net_slot_buffer
    call word_to_decimal_string

    mov ax, [net_vendor_id]
    mov di, net_vendor_buffer
    call word_to_hex_string

    mov ax, [net_device_id]
    mov di, net_device_buffer
    call word_to_hex_string

    mov al, [net_class]
    mov di, net_class_buffer
    call byte_to_hex_string

    mov al, [net_subclass]
    mov di, net_subclass_buffer
    call byte_to_hex_string

    mov si, net_mac_address
    mov di, net_mac_buffer
    call format_mac_address
    jmp .done

.offline:
    push ds
    pop es

    mov si, net_state_offline_msg
    mov di, network_status_buffer
    call copy_zero_string

    mov si, net_none_short_msg
    mov di, net_slot_buffer
    call copy_zero_string

    mov si, net_none_word_msg
    mov di, net_vendor_buffer
    call copy_zero_string

    mov si, net_none_word_msg
    mov di, net_device_buffer
    call copy_zero_string

    mov si, net_none_byte_msg
    mov di, net_class_buffer
    call copy_zero_string

    mov si, net_none_byte_msg
    mov di, net_subclass_buffer
    call copy_zero_string

    mov si, net_none_word_msg
    mov di, net_mac_buffer
    call copy_zero_string

.done:
    pop es
    pop si
    pop di
    pop ax
    ret
