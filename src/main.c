#include "virtio.h"
#include "pl011_uart.h"
#include "util.h"

#include <stdint.h>
#include <stddef.h>

#define KERNEL_LBA           0x2000        // See demo/fwup.conf
#define KERNEL_MAX_LENGTH    (64 * 1024 * 1024)
#define KERNEL_LOAD_ADDR     0x40200000UL

void virtio_blk_init(void);
int virtio_blk_read(uint64_t lba, uint32_t len_bytes, void *buffer);

static int get_el(void)
{
    unsigned long el;
    asm volatile ("mrs %0, CurrentEL" : "=r"(el));
    return (el >> 2);
}

uint32_t be32_to_le32(uint32_t x)
{
    return ((x >> 24) & 0x000000FF) |
           ((x >> 8)  & 0x0000FF00) |
           ((x << 8)  & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
}

struct kernel_header {
  uint32_t code0;	/* Executable code */
  uint32_t code1;	/* Executable code */
  uint64_t text_offset;	/* Image load offset, little endian */
  uint64_t image_size;	/* Effective Image size, little endian */
  uint64_t flags;	/* kernel flags, little endian */
  uint64_t res2;	/* reserved */
  uint64_t res3;	/* reserved */
  uint64_t res4;	/* reserved */
  uint32_t magic;	/* Magic number, little endian, "ARM\x64" */
  uint32_t res5;	/* reserved (used for PE COFF offset) */
};

static size_t load_kernel(uint64_t lba, uint8_t *kernel_base)
{
    int rc = virtio_blk_read(lba, SECTOR_SIZE, kernel_base);
    if (rc != 0)
        fatal("Failed to read kernel header\n");

    struct kernel_header *header = (struct kernel_header*) kernel_base;
    if (header->magic != 0x644d5241)
        fatal("Linux kernel header magic isn't ARM\\x64\n");

    if (header->image_size > KERNEL_MAX_LENGTH)
        fatal("Linux kernel header too large\n");

    int num_sectors = (int) ((header->image_size + SECTOR_SIZE - 1) / SECTOR_SIZE);
    rc = virtio_blk_read(KERNEL_LBA + 1, (num_sectors - 1) * SECTOR_SIZE, kernel_base + SECTOR_SIZE);
    if (rc != 0)
        fatal("Failed to read kernel\n");

    return header->image_size;
}

void load_dtb(uint32_t *dtb_source, uint8_t *dtb_load_addr)
{
    uint32_t magic = dtb_source[0];
    if (magic != 0xedfe0dd0)
        fatal("DTB address needs to be passed in x0, but magic number is 0x%08x", magic);

    uint32_t len = be32_to_le32(dtb_source[1]);

    uint32_t *dest = (uint32_t*) dtb_load_addr;

    for (uint32_t i = 0; i < len / 4; i++)
        dest[i] = dtb_source[i];
}


void rom_main(uint64_t dtb_source) {
    uart_init();
    info(PROGRAM_NAME " " PROGRAM_VERSION_STR);

    int el = get_el();
    if (el != 2)
        fatal("CPU mode must be in EL2 (-M virt,virtualization=on)");

    virtio_blk_init();

    uart_puts("Loading Linux...\n");
    size_t kernel_len = load_kernel(KERNEL_LBA, (uint8_t*) KERNEL_LOAD_ADDR);

    uint8_t *dtb_load_addr = (uint8_t*) (KERNEL_LOAD_ADDR + ((kernel_len + 7) & ~0x7));
    load_dtb((uint32_t*) dtb_source, dtb_load_addr);

    asm volatile(
    "msr cntkctl_el1, %0\n"
    :: "r"(0x3)  // Enable EL0 access to cntvct_el0 and cntfrq_el0
    );

    info("Starting Linux...");
    asm volatile (
        "mov x0, %0\n"              // dtb_addr → x0
        "mov x1, xzr\n"
        "mov x2, xzr\n"
        "mov x3, xzr\n"
        "br %1\n"
        :
        : "r"(dtb_load_addr), "r"(KERNEL_LOAD_ADDR)
        : "x0", "x1", "x2", "x3"
    );

    // unreachable
    fatal("");
}
