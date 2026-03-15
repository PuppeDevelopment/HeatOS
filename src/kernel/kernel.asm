[bits 16]
[org 0x0000]

INPUT_MAX equ 63
COMMAND_MAX equ 15
HISTORY_COUNT equ 8
HISTORY_ENTRY_SIZE equ INPUT_MAX + 1
DAY_TICKS_HI equ 0x0018
DAY_TICKS_LO equ 0x00B0

start:
    cli
    cld
    xor ax, ax
    mov ss, ax
    mov sp, 0x9A00
    sti

    mov ax, cs
    mov ds, ax
    mov es, ax

    mov [boot_drive], dl
    mov [kernel_sectors], cl

    call capture_boot_ticks
    call clear_screen
    call print_banner
    mov si, boot_ready_msg
    call print_string
    call print_boot_summary_line
    mov si, help_hint_msg
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

show_help:
    mov si, help_msg
    call print_string
    jmp shell_loop

do_clear:
    call clear_screen
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
    pop ax
    ret

save_history:
    push ax
    push bx
    push cx
    push di
    push si

    xor bx, bx
    mov bl, [history_next]
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1
    shl bx, 1

    mov di, history_buffer
    add di, bx
    mov cx, HISTORY_ENTRY_SIZE
    call copy_string

    inc byte [history_next]
    cmp byte [history_next], HISTORY_COUNT
    jb .counted
    mov byte [history_next], 0

.counted:
    cmp byte [history_used], HISTORY_COUNT
    jae .done
    inc byte [history_used]

.done:
    pop si
    pop di
    pop cx
    pop bx
    pop ax
    ret

clear_screen:
    push ax
    mov ax, 0x0003
    int 0x10
    pop ax
    ret

print_banner:
    mov si, banner_msg
    call print_string
    ret

print_char:
    push ax
    push bx
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0F
    int 0x10
    pop bx
    pop ax
    ret

print_string:
    pusha
.next_char:
    lodsb
    test al, al
    jz .done
    call print_char
    jmp .next_char
.done:
    popa
    ret

print_newline:
    pusha
    mov al, 13
    call print_char
    mov al, 10
    call print_char
    popa
    ret

read_line:
    pusha
    xor bx, bx

.read_key:
    xor ah, ah
    int 0x16

    cmp al, 13
    je .enter
    cmp al, 27
    je .clear_current
    cmp al, 8
    je .backspace
    cmp al, 32
    jb .read_key
    cmp al, 126
    ja .read_key
    cmp bx, cx
    jae .read_key

    mov [di + bx], al
    inc bx
    call print_char
    jmp .read_key

.backspace:
    cmp bx, 0
    je .read_key
    dec bx
    mov ah, 0x0E
    mov al, 8
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 8
    int 0x10
    jmp .read_key

.clear_current:
    cmp bx, 0
    je .read_key
.clear_loop:
    dec bx
    mov ah, 0x0E
    mov al, 8
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 8
    int 0x10
    cmp bx, 0
    jne .clear_loop
    jmp .read_key

.enter:
    mov byte [di + bx], 0
    call print_newline
    popa
    ret

print_boot_summary_line:
    mov si, boot_summary_a
    call print_string
    mov al, [boot_drive]
    call print_hex_byte
    mov si, boot_summary_b
    call print_string
    xor ax, ax
    mov al, [kernel_sectors]
    call print_word_decimal
    mov si, boot_summary_c
    call print_string
    ret

print_boot_info_inline:
    push ax
    push bx
    push dx
    push si

    mov si, boot_inline_a
    call print_string
    mov al, [boot_drive]
    call print_hex_byte

    mov si, boot_inline_b
    call print_string
    xor ax, ax
    mov al, [kernel_sectors]
    call print_word_decimal

    mov si, boot_inline_c
    call print_string
    xor ax, ax
    mov al, [kernel_sectors]
    mov bx, 512
    mul bx
    call print_dword_decimal

    mov si, boot_inline_d
    call print_string

    pop si
    pop dx
    pop bx
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

banner_msg db "==============================", 13, 10
           db " HeatOS 0.3  real-mode shell  ", 13, 10
           db "==============================", 13, 10, 13, 10, 0
