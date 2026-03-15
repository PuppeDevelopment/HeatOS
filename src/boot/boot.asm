;=============================================================================
; RushOS Boot Sector
; 16-bit real mode -> loads kernel -> enables A20 -> enters 32-bit protected mode
;=============================================================================
[bits 16]
[org 0x7C00]

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 32
%endif

KERNEL_LOAD_SEG     equ 0x1000         ; Load kernel at physical 0x10000
KERNEL_PHYS_ADDR    equ 0x10000
START_SECTOR        equ 2              ; First sector after boot sector
READ_RETRIES        equ 3
SECTORS_PER_TRACK   equ 18
HEAD_COUNT          equ 2

start:
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
    call print16

    ; Load kernel from floppy to KERNEL_LOAD_SEG:0x0000
    mov ax, KERNEL_LOAD_SEG
    mov es, ax
    xor bx, bx
    mov byte [cur_sector], START_SECTOR
    mov byte [cur_head], 0
    mov byte [cur_cyl], 0
    mov byte [sectors_left], KERNEL_SECTORS

.read_loop:
    cmp byte [sectors_left], 0
    je .read_done

    mov di, READ_RETRIES
.retry:
    mov ah, 0x02
    mov al, 1
    mov ch, [cur_cyl]
    mov cl, [cur_sector]
    mov dh, [cur_head]
    mov dl, [boot_drive]
    int 0x13
    jnc .read_ok

    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13
    dec di
    jnz .retry
    jmp disk_error

.read_ok:
    add bx, 512
    jnc .no_seg
    mov ax, es
    add ax, 0x1000
    mov es, ax
    xor bx, bx
.no_seg:
    call advance_chs
    dec byte [sectors_left]
    jmp .read_loop

.read_done:
    mov si, msg_ok
    call print16

    ; ---- Enable A20 line ----
    call enable_a20

    ; ---- Load GDT and enter protected mode ----
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_entry

;=============================================================================
; Enable A20 via keyboard controller
;=============================================================================
enable_a20:
    call .wait_in
    mov al, 0xAD
    out 0x64, al
    call .wait_in
    mov al, 0xD0
    out 0x64, al
    call .wait_out
    in al, 0x60
    push ax
    call .wait_in
    mov al, 0xD1
    out 0x64, al
    call .wait_in
    pop ax
    or al, 2
    out 0x60, al
    call .wait_in
    mov al, 0xAE
    out 0x64, al
    call .wait_in
    ret
.wait_in:
    in al, 0x64
    test al, 2
    jnz .wait_in
    ret
.wait_out:
    in al, 0x64
    test al, 1
    jz .wait_out
    ret

;=============================================================================
; CHS advance
;=============================================================================
advance_chs:
    inc byte [cur_sector]
    cmp byte [cur_sector], SECTORS_PER_TRACK + 1
    jb .done
    mov byte [cur_sector], 1
    inc byte [cur_head]
    cmp byte [cur_head], HEAD_COUNT
    jb .done
    mov byte [cur_head], 0
    inc byte [cur_cyl]
.done:
    ret

;=============================================================================
; 16-bit helpers
;=============================================================================
disk_error:
    mov si, msg_disk_err
    call print16
.hang:
    cli
    hlt
    jmp .hang

print16:
    pusha
.lp:
    lodsb
    test al, al
    jz .dn
    mov ah, 0x0E
    xor bx, bx
    mov bl, 0x07
    int 0x10
    jmp .lp
.dn:
    popa
    ret

;=============================================================================
; GDT - flat model
;=============================================================================
align 8
gdt_start:
    dq 0                                ; Null descriptor
gdt_code:                               ; Selector 0x08
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
gdt_data:                               ; Selector 0x10
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

;=============================================================================
; 32-bit protected mode entry
;=============================================================================
[bits 32]
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; ---- Remap PIC: move IRQs to INT 0x20-0x2F so they don't collide with CPU exceptions ----
    mov al, 0x11        ; ICW1: init + ICW4 needed
    out 0x20, al
    out 0xA0, al
    mov al, 0x20        ; ICW2 master: IRQ 0-7 -> INT 0x20-0x27
    out 0x21, al
    mov al, 0x28        ; ICW2 slave: IRQ 8-15 -> INT 0x28-0x2F
    out 0xA1, al
    mov al, 0x04        ; ICW3 master: slave on IRQ2
    out 0x21, al
    mov al, 0x02        ; ICW3 slave: cascade identity
    out 0xA1, al
    mov al, 0x01        ; ICW4: 8086 mode
    out 0x21, al
    out 0xA1, al

    ; Mask ALL IRQs (we poll keyboard directly)
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al

    jmp KERNEL_PHYS_ADDR

;=============================================================================
; Data
;=============================================================================
[bits 16]
boot_drive      db 0
cur_sector      db 0
cur_head        db 0
cur_cyl         db 0
sectors_left    db 0
msg_boot        db "RushOS: Loading kernel...", 13, 10, 0
msg_ok          db "OK", 13, 10, 0
msg_disk_err    db "Disk error!", 0

times 510-($-$$) db 0
dw 0xAA55
