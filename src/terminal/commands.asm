show_help:
    mov si, help_msg
    call print_string
    jmp shell_loop

do_clear:
    call clear_screen
    jmp shell_loop

do_ls:
    call fs_ls
    jc .err
    jmp shell_loop
.err:
    mov si, unknown_msg
    call print_string
    jmp shell_loop

do_cd:
    call fs_cd
    jc .err
    jmp shell_loop
.err:
    mov si, unknown_msg
    call print_string
    jmp shell_loop

do_pwd:
    call fs_pwd
    jmp shell_loop

do_mkdir:
    mov si, [args_ptr]
    call skip_spaces
    cmp byte [si], 0
    je .err
    call fs_token_from_si
    xor bx, bx
    mov bl, [fs_cwd]
    mov si, fs_token_buf
    call fs_mkdir_child
    jc .err
    jmp shell_loop
.err:
    mov si, unknown_msg
    call print_string
    jmp shell_loop

show_about:
    mov si, about_msg
    call print_string
    jmp shell_loop

show_version:
    mov si, version_msg
    call print_string
    jmp shell_loop

do_echo:
    mov si, [args_ptr]
    cmp byte [si], 0
    jne .print
    mov si, echo_usage_msg
    call print_string
    jmp shell_loop
.print:
    call print_string
    call print_newline
    jmp shell_loop

show_banner:
    call print_banner
    jmp shell_loop

do_beep:
    mov al, 7
    call print_char
    mov si, beep_msg
    call print_string
    jmp shell_loop

show_date:
    mov si, date_prefix_msg
    call print_string
    call print_date_inline
    jc .error
    call print_newline
    jmp shell_loop
.error:
    mov si, unavailable_msg
    call print_string
    call print_newline
    jmp shell_loop

show_time:
    mov si, time_prefix_msg
    call print_string
    call print_time_inline
    jc .error
    call print_newline
    jmp shell_loop
.error:
    mov si, unavailable_msg
    call print_string
    call print_newline
    jmp shell_loop

show_uptime:
    mov si, uptime_prefix_msg
    call print_string
    call print_uptime_inline
    jc .error
    call print_newline
    jmp shell_loop
.error:
    mov si, unavailable_msg
    call print_string
    call print_newline
    jmp shell_loop

show_mem:
    mov si, mem_prefix_msg
    call print_string
    int 0x12
    call print_word_decimal
    mov si, kb_suffix_msg
    call print_string
    call print_newline
    jmp shell_loop

show_boot:
    mov si, boot_prefix_msg
    call print_string
    call print_boot_info_inline
    call print_newline
    jmp shell_loop

show_status:
    mov si, status_header_msg
    call print_string

    mov si, status_version_label
    call print_string
    mov si, version_name_msg
    call print_string
    call print_newline

    mov si, status_boot_label
    call print_string
    call print_boot_info_inline
    call print_newline

    mov si, status_mem_label
    call print_string
    int 0x12
    call print_word_decimal
    mov si, kb_suffix_msg
    call print_string
    call print_newline

    mov si, status_date_label
    call print_string
    call print_date_inline
    jc .date_error
    call print_newline
    jmp .time
.date_error:
    mov si, unavailable_msg
    call print_string
    call print_newline

.time:
    mov si, status_time_label
    call print_string
    call print_time_inline
    jc .time_error
    call print_newline
    jmp .uptime
.time_error:
    mov si, unavailable_msg
    call print_string
    call print_newline

.uptime:
    mov si, status_uptime_label
    call print_string
    call print_uptime_inline
    jc .uptime_error
    call print_newline
    jmp .history
.uptime_error:
    mov si, unavailable_msg
    call print_string
    call print_newline

.history:
    mov si, status_history_label
    call print_string
    xor ax, ax
    mov al, [history_used]
    call print_word_decimal
    mov si, entries_suffix_msg
    call print_string
    call print_newline
    call print_newline
    jmp shell_loop

show_history:
    cmp byte [history_used], 0
    jne .have_entries
    mov si, no_history_msg
    call print_string
    jmp shell_loop

.have_entries:
    mov si, history_header_msg
    call print_string

    xor cx, cx
    mov cl, [history_used]
    mov bl, [history_next]
    cmp byte [history_used], HISTORY_COUNT
    je .loop_ready
    xor bl, bl

.loop_ready:
    xor dx, dx
    mov dl, 1

.print_entry:
    mov ax, dx
    call print_word_decimal
    mov al, '.'
    call print_char
    mov al, ' '
    call print_char

    xor bh, bh
    mov ax, bx
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    mov si, history_buffer
    add si, ax
    call print_string
    call print_newline

    inc bl
    cmp bl, HISTORY_COUNT
    jb .wrapped
    xor bl, bl
.wrapped:
    inc dx
    loop .print_entry

    call print_newline
    jmp shell_loop

do_repeat:
    cmp byte [last_runnable_buffer], 0
    jne .have_command
    mov si, no_repeat_msg
    call print_string
    jmp shell_loop

.have_command:
    mov si, repeat_prefix_msg
    call print_string
    mov si, last_runnable_buffer
    call print_string
    call print_newline

    mov si, last_runnable_buffer
    mov di, input_buffer
    mov cx, HISTORY_ENTRY_SIZE
    call copy_string
    call parse_command
    jmp dispatch_command

show_apps:
    mov si, apps_msg
    call print_string
    jmp shell_loop

show_net:
    call build_network_strings
    mov si, net_console_header_msg
    call print_string
    mov si, net_console_state_label_msg
    call print_string
    mov si, network_status_buffer
    call print_string
    call print_newline

    cmp byte [net_present], 1
    jne .missing

    mov si, net_console_slot_label_msg
    call print_string
    mov si, net_slot_buffer
    call print_string
    call print_newline

    mov si, net_console_vendor_label_msg
    call print_string
    mov si, net_vendor_buffer
    call print_string
    mov si, net_console_device_mid_msg
    call print_string
    mov si, net_device_buffer
    call print_string
    call print_newline

    mov si, net_console_class_label_msg
    call print_string
    mov si, net_class_buffer
    call print_string
    mov si, slash_msg
    call print_string
    mov si, net_subclass_buffer
    call print_string
    call print_newline

    mov si, net_console_hint_msg
    call print_string
    jmp shell_loop

.missing:
    mov si, net_console_missing_msg
    call print_string
    jmp shell_loop

do_ping:
    mov si, [args_ptr]
    cmp byte [si], 0
    jne .has_target
    mov si, ping_usage_msg
    call print_string
    jmp shell_loop

.has_target:
    mov di, ping_loopback_target_msg
    call string_equals
    cmp al, 1
    je .loopback

    mov si, [args_ptr]
    mov di, ping_localhost_target_msg
    call string_equals
    cmp al, 1
    je .loopback

    mov si, ping_stub_msg
    call print_string
    jmp shell_loop

.loopback:
    mov si, ping_reply_msg
    call print_string
    jmp shell_loop

show_arch:
    mov si, arch_msg
    call print_string
    jmp shell_loop

leave_to_desktop:
    mov si, returning_to_desktop_msg
    call print_string
    ret

do_halt:
    mov si, halt_msg
    call print_string
.halt_forever:
    cli
    hlt
    jmp .halt_forever

do_reboot:
    mov si, reboot_msg
    call print_string
    int 0x19
    jmp do_halt
