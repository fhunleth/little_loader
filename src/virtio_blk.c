#include "virtio.h"
#include "util.h"

#include <stdint.h>

struct virtq {
    struct virtq_desc desc[QUEUE_SIZE];
    struct virtq_avail avail;
    uint8_t padding[4096 - sizeof(struct virtq_desc[QUEUE_SIZE]) - sizeof(struct virtq_avail)];
    struct virtq_used used;
} __attribute__((aligned(16)));

static volatile struct virtq_desc desc[QUEUE_SIZE] __attribute__((aligned(16)));
static volatile struct virtq_avail avail __attribute__((aligned(2)));
static volatile struct virtq_used used __attribute__((aligned(4)));
static volatile struct virtio_blk_req req __attribute__((aligned(16)));

static volatile uint8_t status;

void uart_puts(const char *s);

// device feature bits
#define VIRTIO_BLK_F_RO              5	/* Disk is read-only */
#define VIRTIO_BLK_F_SCSI            7	/* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE     11	/* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ             12	/* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT         27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX     29
#define VIRTIO_BLK_F_VERSION_1      32

void virtio_blk_init(void) {
    // This section is a bit of a hack and really should scan for device 2.
    if (VIRT_MMIO_MAGIC != 0x74726976 ||
        VIRT_MMIO_VERSION != 2 ||
        VIRT_MMIO_DEVICE_ID != 2 ||
        VIRT_MMIO_VENDOR_ID != 0x554d4551)
        fatal("Couldn't find virtio disk\n");

    uint32_t mmio_status = 0;
    VIRT_MMIO_STATUS = mmio_status; // RESET

    mmio_status |= 1; // ACKNOWLEDGE
    VIRT_MMIO_STATUS = mmio_status;

    mmio_status |= 2; // DRIVER
    VIRT_MMIO_STATUS = mmio_status;

    // Read and mask unsupported features
    VIRT_MMIO_DEVICE_FEATURES_SEL = 0;
    uint32_t features = VIRT_MMIO_DEVICE_FEATURES;
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    VIRT_MMIO_DRIVER_FEATURES_SEL = 0;
    VIRT_MMIO_DRIVER_FEATURES = features;

    VIRT_MMIO_DEVICE_FEATURES_SEL = 1;
    features = VIRT_MMIO_DEVICE_FEATURES;
    features &= (1 << (VIRTIO_BLK_F_VERSION_1 - 32));
    VIRT_MMIO_DRIVER_FEATURES_SEL = 1;
    VIRT_MMIO_DRIVER_FEATURES = features;

    mmio_status |= 8; // FEATURES_OK
    VIRT_MMIO_STATUS = mmio_status;

    if (VIRT_MMIO_STATUS & 8 == 0)
        fatal("virtio disk didn't like our feature selection?\n");

    VIRT_MMIO_QUEUE_SEL = 0;
    if (VIRT_MMIO_QUEUE_READY != 0)
        fatal("virtio disk queue in use?\n");

    if (VIRT_MMIO_QUEUE_NUM_MAX < QUEUE_SIZE)
        fatal("virtio disk queue num max too low?\n");

    VIRT_MMIO_QUEUE_NUM = QUEUE_SIZE;

    /*
    memset(&desc, 0, sizeof(desc));
    memset(&avail, 0, sizeof(avail));
    memset(&used, 0, sizeof(used));
*/
    avail.idx = 0;
    avail.flags = 0;

    VIRT_MMIO_QUEUE_DESC_LOW  = (uintptr_t)&desc >> 0;
    VIRT_MMIO_QUEUE_DESC_HIGH = (uintptr_t)&desc >> 32;

    VIRT_MMIO_QUEUE_DRIVER_LOW  = (uintptr_t)&avail >> 0;
    VIRT_MMIO_QUEUE_DRIVER_HIGH = (uintptr_t)&avail >> 32;

    VIRT_MMIO_QUEUE_DEVICE_LOW   = (uintptr_t)&used >> 0;
    VIRT_MMIO_QUEUE_DEVICE_HIGH  = (uintptr_t)&used >> 32;

    VIRT_MMIO_QUEUE_READY = 1;
    mmio_status |= 4; // DRIVER_OK
    VIRT_MMIO_STATUS = mmio_status;
}

int virtio_blk_read(uint64_t lba, uint32_t len_bytes, void *buffer) {
    req.type = VIRTIO_BLK_T_IN;
    req.reserved = 0;
    req.sector = lba;

    desc[0].addr = (uintptr_t)&req;
    desc[0].len = sizeof(req);
    desc[0].flags = VIRTQ_DESC_F_NEXT;
    desc[0].next = 1;

    desc[1].addr = (uintptr_t)buffer;
    desc[1].len = len_bytes;
    desc[1].flags = VIRTQ_DESC_F_WRITE | VIRTQ_DESC_F_NEXT;
    desc[1].next = 2;

    status = 0xff; // device writes 0 on success
    desc[2].addr = (uintptr_t)&status;
    desc[2].len = 1;
    desc[2].flags = VIRTQ_DESC_F_WRITE;
    desc[2].next = 0;

    uint16_t idx = avail.idx & (QUEUE_SIZE-1);
    avail.ring[idx] = 0;
    avail.idx++;
    used.idx = 0;
    used.flags = 0;

    VIRT_MMIO_QUEUE_NOTIFY = 0;

    __sync_synchronize();
    while (status == 0xff) {
      __sync_synchronize();
    }

    if (status == VIRTIO_BLK_S_OK)
        return len_bytes;
    else
        return -status;
}

