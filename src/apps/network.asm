network_app:
.loop:
    call render_network_app
    call wait_key
    cmp al, 27
    je .done
    cmp al, 13
    je .done
    cmp al, 'r'
    je .refresh
    cmp al, 'R'
    je .refresh
    cmp al, 't'
    je .terminal
    cmp al, 'T'
    je .terminal
    jmp .loop

.refresh:
    call init_network_subsystem
    jmp .loop

.terminal:
    call terminal_session
.done:
    ret

render_network_app:
    call build_network_strings
    mov si, app_network_title_msg
    call render_standard_app_layout

    mov dh, 4
    mov dl, 8
    mov bl, 0x17
    mov si, network_heading_msg
    call write_string_at

    mov dh, 6
    mov dl, 8
    mov si, network_status_label_msg
    call write_string_at
    mov dl, 28
    mov si, network_status_buffer
    call write_string_at

    mov dh, 7
    mov dl, 8
    mov si, network_slot_label_msg
    call write_string_at
    mov dl, 28
    mov si, net_slot_buffer
    call write_string_at

    mov dh, 8
    mov dl, 8
    mov si, network_vendor_label_msg
    call write_string_at
    mov dl, 28
    mov si, net_vendor_buffer
    call write_string_at

    mov dh, 9
    mov dl, 8
    mov si, network_device_label_msg
    call write_string_at
    mov dl, 28
    mov si, net_device_buffer
    call write_string_at

    mov dh, 10
    mov dl, 8
    mov si, network_class_label_msg
    call write_string_at
    mov dl, 28
    mov si, net_class_buffer
    call write_string_at
    mov dl, 31
    mov si, slash_msg
    call write_string_at
    mov dl, 32
    mov si, net_subclass_buffer
    call write_string_at

    mov dh, 12
    mov dl, 8
    mov si, network_note_line1_msg
    call write_string_at
    mov dh, 13
    mov si, network_note_line2_msg
    call write_string_at
    mov dh, 14
    mov si, network_note_line3_msg
    call write_string_at
    mov dh, 16
    mov si, network_note_line4_msg
    call write_string_at
    ret
