;=============================================================================
; HeatOS 64-bit bootstrap entry
; Starts in 32-bit protected mode (from boot.asm), enables long mode,
; then jumps to 64-bit C kernel_main64().
;=============================================================================
[bits 32]
section .text

global _start
extern kernel_main64
extern __bss_start
extern __bss_end

_start:
    cli

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Clear kernel BSS to guarantee zero-initialized globals.
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    shr ecx, 2
    rep stosd

    ; Clear paging structures (PML4 + PDPT + PD)
    mov edi, pml4_table
    xor eax, eax
    mov ecx, (4096 * 3) / 4
    rep stosd

    ; Identity map low memory with one 2 MiB page.
    mov eax, pdpt_table
    or eax, 0x03
    mov [pml4_table], eax
    mov dword [pml4_table + 4], 0

    mov eax, pd_table
    or eax, 0x03
    mov [pdpt_table], eax
    mov dword [pdpt_table + 4], 0

    ; PDE: present + writable + 2 MiB page (PS)
    mov dword [pd_table], 0x00000083
    mov dword [pd_table + 4], 0x00000000

    ; Load CR3 with PML4 physical address.
    mov eax, pml4_table
    mov cr3, eax

    ; Enable PAE + PSE.
    mov eax, cr4
    or eax, (1 << 5) | (1 << 4)
    mov cr4, eax

    ; Set EFER.LME to enable long mode.
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; Enable paging in CR0 (enters long mode on far jump).
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Load 64-bit GDT and jump to 64-bit code segment.
    lgdt [gdt64_descriptor]
    jmp 0x08:long_mode_start

[bits 64]
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000

    call kernel_main64

.hang:
    cli
    hlt
    jmp .hang

section .data
align 8
gdt64_start:
    dq 0x0000000000000000            ; null
    dq 0x00AF9A000000FFFF            ; 64-bit code segment
    dq 0x00AF92000000FFFF            ; data segment
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start

section .bss
align 4096
pml4_table:  resb 4096
pdpt_table:  resb 4096
pd_table:    resb 4096
