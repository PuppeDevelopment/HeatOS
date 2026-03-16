#include "bga.h"
#include "io.h"

#define BGA_INDEX 0x01CE
#define BGA_DATA  0x01CF

static uint32_t pci_read32_bus0(uint8_t slot, uint8_t reg) {
    uint32_t addr = 0x80000000u | ((uint32_t)slot << 11) | ((uint32_t)reg & 0xFCu);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

static void bga_write_reg(uint16_t index, uint16_t data) {
    outw(BGA_INDEX, index);
    outw(BGA_DATA, data);
}

uint32_t bga_get_framebuffer(void) {
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t id = pci_read32_bus0(slot, 0x00);
        uint16_t vendor = (uint16_t)(id & 0xFFFF);
        uint16_t device = (uint16_t)(id >> 16);
        
        /* 0x1234:0x1111 is QEMU/Bochs VGA */
        if (vendor == 0x1234 && device == 0x1111) {
            uint32_t bar0 = pci_read32_bus0(slot, 0x10);
            return bar0 & 0xFFFFFFF0;
        }
        /* 0x80EE:0xBEEF is VirtualBox Graphics Adapter */
        if (vendor == 0x80EE && device == 0xBEEF) {
            uint32_t bar0 = pci_read32_bus0(slot, 0x10);
            return bar0 & 0xFFFFFFF0;
        }
    }
    return 0;
}

void bga_set_video_mode(uint16_t width, uint16_t height, uint16_t bpp, bool linear, bool clear) {
    /* 4 = VBE_DISPI_INDEX_ENABLE */
    bga_write_reg(4, 0); 

    /* 1 = VBE_DISPI_INDEX_XRES */
    bga_write_reg(1, width);
    /* 2 = VBE_DISPI_INDEX_YRES */
    bga_write_reg(2, height);
    /* 3 = VBE_DISPI_INDEX_BPP */
    bga_write_reg(3, bpp);

    uint16_t enable_flags = 1; /* VBE_DISPI_ENABLED */
    if (linear) enable_flags |= 0x40; /* VBE_DISPI_LFB_ENABLED */
    if (!clear) enable_flags |= 0x80; /* VBE_DISPI_NOCLEARMEM */
    
    bga_write_reg(4, enable_flags);
}
