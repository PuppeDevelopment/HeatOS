desktop_run_dialog:
    call clear_screen
    mov si, run_dialog_title_msg
    call print_string
    mov si, run_dialog_help_msg
    call print_string
    mov si, run_dialog_prompt_msg
    call print_string

    mov di, input_buffer
    mov cx, INPUT_MAX
    call read_line
    call trim_input_buffer

    cmp byte [input_buffer], 0
    je .done

    call parse_command

    mov si, command_buffer
    mov di, run_cmd_terminal
    call string_equals
    cmp al, 1
    je .terminal

    mov si, command_buffer
    mov di, run_cmd_console
    call string_equals
    cmp al, 1
    je .terminal

    mov si, command_buffer
    mov di, run_cmd_system
    call string_equals
    cmp al, 1
    je .system

    mov si, command_buffer
    mov di, run_cmd_files
    call string_equals
    cmp al, 1
    je .files

    mov si, command_buffer
    mov di, run_cmd_notes
    call string_equals
    cmp al, 1
    je .notes

    mov si, command_buffer
    mov di, run_cmd_clock
    call string_equals
    cmp al, 1
    je .clock

    mov si, command_buffer
    mov di, run_cmd_network
    call string_equals
    cmp al, 1
    je .network

    mov si, command_buffer
    mov di, run_cmd_power
    call string_equals
    cmp al, 1
    je .power

    mov si, run_dialog_unknown_msg
    call print_string
    call wait_key
    ret

.terminal:
    mov byte [desktop_selection], APP_TERMINAL
    call open_desktop_app
    ret

.system:
    mov byte [desktop_selection], APP_SYSTEM
    call open_desktop_app
    ret

.files:
    mov byte [desktop_selection], APP_FILES
    call open_desktop_app
    ret

.notes:
    mov byte [desktop_selection], APP_NOTES
    call open_desktop_app
    ret

.clock:
    mov byte [desktop_selection], APP_CLOCK
    call open_desktop_app
    ret

.network:
    mov byte [desktop_selection], APP_NETWORK
    call open_desktop_app
    ret

.power:
    mov byte [desktop_selection], APP_POWER
    call open_desktop_app
    ret

.done:
    ret
