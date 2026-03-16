#include "virtio_gpu.h"
#include "io.h"
#include "string.h"

#define PCI_CFG_ADDR 0xCF8
#define PCI_CFG_DATA 0xCFC

#define PCI_COMMAND_REG      0x04
#define PCI_STATUS_REG       0x06
#define PCI_CAP_PTR_REG      0x34
#define PCI_BAR0_REG         0x10

#define PCI_COMMAND_IO       0x0001
#define PCI_COMMAND_MEMORY   0x0002
#define PCI_COMMAND_MASTER   0x0004
#define PCI_STATUS_CAP_LIST  0x0010

#define PCI_CAP_ID_VENDOR    0x09

#define VIRTIO_VENDOR_ID                0x1AF4u
#define VIRTIO_GPU_DEVICE_ID            16u
#define VIRTIO_GPU_PCI_DEVICE_ID        0x1050u
#define VIRTIO_GPU_PCI_TRANSITIONAL_ID  0x1010u

#define VIRTIO_PCI_CAP_COMMON_CFG 1u
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2u
#define VIRTIO_PCI_CAP_ISR_CFG    3u
#define VIRTIO_PCI_CAP_DEVICE_CFG 4u

#define VIRTIO_PCI_COMMON_DFSELECT   0u
#define VIRTIO_PCI_COMMON_DF         4u
#define VIRTIO_PCI_COMMON_GFSELECT   8u
#define VIRTIO_PCI_COMMON_GF         12u
#define VIRTIO_PCI_COMMON_STATUS     20u
#define VIRTIO_PCI_COMMON_Q_SELECT   22u
#define VIRTIO_PCI_COMMON_Q_SIZE     24u
#define VIRTIO_PCI_COMMON_Q_ENABLE   28u
#define VIRTIO_PCI_COMMON_Q_NOFF     30u
#define VIRTIO_PCI_COMMON_Q_DESCLO   32u
#define VIRTIO_PCI_COMMON_Q_DESCHI   36u
#define VIRTIO_PCI_COMMON_Q_AVAILLO  40u
#define VIRTIO_PCI_COMMON_Q_AVAILHI  44u
#define VIRTIO_PCI_COMMON_Q_USEDLO   48u
#define VIRTIO_PCI_COMMON_Q_USEDHI   52u

#define VIRTIO_STATUS_ACKNOWLEDGE 1u
#define VIRTIO_STATUS_DRIVER      2u
#define VIRTIO_STATUS_DRIVER_OK   4u
#define VIRTIO_STATUS_FEATURES_OK 8u
#define VIRTIO_STATUS_FAILED      128u

#define VIRTIO_F_VERSION_1        32u

#define VIRTIO_GPU_CMD_GET_DISPLAY_INFO       0x0100u
#define VIRTIO_GPU_CMD_RESOURCE_CREATE_2D     0x0101u
#define VIRTIO_GPU_CMD_RESOURCE_UNREF         0x0102u
#define VIRTIO_GPU_CMD_SET_SCANOUT            0x0103u
#define VIRTIO_GPU_CMD_RESOURCE_FLUSH         0x0104u
#define VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D    0x0105u
#define VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING 0x0106u

#define VIRTIO_GPU_RESP_OK_NODATA       0x1100u
#define VIRTIO_GPU_RESP_OK_DISPLAY_INFO 0x1101u

#define VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM 2u

#define VIRTQ_DESC_F_NEXT  1u
#define VIRTQ_DESC_F_WRITE 2u

#define VIRTIO_GPU_QUEUE_SIZE 8u
#define VIRTIO_GPU_RESOURCE_ID 1u
#define VIRTIO_GPU_MAX_SCANOUTS 16u

typedef struct {
    uint32_t base;
    uint32_t length;
    bool io;
} pci_region_t;

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_GPU_QUEUE_SIZE];
    uint16_t used_event;
} __attribute__((packed));

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[VIRTIO_GPU_QUEUE_SIZE];
    uint16_t avail_event;
} __attribute__((packed));

struct virtio_gpu_ctrl_hdr {
    uint32_t type;
    uint32_t flags;
    uint64_t fence_id;
    uint32_t ctx_id;
    uint8_t ring_idx;
    uint8_t padding[3];
} __attribute__((packed));

struct virtio_gpu_rect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} __attribute__((packed));

