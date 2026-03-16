[bits 16]
[org 0x0000]

INPUT_MAX equ 63
COMMAND_MAX equ 15
HISTORY_COUNT equ 8
HISTORY_ENTRY_SIZE equ INPUT_MAX + 1
DAY_TICKS_HI equ 0x0018
DAY_TICKS_LO equ 0x00B0
VIDEO_SEGMENT equ 0xB800

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
    call init_mouse
    call init_network_subsystem
    call ramdisk_init
    ; Boot directly into the terminal shell.
    call terminal_session

.halt:
    cli
    hlt
    jmp .halt


; =============================================================================
; HeatOS - Modular Include Chain
; Each subsystem lives in its own source file for clarity and separation.
; =============================================================================

; --- Hardware Drivers ---------------------------------------------------------
%include "src/drivers/mouse.asm"       ; Mouse: detect, show/hide cursor
%include "src/drivers/pci_net.asm"     ; PCI network: NIC scan, status strings

; --- Low-level Libraries ------------------------------------------------------
%include "src/lib/video.asm"           ; VGA text-mode: fill_rect, write_string_at
%include "src/lib/string.asm"          ; String ops: copy, compare, parse
%include "src/lib/input.asm"           ; Keyboard: wait_key, read_line, history
%include "src/lib/print.asm"           ; BIOS teletype: print_char, print_string
%include "src/lib/math.asm"            ; Numbers: decimal/hex conversions
%include "src/lib/time.asm"            ; RTC: date, time, uptime, tick capture
%include "src/lib/ramdisk.asm"         ; In-memory filesystem (ramdisk)

; --- Terminal Shell -----------------------------------------------------------
%include "src/terminal/terminal.asm"   ; Heat> shell session + dispatch table
%include "src/terminal/commands.asm"   ; All built-in command implementations

; --- Static Data (must be last - sits after all code in the binary) ----------
%include "src/data/strings.asm"        ; Zero-terminated string literals
%include "src/data/variables.asm"      ; Mutable variables and scratch buffers