;=============================================================================
; HeatOS 32-bit Kernel Entry Point
; Bootloader jumps here after switching to protected mode.
; Sets up IDT (to prevent triple-faults) and calls the C kernel_main().
;=============================================================================
[bits 32]
section .text

global _start
extern kernel_main

_start:
    ; Data segments already set by bootloader, but reinforce
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; ---- Build a minimal IDT with 48 entries (32 exceptions + 16 IRQs) ----
    ; Each entry: 8 bytes (offset_lo, selector, zero, type_attr, offset_hi)
    mov edi, idt_table
    mov ecx, 48

    ; All entries point to the same "ignore" stub
    mov eax, isr_ignore
    mov ebx, eax
    and eax, 0xFFFF          ; low 16 bits of handler address
    shr ebx, 16              ; high 16 bits of handler address
    or  eax, 0x00080000      ; selector = 0x08 (code segment)

.fill_idt:
    mov [edi],   eax          ; offset_lo (16) + selector (16)
    mov word [edi+4], 0x8E00  ; present, DPL=0, 32-bit interrupt gate
    mov [edi+6], bx           ; offset_hi (16)
    add edi, 8
    dec ecx
    jnz .fill_idt

    ; Load IDT
    lidt [idt_descriptor]

    ; Enable interrupts (safe now — all IRQs are masked by bootloader, IDT catches exceptions)
    sti

    call kernel_main

    ; kernel_main should never return, but just in case:
.halt:
    cli
    hlt
    jmp .halt

; ---- ISR stub: just IRET (ignore the interrupt/exception) ----
isr_ignore:
    iret

section .data
align 8
idt_table:
    times 48 * 8 db 0        ; 48 entries x 8 bytes each

idt_descriptor:
    dw (48 * 8) - 1          ; limit
    dd idt_table              ; base