struct virtio_gpu_display_one {
    struct virtio_gpu_rect r;
    uint32_t enabled;
    uint32_t flags;
} __attribute__((packed));

struct virtio_gpu_resp_display_info {
    struct virtio_gpu_ctrl_hdr hdr;
    struct virtio_gpu_display_one pmodes[VIRTIO_GPU_MAX_SCANOUTS];
} __attribute__((packed));

struct virtio_gpu_resource_create_2d {
    struct virtio_gpu_ctrl_hdr hdr;
    uint32_t resource_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
} __attribute__((packed));

struct virtio_gpu_mem_entry {
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
} __attribute__((packed));

struct virtio_gpu_resource_attach_backing_req {
    struct virtio_gpu_ctrl_hdr hdr;
    uint32_t resource_id;
    uint32_t nr_entries;
    struct virtio_gpu_mem_entry entry;
} __attribute__((packed));

struct virtio_gpu_set_scanout {
    struct virtio_gpu_ctrl_hdr hdr;
    struct virtio_gpu_rect r;
    uint32_t scanout_id;
    uint32_t resource_id;
} __attribute__((packed));

struct virtio_gpu_transfer_to_host_2d {
    struct virtio_gpu_ctrl_hdr hdr;
    struct virtio_gpu_rect r;
    uint64_t offset;
    uint32_t resource_id;
    uint32_t padding;
} __attribute__((packed));

struct virtio_gpu_resource_flush {
    struct virtio_gpu_ctrl_hdr hdr;
    struct virtio_gpu_rect r;
    uint32_t resource_id;
    uint32_t padding;
} __attribute__((packed));

struct virtio_gpu_resource_unref {
    struct virtio_gpu_ctrl_hdr hdr;
    uint32_t resource_id;
    uint32_t padding;
} __attribute__((packed));

static struct virtq_desc g_desc[VIRTIO_GPU_QUEUE_SIZE] __attribute__((aligned(16)));
static struct virtq_avail g_avail __attribute__((aligned(4096)));
static struct virtq_used g_used __attribute__((aligned(4096)));
static uint8_t g_request_buf[128] __attribute__((aligned(16)));
static uint8_t g_response_buf[512] __attribute__((aligned(16)));

static bool g_ready = false;
static uint8_t g_pci_slot = 0xFFu;
static uint16_t g_queue_size = 0;
static uint16_t g_last_used_idx = 0;
static uint16_t g_width = 0;
static uint16_t g_height = 0;
static uint32_t *g_backbuffer = 0;
static pci_region_t g_common_cfg = {0, 0, false};
static pci_region_t g_notify_cfg = {0, 0, false};
static pci_region_t g_device_cfg = {0, 0, false};
static uint32_t g_notify_multiplier = 0;
static uint32_t g_notify_offset = 0;

static inline void virtio_mb(void) {
    __asm__ volatile("" : : : "memory");
}

static uint32_t pci_read32_bus0(uint8_t slot, uint8_t reg) {
    uint32_t addr = 0x80000000u | ((uint32_t)slot << 11) | ((uint32_t)reg & 0xFCu);
    outl(PCI_CFG_ADDR, addr);
    return inl(PCI_CFG_DATA);
}

static void pci_write32_bus0(uint8_t slot, uint8_t reg, uint32_t value) {
    uint32_t addr = 0x80000000u | ((uint32_t)slot << 11) | ((uint32_t)reg & 0xFCu);
    outl(PCI_CFG_ADDR, addr);
    outl(PCI_CFG_DATA, value);
}

static uint16_t pci_read16_bus0(uint8_t slot, uint8_t reg) {
    uint32_t value = pci_read32_bus0(slot, reg);
    uint32_t shift = (uint32_t)(reg & 2u) * 8u;
    return (uint16_t)((value >> shift) & 0xFFFFu);
}

static uint8_t pci_read8_bus0(uint8_t slot, uint8_t reg) {
    uint32_t value = pci_read32_bus0(slot, reg);
    uint32_t shift = (uint32_t)(reg & 3u) * 8u;
    return (uint8_t)((value >> shift) & 0xFFu);
}

static void pci_write16_bus0(uint8_t slot, uint8_t reg, uint16_t value) {
    uint32_t oldv = pci_read32_bus0(slot, reg);
    uint32_t shift = (uint32_t)(reg & 2u) * 8u;
    uint32_t mask = 0xFFFFu << shift;
    uint32_t newv = (oldv & ~mask) | ((uint32_t)value << shift);
    pci_write32_bus0(slot, reg, newv);
}

