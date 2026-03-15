[bits 16]
[org 0x7C00]

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 16
%endif

KERNEL_SEGMENT equ 0x1000
KERNEL_START_SECTOR equ 2
READ_RETRIES equ 3

start:
jmp 0x0
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9C00
    sti

    mov [boot_drive], dl

    mov si, msg_boot
    call print_string

    mov ax, KERNEL_SEGMENT
    mov es, ax
    mov word [kernel_offset], 0
    mov byte [next_sector], KERNEL_START_SECTOR
    mov byte [sectors_left], KERNEL_SECTORS

.read_next_sector:
    cmp byte [sectors_left], 0
    je .read_done

    mov di, READ_RETRIES
.read_retry:
    mov ah, 0x02
    mov al, 0x01
    mov ch, 0x00
    mov cl, [next_sector]
    mov dh, 0x00
    mov dl, [boot_drive]
    mov bx, [kernel_offset]
    int 0x13
    jnc .read_ok

    mov [disk_error_code], ah
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13

    dec di
    jnz .read_retry
    jmp disk_error

.read_ok:
    add word [kernel_offset], 512
    inc byte [next_sector]
    dec byte [sectors_left]
    jmp .read_next_sector

.read_done:

    jmp KERNEL_SEGMENT:0x0000

disk_error:
    mov si, msg_disk_error
    call print_string
    mov al, [disk_error_code]
    call print_hex_byte
    call print_newline
.hang:
    cli
    hlt
    jmp .hang

print_string:
    pusha
.print_loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp .print_loop
.done:
    popa
    ret

print_newline:
    pusha
    mov ah, 0x0E
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    popa
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
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    ret

boot_drive db 0
next_sector db 0
sectors_left db 0
kernel_offset dw 0
disk_error_code db 0
msg_boot db "RushOS bootloader: loading kernel...", 13, 10, 0
msg_disk_error db "Disk read error. BIOS code 0x", 0

times 510-($-$$) db 0
dw 0xAA55