boot_ready_msg db "Kernel ready with upgraded shell.", 13, 10, 0
help_hint_msg db "Try: help, status, time, mem, history", 13, 10, 13, 10, 0
prompt_msg db "Heat> ", 0
unknown_msg db "Unknown command. Type 'help'.", 13, 10, 0
help_msg db "Commands:", 13, 10
         db "  help      clear/cls   about       version/ver", 13, 10
         db "  echo      banner      beep        mem", 13, 10
         db "  date      time        uptime      boot", 13, 10
         db "  status    history     repeat      halt/shutdown", 13, 10
         db "  reboot/restart", 13, 10
         db "Tip: press Esc while typing to clear the whole line.", 13, 10, 13, 10, 0
about_msg db "HeatOS is a 16-bit educational OS with a BIOS-driven shell.", 13, 10
          db "This build adds command history, RTC date/time, uptime,", 13, 10
          db "memory and boot inspection, echo, banner, and wrappers", 13, 10
          db "for easier launching on Windows.", 13, 10, 13, 10, 0
version_msg db "HeatOS kernel v0.3", 13, 10
            db "Features: history, repeat, rtc, mem, boot, status, echo.", 13, 10, 13, 10, 0
echo_usage_msg db "Usage: echo <text>", 13, 10, 0
date_prefix_msg db "Date: ", 0
time_prefix_msg db "Time: ", 0
uptime_prefix_msg db "Approx uptime: ", 0
mem_prefix_msg db "Conventional memory: ", 0
boot_prefix_msg db "Boot info: ", 0
status_header_msg db "System status:", 13, 10, 0
status_version_label db "  version: ", 0
version_name_msg db "HeatOS kernel v0.3", 0
status_boot_label db "  boot:    ", 0
status_mem_label db "  memory:  ", 0
status_date_label db "  date:    ", 0
status_time_label db "  time:    ", 0
status_uptime_label db "  uptime:  ", 0
status_history_label db "  history: ", 0
kb_suffix_msg db " KB", 0
entries_suffix_msg db " stored", 0
seconds_suffix_msg db " sec", 0
unavailable_msg db "unavailable", 0
history_header_msg db "Command history:", 13, 10, 0
no_history_msg db "No history yet.", 13, 10, 0
repeat_prefix_msg db "Replaying: ", 0
no_repeat_msg db "Nothing to repeat yet.", 13, 10, 0
beep_msg db "Beep sent.", 13, 10, 0
halt_msg db "System halted.", 13, 10, 0
reboot_msg db "Rebooting...", 13, 10, 0
boot_summary_a db "Boot drive 0x", 0
boot_summary_b db ", kernel ", 0
boot_summary_c db " sectors loaded.", 13, 10, 0
boot_inline_a db "drive 0x", 0
boot_inline_b db ", kernel ", 0
boot_inline_c db " sectors / ", 0
boot_inline_d db " bytes", 0

cmd_help db "help", 0
cmd_clear db "clear", 0
cmd_cls db "cls", 0
cmd_about db "about", 0
cmd_version db "version", 0
cmd_ver db "ver", 0
cmd_echo db "echo", 0
cmd_banner db "banner", 0
cmd_beep db "beep", 0
cmd_date db "date", 0
cmd_time db "time", 0
cmd_uptime db "uptime", 0
cmd_mem db "mem", 0
cmd_boot db "boot", 0
cmd_status db "status", 0
cmd_history db "history", 0
cmd_repeat db "repeat", 0
cmd_halt db "halt", 0
cmd_shutdown db "shutdown", 0
cmd_reboot db "reboot", 0
cmd_restart db "restart", 0

boot_drive db 0
kernel_sectors db 0
history_next db 0
history_used db 0
boot_ticks_hi dw 0
boot_ticks_lo dw 0
args_ptr dw 0
number_work_hi dw 0
number_work_lo dw 0

input_buffer times HISTORY_ENTRY_SIZE db 0
command_buffer times COMMAND_MAX + 1 db 0
last_runnable_buffer times HISTORY_ENTRY_SIZE db 0
history_buffer times HISTORY_COUNT * HISTORY_ENTRY_SIZE db 0
decimal_buffer times 11 db 0