static bool pci_get_bar(uint8_t slot, uint8_t bar_index, pci_region_t *out) {
    uint8_t reg = (uint8_t)(PCI_BAR0_REG + bar_index * 4u);
    uint32_t bar = pci_read32_bus0(slot, reg);

    if ((bar & 1u) != 0u) {
        out->io = true;
        out->base = bar & 0xFFFFFFFCu;
        out->length = 0;
        return out->base != 0;
    }

    if (((bar >> 1) & 0x3u) == 0x2u) {
        uint32_t high = pci_read32_bus0(slot, (uint8_t)(reg + 4u));
        if (high != 0u) {
            return false;
        }
    }

    out->io = false;
    out->base = bar & 0xFFFFFFF0u;
    out->length = 0;
    return out->base != 0;
}

static uint8_t region_read8(const pci_region_t *region, uint32_t off) {
    if (region->io) {
        return inb((uint16_t)(region->base + off));
    }
    return *(volatile uint8_t *)(size_t)(region->base + off);
}

static uint16_t region_read16(const pci_region_t *region, uint32_t off) {
    if (region->io) {
        return inw((uint16_t)(region->base + off));
    }
    return *(volatile uint16_t *)(size_t)(region->base + off);
}

static uint32_t region_read32(const pci_region_t *region, uint32_t off) {
    if (region->io) {
        return inl((uint16_t)(region->base + off));
    }
    return *(volatile uint32_t *)(size_t)(region->base + off);
}

static void region_write8(const pci_region_t *region, uint32_t off, uint8_t value) {
    if (region->io) {
        outb((uint16_t)(region->base + off), value);
    } else {
        *(volatile uint8_t *)(size_t)(region->base + off) = value;
    }
}

static void region_write16(const pci_region_t *region, uint32_t off, uint16_t value) {
    if (region->io) {
        outw((uint16_t)(region->base + off), value);
    } else {
        *(volatile uint16_t *)(size_t)(region->base + off) = value;
    }
}

static void region_write32(const pci_region_t *region, uint32_t off, uint32_t value) {
    if (region->io) {
        outl((uint16_t)(region->base + off), value);
    } else {
        *(volatile uint32_t *)(size_t)(region->base + off) = value;
    }
}

static bool virtio_gpu_find_device(void) {
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint32_t id = pci_read32_bus0(slot, 0x00);
        uint16_t vendor = (uint16_t)(id & 0xFFFFu);
        uint16_t device = (uint16_t)(id >> 16);
        uint16_t subsystem = pci_read16_bus0(slot, 0x2Eu);

        if (vendor != VIRTIO_VENDOR_ID) {
            continue;
        }

        if (device != VIRTIO_GPU_PCI_DEVICE_ID &&
            device != VIRTIO_GPU_PCI_TRANSITIONAL_ID &&
            subsystem != VIRTIO_GPU_DEVICE_ID) {
            continue;
        }

        g_pci_slot = slot;
        return true;
    }

    return false;
}

static bool virtio_gpu_find_caps(void) {
    if (g_pci_slot == 0xFFu) return false;

    uint16_t status = pci_read16_bus0(g_pci_slot, PCI_STATUS_REG);
    if ((status & PCI_STATUS_CAP_LIST) == 0u) {
        return false;
    }

    g_common_cfg.base = 0;
    g_notify_cfg.base = 0;
    g_device_cfg.base = 0;
    g_notify_multiplier = 0;

    uint8_t cap = pci_read8_bus0(g_pci_slot, PCI_CAP_PTR_REG);
    uint32_t guard = 48u;

    while (cap >= 0x40u && cap < 0xF0u && guard--) {
        uint8_t cap_id = pci_read8_bus0(g_pci_slot, cap + 0u);
        uint8_t next = pci_read8_bus0(g_pci_slot, cap + 1u);

        if (cap_id == PCI_CAP_ID_VENDOR) {
            uint8_t cfg_type = pci_read8_bus0(g_pci_slot, cap + 3u);
            uint8_t bar = pci_read8_bus0(g_pci_slot, cap + 4u);
            uint32_t offset = pci_read32_bus0(g_pci_slot, (uint8_t)(cap + 8u));
            uint32_t length = pci_read32_bus0(g_pci_slot, (uint8_t)(cap + 12u));
            pci_region_t region;

            if (bar < 6u && pci_get_bar(g_pci_slot, bar, &region)) {
                region.base += offset;
                region.length = length;

                if (cfg_type == VIRTIO_PCI_CAP_COMMON_CFG) {
                    g_common_cfg = region;
                } else if (cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG) {
                    g_notify_cfg = region;
                    g_notify_multiplier = pci_read32_bus0(g_pci_slot, (uint8_t)(cap + 16u));
                } else if (cfg_type == VIRTIO_PCI_CAP_DEVICE_CFG) {
                    g_device_cfg = region;
                }
            }
        }

        if (next == 0u || next == cap) {
            break;
        }
        cap = next;
    }

    return g_common_cfg.base != 0u && g_notify_cfg.base != 0u;
}

