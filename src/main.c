#include "virtio.h"
#include "pl011_uart.h"
#include "uboot_env.h"
#include "util.h"

#include <stdint.h>
#include <stddef.h>

#define UBOOT_ENV_LBA        16
#define UBOOT_ENV_SIZE       (256 * 512) // 128 KiB
#define DEFAULT_KERNEL_LBA   512 // Initially what's not in demo/fwup.conf to avoid missing a U-Boot environment issue
#define KERNEL_MAX_LENGTH    (64 * 1024 * 1024)
#define KERNEL_LOAD_ADDR     0x40200000UL

static struct uboot_env uboot_env;

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

static uint64_t process_uboot_env()
{
    uint8_t *buffer = malloc_(UBOOT_ENV_SIZE);
    struct uboot_env env;

    uboot_env_init(&env, UBOOT_ENV_SIZE);
    int rc = virtio_blk_read(UBOOT_ENV_LBA, UBOOT_ENV_SIZE, buffer);
    if (rc < 0)
        fatal("Failed to read u-boot environment from LBA %d\n", UBOOT_ENV_LBA);

    uint64_t kernel_lba = DEFAULT_KERNEL_LBA;
    char *active_slot = NULL;
    char *upgrade_available = NULL;
    char *bootcount = NULL;
    char *kernel_lba_str = NULL;
    char kernel_lba_key[32];
    strcpy_(kernel_lba_key, "x.kernel_lba");

    OK_OR_CLEANUP_MSG(uboot_env_read(&env, (const char *)buffer), "Failed to read u-boot environment from buffer");
    OK_OR_CLEANUP_MSG(uboot_env_getenv(&env, "nerves_fw_active", &active_slot), "Failed to get `nerves_fw_active` from U-Boot environment");
    OK_OR_CLEANUP_MSG(uboot_env_getenv(&env, "upgrade_available", &upgrade_available), "Failed to get `upgrade_available`. Skipping automatic failback check.");

    if (strcmp_(upgrade_available, "1") == 0) {
        OK_OR_CLEANUP_MSG(uboot_env_getenv(&env, "bootcount", &bootcount), "Failed to get `bootcount`. Skipping automatic failback check.");
        if (strcmp_(bootcount, "1") == 0) {
            // Previous boot failed, so switch back to the other slot
            info("Slot %s didn't validate, so reverting back...", active_slot);
            if (active_slot[0] == 'a')
                active_slot[0] = 'b';
            else
                active_slot[0] = 'a';

            uboot_env_setenv(&env, "nerves_fw_active", active_slot);
            uboot_env_setenv(&env, "upgrade_available", "0");
            uboot_env_setenv(&env, "bootcount", "0");
        } else {
            // First try of new firmware slot, so increment bootcount
            info("Trying slot %s for the first time...", active_slot);
            uboot_env_setenv(&env, "bootcount", "1");
        }

        if (uboot_env_write(&env, buffer) < 0 || virtio_blk_write(UBOOT_ENV_LBA, UBOOT_ENV_SIZE, buffer) < 0)
            info("Failed to write u-boot environment after failback!!");
    }

    kernel_lba_key[0] = active_slot[0];
    OK_OR_CLEANUP_MSG(uboot_env_getenv(&env, kernel_lba_key, &kernel_lba_str), "No '%s' variable found in u-boot environment, using default.", kernel_lba_key);
    kernel_lba = strtoull_(kernel_lba_str, NULL, 10);

    info("Booting from slot %s with kernel LBA %lu", active_slot, kernel_lba);

cleanup:
    free_(kernel_lba_str);
    free_(active_slot);
    free_(upgrade_available);
    free_(bootcount);
    uboot_env_free(&env);
    return kernel_lba;
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
    if (rc < 0)
        fatal("Failed to read kernel header at LBA %lu", lba);

    struct kernel_header *header = (struct kernel_header*) kernel_base;
    if (header->magic != 0x644d5241)
        fatal("Linux kernel header magic isn't ARM\\x64");

    if (header->image_size > KERNEL_MAX_LENGTH)
        fatal("Linux kernel header image size of %lu is larger than max support size of %lu", header->image_size, KERNEL_MAX_LENGTH);

    int num_sectors = (int) ((header->image_size + SECTOR_SIZE - 1) / SECTOR_SIZE);
    rc = virtio_blk_read(lba + 1, (num_sectors - 1) * SECTOR_SIZE, kernel_base + SECTOR_SIZE);
    if (rc < 0)
        fatal("Failed to read kernel");

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

    uint64_t kernel_lba = process_uboot_env();
    size_t kernel_len = load_kernel(kernel_lba, (uint8_t*) KERNEL_LOAD_ADDR);

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
