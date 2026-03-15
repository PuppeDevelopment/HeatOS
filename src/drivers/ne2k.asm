; =============================================================================
; NE2000 PCI Driver
; Basic TX/RX and initialization for QEMU ne2k_pci.
; =============================================================================

NE2K_CR   equ 0x00 ; Command Register
NE2K_PSTART equ 0x01 ; Page Start
NE2K_PSTOP  equ 0x02 ; Page Stop
NE2K_BNRY   equ 0x03 ; Boundary Pointer
NE2K_TPSR   equ 0x04 ; Transmit Page Start
NE2K_TBCR0  equ 0x05 ; Transmit Byte Count 0
NE2K_TBCR1  equ 0x06 ; Transmit Byte Count 1
NE2K_ISR    equ 0x07 ; Interrupt Status
NE2K_RSAR0  equ 0x08 ; Remote Start Address 0
NE2K_RSAR1  equ 0x09 ; Remote Start Address 1
NE2K_RBCR0  equ 0x0A ; Remote Byte Count 0
NE2K_RBCR1  equ 0x0B ; Remote Byte Count 1
NE2K_RCR    equ 0x0C ; Receive Configuration
NE2K_TCR    equ 0x0D ; Transmit Configuration
NE2K_DCR    equ 0x0E ; Data Configuration
NE2K_IMR    equ 0x0F ; Interrupt Mask

NE2K_DATA   equ 0x10 ; Data Port

; Pages
RX_START_PAGE equ 0x46
RX_STOP_PAGE  equ 0x80
TX_START_PAGE equ 0x40

init_ne2k:
    pusha
    mov dx, [net_io_base]
    test dx, dx
    jz .done

    ; Soft Reset
    add dx, 0x1F ; Reset port
    in al, dx
    out dx, al
    sub dx, 0x1F

    ; Wait for reset
    mov cx, 1000
.wait_reset:
    loop .wait_reset

    ; Stop card, Page 0, Abort Remote DMA
    mov al, 0x21
    out dx, al

    ; Data Configuration: Word wide, 8-deep FIFO
    add dx, NE2K_DCR
    mov al, 0x49
    out dx, al
    sub dx, NE2K_DCR

    ; Clear Remote Byte Count
    add dx, NE2K_RBCR0
    xor al, al
    out dx, al
    inc dx ; RBCR1
    out dx, al
    sub dx, NE2K_RBCR1

    ; Receive Configuration: Monitor mode (off for now)
    add dx, NE2K_RCR
    mov al, 0x20
    out dx, al
    sub dx, NE2K_RCR

    ; Transmit Configuration: Loopback mode
    add dx, NE2K_TCR
    mov al, 0x02
    out dx, al
    sub dx, NE2K_TCR

    ; Page Start/Stop
    add dx, NE2K_PSTART
    mov al, RX_START_PAGE
    out dx, al
    inc dx ; PSTOP
    mov al, RX_STOP_PAGE
    out dx, al
    inc dx ; BNRY
    mov al, RX_START_PAGE
    out dx, al
    sub dx, NE2K_BNRY

    ; Clear interrupts
    add dx, NE2K_ISR
    mov al, 0xFF
    out dx, al
    sub dx, NE2K_ISR

    ; Interrupt Mask: None for now (polling)
    add dx, NE2K_IMR
    xor al, al
    out dx, al
    sub dx, NE2K_IMR

    ; Switch to Page 1 to set MAC address
    mov al, 0x61 ; Page 1, Stop
    out dx, al

    ; Read MAC from PROM (Reset port then Remote DMA read usually)
    ; In QEMU we can just read from the PROM area at offset 0
    call ne2k_read_prom

    ; Current Page
    add dx, 0x07 ; CURR register in Page 1
    mov al, RX_START_PAGE + 1
    out dx, al
    sub dx, 0x07

    ; Back to Page 0, Start
    mov al, 0x22
    out dx, al

    ; TCR: Normal mode
    add dx, NE2K_TCR
    xor al, al
    out dx, al
    sub dx, NE2K_TCR

    ; RCR: Accept Broadcast + Physical match
    add dx, NE2K_RCR
    mov al, 0x0C
    out dx, al
    sub dx, NE2K_RCR

.done:
    popa
    ret

ne2k_read_prom:
    ; Read the first 6 bytes from the PROM using Remote DMA
    pusha
    mov dx, [net_io_base]
    
    ; Page 0
    mov al, 0x21
    out dx, al

    add dx, NE2K_RSAR0
    xor al, al
    out dx, al ; RSAR0 = 0
    inc dx
    out dx, al ; RSAR1 = 0
    
    inc dx ; RBCR0
    mov al, 12 ; 12 bytes (MAC is doubled usually)
    out dx, al
    inc dx ; RBCR1
    xor al, al
    out dx, al
    
    sub dx, NE2K_RBCR1
    mov al, 0x0A ; Remote Read, Start
    out dx, al
    
    add dx, NE2K_DATA
    mov di, net_mac_address
    mov cx, 6
.read_loop:
    in ax, dx
    mov [di], al
    inc di
    loop .read_loop

    popa
    ret

; Send a packet
; DS:SI -> packet data, CX -> length (min 60)
ne2k_send_packet:
    pusha
    mov dx, [net_io_base]
    test dx, dx
    jz .done

    mov bx, cx ; Save length

    ; Page 0
    mov al, 0x22
    out dx, al

    ; Remote Byte Count
    add dx, NE2K_RBCR0
    mov ax, bx
    out dx, al
    mov al, ah
    inc dx
    out dx, al
    
    ; Remote Start Address
    inc dx ; RSAR0
    xor al, al
    out dx, al
    mov al, TX_START_PAGE
    inc dx ; RSAR1
    out dx, al
    
    sub dx, NE2K_RSAR1
    mov al, 0x12 ; Remote Write, Start
    out dx, al
    
    add dx, NE2K_DATA
    mov cx, bx
    shr cx, 1 ; Number of words
    adc cx, 0 ; Round up
.write_loop:
    lodsw
    out dx, ax
    loop .write_loop

    ; Start TX
    sub dx, NE2K_DATA
    mov al, 0x22
    out dx, al
    
    add dx, NE2K_TBCR0
    mov ax, bx
    out dx, al
    inc dx
    mov al, ah
    out dx, al
    
    sub dx, NE2K_TPSR
    mov al, TX_START_PAGE
    out dx, al
    
    mov al, 0x26 ; Start TX
    mov dx, [net_io_base]
    out dx, al

.done:
    popa
    ret

