#include "graphics.h"

#include "io.h"
#include "vga.h"

#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC

static gfx_backend_t g_backend = GFX_BACKEND_VGA_TEXT;
static bool g_virtio_present = false;

static uint32_t pci_read32_bus0(uint8_t slot, uint8_t reg) {
    uint32_t addr = 0x80000000u | ((uint32_t)slot << 11) | ((uint32_t)reg & 0xFCu);
    outl(PCI_CFG_ADDR, addr);
    return inl(PCI_CFG_DATA);
}

static bool detect_virtio_gpu(void) {
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t id = pci_read32_bus0(slot, 0x00);
        uint16_t vendor = (uint16_t)(id & 0xFFFFu);
        uint16_t device = (uint16_t)(id >> 16);

        if (vendor == 0xFFFFu) continue;

        uint32_t class_reg = pci_read32_bus0(slot, 0x08);
        uint8_t class_code = (uint8_t)(class_reg >> 24);

        if (vendor == 0x1AF4u && class_code == 0x03u) {
            (void)device;
            return true;
        }
    }

    return false;
}

void gfx_init(void) {
    g_virtio_present = detect_virtio_gpu();
    g_backend = g_virtio_present ? GFX_BACKEND_VIRTIO_TEXT : GFX_BACKEND_VGA_TEXT;
}

gfx_backend_t gfx_backend(void) {
    return g_backend;
}

const char *gfx_backend_name(void) {
    if (g_backend == GFX_BACKEND_VIRTIO_TEXT) return "virtio-text";
    return "vga-text";
}

bool gfx_virtio_present(void) {
    return g_virtio_present;
}

void gfx_clear(uint8_t attr) {
    vga_clear(attr);
}

void gfx_put_cell(int row, int col, char c, uint8_t attr) {
    vga_putchar_at(row, col, c, attr);
}

void gfx_write_text(int row, int col, const char *s, uint8_t attr) {
    vga_write_at(row, col, s, attr);
}

void gfx_fill_rect(int row, int col, int h, int w, char c, uint8_t attr) {
    vga_fill_rect(row, col, h, w, c, attr);
}

void gfx_fill_row(int row, char c, uint8_t attr) {
    vga_fill_row(row, c, attr);
}

uint16_t gfx_read_cell(int row, int col) {
    return vga_read_at(row, col);
}
