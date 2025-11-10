/*
 * Custom KVStore initialization for Pico W
 *
 * Pico W has 2MB flash total
 * Layout:
 *   0x000000 - 0x1C0000 (1.75MB) - Program space (safe margin)
 *   0x1C0000 - 0x1E0000 (128KB)  - KV Store
 *   0x1E0000 - 0x200000 (128KB)  - Bluetooth flash bank
 *
 * This ensures kvstore doesn't overlap with firmware even if it grows larger
 */
#include "blockdevice/flash.h"
#include "kvstore.h"
#include "kvstore_logkvs.h"
#include "hardware/flash.h"
#include <stdio.h>

// Reserve last 256KB of 2MB flash
// Last 128KB for Bluetooth, previous 128KB for KVStore
#define PICO_W_FLASH_SIZE_BYTES (2 * 1024 * 1024) // 2MB total
#define BLUETOOTH_BANK_SIZE (128 * 1024)          // 128KB for Bluetooth
#define KVSTORE_BANK_SIZE (128 * 1024)            // 128KB for KVStore
#define KVSTORE_BANK_OFFSET (PICO_W_FLASH_SIZE_BYTES - BLUETOOTH_BANK_SIZE - KVSTORE_BANK_SIZE)

#ifdef KVSTORE_DEBUG
#include <stdarg.h>
static inline void print_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#else
static inline void print_debug(const char *format, ...) { (void)format; }
#endif

bool kvs_init(void)
{
    printf("KVStore: Using flash region 0x%08x -> 0x%08x (%u KB)\n",
           XIP_BASE + KVSTORE_BANK_OFFSET,
           XIP_BASE + KVSTORE_BANK_OFFSET + KVSTORE_BANK_SIZE,
           KVSTORE_BANK_SIZE / 1024);

    blockdevice_t *bd = blockdevice_flash_create(KVSTORE_BANK_OFFSET, KVSTORE_BANK_SIZE);
    if (!bd)
    {
        printf("KVStore: Failed to create block device\n");
        return false;
    }

    print_debug("KVStore: Creating log-structured key-value store\n");
    kvs_t *kvs = kvs_logkvs_create(bd);
    if (!kvs)
    {
        printf("KVStore: Failed to create kvstore\n");
        return false;
    }

    print_debug("KVStore: Assigning to global instance\n");
    kvs_assign(kvs);

    printf("KVStore: Initialization complete\n");
    return true;
}