static void virtio_gpu_fail(void) {
    if (g_common_cfg.base != 0u) {
        region_write8(&g_common_cfg, VIRTIO_PCI_COMMON_STATUS, VIRTIO_STATUS_FAILED);
    }
}

static bool virtio_gpu_setup_queue(void) {
    region_write16(&g_common_cfg, VIRTIO_PCI_COMMON_Q_SELECT, 0u);

    uint16_t max_queue = region_read16(&g_common_cfg, VIRTIO_PCI_COMMON_Q_SIZE);
    if (max_queue < 2u) {
        return false;
    }

    g_queue_size = max_queue < VIRTIO_GPU_QUEUE_SIZE ? max_queue : VIRTIO_GPU_QUEUE_SIZE;
    g_notify_offset = (uint32_t)region_read16(&g_common_cfg, VIRTIO_PCI_COMMON_Q_NOFF) * g_notify_multiplier;

    memset(g_desc, 0, sizeof(g_desc));
    memset(&g_avail, 0, sizeof(g_avail));
    memset(&g_used, 0, sizeof(g_used));
    g_avail.flags = 1u;
    g_last_used_idx = 0u;

    region_write16(&g_common_cfg, VIRTIO_PCI_COMMON_Q_SIZE, g_queue_size);
    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_Q_DESCLO, (uint32_t)(size_t)g_desc);
    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_Q_DESCHI, 0u);
    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_Q_AVAILLO, (uint32_t)(size_t)&g_avail);
    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_Q_AVAILHI, 0u);
    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_Q_USEDLO, (uint32_t)(size_t)&g_used);
    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_Q_USEDHI, 0u);
    region_write16(&g_common_cfg, VIRTIO_PCI_COMMON_Q_ENABLE, 1u);

    return true;
}

static void virtio_gpu_notify_control_queue(void) {
    region_write16(&g_notify_cfg, g_notify_offset, 0u);
}

static bool virtio_gpu_submit(const void *req, uint32_t req_len, void *resp, uint32_t resp_len) {
    if (!g_ready) return false;

    g_desc[0].addr = (uint64_t)(uint32_t)(size_t)req;
    g_desc[0].len = req_len;
    g_desc[0].flags = VIRTQ_DESC_F_NEXT;
    g_desc[0].next = 1u;

    g_desc[1].addr = (uint64_t)(uint32_t)(size_t)resp;
    g_desc[1].len = resp_len;
    g_desc[1].flags = VIRTQ_DESC_F_WRITE;
    g_desc[1].next = 0u;

    uint16_t avail_idx = g_avail.idx;
    g_avail.ring[avail_idx % g_queue_size] = 0u;
    virtio_mb();
    g_avail.idx = (uint16_t)(avail_idx + 1u);
    virtio_mb();

    virtio_gpu_notify_control_queue();

    uint32_t guard = 5000000u;
    while (guard--) {
        virtio_mb();
        if (g_used.idx != g_last_used_idx) {
            struct virtq_used_elem *used = &g_used.ring[g_last_used_idx % g_queue_size];
            g_last_used_idx = (uint16_t)(g_last_used_idx + 1u);
            return used->id == 0u;
        }
    }

    return false;
}

static bool virtio_gpu_check_response(const void *resp, uint32_t expected_type) {
    const struct virtio_gpu_ctrl_hdr *hdr = (const struct virtio_gpu_ctrl_hdr *)resp;
    return hdr->type == expected_type;
}

