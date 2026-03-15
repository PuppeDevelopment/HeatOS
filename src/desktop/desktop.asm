desktop_main:
    call set_text_mode
    call show_mouse_cursor_if_available

.desktop_loop:
    call render_desktop_home
    call wait_desktop_input

    cmp byte [event_type], 2
    je .open_selected

    cmp ah, 0x48
    je .move_up
    cmp ah, 0x50
    je .move_down
    cmp ah, 0x3B
    je .show_help
    cmp ah, 0x3C
    je .quick_terminal

    cmp al, 13
    je .open_selected
    cmp al, 27
    je .open_power

    cmp al, 'm'
    je .open_menu
    cmp al, 'M'
    je .open_menu
    cmp al, 'r'
    je .run_dialog
    cmp al, 'R'
    je .run_dialog

    cmp al, '1'
    je .shortcut_terminal
    cmp al, '2'
    je .shortcut_system
    cmp al, '3'
    je .shortcut_files
    cmp al, '4'
    je .shortcut_notes
    cmp al, '5'
    je .shortcut_clock
    cmp al, '6'
    je .shortcut_network
    cmp al, '7'
    je .shortcut_power

    cmp al, 't'
    je .shortcut_terminal
    cmp al, 'T'
    je .shortcut_terminal
    cmp al, 's'
    je .shortcut_system
    cmp al, 'S'
    je .shortcut_system
    cmp al, 'f'
    je .shortcut_files
    cmp al, 'F'
    je .shortcut_files
    cmp al, 'n'
    je .shortcut_notes
    cmp al, 'N'
    je .shortcut_notes
    cmp al, 'c'
    je .shortcut_clock
    cmp al, 'C'
    je .shortcut_clock
    cmp al, 'w'
    je .shortcut_network
    cmp al, 'W'
    je .shortcut_network
    cmp al, 'p'
    je .shortcut_power
    cmp al, 'P'
    je .shortcut_power

    jmp .desktop_loop

.move_up:
    cmp byte [desktop_selection], APP_TERMINAL
    jne .dec_selection
    mov byte [desktop_selection], DESKTOP_APP_COUNT - 1
    jmp .desktop_loop

.dec_selection:
    dec byte [desktop_selection]
    jmp .desktop_loop

.move_down:
    cmp byte [desktop_selection], DESKTOP_APP_COUNT - 1
    jne .inc_selection
    mov byte [desktop_selection], APP_TERMINAL
    jmp .desktop_loop

.inc_selection:
    inc byte [desktop_selection]
    jmp .desktop_loop

.show_help:
    call hide_mouse_cursor_if_visible
    call desktop_help_app
    call show_mouse_cursor_if_available
    jmp .desktop_loop

.quick_terminal:
    mov byte [desktop_selection], APP_TERMINAL
    call open_desktop_app
    jmp .desktop_loop

.open_menu:
    call hide_mouse_cursor_if_visible
    call kickoff_menu_app
    call show_mouse_cursor_if_available
    jmp .desktop_loop

.run_dialog:
    call hide_mouse_cursor_if_visible
    call desktop_run_dialog
    call show_mouse_cursor_if_available
    jmp .desktop_loop

.open_selected:
    call open_desktop_app
    jmp .desktop_loop

.open_power:
    mov byte [desktop_selection], APP_POWER
    call open_desktop_app
    jmp .desktop_loop

.shortcut_terminal:
    mov byte [desktop_selection], APP_TERMINAL
    call open_desktop_app
    jmp .desktop_loop

.shortcut_system:
    mov byte [desktop_selection], APP_SYSTEM
    call open_desktop_app
    jmp .desktop_loop

.shortcut_files:
    mov byte [desktop_selection], APP_FILES
    call open_desktop_app
    jmp .desktop_loop

.shortcut_notes:
    mov byte [desktop_selection], APP_NOTES
    call open_desktop_app
    jmp .desktop_loop

.shortcut_clock:
    mov byte [desktop_selection], APP_CLOCK
    call open_desktop_app
    jmp .desktop_loop

.shortcut_network:
    mov byte [desktop_selection], APP_NETWORK
    call open_desktop_app
    jmp .desktop_loop

.shortcut_power:
    mov byte [desktop_selection], APP_POWER
    call open_desktop_app
    jmp .desktop_loop

open_desktop_app:
    call hide_mouse_cursor_if_visible

    cmp byte [desktop_selection], APP_TERMINAL
    je .run_terminal
    cmp byte [desktop_selection], APP_SYSTEM
    je .run_system
    cmp byte [desktop_selection], APP_FILES
    je .run_files
    cmp byte [desktop_selection], APP_NOTES
    je .run_notes
    cmp byte [desktop_selection], APP_CLOCK
    je .run_clock
    cmp byte [desktop_selection], APP_NETWORK
    je .run_network
    jmp .run_power

.run_terminal:
    call terminal_session
    jmp .done

.run_system:
    call system_app
    jmp .done

.run_files:
    call files_app
    jmp .done

.run_notes:
    call notes_app
    jmp .done

.run_clock:
    call clock_app
    jmp .done

.run_network:
    call network_app
    jmp .done

.run_power:
    call power_app

.done:
    call show_mouse_cursor_if_available
    ret

desktop_help_app:
.loop:
    call render_help_app
    call wait_key
    cmp al, 27
    je .done
    cmp al, 13
    je .done
    cmp al, 't'
    je .terminal
    cmp al, 'T'
    je .terminal
    jmp .loop

.terminal:
    call terminal_session
.done:
    ret
