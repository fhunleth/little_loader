#pragma once
#include <stdint.h>

// See https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf

// HACK: It could be anywhere from 0x0a000000UL to 0xa003e00 according to the dtb. I found it manually.
//#define VIRTIO_BLK_MMIO_BASE 0xa003e00UL
#define VIRTIO_BLK_MMIO_BASE 0xa000000UL

#define REG(offset) (*(volatile uint32_t *)(VIRTIO_BLK_MMIO_BASE + (offset)))

// Common MMIO header
#define VIRT_MMIO_MAGIC         REG(0x000)
#define VIRT_MMIO_VERSION       REG(0x004)
#define VIRT_MMIO_DEVICE_ID     REG(0x008)
#define VIRT_MMIO_VENDOR_ID     REG(0x00C)

// Feature negotiation
#define VIRT_MMIO_DEVICE_FEATURES     REG(0x010)
#define VIRT_MMIO_DEVICE_FEATURES_SEL REG(0x014)
#define VIRT_MMIO_DRIVER_FEATURES     REG(0x020)
#define VIRT_MMIO_DRIVER_FEATURES_SEL REG(0x024)

// Queue configuration
#define VIRT_MMIO_QUEUE_SEL           REG(0x030)
#define VIRT_MMIO_QUEUE_NUM_MAX       REG(0x034)
#define VIRT_MMIO_QUEUE_NUM           REG(0x038)

#define VIRT_MMIO_QUEUE_READY         REG(0x044)
#define VIRT_MMIO_QUEUE_NOTIFY        REG(0x050)

#define VIRT_MMIO_INTERRUPT_STATUS    REG(0x060)
#define VIRT_MMIO_INTERRUPT_ACK       REG(0x064)

#define VIRT_MMIO_STATUS              REG(0x070)

#define VIRT_MMIO_QUEUE_DESC_LOW      REG(0x080)
#define VIRT_MMIO_QUEUE_DESC_HIGH     REG(0x084)
#define VIRT_MMIO_QUEUE_DRIVER_LOW    REG(0x090)
#define VIRT_MMIO_QUEUE_DRIVER_HIGH   REG(0x094)
#define VIRT_MMIO_QUEUE_DEVICE_LOW    REG(0x0A0)
#define VIRT_MMIO_QUEUE_DEVICE_HIGH   REG(0x0A4)

#define VIRT_MMIO_CONFIG_GENERATION   REG(0x0FC)

#define SECTOR_SIZE          512

#define VIRTIO_BLK_T_IN  0
#define VIRTQ_DESC_F_NEXT  1
#define VIRTQ_DESC_F_WRITE 2

#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2

#define QUEUE_SIZE 8

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[QUEUE_SIZE];
    uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */
} __attribute__((packed));

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[QUEUE_SIZE];
    uint16_t avail_event; /* Only if VIRTIO_F_EVENT_IDX */
} __attribute__((packed));

struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
} __attribute__((packed));