static bool virtio_gpu_create_resource(void) {
    struct virtio_gpu_resource_create_2d *req = (struct virtio_gpu_resource_create_2d *)g_request_buf;
    struct virtio_gpu_ctrl_hdr *resp = (struct virtio_gpu_ctrl_hdr *)g_response_buf;

    memset(req, 0, sizeof(*req));
    memset(resp, 0, sizeof(*resp));
    req->hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
    req->resource_id = VIRTIO_GPU_RESOURCE_ID;
    req->format = VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM;
    req->width = g_width;
    req->height = g_height;

    return virtio_gpu_submit(req, sizeof(*req), resp, sizeof(*resp)) &&
           virtio_gpu_check_response(resp, VIRTIO_GPU_RESP_OK_NODATA);
}

static bool virtio_gpu_attach_backing(void) {
    struct virtio_gpu_resource_attach_backing_req *req = (struct virtio_gpu_resource_attach_backing_req *)g_request_buf;
    struct virtio_gpu_ctrl_hdr *resp = (struct virtio_gpu_ctrl_hdr *)g_response_buf;

    memset(req, 0, sizeof(*req));
    memset(resp, 0, sizeof(*resp));
    req->hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
    req->resource_id = VIRTIO_GPU_RESOURCE_ID;
    req->nr_entries = 1u;
    req->entry.addr = (uint64_t)(uint32_t)(size_t)g_backbuffer;
    req->entry.length = (uint32_t)g_width * (uint32_t)g_height * 4u;

    return virtio_gpu_submit(req, sizeof(*req), resp, sizeof(*resp)) &&
           virtio_gpu_check_response(resp, VIRTIO_GPU_RESP_OK_NODATA);
}

static bool virtio_gpu_set_scanout(uint32_t resource_id) {
    struct virtio_gpu_set_scanout *req = (struct virtio_gpu_set_scanout *)g_request_buf;
    struct virtio_gpu_ctrl_hdr *resp = (struct virtio_gpu_ctrl_hdr *)g_response_buf;

    memset(req, 0, sizeof(*req));
    memset(resp, 0, sizeof(*resp));
    req->hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT;
    req->r.width = g_width;
    req->r.height = g_height;
    req->scanout_id = 0u;
    req->resource_id = resource_id;

    return virtio_gpu_submit(req, sizeof(*req), resp, sizeof(*resp)) &&
           virtio_gpu_check_response(resp, VIRTIO_GPU_RESP_OK_NODATA);
}

static bool virtio_gpu_transfer_full(void) {
    struct virtio_gpu_transfer_to_host_2d *req = (struct virtio_gpu_transfer_to_host_2d *)g_request_buf;
    struct virtio_gpu_ctrl_hdr *resp = (struct virtio_gpu_ctrl_hdr *)g_response_buf;

    memset(req, 0, sizeof(*req));
    memset(resp, 0, sizeof(*resp));
    req->hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    req->r.width = g_width;
    req->r.height = g_height;
    req->offset = 0u;
    req->resource_id = VIRTIO_GPU_RESOURCE_ID;

    return virtio_gpu_submit(req, sizeof(*req), resp, sizeof(*resp)) &&
           virtio_gpu_check_response(resp, VIRTIO_GPU_RESP_OK_NODATA);
}

static bool virtio_gpu_flush_full(void) {
    struct virtio_gpu_resource_flush *req = (struct virtio_gpu_resource_flush *)g_request_buf;
    struct virtio_gpu_ctrl_hdr *resp = (struct virtio_gpu_ctrl_hdr *)g_response_buf;

    memset(req, 0, sizeof(*req));
    memset(resp, 0, sizeof(*resp));
    req->hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    req->r.width = g_width;
    req->r.height = g_height;
    req->resource_id = VIRTIO_GPU_RESOURCE_ID;

    return virtio_gpu_submit(req, sizeof(*req), resp, sizeof(*resp)) &&
           virtio_gpu_check_response(resp, VIRTIO_GPU_RESP_OK_NODATA);
}

static void virtio_gpu_unreference_resource(void) {
    struct virtio_gpu_resource_unref *req = (struct virtio_gpu_resource_unref *)g_request_buf;
    struct virtio_gpu_ctrl_hdr *resp = (struct virtio_gpu_ctrl_hdr *)g_response_buf;

    memset(req, 0, sizeof(*req));
    memset(resp, 0, sizeof(*resp));
    req->hdr.type = VIRTIO_GPU_CMD_RESOURCE_UNREF;
    req->resource_id = VIRTIO_GPU_RESOURCE_ID;
    (void)virtio_gpu_submit(req, sizeof(*req), resp, sizeof(*resp));
}

