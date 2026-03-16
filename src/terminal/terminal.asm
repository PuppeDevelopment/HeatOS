terminal_session:
    call hide_mouse_cursor_if_visible
    call clear_screen
    call print_banner
    mov si, terminal_ready_msg
    call print_string
    call print_boot_summary_line
    mov si, terminal_hint_msg
    call print_string

shell_loop:
    mov si, prompt_msg
    call print_string

    mov di, input_buffer
    mov cx, INPUT_MAX
    call read_line

process_input:
    call trim_input_buffer
    cmp byte [input_buffer], 0
    je shell_loop

    mov si, input_buffer
    call save_history
    call parse_command

    mov si, command_buffer
    mov di, cmd_repeat
    call string_equals
    cmp al, 1
    je dispatch_command

    mov si, input_buffer
    mov di, last_runnable_buffer
    mov cx, HISTORY_ENTRY_SIZE
    call copy_string

dispatch_command:
    mov si, command_buffer
    mov di, cmd_help
    call string_equals
    cmp al, 1
    je show_help

    mov si, command_buffer
    mov di, cmd_clear
    call string_equals
    cmp al, 1
    je do_clear

    mov si, command_buffer
    mov di, cmd_cls
    call string_equals
    cmp al, 1
    je do_clear

    mov si, command_buffer
    mov di, cmd_about
    call string_equals
    cmp al, 1
    je show_about

    mov si, command_buffer
    mov di, cmd_version
    call string_equals
    cmp al, 1
    je show_version

    mov si, command_buffer
    mov di, cmd_ver
    call string_equals
    cmp al, 1
    je show_version

    mov si, command_buffer
    mov di, cmd_echo
    call string_equals
    cmp al, 1
    je do_echo

    mov si, command_buffer
    mov di, cmd_banner
    call string_equals
    cmp al, 1
    je show_banner

    mov si, command_buffer
    mov di, cmd_beep
    call string_equals
    cmp al, 1
    je do_beep

    mov si, command_buffer
    mov di, cmd_date
    call string_equals
    cmp al, 1
    je show_date

    mov si, command_buffer
    mov di, cmd_time
    call string_equals
    cmp al, 1
    je show_time

    mov si, command_buffer
    mov di, cmd_uptime
    call string_equals
    cmp al, 1
    je show_uptime

    mov si, command_buffer
    mov di, cmd_mem
    call string_equals
    cmp al, 1
    je show_mem

    mov si, command_buffer
    mov di, cmd_boot
    call string_equals
    cmp al, 1
    je show_boot

    mov si, command_buffer
    mov di, cmd_status
    call string_equals
    cmp al, 1
    je show_status

    mov si, command_buffer
    mov di, cmd_history
    call string_equals
    cmp al, 1
    je show_history

    mov si, command_buffer
    mov di, cmd_repeat
    call string_equals
    cmp al, 1
    je do_repeat

    mov si, command_buffer
    mov di, cmd_ls
    call string_equals
    cmp al, 1
    je do_ls

    mov si, command_buffer
    mov di, cmd_cd
    call string_equals
    cmp al, 1
    je do_cd

    mov si, command_buffer
    mov di, cmd_pwd
    call string_equals
    cmp al, 1
    je do_pwd

    mov si, command_buffer
    mov di, cmd_mkdir
    call string_equals
    cmp al, 1
    je do_mkdir

    mov si, command_buffer
    mov di, cmd_net
    call string_equals
    cmp al, 1
    je show_net

    mov si, command_buffer
    mov di, cmd_ping
    call string_equals
    cmp al, 1
    je do_ping

    mov si, command_buffer
    mov di, cmd_arch
    call string_equals
    cmp al, 1
    je show_arch

    mov si, command_buffer
    mov di, cmd_apps
    call string_equals
    cmp al, 1
    je show_apps

    mov si, command_buffer
    mov di, cmd_halt
    call string_equals
    cmp al, 1
    je do_halt

    mov si, command_buffer
    mov di, cmd_shutdown
    call string_equals
    cmp al, 1
    je do_halt

    mov si, command_buffer
    mov di, cmd_reboot
    call string_equals
    cmp al, 1
    je do_reboot

    mov si, command_buffer
    mov di, cmd_restart
    call string_equals
    cmp al, 1
    je do_reboot

    mov si, unknown_msg
    call print_string
    jmp shell_loop