static bool virtio_gpu_query_display(void) {
    struct virtio_gpu_ctrl_hdr *req = (struct virtio_gpu_ctrl_hdr *)g_request_buf;
    struct virtio_gpu_resp_display_info *resp = (struct virtio_gpu_resp_display_info *)g_response_buf;

    memset(req, 0, sizeof(*req));
    memset(resp, 0, sizeof(*resp));
    req->type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;

    return virtio_gpu_submit(req, sizeof(*req), resp, sizeof(*resp)) &&
           virtio_gpu_check_response(resp, VIRTIO_GPU_RESP_OK_DISPLAY_INFO);
}

bool virtio_gpu_init(uint16_t width, uint16_t height, uint32_t *backbuffer) {
    if (!backbuffer) return false;

    if (!virtio_gpu_find_device()) {
        return false;
    }

    if (!virtio_gpu_find_caps()) {
        return false;
    }

    uint16_t pci_command = pci_read16_bus0(g_pci_slot, PCI_COMMAND_REG);
    pci_command |= (uint16_t)(PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
    pci_write16_bus0(g_pci_slot, PCI_COMMAND_REG, pci_command);

    region_write8(&g_common_cfg, VIRTIO_PCI_COMMON_STATUS, 0u);
    if (region_read8(&g_common_cfg, VIRTIO_PCI_COMMON_STATUS) != 0u) {
        return false;
    }

    region_write8(&g_common_cfg, VIRTIO_PCI_COMMON_STATUS, (uint8_t)(VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER));

    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_DFSELECT, 1u);
    uint32_t features_hi = region_read32(&g_common_cfg, VIRTIO_PCI_COMMON_DF);
    if ((features_hi & 1u) == 0u) {
        virtio_gpu_fail();
        return false;
    }

    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_GFSELECT, 0u);
    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_GF, 0u);
    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_GFSELECT, 1u);
    region_write32(&g_common_cfg, VIRTIO_PCI_COMMON_GF, 1u << (VIRTIO_F_VERSION_1 - 32u));

    uint8_t status = (uint8_t)(VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);
    region_write8(&g_common_cfg, VIRTIO_PCI_COMMON_STATUS, status);
    if ((region_read8(&g_common_cfg, VIRTIO_PCI_COMMON_STATUS) & VIRTIO_STATUS_FEATURES_OK) == 0u) {
        virtio_gpu_fail();
        return false;
    }

    if (!virtio_gpu_setup_queue()) {
        virtio_gpu_fail();
        return false;
    }

    status = (uint8_t)(status | VIRTIO_STATUS_DRIVER_OK);
    region_write8(&g_common_cfg, VIRTIO_PCI_COMMON_STATUS, status);

    g_width = width;
    g_height = height;
    g_backbuffer = backbuffer;
    g_ready = true;

    if (g_device_cfg.base != 0u && g_device_cfg.length >= 12u) {
        uint32_t num_scanouts = region_read32(&g_device_cfg, 8u);
        if (num_scanouts == 0u) {
            virtio_gpu_shutdown();
            return false;
        }
    }

    (void)virtio_gpu_query_display();

    if (!virtio_gpu_create_resource() ||
        !virtio_gpu_attach_backing() ||
        !virtio_gpu_set_scanout(VIRTIO_GPU_RESOURCE_ID) ||
        !virtio_gpu_transfer_full() ||
        !virtio_gpu_flush_full()) {
        virtio_gpu_shutdown();
        return false;
    }

    return true;
}

void virtio_gpu_present(void) {
    if (!g_ready) return;
    if (!virtio_gpu_transfer_full()) return;
    (void)virtio_gpu_flush_full();
}

void virtio_gpu_shutdown(void) {
    if (!g_ready) return;

    (void)virtio_gpu_set_scanout(0u);
    virtio_gpu_unreference_resource();
    region_write8(&g_common_cfg, VIRTIO_PCI_COMMON_STATUS, 0u);
    while (region_read8(&g_common_cfg, VIRTIO_PCI_COMMON_STATUS) != 0u) {}

    g_ready = false;
    g_pci_slot = 0xFFu;
    g_queue_size = 0u;
    g_last_used_idx = 0u;
    g_notify_offset = 0u;
    g_backbuffer = 0;
}